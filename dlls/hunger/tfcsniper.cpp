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

enum tfcsniper_e
{
	TFCSNIPER_IDLE = 0,
	TFCSNIPER_AIM,
	TFCSNIPER_FIRE,
	TFCSNIPER_DRAW,
	TFCSNIPER_HOLSTER,
	TFCSNIPER_AUTOIDLE,
	TFCSNIPER_AUTOFIRE,
	TFCSNIPER_AUTODRAW,
	TFCSNIPER_AUTOHOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_einar1, CWeaponEinarTFCSniper )

int CWeaponEinarTFCSniper::AddToPlayer( CBasePlayer *pPlayer )
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

int CWeaponEinarTFCSniper::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "bullets";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_TFCSNIPER;
	p->iWeight = SNIPER_WEIGHT;

	return 1;
}

void CWeaponEinarTFCSniper::Spawn()
{
	Precache();
	m_iId = WEAPON_TFCSNIPER;
	SET_MODEL( ENT( pev ), "models/w_isotopebox.mdl" );

	m_iDefaultAmmo = SNIPER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CWeaponEinarTFCSniper::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_sniper.mdl" );
	PRECACHE_MODEL( "models/w_isotopebox.mdl" );
	PRECACHE_MODEL( "models/p_sniper.mdl" );

	PRECACHE_SOUND( "weapons/sniper.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );

	m_usFireSniper = PRECACHE_EVENT( 1, "events/sniper2.sc" );
}

BOOL CWeaponEinarTFCSniper::Deploy()
{
	m_fInSpecialReload = 0;
	return DefaultDeploy( "models/v_tfc_sniper.mdl", "models/p_sniper.mdl", TFCSNIPER_AUTODRAW, "bow" );
}

void CWeaponEinarTFCSniper::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CWeaponEinarTFCSniper::PrimaryAttack()
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
		{
			SendWeaponAnim( TFCSNIPER_AUTOIDLE );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		}
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

void CWeaponEinarTFCSniper::SecondaryAttack()
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

void CWeaponEinarTFCSniper::Reload()
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SNIPER_MAX_CLIP )
		return;

        if( m_fInZoom )
                SecondaryAttack();
                
	if( DefaultReload( SNIPER_MAX_CLIP, TFCSNIPER_AUTOHOLSTER, 0.5 ) )
	{
		m_fInSpecialReload = 2;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5, 10 );		
	}
}

void CWeaponEinarTFCSniper::WeaponIdle()
{
	ResetEmptySound();
	if( m_pPlayer->m_flNextAttack < UTIL_WeaponTimeBase() )
	{
		if( m_fInSpecialReload == 3 )
		{
			SendWeaponAnim( TFCSNIPER_AUTODRAW, 0 );
			m_fInSpecialReload = 1;
		}
		if( m_fInSpecialReload == 2 )
		{
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0x1f ) );
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
			m_fInSpecialReload = 3;
		}
		if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			SendWeaponAnim( TFCSNIPER_AUTOIDLE, 1 );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 15, 20 );
		}
	}
}
