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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

enum glock_e {
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock );
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock );

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
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

void CGlock::Spawn( )
{
	Precache( );
	m_iId = WEAPON_GLOCK;
	SET_MODEL(ENT(pev), "models/w_all_items2.mdl");
	pev->body = 2;

	m_iDefaultAmmo = 15;

	#ifndef CLIENT_DLL
	if ( FClassnameIs( pev, "weapon_glock" ) ){
	m_iDefaultAmmo = 10;
	}
	#endif

	pev->classname = MAKE_STRING("weapon_9mmhandgun"); // hack to allow for old names

	FallInit();// get ready to fall down.
}


void CGlock::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );
}

int CGlock::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 15;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

BOOL CGlock::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	return DefaultDeploy( "models/v_9mmhandgun.mdl", 0, GLOCK_DRAW, "python", /*UseDecrement() ? 1 : 0*/ 0,893 );
}

void CGlock::SecondaryAttack( void )
{
	GlockFire( 1, 0.15, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	if ( !TriggerReleased ){
	return;
	}

	TriggerReleased = FALSE;

	GlockFire( 0, 0.2, TRUE );
}

void CGlock::GlockFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
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
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

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

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;

	if(flSpread == 1){
		if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
		{	
			if ( m_pPlayer->pev->flags & FL_DUCKING ) {
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.032, 0.032, 0.032 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
			{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.04, 0.04, 0.04 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.036, 0.036, 0.036 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.05, 0.05, 0.05 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}
	else{
		if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
		{	
			if ( m_pPlayer->pev->flags & FL_DUCKING ) {
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.01, 0.01, 0.01 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
			{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.014, 0.014, 0.014 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.011, 0.011, 0.011 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.017, 0.017, 0.017 ), 3000, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}


	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		 return;

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( 15, GLOCK_RELOAD, 1.6 );
	else
		iResult = DefaultReload( 15, GLOCK_RELOAD_NOT_EMPTY, 1.6 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CGlock::WeaponIdle( void )
{
	TriggerReleased = TRUE;
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}








class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 6;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 15, "9mm", _9MM_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo );
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo );















