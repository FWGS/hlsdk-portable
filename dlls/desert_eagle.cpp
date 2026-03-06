/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
*	This weapon was made by XF-Alien
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum deagle_e {
	DEAGLE_IDLE = 0,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_NOT_EMPTY,
	DEAGLE_DRAW,
	DEAGLE_DRAW_DRY,
	DEAGLE_HOLSTER,
	DEAGLE_HOLSTER_DRY,
};

LINK_ENTITY_TO_CLASS(weapon_44desert_eagle, CDeagle);


void CDeagle::Spawn()
{
	pev->classname = MAKE_STRING("weapon_44desert_eagle"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_44DESERT_EAGLE;
	SET_MODEL(ENT(pev), "models/w_44_desert_eagle.mdl");

	m_iDefaultAmmo = RANDOM_LONG(5, 8);

	FallInit();// get ready to fall down.
}

void CDeagle::Precache(void)
{
	PRECACHE_MODEL("models/v_44_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_44_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_44_desert_eagle.mdl");

	m_iShell = PRECACHE_MODEL("models/44shell.mdl");// brass shell

	PRECACHE_SOUND("items/44insert1.wav");
	PRECACHE_SOUND("items/44release1.wav");
	PRECACHE_SOUND("items/9mmclip_pickup.wav");

	PRECACHE_SOUND("weapons/44_gun_fire.wav");//handgun

	PRECACHE_SOUND("fvox/ammo_low.wav");

	m_usFireDeagle1 = PRECACHE_EVENT(1, "events/44desert_eagle1.sc");
	m_usFireDeagle2 = PRECACHE_EVENT(1, "events/44desert_eagle2.sc");
}

int CDeagle::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "44";
	p->iMaxAmmo1 = _44_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_44DESERT_EAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

int CDeagle::AddToPlayer(CBasePlayer* pPlayer)
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

BOOL CDeagle::Deploy()
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.6;

	if (m_iClip <= 0)
	{
		return DefaultDeploy("models/v_44_desert_eagle.mdl", "models/p_44_desert_eagle.mdl", DEAGLE_DRAW_DRY, "onehanded", /*UseDecrement() ? 1 : 0*/ 0);
	}
	else
	{
		return DefaultDeploy("models/v_44_desert_eagle.mdl", "models/p_44_desert_eagle.mdl", DEAGLE_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0);
	}
}

void CDeagle::Holster(int skiplocal /* = 0 */)
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.35;
	if (m_iClip <= 0)
	{
		SendWeaponAnim(DEAGLE_HOLSTER_DRY);
	}
	else
	{
		SendWeaponAnim(DEAGLE_HOLSTER);
	}
}

void CDeagle::PrimaryAttack(void)
{
	DeagleFire(0.01, 0.5, TRUE);
}

void CDeagle::SecondaryAttack(void)
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
	}

	DeagleFire(0.1, 0.3, TRUE);
}

void CDeagle::DeagleFire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.5;
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	if (m_iClip == 2)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

#ifndef CLIENT_DLL

	UTIL_ScreenShake(pev->origin, 6, 150.0, 0.25, 120);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);	// player "shoot" animation

#endif

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_44, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireDeagle1 : m_usFireDeagle2, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CDeagle::Reload(void)
{
	int iResult;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == DEAGLE_MAX_CLIP)
	{
		return;
	}
	else
	{
		if (m_iClip == 0)
		{
			iResult = DefaultReload(8, DEAGLE_RELOAD, 1.2);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.9;
		}
		else
		{
			iResult = DefaultReload(8, DEAGLE_RELOAD_NOT_EMPTY, 1.2);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.55;
		}
	}

	if (iResult)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

}

void CDeagle::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iClip != 0)
	{
		SendWeaponAnim(DEAGLE_IDLE);

		m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
	}
}

class CDeagleAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_44clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_44clip.mdl");
		PRECACHE_SOUND("items/9mmclip_pickup.wav");
	}
	BOOL AddAmmo(CBaseEntity* pOther)
	{
		if (pOther->GiveAmmo(AMMO_DEAGLECLIP_GIVE, "44", _44_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip_pickup.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_44clip, CDeagleAmmo);
