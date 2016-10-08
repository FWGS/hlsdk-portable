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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

enum einar1_e {
	EINAR1_IDLE = 0,
	EINAR1_AIM,
	EINAR1_FIRE,
	EINAR1_DRAW,
	EINAR1_HOLSTER,
	EINAR1_AUTOIDLE,
	EINAR1_AUTOFIRE,
	EINAR1_AUTODRAW,
	EINAR1_AUTOHOLSTER,
};


LINK_ENTITY_TO_CLASS(weapon_einar1, CEinar1);


int CEinar1::GetPrimaryAttackActivity(void)
{
	return EINAR1_AUTOFIRE;
}

int CEinar1::GetZoomedAttackActivity(void)
{
	return EINAR1_FIRE;
}

int CEinar1::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "sniper";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_EINAR1;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}

void CEinar1::Spawn()
{
	Precache();
	m_iId = WEAPON_EINAR1;
	SET_MODEL(ENT(pev), "models/w_isotopebox.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	m_fInZoom = FALSE;
	m_fInAttack = 0;
	m_fInSpecialReload = 0;

	FallInit();// get ready to fall down.
}

void CEinar1::Precache(void)
{
	PRECACHE_MODEL("models/v_tfc_sniper.mdl");
	PRECACHE_MODEL("models/w_isotopebox.mdl");
	PRECACHE_MODEL("models/p_sniper.mdl");

	PRECACHE_MODEL("models/w_antidote.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/sniper.wav");
	PRECACHE_SOUND("weapons/reload3.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usFireSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

BOOL CEinar1::Deploy()
{
	return DefaultDeploy("models/v_tfc_sniper.mdl", "models/p_sniper.mdl", EINAR1_AUTODRAW, "einar1");
}

void CEinar1::Holster(int skiplocal /* = 0 */)
{
	CSniper::Holster(skiplocal);

	m_fInZoom = FALSE;
	m_fInAttack = 0;
	m_fInSpecialReload = 0;

	SendWeaponAnim( EINAR1_AUTOHOLSTER );
}

void CEinar1::PrimaryAttack()
{
	if (m_fInSpecialReload != 0)
		return;

	CSniper::PrimaryAttack();

	m_fInAttack = 1;
	m_fInSpecialReload = 0;
}

void CEinar1::SecondaryAttack(void)
{
	if (m_fInSpecialReload != 0)
		return;

	CSniper::SecondaryAttack();

	m_flNextPrimaryAttack = GetNextAttackDelay(1);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5; // idle pretty soon after shooting.

	m_fInAttack = 1;
	m_fInSpecialReload = 0;
}

void CEinar1::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPER_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_fInZoom)
	{
		SetZoomState(FALSE);
	}

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(EINAR1_AUTOHOLSTER);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.0; // 0.5
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.6);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;
	}
	else
	{
		DefaultReload( SNIPER_MAX_CLIP, EINAR1_AUTODRAW, 0.5);

		m_fInSpecialReload = 0;
	}
}

void CEinar1::WeaponIdle(void)
{
	CSniper::WeaponIdle();

	if (m_fInSpecialReload == 0 && m_fInAttack == 1 && (gpGlobals->time - m_flLastFireTime) > 0.1f)
	{
		SendWeaponAnim(m_fInZoom ? EINAR1_IDLE : EINAR1_AUTOIDLE);
		m_fInAttack = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		Reload();
	}
	else if (m_fInSpecialReload != 0)
	{
		if (m_iClip != SNIPER_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else
		{
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
		}
	}
	else if (m_fInSpecialReload == 0)
	{
		int iAnim;

		if (m_fInZoom)
		{
			iAnim = EINAR1_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (31.0 / 10.0);
		}
		else
		{
			iAnim = EINAR1_AUTOIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (31.0 / 15.0);
		}

		SendWeaponAnim(iAnim, UseDecrement());
	}
}

BOOL CEinar1::ShouldWeaponIdle(void)
{
	return  (m_iClip == 0) || (m_fInSpecialReload != 0);
}