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

enum ap9_e
{
	AP9_IDLE = 0,
	AP9_RELOAD,
	AP9_DRAW,
	AP9_SHOOT1,
	AP9_SHOOT2,
	AP9_SHOOT3
};

LINK_ENTITY_TO_CLASS( weapon_th_ap9, CWeaponEinarAP9 )

void CWeaponEinarAP9::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_ap9.mdl" );
	m_iId = WEAPON_AP9;

	m_iDefaultAmmo = AP9_DEFAULT_GIVE;

	m_iBurstShots = 0;

	FallInit();// get ready to fall down.
}

void CWeaponEinarAP9::Precache()
{
	PRECACHE_MODEL( "models/v_ap9.mdl" );
	PRECACHE_MODEL( "models/w_ap9.mdl" );
	PRECACHE_MODEL( "models/p_ap9.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shellTE_MODEL

	PRECACHE_MODEL( "models/w_ap9clip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/ap9_bolt.wav" );
	PRECACHE_SOUND( "weapons/ap9_clipin.wav" );
	PRECACHE_SOUND( "weapons/ap9_clipout.wav" );
	PRECACHE_SOUND( "weapons/ap9_fire.wav" );

	m_usFireAP9 = PRECACHE_EVENT( 1, "events/ap9.sc" );
}

int CWeaponEinarAP9::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "ap9";
	p->iMaxAmmo1 = AP9_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AP9_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AP9;
	p->iWeight = AP9_WEIGHT;

	return 1;
}

int CWeaponEinarAP9::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarAP9::Deploy()
{
	return DefaultDeploy( "models/v_ap9.mdl", "models/p_ap9.mdl", AP9_DRAW, "onehanded" );
}

void CWeaponEinarAP9::SecondaryAttack()
{
	m_iBurstShots += 2;
	AP9Fire( 0.035f, 0.6f, FALSE );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.04f;
}

void CWeaponEinarAP9::PrimaryAttack()
{
	AP9Fire( 0.07f, 0.125f, TRUE );
}

void CWeaponEinarAP9::AP9Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	int iBulletType;

	if( m_iClip <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		}
		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;
	
	if( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

#ifdef CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		iBulletType = BULLET_PLAYER_9MM;
	else
		iBulletType = BULLET_PLAYER_MP5;
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, iBulletType, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireAP9, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, fUseAutoAim, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CWeaponEinarAP9::Reload()
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == AP9_MAX_CLIP )
		return;

	if( DefaultReload( AP9_MAX_CLIP, AP9_RELOAD, 1.8 ) )
	{
#ifdef CLIENT_DLL
		if( !bIsMultiplayer() )
#else
		if( !g_pGameRules->IsMultiplayer() )
#endif
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.9f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CWeaponEinarAP9::WeaponIdle()
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_iBurstShots >= 2 )
	{
		if( m_pPlayer->m_flNextAttack > UTIL_WeaponTimeBase() )
			return;
		SecondaryAttack();
		if( m_iBurstShots >= 6 )
			m_iBurstShots = 0;
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_iClip )
	{
		SendWeaponAnim( AP9_IDLE );

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f; // how long till we do this again.
	}
}

class CAP9Ammo : public CBasePlayerAmmo
{
	void Spawn()
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_ap9clip.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL( "models/w_ap9clip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = ( pOther->GiveAmmo( AMMO_AP9_GIVE, "ap9", AP9_MAX_CARRY ) != -1 );
		if( bResult )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_th_ap9, CAP9Ammo )
