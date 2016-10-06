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

enum holster_e {
	HOLSTER_IDLE = 0,
	HOLSTER_DRAW,
	HOLSTER_HOLSTER,
	HOLSTER_ATTACK1HIT,
	HOLSTER_ATTACK1MISS,
	HOLSTER_ATTACK2MISS,
	HOLSTER_ATTACK2HIT,
	HOLSTER_ATTACK3MISS,
	HOLSTER_ATTACK3HIT,
	HOLSTER_IDLE2,
	HOLSTER_IDLE3,
};

LINK_ENTITY_TO_CLASS(weapon_holster, CHolster);

void CHolster::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_holster.mdl");
	m_iId = WEAPON_HOLSTER;
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CHolster::Precache(void)
{
	PRECACHE_MODEL("models/v_holster.mdl");
	PRECACHE_MODEL("models/w_holster.mdl");
	PRECACHE_MODEL("models/p_holster.mdl");
}

int CHolster::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_HOLSTER;
	p->iWeight = HOLSTER_WEIGHT;

	return 1;
}

int CHolster::AddToPlayer(CBasePlayer *pPlayer)
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

BOOL CHolster::Deploy()
{
	return DefaultDeploy("models/v_holster.mdl", "models/p_holster.mdl", HOLSTER_DRAW, "holster");
}

void CHolster::Holster(int skiplocal /*= 0*/)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CHolster::WeaponIdle(void)
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(HOLSTER_IDLE);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}