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

LINK_ENTITY_TO_CLASS(spore, CSpore)

TYPEDESCRIPTION	CSpore::m_SaveData[] =
{
	DEFINE_FIELD(CSpore, m_SporeType, FIELD_INTEGER),
	DEFINE_FIELD(CSpore, m_flIgniteTime, FIELD_TIME),
	DEFINE_FIELD(CSpore, m_bIsAI, FIELD_BOOLEAN),
	DEFINE_FIELD(CSpore, m_hSprite, FIELD_EHANDLE)
};

IMPLEMENT_SAVERESTORE(CSpore, CGrenade)

int gSporeExplode, gSporeExplodeC;

void CSpore::Precache(void)
{
	PRECACHE_MODEL("models/spore.mdl");
	PRECACHE_MODEL("sprites/glow01.spr");

	m_iBlow = PRECACHE_MODEL("sprites/spore_exp_01.spr");
	m_iBlowSmall = PRECACHE_MODEL("sprites/spore_exp_c_01.spr");
	m_iSpitSprite = m_iTrail = PRECACHE_MODEL("sprites/tinyspit.spr");

	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
}

void CSpore::Spawn()
{
	Precache();

	if (m_SporeType == GRENADE)
		pev->movetype = MOVETYPE_BOUNCE;
	else
		pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;

	SET_MODEL(edict(), "models/spore.mdl");

	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	UTIL_SetOrigin(pev, pev->origin);

	SetThink(&CSpore::FlyThink);

	if (m_SporeType == GRENADE)
	{
		SetTouch(&CSpore::MyBounceTouch);

		if (!m_bPuked)
		{
			pev->angles.x -= RANDOM_LONG(-5, 5) + 30;
		}
	}
	else
	{
		SetTouch(&CSpore::RocketTouch);
	}

	UTIL_MakeVectors(pev->angles);

	if (!m_bIsAI)
	{
		if (m_SporeType != GRENADE)
		{
			pev->velocity = gpGlobals->v_forward * 1200;
		}

		pev->gravity = 1;
	}
	else
	{
		pev->gravity = 0.5;
		pev->friction = 0.7;
	}

	pev->dmg = gSkillData.plrDmgSpore;

	m_flIgniteTime = gpGlobals->time;

	pev->nextthink = gpGlobals->time + 0.01;

	CSprite* sprite = CSprite::SpriteCreate("sprites/glow01.spr", pev->origin, false);
	if (sprite) {
		sprite->SetTransparency(kRenderTransAdd, 180, 180, 40, 100, kRenderFxDistort);
		sprite->SetScale(0.8);
		sprite->SetAttachment(edict(), 0);
		m_hSprite = sprite;
	}

	m_fRegisteredSound = false;

	m_flSoundDelay = gpGlobals->time;
}

void CSpore::BounceSound()
{
	//Nothing
}

void CSpore::IgniteThink()
{
	SetThink(NULL);
	SetTouch(NULL);

	if (m_hSprite)
	{
		UTIL_Remove(m_hSprite);
		m_hSprite = 0;
	}

	EMIT_SOUND(edict(), CHAN_WEAPON, "weapons/splauncher_impact.wav", VOL_NORM, ATTN_NORM);

	const Vector vecDir = pev->velocity.Normalize();

	TraceResult tr;

	UTIL_TraceLine(
		pev->origin, pev->origin + vecDir * (m_SporeType == GRENADE ? 64 : 32),
		dont_ignore_monsters, edict(), &tr);

	UTIL_DecalTrace(&tr, DECAL_SPR_SPLT1 + RANDOM_LONG(0, 2));

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( tr.vecPlaneNormal.x );
	WRITE_COORD( tr.vecPlaneNormal.y );
	WRITE_COORD( tr.vecPlaneNormal.z );
	WRITE_SHORT(m_iSpitSprite);
	WRITE_BYTE(100);
	WRITE_BYTE(40);
	WRITE_BYTE(180);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE(10);
	WRITE_BYTE(15);
	WRITE_BYTE(220);
	WRITE_BYTE(40);
	WRITE_BYTE(5);
	WRITE_BYTE(10);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_SHORT(RANDOM_LONG(0, 1) ? m_iBlow : m_iBlowSmall);
	WRITE_BYTE(20);
	WRITE_BYTE(128);
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD(RANDOM_FLOAT(-1, 1));
	WRITE_COORD(1);
	WRITE_COORD(RANDOM_FLOAT(-1, 1));
	WRITE_SHORT(m_iTrail);
	WRITE_BYTE(2);
	WRITE_BYTE(20);
	WRITE_BYTE(80);
	MESSAGE_END();

	::RadiusDamage(pev->origin, pev, VARS(pev->owner), pev->dmg, 200, CLASS_NONE, DMG_ALWAYSGIB | DMG_BLAST);

	SetThink(&CSpore::SUB_Remove);

	pev->nextthink = gpGlobals->time;
}

void CSpore::FlyThink()
{
	const float flDelay = m_bIsAI ? 4.0 : 2.0;

	if (m_SporeType != GRENADE || (gpGlobals->time <= m_flIgniteTime + flDelay))
	{
		Vector velocity = pev->velocity.Normalize();

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( velocity.x );
		WRITE_COORD( velocity.y );
		WRITE_COORD( velocity.z );
		WRITE_SHORT(m_iTrail);
		WRITE_BYTE(2);
		WRITE_BYTE(20);
		WRITE_BYTE(80);
		MESSAGE_END();
	}
	else
	{
		SetThink(&CSpore::IgniteThink);
	}

	pev->nextthink = gpGlobals->time + 0.03;
}

void CSpore::GibThink()
{
	//Nothing
}

void CSpore::RocketTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage != DAMAGE_NO)
	{
		pOther->TakeDamage(pev, VARS(pev->owner), gSkillData.plrDmgSpore, DMG_GENERIC);
	}

	IgniteThink();
}

void CSpore::MyBounceTouch(CBaseEntity* pOther)
{
	if (pOther->pev->takedamage == DAMAGE_NO)
	{
		if (pOther->edict() != pev->owner)
		{
			if (gpGlobals->time > m_flSoundDelay)
			{
				CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, (int)(pev->dmg * 2.5f), 0.3);

				m_flSoundDelay = gpGlobals->time + 1.0;
			}

			if ((pev->flags & FL_ONGROUND) != 0)
			{
				pev->velocity = pev->velocity * 0.5;
			}
			else
			{
				EMIT_SOUND_DYN(edict(), CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.25, ATTN_NORM, 0, PITCH_NORM);
			}
		}
	}
	else
	{
		pOther->TakeDamage(pev, VARS(pev->owner), gSkillData.plrDmgSpore, DMG_GENERIC);

		IgniteThink();
	}
}

CSpore* CSpore::CreateSpore(const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner, SporeType sporeType, bool bIsAI, bool bPuked)
{
	CSpore* pSpore = GetClassPtr<CSpore>(NULL);

	UTIL_SetOrigin(pSpore->pev, vecOrigin);

	pSpore->m_SporeType = sporeType;

	if (bIsAI)
	{
		pSpore->pev->velocity = vecAngles;

		pSpore->pev->angles = UTIL_VecToAngles(vecAngles);
	}
	else
	{
		pSpore->pev->angles = vecAngles;
	}

	pSpore->m_bIsAI = bIsAI;
	pSpore->m_bPuked = bPuked;

	pSpore->Spawn();

	pSpore->pev->owner = pOwner->edict();
	pSpore->pev->classname = MAKE_STRING("spore");

	return pSpore;
}

void CSpore::UpdateOnRemove()
{
	CGrenade::UpdateOnRemove();
	if (m_hSprite)
	{
		UTIL_Remove(m_hSprite);
		m_hSprite = 0;
	}
}
