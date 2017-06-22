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
#include "gamerules.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e
{
	ASHOTGUN_IDLE = 0,
	ASHOTGUN_FIRE,
	ASHOTGUN_FIRE2,
	ASHOTGUN_RELOAD,
	ASHOTGUN_PUMP,
	ASHOTGUN_START_RELOAD,
	ASHOTGUN_DRAW,
	ASHOTGUN_HOLSTER,
	ASHOTGUN_IDLE4,
	ASHOTGUN_IDLE_DEEP
};

LINK_ENTITY_TO_CLASS( weapon_ashotgun, CShotgun )
LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun )

void CShotgun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_ashotgun");
	Precache();
	m_iId = WEAPON_ASHOTGUN;
	SET_MODEL( ENT( pev ), "models/wmodels/w_ashotgun.mdl" );

	m_iDefaultAmmo = ASHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CShotgun::Precache( void )
{
	PRECACHE_MODEL( "models/vmodels/v_ashotgun.mdl" );
	PRECACHE_MODEL( "models/wmodels/w_ashotgun.mdl" );
	PRECACHE_MODEL( "models/pmodels/p_ashotgun.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );// shotgun shell

	PRECACHE_SOUND( "weapons/reload1.wav" );	// shotgun reload
	PRECACHE_SOUND( "weapons/reload3.wav" );	// shotgun reload

	//PRECACHE_SOUND( "weapons/sshell1.wav" );	// shotgun reload - played on client
	//PRECACHE_SOUND( "weapons/sshell3.wav" );	// shotgun reload - played on client
	
	PRECACHE_SOUND( "weapons/ashotgun1.wav" );
	PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/ashotgun1.sc" );
	m_usDoubleFire = PRECACHE_EVENT( 1, "events/ashotgun2.sc" );
}

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
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

int CShotgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = ASHOTGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ASHOTGUN;
	p->iWeight = ASHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun::Deploy()
{
	return DefaultDeploy( "models/vmodels/v_ashotgun.mdl", "models/pmodels/p_ashotgun.mdl", ASHOTGUN_DRAW, "shotgun" );
}

void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if( m_iClip <= 0 )
	{
		Reload();
		if( m_iClip == 0 )
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( m_iClip != 0 )
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}

void CShotgun::SecondaryAttack( void )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if( m_iClip <= 1 )
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 16, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usDoubleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( m_iClip != 0 )
		m_flPumpTime = gpGlobals->time + 0.95;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;
}

void CShotgun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == ASHOTGUN_MAX_CLIP )
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;

	// check to see if we're ready to reload
	if( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( ASHOTGUN_START_RELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if( m_fInSpecialReload == 1 )
	{
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );
		else
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );

		SendWeaponAnim( ASHOTGUN_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}

void CShotgun::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
		m_flPumpTime = 0;
	}

	if( m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if( m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			Reload();
		}
		else if( m_fInSpecialReload != 0 )
		{
			if( m_iClip != 16 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( ASHOTGUN_PUMP );
				
				// play cocking sound
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.8 )
			{
				iAnim = ASHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 60.0 / 12.0 );// * RANDOM_LONG( 2, 5 );
			}
			else if( flRand <= 0.95 )
			{
				iAnim = ASHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0 / 9.0 );
			}
			else
			{
				iAnim = ASHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0 / 9.0 );
			}
			SendWeaponAnim( iAnim );
		}
	}
}

class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/ammo/w_buckshot.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/ammo/w_buckshot.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if( pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshotbox, CShotgunAmmo )
