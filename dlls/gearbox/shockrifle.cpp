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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


enum shockrifle_e {
	SHOCK_IDLE1 = 0,
	SHOCK_FIRE,
	SHOCK_DRAW,
	SHOCK_HOLSTER,
	SHOCK_IDLE3,
};

LINK_ENTITY_TO_CLASS(weapon_shockrifle, CShockrifle);

void CShockrifle::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOCKRIFLE;
	SET_MODEL(ENT(pev), "models/w_shock.mdl");

	m_iDefaultAmmo = SHOCKRIFLE_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.

	m_fShouldUpdateEffects = FALSE;
	m_flBeamLifeTime = 0.0f;
}


void CShockrifle::Precache(void)
{
	PRECACHE_MODEL("models/v_shock.mdl");
	PRECACHE_MODEL("models/w_shock.mdl");
	PRECACHE_MODEL("models/p_shock.mdl");

	PRECACHE_SOUND("weapons/shock_discharge.wav");
	PRECACHE_SOUND("weapons/shock_draw.wav");
	PRECACHE_SOUND("weapons/shock_fire.wav");
	PRECACHE_SOUND("weapons/shock_impact.wav");
	PRECACHE_SOUND("weapons/shock_recharge.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.

	m_usShockFire = PRECACHE_EVENT(1, "events/shock.sc");

	UTIL_PrecacheOther("shock");
}

int CShockrifle::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{

#ifndef CLIENT_DLL
		if (g_pGameRules->IsMultiplayer())
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = SHOCK_MAX_CARRY;
		}
#endif

		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CShockrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Shocks";
	p->iMaxAmmo1 = SHOCK_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 6;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_SHOCKRIFLE;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}


BOOL CShockrifle::Deploy()
{
	return DefaultDeploy("models/v_shock.mdl", "models/p_shock.mdl", SHOCK_DRAW, "shockrifle");
}

void CShockrifle::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(SHOCK_HOLSTER);

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}


void CShockrifle::PrimaryAttack()
{
	Reload();

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
	{
		return;
	}

#ifndef CLIENT_DLL
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	Vector vecSrc;

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc = vecSrc + gpGlobals->v_forward * 8;
	vecSrc = vecSrc + gpGlobals->v_right * 8;
	vecSrc = vecSrc + gpGlobals->v_up * -12;

	CBaseEntity *pShock = CBaseEntity::Create("shock", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict());
	pShock->pev->velocity = gpGlobals->v_forward * 900;

	m_flRechargeTime = gpGlobals->time + 0.5;
#endif

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;


	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usShockFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

	if (!m_fShouldUpdateEffects)
	{
		// Toggle need to show effects.
		m_fShouldUpdateEffects = TRUE;
		m_flBeamLifeTime = gpGlobals->time + 1.0f;
	}
	else
	{
		UpdateEffects();
		m_flBeamLifeTime = gpGlobals->time + 0.5f;
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.2);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}



void CShockrifle::SecondaryAttack(void)
{
}

void CShockrifle::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= SHOCK_MAX_CARRY)
		return;

	while (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < SHOCK_MAX_CARRY && m_flRechargeTime < gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shock_recharge.wav", 1, ATTN_NORM);

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime += 0.5;
	}
}


void CShockrifle::WeaponIdle(void)
{
	Reload();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = SHOCK_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.3f;
	}
	else
	{
		iAnim = SHOCK_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.4f;
	}
	SendWeaponAnim(iAnim);
}

void CShockrifle::UpdateEffects()
{
	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usShockFire,
		0.0,
		(float *)&g_vecZero,
		(float *)&g_vecZero,
		0.0,
		0.0,
		TRUE,
		0,
		0,
		0);
}

void CShockrifle::ItemPostFrame(void)
{
	CBasePlayerWeapon::ItemPostFrame();

	if (!m_pPlayer->pev->button & IN_ATTACK)
	{
		if (m_fShouldUpdateEffects)
		{
			if (gpGlobals->time <= m_flBeamLifeTime)
			{
				UpdateEffects();
			}
			else
			{
				m_fShouldUpdateEffects = FALSE;
				m_flBeamLifeTime = 0.0f;
			}
		}
	}
}




#endif
