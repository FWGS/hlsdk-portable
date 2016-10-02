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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

enum colts_e {
	COLTS_IDLE1 = 0,
	COLTS_IDLE2,
	COLTS_FIDGET,
	COLTS_DRAW,
	COLTS_LEFTFIRE,
	COLTS_RIGHTFIRE,
	COLTS_DUALFIRE,
	COLTS_RELOAD,
	COLTS_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_colts, CColts);

int CColts::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "colts";
	p->iMaxAmmo1 = COLTS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = COLTS_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_COLTS;
	p->iWeight = COLTS_WEIGHT;

	return 1;
}

int CColts::AddToPlayer(CBasePlayer *pPlayer)
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

void CColts::Spawn()
{
	Precache();
	m_iId = WEAPON_COLTS;
	SET_MODEL(ENT(pev), "models/w_colts.mdl");

	m_iDefaultAmmo = COLTS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CColts::Precache(void)
{
	PRECACHE_MODEL("models/v_colts.mdl");
	PRECACHE_MODEL("models/w_colts.mdl");
	PRECACHE_MODEL("models/p_colts.mdl");

	PRECACHE_MODEL("models/w_coltsbox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/coltsfire1.wav");
	PRECACHE_SOUND("weapons/coltsfire2.wav");
	PRECACHE_SOUND("weapons/coltsreload1.wav");
	PRECACHE_SOUND("weapons/colts_dryfire.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usColts = PRECACHE_EVENT(1, "events/colts.sc");
}

BOOL CColts::Deploy()
{
#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy("models/v_colts.mdl", "models/p_colts.mdl", COLTS_DRAW, "colts", UseDecrement());
}


void CColts::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(COLTS_HOLSTER);
}

void CColts::SecondaryAttack(void)
{
	ColtsFire(0.1f, 0.6f, TRUE, FALSE);
}

void CColts::PrimaryAttack()
{
	ColtsFire(0.1f, 0.6f, FALSE, !m_fLeftAttack);
}


void CColts::ColtsFire(float flSpread, float flCycleTime, BOOL fDualFire, BOOL fLeftAttack)
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

	m_iClip = (fDualFire) ? m_iClip - 2 : m_iClip - 1;

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
	m_pPlayer->m_iWeaponFlash  = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	Vector vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(),
		m_usColts,
		0.0,
		(float *)&g_vecZero,
		(float *)&g_vecZero,
		vecDir.x,
		vecDir.y,
		0,
		0,
		fDualFire,
		fLeftAttack);

	m_fLeftAttack = fLeftAttack;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

BOOL CColts::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/colts_dryfire.wav", 0.8, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}


void CColts::Reload(void)
{
	if (m_pPlayer->ammo_colts <= 0)
		return;

	DefaultReload(COLTS_MAX_CLIP, COLTS_RELOAD, 2.0);
}


void CColts::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = COLTS_IDLE1;
		m_flTimeWeaponIdle = (70.0 / 30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = COLTS_IDLE2;
		m_flTimeWeaponIdle = (60.0 / 30.0);
	}
	else
	{
		iAnim = COLTS_FIDGET;
		m_flTimeWeaponIdle = (170.0 / 30.0);
	}

	SendWeaponAnim(iAnim, UseDecrement() ? 1 : 0);
}


class CColtsBoxAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_coltsbox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_coltsbox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_COLTS_GIVE, "colts", COLTS_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_colts, CColtsBoxAmmo);


#endif