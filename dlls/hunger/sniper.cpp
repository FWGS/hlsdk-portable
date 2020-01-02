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
#include "shake.h"

enum sniper_e
{
	SNIPER_IDLE = 0,
	SNIPER_RELOAD,
	SNIPER_DRAW,
	SNIPER_SHOOT1
};

LINK_ENTITY_TO_CLASS( weapon_th_sniper, CWeaponEinarSniper )

int CWeaponEinarSniper::AddToPlayer( CBasePlayer *pPlayer )
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

void CWeaponEinarSniper::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if( m_fInZoom )
		SecondaryAttack();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

int CWeaponEinarSniper::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "bullets";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SNIPER;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}

void CWeaponEinarSniper::Spawn()
{
	Precache();
	m_iId = WEAPON_SNIPER;
	SET_MODEL( ENT( pev ), "models/w_hkg36.mdl" );

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CWeaponEinarSniper::Precache()
{
	PRECACHE_MODEL( "models/v_hkg36.mdl" );
	PRECACHE_MODEL( "models/w_hkg36.mdl" );
	PRECACHE_MODEL( "models/p_hkg36.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/sniper.wav" );

	PRECACHE_SOUND( "weapons/ap9_bolt.wav" );
	PRECACHE_SOUND( "weapons/ap9_clipin.wav" );
	PRECACHE_SOUND( "weapons/ap9_clipout.wav" );

	m_usFireSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
}

BOOL CWeaponEinarSniper::Deploy()
{
	return DefaultDeploy( "models/v_hkg36.mdl", "models/p_hkg36.mdl", SNIPER_DRAW, "bow" );
}

void CWeaponEinarSniper::SecondaryAttack()
{
	int iFadeFlags = 0;

	if( m_fInZoom )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;
		m_fInZoom = 0;
		SetBits( iFadeFlags, FFADE_IN );
	}
	else
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
		SetBits( iFadeFlags, FFADE_OUT | FFADE_STAYOUT );
	}
#ifndef CLIENT_DLL
	UTIL_ScreenFade( m_pPlayer, Vector( 0, 255, 0 ), 0.0, 0.0, 70, iFadeFlags );
#endif
	pev->nextthink = UTIL_WeaponTimeBase() + 0.11f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;
}

void CWeaponEinarSniper::PrimaryAttack()
{
	float flSpread;
	int iDmg;

	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
		}
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

#ifdef CLIENT_DLL
	if( bIsMultiplayer() )
#else   
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		if( m_fInZoom )
		{
			flSpread = 0.002f;
			iDmg = 80;
		}
		else
		{
			flSpread = 0.07846f;
			iDmg = 40;
		}
	}
	else
	{
		if( m_fInZoom )
		{
			flSpread = 0.004f;
		}
		else
		{
			flSpread = 0.08716f;
		}
		iDmg = 0;
	}

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

        int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, m_fInZoom, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	if( m_fInZoom )	
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
	}
	else
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.11f;
		m_pPlayer->pev->punchangle.x -= ( RANDOM_FLOAT( 0.0f, 2.0f ) + 4.0f );
		m_pPlayer->pev->punchangle.y -= ( 1.0f - RANDOM_FLOAT( 0.0f, 2.0f ) );
	}
}

void CWeaponEinarSniper::Reload()
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPER_MAX_CLIP )
		return;

	if( m_fInZoom )
		SecondaryAttack();

	int iResult = DefaultReload( SNIPER_MAX_CLIP, SNIPER_RELOAD, 3.3f );
}

void CWeaponEinarSniper::WeaponIdle()
{
	ResetEmptySound();
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f;
}

class CSniperAmmo : public CBasePlayerAmmo
{
	void Spawn()
	{
		Precache();
		pev->classname = MAKE_STRING( "ammo_th_sniper" );
		SET_MODEL( ENT( pev ), "models/w_antidote.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache()
	{
		PRECACHE_MODEL( "models/w_antidote.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_SNIPER_GIVE, "bullets", SNIPER_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_th_sniper, CSniperAmmo )
LINK_ENTITY_TO_CLASS( ammo_einar1, CSniperAmmo )
