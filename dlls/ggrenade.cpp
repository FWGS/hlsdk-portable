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
/*
 
===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"
#include "decals.h"
#include "skill.h"
#include "hltv.h"
#include "func_break.h"

extern short g_sModelIndexFireball2;
extern short g_sModelIndexFireball3;
extern short g_sModelIndexC4Glow;
unsigned short g_usExplosion;

class CWeaponEffects : public CPointEntity
{
public:
	void Spawn() override;
	void EXPORT FlashEffectThink();
	void EXPORT SmokeEffectThink();
	void SetEffectType( int iType ) { m_iEffectType = iType; }

	float m_flNextSwitchTime;
	float m_flDieTime;
	float m_flRadius;
	int m_iEffectType;
	int m_throwerClasstype;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};


//===================grenade


LINK_ENTITY_TO_CLASS( grenade, CGrenade );
LINK_ENTITY_TO_CLASS( weapon_effectspoint, CWeaponEffects );

void CWeaponEffects::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	if ( m_iEffectType == ACCESSORY_SMOKEGRENADE )
	{
		m_flRadius = 128;
		m_flDieTime = gpGlobals->time + 25;
		m_flNextSwitchTime = gpGlobals->time + 0.25f;
		pev->nextthink = m_flNextSwitchTime;
		SetThink( &CWeaponEffects::SmokeEffectThink );
	}
	else if ( m_iEffectType == ACCESSORY_FLASHBANG )
	{
		m_flRadius = 256;
		m_flNextSwitchTime = gpGlobals->time + 0.25f;
		pev->nextthink = m_flNextSwitchTime;
		SetThink( &CWeaponEffects::FlashEffectThink );
	}
}

void CWeaponEffects::FlashEffectThink()
{
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, m_flRadius ) ) != NULL )
	{
		if ( pEntity->pev->flags & FL_MONSTER )
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if ( pMonster && pMonster->m_classtype != m_throwerClasstype )
				pMonster->SetWeaponEffect( m_iEffectType, TRUE );
		}
	}
	UTIL_Remove( this );
}

void CWeaponEffects::SmokeEffectThink()
{
	m_flRadius += 3;
	CBaseMonster* pPlayer = (CBaseMonster*)CBaseEntity::Instance( INDEXENT( 1 ) );
	pPlayer->SetWeaponEffect( 0, FALSE );
	BOOL fActive = TRUE;
	if ( gpGlobals->time > m_flDieTime )
	{
		fActive = FALSE;
		UTIL_Remove( this );
	}
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, m_flRadius ) ) != NULL )
	{
		if ( ( pEntity->pev->flags & FL_MONSTER ) || pEntity->IsPlayer() )
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if ( pMonster )
				pMonster->SetWeaponEffect( m_iEffectType, fActive );
		}
	}
	if ( gpGlobals->time <= m_flDieTime )
	{
		m_flNextSwitchTime = gpGlobals->time + 0.25f;
		SetThink( &CWeaponEffects::SmokeEffectThink );
		pev->nextthink = m_flNextSwitchTime;
	}
}

TYPEDESCRIPTION CGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CGrenade, m_fAttenu, FIELD_FLOAT ),
	DEFINE_FIELD( CGrenade, m_flNextFreq, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_flC4Blow, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_flNextFreqInterval, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_flNextBeep, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_flDefuseCountDown, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_flNextBlink, FIELD_TIME ),
	DEFINE_FIELD( CGrenade, m_pentCurBombTarget, FIELD_EDICT ),
	DEFINE_FIELD( CGrenade, m_sBeepName, FIELD_POINTER ),
	DEFINE_FIELD( CGrenade, m_bIsC4, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGrenade, m_bStartDefuse, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGrenade, m_SGSmoke, FIELD_INTEGER ),
	DEFINE_FIELD( CGrenade, m_bJustBlew, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGrenade, m_bLightSmoke, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGrenade, m_usEvent, FIELD_INTEGER ),
	DEFINE_FIELD( CGrenade, m_throwerClasstype, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CGrenade, CBaseMonster );

void CGrenade::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "grenade" );
	m_bIsC4 = false;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/grenade.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->dmg = 30;
	m_fRegisteredSound = FALSE;
}

CGrenade* CGrenade::ShootTimed( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity,
	float time, float damage, int )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->SetTouch( &CGrenade::BounceTouch );	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = g_vecZero;
	}
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8f;
	SET_MODEL( ENT( pGrenade->pev ), "models/w_flashbang.mdl" );
	pGrenade->pev->dmg = damage;
	return pGrenade;
}

CGrenade* CGrenade::ShootTimed2( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity,
		float time, int iTeam, unsigned short usEvent )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->m_usEvent = usEvent;
	pGrenade->SetTouch( &CGrenade::BounceTouch );
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::TumbleThink );
	pGrenade->pev->nextthink = (float)( gpGlobals->time + 0.1 );
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.55f;
	pGrenade->pev->friction = 0.7f;
	pGrenade->m_iTeam = iTeam;
	SET_MODEL( ENT( pGrenade->pev ), "models/w_hegrenade.mdl" );
	pGrenade->pev->dmg = 175;
	return pGrenade;
}

CGrenade* CGrenade::ShootSmokeGrenade( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity,
		float time, unsigned short usEvent )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->Spawn();
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = pevOwner->angles;
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->m_bLightSmoke = false;
	pGrenade->m_bDetonated = false;
	pGrenade->SetTouch( &CGrenade::BounceTouch );
	pGrenade->m_usEvent = usEvent;
	pGrenade->pev->dmgtime = gpGlobals->time + time;
	pGrenade->SetThink( &CGrenade::SG_TumbleThink );
	pGrenade->pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( time < 0.1 )
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = g_vecZero;
	}
	pGrenade->pev->sequence = RANDOM_LONG( 3, 6 );
	pGrenade->pev->framerate = 1;
	pGrenade->m_bJustBlew = true;
	pGrenade->pev->gravity = 0.5;
	pGrenade->pev->friction = 0.8f;
	pGrenade->m_SGSmoke = 0;
	SET_MODEL( ENT( pGrenade->pev ), "models/w_smokegrenade.mdl" );
	pGrenade->pev->dmg = 35;
	return pGrenade;
}

CGrenade* CGrenade::ShootContact( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->Spawn();
	// contact grenades arc lower
	pGrenade->pev->gravity = 0.5; // lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles( vecVelocity );
	pGrenade->pev->owner = ENT( pevOwner );

	// make monsters afaid of it while in the air
	pGrenade->SetThink( &CGrenade::DangerSoundThink );
	pGrenade->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pGrenade->pev->avelocity.x = RANDOM_FLOAT( -100, -500 );

	// Explode on contact
	pGrenade->SetTouch( &CGrenade::ExplodeTouch );
	pGrenade->pev->dmg = gSkillData.plrDmgM203Grenade;
	pGrenade->m_bJustBlew = true;
	return pGrenade;
}

CGrenade* CGrenade::ShootSatchelCharge( entvars_t* pevOwner, Vector vecStart, Vector vecAngles )
{
	CGrenade* pGrenade = GetClassPtr( (CGrenade*)NULL );
	pGrenade->pev->movetype = MOVETYPE_TOSS;
	pGrenade->pev->classname = MAKE_STRING( "grenade" );
	pGrenade->pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pGrenade->pev ), "models/w_c4.mdl" );
	UTIL_SetSize( pGrenade->pev, g_vecZero, Vector( 8, 8, 8 ) );
	pGrenade->pev->dmg = 100;
	UTIL_SetOrigin( pGrenade->pev, vecStart );
	pGrenade->pev->velocity = g_vecZero;
	pGrenade->pev->angles = vecAngles;
	pGrenade->pev->owner = ENT( pevOwner );
	pGrenade->SetThink( &CGrenade::C4Think );
	pGrenade->SetTouch( &CGrenade::C4Touch );
	pGrenade->pev->spawnflags = 1;
	pGrenade->pev->nextthink = (float)( gpGlobals->time + 0.1 );
	pGrenade->m_flNextFreq = gpGlobals->time;
	pGrenade->m_iCurWave = 0;
	pGrenade->m_fAttenu = 0;
	pGrenade->m_sBeepName = NULL;
	pGrenade->m_flNextBeep = (float)( gpGlobals->time + 0.5 );
	pGrenade->m_bIsC4 = true;
	pGrenade->m_fNextDefuse = 0;
	pGrenade->m_bStartDefuse = false;
	pGrenade->m_flNextBlink = gpGlobals->time + 2;
	pGrenade->pev->friction = 0.9f;
	pGrenade->m_bJustBlew = false;
	CBaseEntity::Instance( pevOwner );
	return pGrenade;
}

void CGrenade::UseSatchelCharges( entvars_t* pevOwner, SATCHELCODE code )
{
	if ( !pevOwner )
		return;

	CBaseEntity* pOwner = CBaseEntity::Instance( pevOwner );
	edict_t* pentOwner = pOwner->edict();
	edict_t* pentGrenade = NULL;
	while ( !FNullEnt( pentGrenade = FIND_ENTITY_BY_CLASSNAME( pentGrenade, "grenade" ) ) )
	{
		CBaseEntity* pGrenade = CBaseEntity::Instance( pentGrenade );
		if ( pGrenade && ( pGrenade->pev->spawnflags & 1 ) && pGrenade->pev->owner == pentOwner )
		{
			if ( code == SATCHEL_DETONATE )
				pGrenade->Use( pOwner, pOwner, USE_ON, 0 );
			else
				pGrenade->pev->owner = NULL;
		}
	}
}

void CGrenade::BounceSound()
{
	if ( pev->dmg > 50 )
	{
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/he_bounce-1.wav", 0.25, ATTN_NORM );
	}
	else
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM );
			break;
		}
	}
}

void CGrenade::SlideTouch( CBaseEntity* pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;
	if ( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.95f;
	else
		BounceSound();
}

void CGrenade::BounceTouch( CBaseEntity* pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;
	if ( FClassnameIs( pOther->pev, "func_breakable" ) && pOther->pev->rendermode != kRenderNormal
		&& ( (CBreakable*)pOther )->m_grenadeTouch )
	{
		pev->velocity = pev->velocity * -1.5;
		return;
	}
	if ( !m_fRegisteredSound )
	{
		Vector vecVelocity = pev->velocity;
		vecVelocity.z = (float)( vecVelocity.z * 0.7 );
		if ( !( vecVelocity.Length() > 60 ) )
		{
			//ALERT( at_console, "Grenade Registered!: %f\n", vecTestVelocity.Length() );

			// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
			// go ahead and emit the danger sound.

			// register a radius louder than the explosion, so we make sure everyone gets out of the way
			CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, (int)( pev->dmg / 0.4 ), 0.3f );
			m_fRegisteredSound = TRUE;
		}
	}
	if ( pev->flags & FL_ONGROUND )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8f;
		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else if ( ( pev->origin - m_vecLastBounce ).Length() > 2.0f )
	{
		// play bounce sound
		BounceSound();
	}
	// czeror test 11.11, cz_recoil kabloom2: a grenade trapped in a solid touches it every frame without
	// moving (the engine clears its velocity, gravity adds a little before each touch); it rattled for the
	// 20 s of its smoke. A real bounce happens somewhere else than the last one.
	m_vecLastBounce = pev->origin;
	pev->framerate = pev->velocity.Length() / 200.0;
	if ( !( pev->framerate <= 1 ) )
		pev->framerate = 1;
	else if ( pev->framerate < 0.5 )
		pev->framerate = 0;
}


//
// Contact grenade, explode when it touches something
// 
void CGrenade::ExplodeTouch( CBaseEntity* pOther )
{
	pev->enemy = pOther->edict();
	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, edict(), &tr );
	Explode3( &tr, DMG_BLAST );
}

void CGrenade::C4Touch( CBaseEntity* )
{
}

void CGrenade::DetonateUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CGrenade::PreDetonate()
{
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, 400, 0.3f );
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time + 1;
}

void CGrenade::TumbleThink()
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}
	StudioFrameAdvance();
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( gpGlobals->time > pev->dmgtime - 1 )
	{
		Vector vecSpot = pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time );
		CSoundEnt::InsertSound( bits_SOUND_DANGER, vecSpot, 400, 0.1f );
	}
	if ( gpGlobals->time >= pev->dmgtime )
	{
		if ( pev->dmg > 40 )
			SetThink( &CGrenade::Detonate3 );
		else
			SetThink( &CGrenade::Detonate );
	}
	if ( pev->waterlevel )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2f;
	}
}

void CGrenade::SG_TumbleThink()
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}
	if ( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.95f;
	StudioFrameAdvance();
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( gpGlobals->time > pev->dmgtime - 1 )
	{
		Vector vecSpot = pev->origin + pev->velocity * ( pev->dmgtime - gpGlobals->time );
		CSoundEnt::InsertSound( bits_SOUND_DANGER, vecSpot, 400, 0.1f );
	}
	if ( gpGlobals->time >= pev->dmgtime && ( pev->flags & FL_ONGROUND ) )
		SetThink( &CGrenade::SG_Detonate );
	// czeror test 11.11, cz_recoil kabloom2: a grenade trapped in a solid never gets FL_ONGROUND
	// (the engine only clears its velocity) and rattled forever; it goes off 2 s after its time
	else if ( gpGlobals->time >= pev->dmgtime + 2.0f )
		SetThink( &CGrenade::SG_Detonate );
	if ( pev->waterlevel )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2f;
	}
}

void CGrenade::DangerSoundThink()
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}
	CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5,
		(int)pev->velocity.Length(), 0.2f );
	pev->nextthink = (float)( gpGlobals->time + 0.2 );
	if ( pev->waterlevel )
		pev->velocity = pev->velocity * 0.5;
}

void CGrenade::Killed( entvars_t*, int )
{
	Detonate();
}

void CGrenade::Explode( Vector, Vector )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -32 ), ignore_monsters, edict(), &tr );
	Explode( &tr, DMG_BLAST );
}

void CGrenade::Detonate()
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, edict(), &tr );
	Explode( &tr, DMG_BLAST );
	edict_t* pentEffect = CREATE_NAMED_ENTITY( ALLOC_STRING( "weapon_effectspoint" ) );
	if ( !FNullEnt( pentEffect ) )
	{
		pentEffect->v.origin = pev->origin;
		pentEffect->v.angles = pev->angles;
		CWeaponEffects* pEffect = (CWeaponEffects*)CBaseEntity::Instance( pentEffect );
		pEffect->SetEffectType( ACCESSORY_FLASHBANG );
		pEffect->m_throwerClasstype = m_throwerClasstype;
		DispatchSpawn( pentEffect );
		pentEffect->v.owner = edict();
	}
	else
		ALERT( at_console, "NULL Ent in Smoke Grenade!\n" );
}

void CGrenade::Detonate2()
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, edict(), &tr );
	Explode2( &tr, DMG_BLAST );
}

void CGrenade::Detonate3()
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, edict(), &tr );
	Explode3( &tr, DMG_EXPLOSION );
}

void CGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1 )
	{
		float flOffset = pev->dmg - 24;
		if ( !( flOffset >= 4 ) )
			flOffset = 4;
		pev->origin = pTrace->vecEndPos + pTrace->vecPlaneNormal * flOffset * 0.6f;
	}
	int iContents = UTIL_PointContents( pev->origin );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3 );
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	::RadiusFlash( pev->origin, pev, pevAttacker, 4, CLASS_NONE, bitsDamageType );
	if ( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	RANDOM_FLOAT( 0, 1 );
	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/flashbang-1.wav", 0.55f, ATTN_NORM );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/flashbang-2.wav", 0.55f, ATTN_NORM );
	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = (float)( gpGlobals->time + 0.3 );
	if ( iContents != CONTENTS_WATER )
	{
		int count = RANDOM_LONG( 0, 3 );
		for ( int i = 0; i < count; ++i )
			CBaseEntity::Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::SG_Explode( TraceResult* pTrace, int )
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	if ( pTrace->flFraction != 1 )
		pev->origin = pTrace->vecEndPos + pTrace->vecPlaneNormal * ( pev->dmg - 24 ) * 0.6f;
	int iContents = UTIL_PointContents( pev->origin );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3 );
	pev->owner = NULL;
	if ( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	RANDOM_FLOAT( 0, 1 );
	if ( RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/flashbang-1.wav", 0.55f, ATTN_NORM );
	else
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/flashbang-2.wav", 0.55f, ATTN_NORM );
	SetThink( &CGrenade::SG_Smoke );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( iContents != CONTENTS_WATER )
	{
		int count = RANDOM_LONG( 0, 3 );
		for ( int i = 0; i < count; ++i )
			CBaseEntity::Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::Explode2( TraceResult* pTrace, int )
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	UTIL_ScreenShake( pTrace->vecEndPos, 25, 150, 1, 3000 );
	m_bJustBlew = true;
	if ( pTrace->flFraction != 1 )
		pev->origin = pTrace->vecEndPos + pTrace->vecPlaneNormal * ( pev->dmg - 24 ) * 0.6f;
	int iContents = UTIL_PointContents( pev->origin );
	for ( int i = 0; i < 4; ++i )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITE );
			if ( !i )
			{
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z - 10 );
			}
			else
			{
				WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -512, 512 ) );
				WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -512, 512 ) );
				WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
			}
			WRITE_SHORT( i == 1 ? g_sModelIndexFireball2 : i == 3 ? g_sModelIndexFireball : g_sModelIndexFireball3 );
			WRITE_BYTE( (int)( ( pev->dmg - 275 ) * 0.6 ) );
			WRITE_BYTE( i == 3 ? 17 : 150 );
		MESSAGE_END();
	}
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/c4_explode1.wav", 1, 0.25 );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3 );
	pev->owner = NULL;
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );
		WRITE_BYTE( DRC_CMD_EVENT );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( 0 );
		WRITE_LONG( 15 | DRC_FLAG_FINAL );
	MESSAGE_END();
	if ( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	RANDOM_FLOAT( 0, 1 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55f, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55f, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55f, ATTN_NORM );
		break;
	}
	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke2 );
	pev->velocity = g_vecZero;
	pev->nextthink = (float)( gpGlobals->time + 0.85 );
	if ( iContents != CONTENTS_WATER )
	{
		int count = RANDOM_LONG( 0, 3 );
		for ( int i = 0; i < count; ++i )
			CBaseEntity::Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}
}

void CGrenade::Explode3( TraceResult* pTrace, int bitsDamageType )
{
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	if ( pTrace->flFraction != 1 )
		pev->origin = pTrace->vecEndPos + pTrace->vecPlaneNormal * ( pev->dmg - 24 ) * 0.6f;
	if ( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		PLAYBACK_EVENT_FULL( FEV_GLOBAL, NULL, g_usExplosion, 0, pev->origin, (float*)&g_vecZero,
			0, 0, 0, 0, 0, 0 );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3 );
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	RadiusDamage( pev, pevAttacker, pev->dmg, CLASS_NONE, bitsDamageType );
	if ( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( pTrace, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( pTrace, DECAL_SCORCH1 );
	RANDOM_FLOAT( 0, 1 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55f, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55f, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55f, ATTN_NORM );
		break;
	}
	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke3_C );
	pev->velocity = g_vecZero;
	pev->nextthink = (float)( gpGlobals->time + 0.55 );
}

void CGrenade::SG_Detonate()
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, edict(), &tr );
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/sg_explode.wav", 1, ATTN_NORM );
	edict_t* pentGrenade = NULL;
	while ( !FNullEnt( pentGrenade = FIND_ENTITY_BY_CLASSNAME( pentGrenade, "grenade" ) ) )
	{
		CBaseEntity* pGrenade = CBaseEntity::Instance( pentGrenade );
		if ( pGrenade )
		{
			float distance = ( pGrenade->pev->origin - pev->origin ).Length();
			if ( distance != 0 && distance <= 250 && gpGlobals->time > pGrenade->pev->dmgtime )
				m_bLightSmoke = true;
		}
	}
	m_bDetonated = true;
	PLAYBACK_EVENT_FULL( 0, NULL, m_usEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 0, 1, m_bLightSmoke, 0 );
	pev->velocity.x = RANDOM_FLOAT( -175, 175 );
	pev->velocity.y = RANDOM_FLOAT( -175, 175 );
	pev->velocity.z = RANDOM_FLOAT( 250, 350 );
	pev->avelocity = Vector( RANDOM_FLOAT( 150, 450 ), RANDOM_FLOAT( 150, 450 ), RANDOM_FLOAT( 150, 450 ) );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CGrenade::SG_Smoke );
	edict_t* pentEffect = CREATE_NAMED_ENTITY( ALLOC_STRING( "weapon_effectspoint" ) );
	if ( !FNullEnt( pentEffect ) )
	{
		pentEffect->v.origin = pev->origin;
		pentEffect->v.angles = pev->angles;
		CWeaponEffects* pEffect = (CWeaponEffects*)CBaseEntity::Instance( pentEffect );
		pEffect->SetEffectType( ACCESSORY_SMOKEGRENADE );
		pEffect->m_throwerClasstype = m_throwerClasstype;
		DispatchSpawn( pentEffect );
		pentEffect->v.owner = edict();
	}
	else
		ALERT( at_console, "NULL Ent in Smoke Grenade!\n" );
}

void CGrenade::SG_Smoke()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		UTIL_MakeVectors( pev->angles );
		float flVelocity = RANDOM_FLOAT( 3, 8 );
		float x = gpGlobals->v_forward.x * flVelocity;
		float y = gpGlobals->v_forward.y * flVelocity;
		int angle = m_angle;
		m_angle = ( m_angle + 30 ) % 360;
		float flRadians = (float)( angle * 0.01745287222222222 );
		double flCos = cos( flRadians );
		double flSin = sin( flRadians );
		float flX = (float)( flCos * x - flSin * y );
		float flY = (float)( flCos * y + flSin * x );
		float speed = RANDOM_FLOAT( 1.5, 3.5 ) * 100;
		PLAYBACK_EVENT_FULL( 0, NULL, m_usEvent, 0, pev->origin, (float*)&g_vecZero,
			flX, flY, (int)speed, 0, m_bLightSmoke, 6 );
	}
	if ( m_SGSmoke <= 20 )
	{
		pev->nextthink = gpGlobals->time + 1;
		++m_SGSmoke;
		SetThink( &CGrenade::SG_Smoke );
		return;
	}
	pev->effects |= EF_NODRAW;
	UTIL_Remove( this );
}

void CGrenade::Smoke()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 25 );
			WRITE_BYTE( 6 );
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Smoke2()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 150 );
			WRITE_BYTE( 8 );
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Smoke3_A()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (int)(float)( RANDOM_FLOAT( 0, 10 ) + 15.0f ) );
			WRITE_BYTE( 12 );
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Smoke3_B()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -128, 128 ) );
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -10, 10 ) );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (int)(float)( RANDOM_FLOAT( 0, 10 ) + 15.0f ) );
			WRITE_BYTE( 10 );
		MESSAGE_END();
	}
	pev->nextthink = (float)( gpGlobals->time + 0.15 );
	SetThink( &CGrenade::Smoke3_A );
}

void CGrenade::Smoke3_C()
{
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_WATER )
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z - 5 );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (int)(float)( RANDOM_FLOAT( 0, 10 ) + 35.0 ) );
			WRITE_BYTE( 5 );
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CGrenade::Use( CBaseEntity* pActivator, CBaseEntity*, USE_TYPE, float )
{
	if ( !m_bIsC4 )
		return;
	CBasePlayer* pPlayer = GetClassPtr( (CBasePlayer*)pActivator->pev );
	if ( pPlayer->m_iTeam != CT )
		return;
	if ( m_bStartDefuse )
	{
		m_fNextDefuse = (float)( gpGlobals->time + 0.5 );
		return;
	}
	g_engfuncs.pfnSetClientMaxspeed( pPlayer->edict(), 1 );
	if ( pPlayer->m_bHasDefuser )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_With_Kit\"\n",
			STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ) );
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Defusing_Bomb_With_Defuse_Kit" );
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "weapons/c4_disarm.wav", 1, ATTN_NORM );
		pPlayer->m_bIsDefusing = true;
		m_pBombDefuser = pActivator;
		m_bStartDefuse = true;
		m_flDefuseCountDown = gpGlobals->time + 5.0;
		m_fNextDefuse = (float)( gpGlobals->time + 0.5 );
		pPlayer->SetProgressBarTime( 5 );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Begin_Bomb_Defuse_Without_Kit\"\n",
			STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ),
			GETPLAYERAUTHID( pPlayer->edict() ) );
		ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Defusing_Bomb_Without_Defuse_Kit" );
		EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "weapons/c4_disarm.wav", 1, ATTN_NORM );
		pPlayer->m_bIsDefusing = true;
		m_pBombDefuser = pActivator;
		m_bStartDefuse = true;
		m_flDefuseCountDown = gpGlobals->time + 10.0;
		m_fNextDefuse = (float)( gpGlobals->time + 0.5 );
		pPlayer->SetProgressBarTime( 10 );
	}
}

void CGrenade::C4Think()
{
	if ( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}
	pev->nextthink = (float)( gpGlobals->time + 0.12 );
	if ( gpGlobals->time >= m_flNextFreq )
	{
		m_flNextFreq = gpGlobals->time + m_flNextFreqInterval;
		m_flNextFreqInterval = (float)( 0.9 * m_flNextFreqInterval );
		switch ( m_iCurWave )
		{
		case 0:
			m_sBeepName = "weapons/c4_beep1.wav";
			m_fAttenu = 1.5;
			break;
		case 1:
			m_sBeepName = "weapons/c4_beep2.wav";
			m_fAttenu = 1;
			break;
		case 2:
			m_sBeepName = "weapons/c4_beep3.wav";
			m_fAttenu = 0.8f;
			break;
		case 3:
			m_sBeepName = "weapons/c4_beep4.wav";
			m_fAttenu = 0.5;
			break;
		case 4:
			m_sBeepName = "weapons/c4_beep5.wav";
			m_fAttenu = 0.2f;
			break;
		}
		++m_iCurWave;
	}
	if ( gpGlobals->time >= m_flNextBeep )
	{
		m_flNextBeep = (float)( gpGlobals->time + 1.4 );
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, m_sBeepName, 1, m_fAttenu );
	}
	if ( gpGlobals->time >= m_flNextBlink )
	{
		m_flNextBlink = gpGlobals->time + 2.0;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_GLOWSPRITE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z + 5 );
			WRITE_SHORT( g_sModelIndexC4Glow );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 3 );
			WRITE_BYTE( 255 );
		MESSAGE_END();
	}
	if ( gpGlobals->time >= m_flC4Blow )
	{
		if ( m_pentCurBombTarget )
		{
			CBaseEntity* pTarget = CBaseEntity::Instance( m_pentCurBombTarget );
			if ( pTarget )
				pTarget->Use( CBaseEntity::Instance( pev->owner ), this, USE_TOGGLE, 0 );
		}
		if ( pev->waterlevel )
			UTIL_Remove( this );
		else
			SetThink( &CGrenade::Detonate2 );
	}
	if ( !m_bStartDefuse )
		return;
	if ( !m_pBombDefuser )
		return;
	if ( (CBaseEntity*)m_pBombDefuser && m_flDefuseCountDown > gpGlobals->time )
	{
		int onGround = m_pBombDefuser->pev->flags & FL_ONGROUND;
		if ( gpGlobals->time <= m_fNextDefuse && onGround )
			return;
		if ( !onGround )
			ClientPrint( m_pBombDefuser->pev, HUD_PRINTCENTER, "#C4_Defuse_Must_Be_On_Ground" );
		( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->ResetMaxSpeed();
		( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->m_bIsDefusing = false;
		( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->SetProgressBarTime( 0 );
		m_pBombDefuser = NULL;
		m_bStartDefuse = false;
		m_flDefuseCountDown = 0;
		return;
	}
	if ( !(CBaseEntity*)m_pBombDefuser || m_pBombDefuser->pev->deadflag != DEAD_NO )
	{
		if ( (CBaseEntity*)m_pBombDefuser )
		{
			( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->ResetMaxSpeed();
			( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->m_bIsDefusing = false;
		}
		m_bStartDefuse = false;
		m_pBombDefuser = NULL;
		return;
	}
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );
		WRITE_BYTE( DRC_CMD_EVENT );
		WRITE_SHORT( m_pBombDefuser->entindex() );
		WRITE_SHORT( 0 );
		WRITE_LONG( 15 | DRC_FLAG_FINAL | DRC_FLAG_FACEPLAYER | DRC_FLAG_DRAMATIC );
	MESSAGE_END();
	UTIL_LogPrintf( "\"%s<%i><%s><CT>\" triggered \"Defused_The_Bomb\"\n",
		STRING( m_pBombDefuser->pev->netname ), GETPLAYERUSERID( m_pBombDefuser->edict() ),
		GETPLAYERAUTHID( m_pBombDefuser->edict() ) );
	UTIL_EmitAmbientSound( ENT( pev ), pev->origin, "weapons/c4_beep5.wav", 0, 0, SND_STOP, 0 );
	EMIT_SOUND( m_pBombDefuser->edict(), CHAN_WEAPON, "weapons/c4_disarmed.wav", 1, ATTN_NORM );
	UTIL_Remove( this );
	m_bJustBlew = true;
	( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->ResetMaxSpeed();
	( (CBasePlayer*)(CBaseEntity*)m_pBombDefuser )->m_bIsDefusing = false;
	m_pBombDefuser = NULL;
	m_bStartDefuse = false;
}

//======================end grenade

