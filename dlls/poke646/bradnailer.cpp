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
#include "nail.h"

enum bradnailer_e {
	BRADNAILER_IDLE1 = 0,
	BRADNAILER_IDLE2,
	BRADNAILER_IDLE3,
	BRADNAILER_SHOOT,
	BRADNAILER_SHOOT_EMPTY,
	BRADNAILER_RELOAD,
	BRADNAILER_RELOAD_NOT_EMPTY,
	BRADNAILER_DRAW,
	BRADNAILER_HOLSTER,
	BRADNAILER_ADD_SILENCER,
	BRADNAILER_UPRIGHT_TO_TILT,
	BRADNAILER_TILT_TO_UPRIGHT,
	BRADNAILER_FASTSHOOT,
};

LINK_ENTITY_TO_CLASS(weapon_bradnailer, CBradnailer);

void CBradnailer::Spawn()
{
	Precache();
	m_iId = WEAPON_BRADNAILER;
	SET_MODEL(ENT(pev), "models/w_bradnailer.mdl");

	m_iDefaultAmmo = BRADNAILER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CBradnailer::Precache(void)
{
	PRECACHE_MODEL("models/v_bradnailer.mdl");
	PRECACHE_MODEL("models/w_bradnailer.mdl");
	PRECACHE_MODEL("models/p_bradnailer.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/bradnailer.wav");

	m_usFireBradnailer = PRECACHE_EVENT(1, "events/bradnailer.sc");
	m_usReload = PRECACHE_EVENT(1, "events/reload.sc");

	UTIL_PrecacheOther( "nail" );
}

int CBradnailer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "nails";
	p->iMaxAmmo1 = NAILS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BRADNAILER_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BRADNAILER;
	p->iWeight = BRADNAILER_WEIGHT;

	return 1;
}

BOOL CBradnailer::Deploy()
{
	return DefaultDeploy("models/v_bradnailer.mdl", "models/p_bradnailer.mdl", BRADNAILER_DRAW, "bradnailer", 0);
}

BOOL CBradnailer::CanHolster(void)
{
	return (m_fInAttack == 0);
}

void CBradnailer::Holster(int skiplocal /*= 0*/)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(BRADNAILER_HOLSTER);

	m_fInAttack = 0;
}

void CBradnailer::SecondaryAttack(void)
{
	if (m_iClip <= 0)
	{
		Reload();
		m_flNextSecondaryAttack = 0.15f;
		return;
	}

	if (m_flNextSecondaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack == 0)
	{
#ifndef CLIENT_DLL
		//ALERT(at_console, "BRADNAILER_UPRIGHT_TO_TILT\n" );
#endif

		SendWeaponAnim(BRADNAILER_UPRIGHT_TO_TILT, UseDecrement());

		m_fInAttack = 1;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
		m_flNextSecondaryAttack = GetNextAttackDelay(0.7);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.0;
		return;
	}
	else if (m_fInAttack == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
#ifndef CLIENT_DLL
			//ALERT(at_console, "Fire\n");
#endif
			Fire(0.05, 0.2, FALSE, TRUE);

			m_fInAttack = 1;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.2f;
			m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		}
	}
}

void CBradnailer::PrimaryAttack(void)
{
	if (m_fInAttack != 0)
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	Fire(0.01f, 0.3f, TRUE, FALSE);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.3f);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CBradnailer::Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL fFastShoot)
{
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

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	// Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecSrc;
	Vector vecAiming;

	if (fFastShoot)
	{
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 2 + gpGlobals->v_up * -2;
	}
	else
	{
		vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 8 + gpGlobals->v_right * 3 + gpGlobals->v_up * -2;
	}

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_NAIL1, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireBradnailer, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, fFastShoot, 0);
	
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

}


void CBradnailer::Reload(void)
{
	if (m_fInAttack != 0)
		return;

	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_pPlayer->ammo_nails <= 0)
		return;

	int iAnim = (m_iClip == 0) ? BRADNAILER_RELOAD : BRADNAILER_RELOAD_NOT_EMPTY;

	int iResult = DefaultReload(BRADNAILER_MAX_CLIP, iAnim, 2.3);

	if (iResult)
	{
		m_fInAttack = 0;

		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usReload, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, iAnim, 0, 0, 0);
	}
}

BOOL CBradnailer::ShouldWeaponIdle(void)
{
	return (m_iClip == 0) || ((m_fInAttack != 0) && (m_pPlayer->pev->button & IN_ATTACK));
}

void CBradnailer::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack != 0)
	{
#ifndef CLIENT_DLL
		//ALERT(at_console, "BRADNAILER_TILT_TO_UPRIGHT\n");
#endif

		SendWeaponAnim(BRADNAILER_TILT_TO_UPRIGHT, UseDecrement());

		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.5f);
	}
	else
	{
		// only idle if the slid isn't back
		if (m_iClip != 0)
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

			if (flRand <= 0.3 + 0 * 0.75)
			{
				iAnim = BRADNAILER_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
			}
			else if (flRand <= 0.6 + 0 * 0.875)
			{
				iAnim = BRADNAILER_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
			}
			else
			{
				iAnim = BRADNAILER_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
			}
			SendWeaponAnim(iAnim, 1);
		}
	}
}
