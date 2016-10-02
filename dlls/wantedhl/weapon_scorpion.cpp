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
#include "squeakgrenade.h"

enum scorp_e {
	SCORP_IDLE = 0,
	SCORP_FIDGET,
	SCORP_IDLE1,
	SCORP_HOLSTER,
	SCORP_DRAW,
	SCORP_THROW
};


LINK_ENTITY_TO_CLASS(weapon_scorpion, CScorp);

const char* CScorp::pHuntSounds[] =
{
	"scorpion/scorp_hunt1.wav",
	"scorpion/scorp_hunt2.wav",
	"scorpion/scorp_hunt3.wav",
};

void CScorp::Spawn()
{
	Precache();
	m_iId = WEAPON_SCORPION;
	SET_MODEL(ENT(pev), "models/w_scorpion.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = SCORPION_DEFAULT_GIVE;

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}


void CScorp::Precache(void)
{
	PRECACHE_MODEL("models/w_scorpion.mdl");
	PRECACHE_MODEL("models/v_scorpion.mdl");
	PRECACHE_MODEL("models/p_scorpion.mdl");

	PRECACHE_SOUND_ARRAY(pHuntSounds);

	UTIL_PrecacheOther("monster_scorpion");

	m_usScorp = PRECACHE_EVENT(1, "events/scorpion.sc");
}


int CScorp::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Scorpions";
	p->iMaxAmmo1 = SCORPION_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SCORPION;
	p->iWeight = SCORPION_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}



BOOL CScorp::Deploy()
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT(0, 1);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pHuntSounds), 1, ATTN_NORM, 0, 100);

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy("models/v_scorpion.mdl", "models/p_scorpion.mdl", SCORP_DRAW, "scorpion");
}


void CScorp::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_SCORPION);
		SetThink(&CScorp::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim(SCORP_HOLSTER);
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}


void CScorp::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if (m_pPlayer->pev->flags & FL_DUCKING)
		{
			trace_origin = trace_origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
		}

		// find place to toss monster
		UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr);

		int flags;
#ifdef CLIENT_WEAPONS
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif

		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usScorp, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

		if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
			CBaseEntity *pScorpion = CBaseEntity::Create("monster_scorpion", tr.vecEndPos, m_pPlayer->pev->v_angle, m_pPlayer->edict());
			pScorpion->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;
#endif
			// play hunt sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pHuntSounds), 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = GetNextAttackDelay(0.3);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		}
	}
}


void CScorp::SecondaryAttack(void)
{

}


void CScorp::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim(SCORP_DRAW);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.75)
	{
		iAnim = SCORP_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (30.0 / 15);
	}
	else if (flRand <= 0.875)
	{
		iAnim = SCORP_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (30.0 / 17.0);
	}
	else
	{
		iAnim = SCORP_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (30.0 / 17.0);
	}
	SendWeaponAnim(iAnim);
}