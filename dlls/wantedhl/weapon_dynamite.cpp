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

#define	DYNAMITE_PRIMARY_VOLUME		450

enum dynamite_e {
	DYNAMITE_IDLE = 0,
	DYNAMITE_FIDGET,
	DYNAMITE_PINPULL,
	DYNAMITE_THROW1,	// toss
	DYNAMITE_THROW2,	// medium
	DYNAMITE_THROW3,	// hard
	DYNAMITE_HOLSTER,
	DYNAMITE_DRAW,
};

LINK_ENTITY_TO_CLASS(weapon_dynamite, CHandDynamite);

void CHandDynamite::Spawn()
{
	Precache();
	m_iId = WEAPON_DYNAMITE;
	SET_MODEL(ENT(pev), "models/w_dynamite.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_iDefaultAmmo = DYNAMITE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CHandDynamite::Precache(void)
{
	PRECACHE_MODEL("models/w_dynamite.mdl");
	PRECACHE_MODEL("models/v_dynamite.mdl");
	PRECACHE_MODEL("models/p_dynamite.mdl");
}

int CHandDynamite::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "dynamite";
	p->iMaxAmmo1 = DYNAMITE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_DYNAMITE;
	p->iWeight = DYNAMITE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}


BOOL CHandDynamite::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy("models/v_dynamite.mdl", "models/p_dynamite.mdl", DYNAMITE_DRAW, "dynamite");
}

void CHandDynamite::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(DYNAMITE_PINPULL);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	}
}



void CHandDynamite::WeaponIdle(void)
{
	if (m_flReleaseThrow == 0 && m_flStartThrow)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 4;
		if (flVel > 500)
			flVel = 500;

		UTIL_MakeVectors(angThrow);

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 5.0;
		if (time < 0)
			time = 0;

		CDynamite::ShootTimed(m_pPlayer->pev, vecSrc, vecThrow, time);

		if (flVel < 500)
		{
			SendWeaponAnim(DYNAMITE_THROW1);
		}
		else if (flVel < 1000)
		{
			SendWeaponAnim(DYNAMITE_THROW2);
		}
		else
		{
			SendWeaponAnim(DYNAMITE_THROW3);
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim(DYNAMITE_DRAW);
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		m_flReleaseThrow = -1;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			iAnim = DYNAMITE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// how long till we do this again.
		}
		else
		{
			iAnim = DYNAMITE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim(iAnim);
	}
}