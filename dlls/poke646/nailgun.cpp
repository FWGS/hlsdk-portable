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
#include "nail.h"

enum nailgun_e
{
	NAILGUN_LONGIDLE = 0,
	NAILGUN_IDLE1,
	NAILGUN_LAUNCH,
	NAILGUN_RELOAD,
	NAILGUN_DEPLOY,
	NAILGUN_FIRE1,
	NAILGUN_FIRE2,
	NAILGUN_FIRE3,
	NAILGUN_DEPLOY_EMPTY,
	NAILGUN_LONGIDLE_EMPTY,
	NAILGUN_IDLE1_EMPTY,
};


LINK_ENTITY_TO_CLASS(weapon_nailgun, CNailgun);

void CNailgun::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_nailgun.mdl");
	m_iId = WEAPON_NAILGUN;

	m_iDefaultAmmo = NAILGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CNailgun::Precache(void)
{
	PRECACHE_MODEL("models/v_nailgun.mdl");
	PRECACHE_MODEL("models/w_nailgun.mdl");
	PRECACHE_MODEL("models/p_nailgun.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_nailclip.mdl");
	PRECACHE_MODEL("models/w_nailround.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/nailgun.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usNailgun = PRECACHE_EVENT(1, "events/nailgun.sc");
	m_usReload = PRECACHE_EVENT(1, "events/reload.sc");

	UTIL_PrecacheOther( "nail" );
}

int CNailgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "nails";
	p->iMaxAmmo1 = NAILS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = NAILGUN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 1;
	p->iId = m_iId = WEAPON_NAILGUN;
	p->iWeight = NAILGUN_WEIGHT;

	return 1;
}

int CNailgun::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CNailgun::Deploy()
{
	int iAnim;
	if (m_iClip == 0)
		iAnim = NAILGUN_DEPLOY_EMPTY;
	else
		iAnim = NAILGUN_DEPLOY;

	return DefaultDeploy("models/v_nailgun.mdl", "models/p_nailgun.mdl", iAnim, "nailgun");
}


void CNailgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 8 + gpGlobals->v_right * 4 + gpGlobals->v_up * -4;
	Vector vecAiming = gpGlobals->v_forward;

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_NAIL, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usNailgun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

#ifndef CLIENT_DLL
	CNail *pNail = CNail::NailCreate();
	pNail->pev->origin = vecSrc;
	pNail->pev->angles = anglesAim;
	pNail->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pNail->pev->velocity = vecAiming * NAIL_WATER_VELOCITY;
		pNail->pev->speed = NAIL_WATER_VELOCITY;
	}
	else
	{
		pNail->pev->velocity = vecAiming * NAIL_AIR_VELOCITY;
		pNail->pev->speed = NAIL_AIR_VELOCITY;
	}
	pNail->pev->avelocity.z = 10;
#endif

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CNailgun::Reload(void)
{
	if (m_pPlayer->ammo_nails <= 0)
		return;

	int iAnim = NAILGUN_RELOAD;
	int iResult = DefaultReload(NAILGUN_MAX_CLIP, iAnim, 1.6);

	if (iResult)
	{
		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usReload, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, iAnim, 0, 0, 0);
	}
}


void CNailgun::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = (m_iClip == 0) ? NAILGUN_LONGIDLE_EMPTY : NAILGUN_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = (m_iClip == 0) ? NAILGUN_IDLE1_EMPTY : NAILGUN_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}



class CNailAmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_nailclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_nailclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_NAILCLIP_GIVE, "nails", NAILS_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_nailclip, CNailAmmoClip);


class CNailAmmoRound : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_nailround.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_nailround.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_NAILROUND_GIVE, "nails", NAILS_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_nailround, CNailAmmoRound);