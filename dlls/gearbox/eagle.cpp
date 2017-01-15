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

enum eagle_e {
	EAGLE_IDLE1 = 0,
	EAGLE_IDLE2,
	EAGLE_IDLE3,
	EAGLE_IDLE4,
	EAGLE_IDLE5,
	EAGLE_SHOOT,
	EAGLE_SHOOT_EMPTY,
	EAGLE_RELOAD,
	EAGLE_RELOAD_NOT_EMPTY,
	EAGLE_DRAW,
	EAGLE_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_eagle, CEagle);


void CEagle::Spawn()
{
	Precache();
	m_iId = WEAPON_EAGLE;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");

	m_iDefaultAmmo = EAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CEagle::Precache(void)
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav");
	PRECACHE_SOUND("weapons/desert_eagle_reload.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight.wav");
	PRECACHE_SOUND("weapons/desert_eagle_sight2.wav");

	m_usFireEagle = PRECACHE_EVENT(1, "events/eagle.sc");
}

int CEagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = EAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_EAGLE;
	p->iWeight = EAGLE_WEIGHT;

	return 1;
}

BOOL CEagle::Deploy()
{
	return DefaultDeploy("models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", EAGLE_DRAW, "eagle", UseDecrement());
}


void CEagle::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(EAGLE_HOLSTER);

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif

}

void CEagle::SecondaryAttack(void)
{
	m_fSpotActive = !m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NORMAL);
		m_pSpot = NULL;
	}
#endif

	if (m_fSpotActive)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/desert_eagle_sight2.wav", 1, ATTN_NORM);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
}

void CEagle::PrimaryAttack(void)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;
	const Vector& vecSpread = VECTOR_CONE_2DEGREES;

	float flCycleTime;

	if (!m_fSpotActive)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
		flCycleTime = 0.25f;
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
		flCycleTime = 0.5f;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, (m_fSpotActive) ? VECTOR_CONE_1DEGREES : VECTOR_CONE_10DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireEagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	UpdateSpot();
}

void CEagle::Reload(void)
{
	if (m_pPlayer->ammo_357 <= 0)
		return;

	if (m_fSpotActive)
	{
		SecondaryAttack();
	}

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(EAGLE_MAX_CLIP, EAGLE_RELOAD, 1.5);
	else
		iResult = DefaultReload(EAGLE_MAX_CLIP, EAGLE_RELOAD_NOT_EMPTY, 1.5);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}



void CEagle::WeaponIdle(void)
{
	UpdateSpot();

	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.2)
		{
			iAnim = EAGLE_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
		}
		else if (flRand <= 0.4)
		{
			iAnim = EAGLE_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
		}
		else if (flRand <= 0.6)
		{
			iAnim = EAGLE_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6f;
		}
		else if (flRand <= 0.8)
		{
			iAnim = EAGLE_IDLE4;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5f;
		}
		else
		{
			iAnim = EAGLE_IDLE5;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		}
		SendWeaponAnim(iAnim, 1);
	}
}


void CEagle::UpdateSpot(void)
{

#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

		UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);
	}
#endif

}