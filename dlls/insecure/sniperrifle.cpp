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
#include "player.h"

#define VECTOR_CONE_SNIPER Vector(0.00015, 0.00015, 0.00015)

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CSniperRifle);

void CSniperRifle::Spawn()
{
	pev->classname = MAKE_STRING("weapon_sniperrifle"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_SNIPERRIFLE;
	SET_MODEL(ENT(pev), "models/w_m40a1.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}


void CSniperRifle::Precache()
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_bolt2.wav");
	PRECACHE_SOUND("weapons/sniper_fire.wav");
	PRECACHE_SOUND("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload3.wav");
	PRECACHE_SOUND("weapons/sniper_zoom.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usFireSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

int CSniperRifle::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762";
	p->iMaxAmmo1 = _762_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SNIPERRIFLE;
	p->iWeight = SNIPER_WEIGHT;

	return true;
}

BOOL CSniperRifle::Deploy()
{
	return DefaultDeploy("models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "gauss");
}

void CSniperRifle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = false; // cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM);
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(SNIPER_HOLSTER);
}

void CSniperRifle::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
	}
	else if (m_pPlayer->m_iFOV != 18)
	{
		m_pPlayer->m_iFOV = 18;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM);
	m_flNextSecondaryAttack = 0.5;
}

void CSniperRifle::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_flNextPrimaryAttack = 1.75;
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);


	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_SNIPER, 8192, BULLET_PLAYER_762, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireSniper, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, (m_iClip == 0) ? 1 : 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = 2.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CSniperRifle::Reload()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPER_MAX_CLIP)
		return;

	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1.0, ATTN_NORM);
	}

	if (0 != m_iClip)
	{
		if (DefaultReload(SNIPER_MAX_CLIP, SNIPER_RELOAD3, 2.324))
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.324;
		}
	}
	else if (DefaultReload(SNIPER_MAX_CLIP, SNIPER_RELOAD, 2.324))
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 4.102;
		m_flReloadStart = gpGlobals->time;
		m_bReloading = true;
	}
	else
	{
		m_bReloading = false;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.102;
}



void CSniperRifle::WeaponIdle()
{
	//Update autoaim
	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	ResetEmptySound();

	if (m_bReloading && gpGlobals->time >= m_flReloadStart + 2.324)
	{
		SendWeaponAnim(SNIPER_RELOAD2);
		m_bReloading = false;
	}

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (0 != m_iClip)
			SendWeaponAnim(SNIPER_SLOWIDLE);
		else
			SendWeaponAnim(SNIPER_SLOWIDLE2);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.348;
	}
}

class C762Ammo : public CBasePlayerAmmo
{
	void Spawn()
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_m40a1clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache()
	{
		PRECACHE_MODEL("models/w_m40a1clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity* pOther)
	{
		if (pOther->GiveAmmo(AMMO_762CLIP_GIVE, "762", _762_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(ammo_762, C762Ammo);
