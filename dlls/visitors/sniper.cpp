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

enum sniper_e {
	SNIPER_IDLE1 = 0,
	SNIPER_SHOOT1,
	SNIPER_SHOOT2,
	SNIPER_RELOAD,
	SNIPER_DRAW,
};

LINK_ENTITY_TO_CLASS(weapon_sniper, CSniper);

int CSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_SNIPER;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}

int CSniper::AddToPlayer(CBasePlayer *pPlayer)
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

void CSniper::Spawn()
{
	Precache();
	m_iId = WEAPON_SNIPER;
	SET_MODEL(ENT(pev), "models/w_sniper.mdl");

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSniper::Precache(void)
{
	PRECACHE_MODEL("models/v_sniper.mdl");
	PRECACHE_MODEL("models/w_sniper.mdl");
	PRECACHE_MODEL("models/p_sniper.mdl");

	PRECACHE_MODEL("models/w_sniperclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/sniper_fire1.wav");
	PRECACHE_SOUND("weapons/sniper_fire2.wav");
	PRECACHE_SOUND("weapons/sniper_reload1.wav");
	PRECACHE_SOUND("weapons/zoom.wav");

	PRECACHE_SOUND("weapons/scout_bolt.wav");
	PRECACHE_SOUND("weapons/scout_clipin.wav");
	PRECACHE_SOUND("weapons/scout_clipout.wav");

	m_usFireSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

BOOL CSniper::Deploy()
{
	return DefaultDeploy("models/v_sniper.mdl", "models/p_sniper.mdl", SNIPER_DRAW, "sniper", UseDecrement());
}

void CSniper::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_fInZoom)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CSniper::SecondaryAttack(void)
{
	if (m_pPlayer->pev->fov != 0)
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if (m_pPlayer->pev->fov != 40)
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;
	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_STATIC, "weapons/zoom.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 100, 150 ) );
}

void CSniper::PrimaryAttack()
{
	if( FBitSet( m_pPlayer->m_afButtonLast, IN_ATTACK ) )
		return;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
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
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_SNIPER, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireSniper, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 || m_iClip > 0 )
		m_flNextPrimaryAttack = 1.5f;
	else
	{
		m_flNextPrimaryAttack = 2.75f;
		m_flNextSecondaryAttack = 0.75f;
	}

	if( m_iClip > 0 )
		m_flTimeWeaponIdle = 2.5f;
	else
		m_flTimeWeaponIdle = 1.75f;
}


void CSniper::Reload(void)
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPER_MAX_CLIP )
		return;

	int iResult = DefaultReload(SNIPER_MAX_CLIP, SNIPER_RELOAD, 2.25);

	if (iResult)
	{
		if( m_fInZoom )
			SecondaryAttack();
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/sniper_reload1.wav", RANDOM_FLOAT( 0.9, 1.0 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 15 ) );
	}
}


void CSniper::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	SendWeaponAnim(SNIPER_IDLE1, UseDecrement() ? 1 : 0);
}


class CSniperAmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_sniperclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_sniperclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_357BOX_GIVE, "357", _357_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS(ammo_sniper, CSniperAmmoClip);
