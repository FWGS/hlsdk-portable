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
#include "soundent.h"
#include "gamerules.h"

enum mw2_e
{
	MW2_LONGIDLE = 0,
	MW2_IDLE1,
	MW2_GRENADE,
	MW2_RELOAD,
	MW2_DRAW,
	MW2_SHOOT1,
	MW2_SHOOT2,
	MW2_SHOOT3,
	MW2_DEPLOY,
};



LINK_ENTITY_TO_CLASS( weapon_mw2, CMW2 )
LINK_ENTITY_TO_CLASS( weapon_gauss, CMW2 )
LINK_ENTITY_TO_CLASS( ammo_gaussclip, CMW2 )

//=========================================================
//=========================================================

void CMW2::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mw2"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_mw2.mdl");
	m_iId = WEAPON_MW2;

	m_iDefaultAmmo = MW2_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMW2::Precache( void )
{
	PRECACHE_MODEL("models/v_mw2.mdl");
	PRECACHE_MODEL("models/w_mw2.mdl");
	PRECACHE_MODEL("models/p_mw2.mdl");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shell

	PRECACHE_SOUND("weapons/m4a1_clipin.wav");

	PRECACHE_MODEL ("models/w_9mmclip.mdl");

	PRECACHE_SOUND ("weapons/m4a1_unsil-1.wav");
	PRECACHE_SOUND ("weapons/m4a1_unsil-2.wav");

	PRECACHE_SOUND ("weapons/m4a1_boltpull.wav");

	m_usMW2 = PRECACHE_EVENT( 1, "events/mw2.sc" );
}

int CMW2::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "mw2";
	p->iMaxAmmo1 = MW2_MAX_CARRY;
	p->iMaxClip = MW2_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MW2;
	p->iWeight = MW2_WEIGHT;

	return 1;
}

int CMW2::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMW2::Deploy( )
{
	return DefaultDeploy( "models/v_mw2.mdl", "models/p_mw2.mdl", MW2_DRAW, "mp5" );
}


void CMW2::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_9DEGREES, 8192, BULLET_PLAYER_MW2, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MW2, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

#ifndef CLIENT_DLL
		UTIL_ScreenShake( m_pPlayer->pev->origin, 250.0, 200.0, 2.5, 1 );
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMW2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMW2::Reload( void )
{
	DefaultReload( MW2_MAX_CLIP, MW2_RELOAD, 3.0 );
}


void CMW2::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	iAnim = MW2_IDLE1;

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CMW2AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_MW2_GIVE, "mw2", MW2_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mw2, CMW2AmmoClip )
