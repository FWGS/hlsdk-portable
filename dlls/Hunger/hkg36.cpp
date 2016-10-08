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

enum hkg36_e {
	HKG36_IDLE = 0,
	HKG36_RELOAD,
	HKG36_DRAW,
	HKG36_SHOOT1,
};

LINK_ENTITY_TO_CLASS(weapon_th_sniper, CHKG36);

int CHKG36::GetPrimaryAttackActivity(void)
{
	return HKG36_SHOOT1;
}

int CHKG36::GetZoomedAttackActivity(void)
{
	return HKG36_SHOOT1;
}

int CHKG36::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "sniper";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HKG36;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}

void CHKG36::Spawn()
{
	Precache();
	m_iId = WEAPON_HKG36;
	SET_MODEL(ENT(pev), "models/w_hkg36.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHKG36::Precache(void)
{
	PRECACHE_MODEL("models/v_hkg36.mdl");
	PRECACHE_MODEL("models/w_hkg36.mdl");
	PRECACHE_MODEL("models/p_hkg36.mdl");

	PRECACHE_MODEL("models/w_antidote.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/sniper.wav");
	PRECACHE_SOUND("weapons/ap9_bolt.wav");
	PRECACHE_SOUND("weapons/ap9_clipin.wav");
	PRECACHE_SOUND("weapons/ap9_clipout.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usFireSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

BOOL CHKG36::Deploy()
{
	BOOL bResult = DefaultDeploy("models/v_hkg36.mdl", "models/p_hkg36.mdl", HKG36_DRAW, "hkg36", UseDecrement());

	if ( bResult )
	{
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	}

	return bResult;
}

void CHKG36::Reload(void)
{
	if (m_pPlayer->ammo_sniper <= 0)
		return;

	int iResult = DefaultReload(SNIPER_MAX_CLIP, HKG36_RELOAD, 3.8);

	if (iResult)
	{
		CSniper::Reload();
	}
}

void CHKG36::WeaponIdle(void)
{
	CSniper::WeaponIdle();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	m_flTimeWeaponIdle = 17.0 / 30.0;

	SendWeaponAnim(HKG36_IDLE, UseDecrement());
}