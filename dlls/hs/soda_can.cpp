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
#include "hornet.h"
#include "gamerules.h"


enum soda_e {
	SODA_IDLE1 = 0,
	SODA_FIDGET,
	SODA_DRINK
};


LINK_ENTITY_TO_CLASS( weapon_soda, CSodaCan );

BOOL CSodaCan::IsUseable( void )
{
	return TRUE;
}

void CSodaCan::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SODA;
	SET_MODEL(ENT(pev), "models/w_sodacan.mdl");

	m_iDefaultAmmo = SODA_USUAL_DRINKS;

	FallInit();// get ready to fall down.
}


void CSodaCan::Precache( void )
{
	PRECACHE_MODEL("models/v_sodacan.mdl");
	PRECACHE_MODEL("models/w_sodacan.mdl");
	PRECACHE_MODEL("models/p_sodacan.mdl");

	PRECACHE_SOUND("weapons/soda1.wav");
	PRECACHE_SOUND("weapons/soda2.wav");
	PRECACHE_SOUND("weapons/sodaup.wav");

	m_usSodaDrink = PRECACHE_EVENT ( 1, "events/soda.sc" );
}

int CSodaCan::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{

#if !CLIENT_DLL
		if ( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] = SODA_USUAL_DRINKS;
		}
#endif

		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CSodaCan::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Soda";
	p->iMaxAmmo1 = SODA_USUAL_DRINKS;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SODA;
	p->iFlags = ITEM_FLAG_EXHAUSTIBLE;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}


BOOL CSodaCan::Deploy( )
{
	EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/sodaup.wav", 1, ATTN_NORM );
	return DefaultDeploy( "models/v_sodacan.mdl", "models/p_sodacan.mdl", SODA_IDLE1, "soda" );
}

void CSodaCan::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( SODA_IDLE1 );
}


void CSodaCan::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;
	
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->TakeHealth( 15, DMG_GENERIC );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSodaDrink, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, 1, 0, 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 3;

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 105.0f / 30.0f;
}

void CSodaCan::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3f)
	{
		iAnim = SODA_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 20.0f;
	}
	else if (flRand <= 0.6f)
	{
		iAnim = SODA_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61.0f / 30.0f;
	}
	else
	{
		iAnim = SODA_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 20.0f;
	}
	SendWeaponAnim( iAnim, 1 );
}
