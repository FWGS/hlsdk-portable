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

LINK_ENTITY_TO_CLASS( weapon_dualdbarrel, CDualdbarrel );

void CDualdbarrel::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DUALDBARREL;
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 10;

	m_iDefaultAmmo = 4;

	pev->classname = MAKE_STRING("weapon_dualdbarrel"); // hack to allow for old names

	FallInit();// get ready to fall
}


void CDualdbarrel::Precache( void )
{
	PRECACHE_MODEL("models/v_dualdbarrel.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND ("weapons/dbarrel_shoot1.wav");//shotgun
	PRECACHE_SOUND ("weapons/dbarrel_foley1.wav");//shotgun
	PRECACHE_SOUND ("weapons/dbarrel_foley4.wav");	// shotgun reload

	m_usdualdbarrelFire = PRECACHE_EVENT( 1, "events/dualdbarrel1.sc" );
}

int CDualdbarrel::AddToPlayer( CBasePlayer *pPlayer )
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


int CDualdbarrel::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 4;
	p->iSlot = 5;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DUALDBARREL;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}


BOOL CDualdbarrel::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	return DefaultDeploy( "models/v_dualdbarrel.mdl", 0, 2, "shotgun" );
}

void CDualdbarrel::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}

	if (m_iClip <= 1)
	{
		Reload( );
		if (m_iClip == 0)
			PlayEmptySound( );
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


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 20, vecSrc, vecAiming, Vector( 0.09, 0.09, 0.09 ), 3000, BULLET_PLAYER_BUCKSHOT, 0, 623, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usdualdbarrelFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;

	m_fInSpecialReload = 0;
}


void CDualdbarrel::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 4)
		return;

	DefaultReload( 4, 3, 2.0 );
}


void CDualdbarrel::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
}