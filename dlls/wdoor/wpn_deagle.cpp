/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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


enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_IDLE3,
	DEAGLE_IDLE4,
	DEAGLE_FIRE1,
	DEAGLE_FIRE2,
    DEAGLE_RELOAD,
    DEAGLE_DRAW,
	DEAGLE_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDeagle );
LINK_ENTITY_TO_CLASS( weapon_eagle, CDeagle );

int CDeagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 7;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

void CDeagle::SecondaryAttack()
{
	m_fSpotActive = ! m_fSpotActive;

#ifndef CLIENT_DLL
	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NORMAL );
		m_pSpot = NULL;
	}
#endif
	if(m_fSpotActive == 1){
	m_fInZoom = true;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/laser_sight.wav", 1, ATTN_NORM);
	}
	else{
	m_fInZoom = false;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/laser_sight2.wav", 1, ATTN_NORM);
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
}

int CDeagle::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CDeagle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_deagle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 2;

	m_iDefaultAmmo = 7;

	FallInit();// get ready to fall down.
}


void CDeagle::Precache( void )
{
	PRECACHE_MODEL("models/v_deagle.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/de_shot1.wav");

	PRECACHE_SOUND ("weapons/laser_sight.wav");
	PRECACHE_SOUND ("weapons/laser_sight2.wav");
	
	m_usFireDeagle = PRECACHE_EVENT( 1, "events/deagle.sc" );
	m_usFireDeagle2 = PRECACHE_EVENT( 1, "events/deagle2.sc" );
}

BOOL CDeagle::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	pev->body = 0;

	return DefaultDeploy( "models/v_deagle.mdl", 0, DEAGLE_DRAW, "python", UseDecrement(), pev->body );
}


void CDeagle::Holster( int skiplocal /* = 0 */ )
{

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed( NULL, GIB_NEVER );
		m_pSpot = NULL;
	}
#endif

	m_fInZoom = false;
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CDeagle::PrimaryAttack()
{
	if ( !TriggerReleased ){
	return;
	}

	TriggerReleased = FALSE;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.3;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;

	if (m_fSpotActive == 1){
		if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
		{	
			if ( m_pPlayer->pev->flags & FL_DUCKING ) {
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.01, 0.01, 0.01 ), 8000, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
			{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.014, 0.014, 0.014 ), 8000, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.011, 0.011, 0.011 ), 8000, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.017, 0.017, 0.017 ), 8000, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}
	else{
		if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
		{	
			if ( m_pPlayer->pev->flags & FL_DUCKING ) {
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.03, 0.03, 0.03 ), 5120, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
			{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.036, 0.036, 0.036 ), 5120, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.032, 0.032, 0.032 ), 5120, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.04, 0.04, 0.04 ), 5120, BULLET_50AE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}


    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if (m_fSpotActive == 1){
	m_flNextPrimaryAttack = 0.5;
	}
	else{
	m_flNextPrimaryAttack = 0.25;
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireDeagle, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	UpdateSpot( );
}


void CDeagle::Reload( void )
{
	UpdateSpot( );
	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	if (DefaultReload( 7, DEAGLE_RELOAD, 1.6, FALSE ))
	{
		m_flSoundDelay = 1.5;
		#ifndef CLIENT_DLL
			if ( m_pSpot && m_fSpotActive )
			{
				m_pSpot->Suspend( 2.1 );
				m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.5;
			}
		#endif
	}
}

void CDeagle::UpdateSpot( void )
{
#ifndef CLIENT_DLL
	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		Vector vecSrc = m_pPlayer->GetGunPosition( );;
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr );
		
		UTIL_SetOrigin( m_pSpot->pev, tr.vecEndPos );
	}
#endif

}

void CDeagle::WeaponIdle( void )
{
	UpdateSpot( );
    TriggerReleased = TRUE;
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	if (flRand <= 0.5)
	{
		iAnim = DEAGLE_IDLE1;
		m_flTimeWeaponIdle = (70.0/30.0);
	}
	else if (flRand <= 0.7)
	{
		iAnim = DEAGLE_IDLE2;
		m_flTimeWeaponIdle = (60.0/30.0);
	}
	else if (flRand <= 0.9)
	{
		iAnim = DEAGLE_IDLE3;
		m_flTimeWeaponIdle = (88.0/30.0);
	}
	else
	{
		iAnim = DEAGLE_IDLE4;
		m_flTimeWeaponIdle = (170.0/30.0);
	}
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0, FALSE );
}

#endif