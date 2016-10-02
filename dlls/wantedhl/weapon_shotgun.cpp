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
#include "gamerules.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum dbarrel_e {
	DBARREL_IDLE1 = 0,
	DBARREL_IDLE2,
	DBARREL_IDLE3,
	DBARREL_HOLSTER,
	DBARREL_DRAW,
	DBARREL_SHOOT,
	DBARREL_SHOOT2,
	DBARREL_RELOAD,
};

LINK_ENTITY_TO_CLASS(weapon_shotgun, CShotgun2);

void CShotgun2::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL(ENT(pev), "models/w_shotgun.mdl");

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CShotgun2::Precache(void)
{
	PRECACHE_MODEL("models/v_shotgun.mdl");
	PRECACHE_MODEL("models/w_shotgun.mdl");
	PRECACHE_MODEL("models/p_shotgun.mdl");

	// m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/dbarrel1.wav");//shotgun
	PRECACHE_SOUND("weapons/sbarrel1.wav");//shotgun

	PRECACHE_SOUND("weapons/reload1.wav");	// shotgun reload
	PRECACHE_SOUND("weapons/reload3.wav");	// shotgun reload


	PRECACHE_SOUND("weapons/357_cock1.wav"); // gun empty sound
	PRECACHE_SOUND("weapons/scock1.wav");	// cock gun

	m_usShotgun = PRECACHE_EVENT(1, "events/shotgun.sc");
}

int CShotgun2::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun2::Deploy()
{
	return DefaultDeploy("models/v_shotgun.mdl", "models/p_shotgun.mdl", DBARREL_DRAW, "shotgun");
}

void CShotgun2::PrimaryAttack()
{
	ShotgunFire(0.01f, 0.75f, 1.5f, TRUE, FALSE);
}


void CShotgun2::SecondaryAttack(void)
{
	ShotgunFire(0.01f, 1.5f, 5.0f, TRUE, TRUE);
}


void CShotgun2::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	BOOL fRet = DefaultReload(SHOTGUN_MAX_CLIP, DBARREL_RELOAD, 3.5);

	if (fRet)
	{
		if (RANDOM_LONG(0, 1))
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/reload1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/reload3.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
}


void CShotgun2::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		return;
	
	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.8)
	{
		iAnim = DBARREL_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (90.0 / 30.0);
	}
	else if (flRand <= 0.95)
	{
		iAnim = DBARREL_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (150.0 / 40.0);
	}
	else
	{
		iAnim = DBARREL_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (150.0 / 40.0);
	}

	SendWeaponAnim(iAnim);
}

void CShotgun2::ShotgunFire(float flSpread, float flCycleTime, float flIdleTime, BOOL fUseAutoAim, BOOL fSecondary)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (fSecondary && m_iClip <= 1)
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip = (!fSecondary) ? m_iClip - 1 : m_iClip - 2;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// tuned for deathmatch
		vecDir = m_pPlayer->FireBulletsPlayer(!fSecondary ? 4 : 8, vecSrc, vecAiming, VECTOR_CONE_DM_DOUBLESHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// untouched default single player
		vecDir = m_pPlayer->FireBulletsPlayer(!fSecondary ? 8 : 12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usShotgun, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, !fSecondary ? 1 : 2, 0, fSecondary, 0);

	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.95;

	m_flNextPrimaryAttack = GetNextAttackDelay(flCycleTime);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flIdleTime;
	else
		m_flTimeWeaponIdle = flIdleTime;

	m_fInSpecialReload = 0;
}