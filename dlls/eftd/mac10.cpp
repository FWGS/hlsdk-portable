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

enum mac10_e
{
	MAC10_LONGIDLE = 0,
	MAC10_IDLE1,
	MAC10_LAUNCH,
	MAC10_RELOAD,
	MAC10_DEPLOY,
	MAC10_FIRE1,
	MAC10_FIRE2,
	MAC10_FIRE3,
};


LINK_ENTITY_TO_CLASS(weapon_mac10, CMac10);

void CMac10::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_mac10.mdl");
	m_iId = WEAPON_MAC10;

	m_iDefaultAmmo = MAC10_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMac10::Precache(void)
{
	PRECACHE_MODEL("models/v_mac10.mdl");
	PRECACHE_MODEL("models/w_mac10.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_mac10clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/mac10clipinsert1.wav");
	PRECACHE_SOUND("items/mac10cliprelease1.wav");

	PRECACHE_SOUND("weapons/mac1.wav");
	PRECACHE_SOUND("weapons/mac2.wav");
	PRECACHE_SOUND("weapons/mac3.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usMac10 = PRECACHE_EVENT(1, "events/mac10.sc");
}

int CMac10::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "mac10";
	p->iMaxAmmo1 = MAC10_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MAC10_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MAC10;
	p->iWeight = MAC10_WEIGHT;

	return 1;
}

BOOL CMac10::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CMac10::Deploy()
{
	return DefaultDeploy("models/v_mac10.mdl", "models/p_9mmAR.mdl", MAC10_DEPLOY, "mac10");
}

void CMac10::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;
	Vector vecSpread;

	vecSpread = Vector( m_pPlayer->m_flBulletSpreadCoefficient * 1.2f + 0.03f, m_pPlayer->m_flBulletSpreadCoefficient * 0.9f + 0.02f, 0 );

	if( m_pPlayer->m_flBulletSpreadCoefficient < 0.06f )
		m_pPlayer->m_flBulletSpreadCoefficient += 0.0027f;

	if( m_pPlayer->m_flBulletSpreadCoefficient < 0.1f )
		m_pPlayer->m_flBulletSpreadCoefficient += 0.002f;

	// single player spread
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_MAC10, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usMac10, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.075f);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.075f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CMac10::Reload(void)
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == MAC10_MAX_CLIP )
		return;

	DefaultReload(MAC10_MAX_CLIP, MAC10_RELOAD, 3.2);
}


void CMac10::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = MAC10_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = MAC10_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}


class CMac10AmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_mac10clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_mac10clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_MAC10_GIVE, "mac10", MAC10_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_mac10, CMac10AmmoClip);
