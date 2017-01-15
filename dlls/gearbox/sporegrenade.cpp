/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "decals.h"
#include "sporegrenade.h"
#include "gearbox_weapons.h"

LINK_ENTITY_TO_CLASS(monster_spore, CSporeGrenade);

TYPEDESCRIPTION	CSporeGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CSporeGrenade, m_pSporeGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CSporeGrenade, m_flNextSpriteTrailSpawn, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CSporeGrenade, CGrenade);

void CSporeGrenade::Precache(void)
{
	PRECACHE_MODEL("sprites/glow01.spr");
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CSporeGrenade::Explode(TraceResult *pTrace, int bitsDamageType)
{
	//float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if (pTrace->flFraction != 1.0)
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	Vector vecSpraySpot = pTrace->vecEndPos;
	float flSpraySpeed = RANDOM_LONG(10, 15);

	// If the trace is pointing up, then place
	// spawn position a few units higher.
	if (pTrace->vecPlaneNormal.z > 0)
	{
		vecSpraySpot = vecSpraySpot + (pTrace->vecPlaneNormal * 8);
		flSpraySpeed *= 2; // Double the speed to make them fly higher
						   // in the air.
	}

	// Spawn small particles at the explosion origin.
	SpawnExplosionParticles(
		vecSpraySpot,				// position
		pTrace->vecPlaneNormal,			// direction
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG(8, 10),				// count
		flSpraySpeed,					// speed
		RANDOM_FLOAT(10, 20) * 10);		// noise

	// Play explode sound.
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/splauncher_impact.wav", 1, ATTN_NORM);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0);
	entvars_t *pevOwner;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage(pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType);

	// Place a decal on the surface that was hit.
	UTIL_DecalTrace(pTrace, DECAL_YBLOOD5 + RANDOM_LONG(0, 1));

	pev->effects |= EF_NODRAW;
	SetThink(&CSporeGrenade::Smoke);
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if (m_pSporeGlow)
	{
		UTIL_Remove(m_pSporeGlow);
		m_pSporeGlow = NULL;
	}
}

void CSporeGrenade::Smoke(void)
{
	if (UTIL_PointContents(pev->origin) == CONTENTS_WATER)
	{
		UTIL_Bubbles(pev->origin - Vector(64, 64, 64), pev->origin + Vector(64, 64, 64), 100);
	}
	UTIL_Remove(this);
}

void CSporeGrenade::Killed(entvars_t *pevAttacker, int iGib)
{
	Detonate();
}

// Timed grenade, this think is called when time runs out.
void CSporeGrenade::DetonateUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CSporeGrenade::Detonate);
	pev->nextthink = gpGlobals->time;
}

void CSporeGrenade::PreDetonate(void)
{
	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, 400, 0.3);

	SetThink(&CSporeGrenade::Detonate);
	pev->nextthink = gpGlobals->time + 1;
}

void CSporeGrenade::Detonate(void)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -40), ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}


void CSporeGrenade::BounceSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit1.wav", 0.25, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit2.wav", 0.25, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/spore_hit3.wav", 0.25, ATTN_NORM);	break;
	}
}

void CSporeGrenade::TumbleThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1);
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink(&CSporeGrenade::Detonate);
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG( 5, 10 ),			// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

//
// Contact grenade, explode when it touches something
// 
void CSporeGrenade::ExplodeTouch(CBaseEntity *pOther)
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine(vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST);
}



void CSporeGrenade::DangerSoundThink(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, pev->velocity.Length(), 0.2);
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}

	// Spawn particles.
	SpawnTrailParticles(
		pev->origin,					// position
		-pev->velocity.Normalize(),		// dir
		g_sModelIndexTinySpit,			// modelindex
		RANDOM_LONG(5, 10),				// count
		RANDOM_FLOAT(10, 15),			// speed
		RANDOM_FLOAT(2, 3) * 100);		// noise ( client will divide by 100 )
}

void CSporeGrenade::BounceTouch(CBaseEntity *pOther)
{
#if 0
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;
#endif

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 100)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		if (pevOwner)
		{
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, 1, gpGlobals->v_forward, &tr, DMG_CLUB);
			ApplyMultiDamage(pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity;
	vecTestVelocity.z *= 1.25; // 0.45

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the danger sound.

		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3);
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.01;

		pev->sequence = RANDOM_LONG(1, 1);
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;
}


void CSporeGrenade::SlideTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;

		if (pev->velocity.x != 0 || pev->velocity.y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CSporeGrenade::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/spore.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->gravity = 0.5; // 0.5
	pev->friction = 0.2; // 0.8

	pev->dmg = gSkillData.plrDmgSpore;
	m_fRegisteredSound = FALSE;

	m_flNextSpriteTrailSpawn = gpGlobals->time;

	m_pSporeGlow = CSprite::SpriteCreate("sprites/glow01.spr", pev->origin, FALSE);

	if (m_pSporeGlow)
	{
		m_pSporeGlow->SetTransparency(kRenderGlow, 217, 241, 152, 200, kRenderFxNoDissipation);
		m_pSporeGlow->SetAttachment(edict(), 1);
		m_pSporeGlow->SetScale(.5f);
	}
}

CGrenade * CSporeGrenade::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->SetTouch(&CSporeGrenade::BounceTouch);	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink(&CSporeGrenade::TumbleThink);
	pGrenade->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}

	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.2; // 0.8

	SET_MODEL(ENT(pGrenade->pev), "models/spore.mdl");
	pGrenade->pev->dmg = gSkillData.plrDmgSpore;

	return pGrenade;
}

CGrenade *CSporeGrenade::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CSporeGrenade *pGrenade = GetClassPtr((CSporeGrenade *)NULL);
	pGrenade->Spawn();
	pGrenade->pev->movetype = MOVETYPE_FLY;
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	// make monsters afaid of it while in the air
	pGrenade->SetThink(&CSporeGrenade::DangerSoundThink);
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air

	// Explode on contact
	pGrenade->SetTouch(&CSporeGrenade::ExplodeTouch);

	pGrenade->pev->dmg = gSkillData.plrDmgSpore;

	return pGrenade;
}

void CSporeGrenade::SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}

void CSporeGrenade::SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise)
{

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(origin.x);				// pos
		WRITE_COORD(origin.y);
		WRITE_COORD(origin.z);
		WRITE_COORD(direction.x);			// dir
		WRITE_COORD(direction.y);
		WRITE_COORD(direction.z);
		WRITE_SHORT(modelindex);			// model
		WRITE_BYTE(count);					// count
		WRITE_BYTE(speed);					// speed
		WRITE_BYTE(noise);					// noise ( client will divide by 100 )
	MESSAGE_END();
}