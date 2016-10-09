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

enum taurus_e {
	TAURUS_IDLE1 = 0,
	TAURUS_IDLE2,
	TAURUS_IDLE3,
	TAURUS_SHOOT,
	TAURUS_SHOOT2,
	TAURUS_SHOOT3,
	TAURUS_SHOOT_EMPTY,
	TAURUS_RELOAD,
	TAURUS_RELOAD2,
	TAURUS_DRAW,
	TAURUS_DRAW2,
};

LINK_ENTITY_TO_CLASS(weapon_th_taurus, CTaurus);

void CTaurus::Spawn()
{
	Precache();
	m_iId = WEAPON_TAURUS;
	SET_MODEL(ENT(pev), "models/w_taurus.mdl");

	m_iDefaultAmmo = TAURUS_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CTaurus::Precache(void)
{
	PRECACHE_MODEL("models/v_taurus.mdl");
	PRECACHE_MODEL("models/w_taurus.mdl");
	PRECACHE_MODEL("models/p_taurus.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/tau_back.wav");
	PRECACHE_SOUND("weapons/tau_clipin.wav");
	PRECACHE_SOUND("weapons/tau_clipout.wav");
	PRECACHE_SOUND("weapons/tau_fire.wav");
	PRECACHE_SOUND("weapons/tau_release.wav");

	m_usFireTaurus = PRECACHE_EVENT(1, "events/taurus.sc");
}

int CTaurus::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "taurus";
	p->iMaxAmmo1 = TAURUS_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TAURUS_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TAURUS;
	p->iWeight = TAURUS_WEIGHT;

	return 1;
}

BOOL CTaurus::Deploy()
{
	return DefaultDeploy("models/v_taurus.mdl", "models/p_taurus.mdl", TAURUS_DRAW2, "taurus", 0);
}

void CTaurus::PrimaryAttack(void)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.28);
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

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_TAURUS, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireTaurus, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.28);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CTaurus::Reload(void)
{
	if (m_pPlayer->ammo_taurus <= 0)
		return;

	int iResult = DefaultReload(TAURUS_MAX_CLIP, TAURUS_RELOAD2, 2.0);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}

void CTaurus::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = TAURUS_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 46.0 / 18.0;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = TAURUS_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 46.0 / 16.0;
		}
		else
		{
			iAnim = TAURUS_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 46.0 / 20.0;
		}
		SendWeaponAnim(iAnim, 1);
	}
}








class CTaurusAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_taurusclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_taurusclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_TAURUS_GIVE, "taurus", TAURUS_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_th_taurus, CTaurusAmmo);