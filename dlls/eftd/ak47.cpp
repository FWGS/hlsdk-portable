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

enum ak47_e
{
	AK47_LONGIDLE = 0,
	AK47_IDLE1,
	AK47_LAUNCH,
	AK47_RELOAD,
	AK47_DEPLOY,
	AK47_FIRE1,
	AK47_FIRE2,
	AK47_FIRE3,
};


LINK_ENTITY_TO_CLASS(weapon_ak47, CAK47);

void CAK47::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_ak47.mdl");
	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = AK47_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAK47::Precache(void)
{
	PRECACHE_MODEL("models/v_ak47.mdl");
	PRECACHE_MODEL("models/w_ak47.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_ak47clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/akclipinsert1.wav");
	PRECACHE_SOUND("items/akcliprelease1.wav");

	PRECACHE_SOUND("weapons/ak1.wav");
	PRECACHE_SOUND("weapons/ak2.wav");
	PRECACHE_SOUND("weapons/ak3.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usAK47 = PRECACHE_EVENT(1, "events/ak47.sc");
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ak47";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

BOOL CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAK47::Deploy()
{
	return DefaultDeploy("models/v_ak47.mdl", "models/p_9mmAR.mdl", AK47_DEPLOY, "ak47");
}

void CAK47::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
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

	// Allow for higher accuracy when the player is crouching.
	if( m_pPlayer->pev->flags & FL_DUCKING )
	{
		vecSpread = Vector( m_pPlayer->m_flBulletSpreadCoefficient * 1.6 + 0.01, m_pPlayer->m_flBulletSpreadCoefficient * 1.4 + 0.01, 0 );
		if( m_pPlayer->m_flBulletSpreadCoefficient < 0.045 )
			m_pPlayer->m_flBulletSpreadCoefficient += 0.006;
	}
	else
	{
		if( m_pPlayer->pev->button & IN_JUMP )
		{
			vecSpread = Vector( m_pPlayer->m_flBulletSpreadCoefficient * 2.4 + 0.065, m_pPlayer->m_flBulletSpreadCoefficient * 2.1 + 0.05, 0 );
			if( m_pPlayer->m_flBulletSpreadCoefficient < 0.08 )
				m_pPlayer->m_flBulletSpreadCoefficient += 0.009;
		}
		else
		{
			vecSpread = Vector( m_pPlayer->m_flBulletSpreadCoefficient * 8.0 + 0.3, m_pPlayer->m_flBulletSpreadCoefficient * 6.0 + 0.2, 0 );
			if( m_pPlayer->m_flBulletSpreadCoefficient < 0.1 )
				m_pPlayer->m_flBulletSpreadCoefficient += 0.012;
		}
	}

	// single player spread
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usAK47, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CAK47::Reload(void)
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == AK47_MAX_CLIP )
		return;

	DefaultReload(AK47_MAX_CLIP, AK47_RELOAD, 2.0);
}


void CAK47::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = AK47_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = AK47_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}


class CAK47AmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_ak47clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_ak47clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_AK47_GIVE, "ak47", AK47_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_ak47, CAK47AmmoClip);
