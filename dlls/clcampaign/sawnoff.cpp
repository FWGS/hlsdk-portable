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

enum sawnoff_e
{
	SAWNOFF_IDLE = 0,
	SAWNOFF_FIRE,
	SAWNOFF_FIRE2,
	SAWNOFF_RELOAD,
	SAWNOFF_PUMP,
	SAWNOFF_START_RELOAD,
	SAWNOFF_DRAW,
	SAWNOFF_HOLSTER,
	SAWNOFF_IDLE4,
	SAWNOFF_IDLE_DEEP
};

LINK_ENTITY_TO_CLASS( weapon_sawnoff, CSawnoff )

void CSawnoff::Spawn()
{
	Precache();
	m_iId = WEAPON_SAWNOFF;
	SET_MODEL( ENT( pev ), "models/w_sawnoff.mdl" );

	m_iDefaultAmmo = SAWNOFF_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CSawnoff::Precache( void )
{
	PRECACHE_MODEL( "models/v_sawnoff.mdl" );
	PRECACHE_MODEL( "models/w_sawnoff.mdl" );
	PRECACHE_MODEL( "models/p_sawnoff.mdl" );
	PRECACHE_MODEL( "models/w_shotbox.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/sshotgun_shoot.wav" );//shotgun
	PRECACHE_SOUND( "weapons/sshotgun_reload.wav" );//shotgun reload
	PRECACHE_SOUND( "weapons/sshotgun_cock1.wav" );
	PRECACHE_SOUND( "weapons/sshotgun_cock2.wav" );
	PRECACHE_SOUND( "weapons/sshotgun_cock3.wav" );
	
	//PRECACHE_SOUND( "weapons/357_cock1.wav" ); // gun empty sound
	//PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun

	m_usSawnoff = PRECACHE_EVENT( 1, "events/sawnoff.sc" );
}

int CSawnoff::AddToPlayer( CBasePlayer *pPlayer )
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

int CSawnoff::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SAWNOFF_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SAWNOFF;
	p->iWeight = SAWNOFF_WEIGHT;

	return 1;
}

BOOL CSawnoff::Deploy()
{
	return DefaultDeploy( "models/v_sawnoff.mdl", "models/p_sawnoff.mdl", SAWNOFF_DRAW, "Sawnoff" );
}

void CSawnoff::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( m_iClip <= 0 )
	{
		Reload();
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/sshotgun_cock3.wav", 0.8, ATTN_NORM );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
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

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	SendWeaponAnim( SAWNOFF_FIRE );

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 18, vecSrc, vecAiming, VECTOR_CONE_20DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSawnoff, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSawnoff::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SAWNOFF_MAX_CLIP )
		return;
	DefaultReload( SAWNOFF_MAX_CLIP, SAWNOFF_RELOAD, 1.3 );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "sshotgun_reload.wav", 1, ATTN_NORM );
}
			


void CSawnoff::WeaponIdle( void )
{
	ResetEmptySound();

        m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

        if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
                return;

        m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
        SendWeaponAnim( SAWNOFF_IDLE );
}

