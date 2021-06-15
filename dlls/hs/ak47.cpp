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

// The Following is Based on Counter-Strike's AK-47 Weapon
// In no way am I trying to break the law
// I am simply practicing.
// Valve or anyone will authority, please don't sue me.


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum ak47_e
{
	AK47_IDLE1 = 0,
	AK47_RELOAD,
	AK47_DRAW,
	AK47_SHOOT1,
	AK47_SHOOT2,
	AK47_SHOOT3,
};



LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );
//LINK_ENTITY_TO_CLASS( weapon_9mmAR, CMP5 );


//=========================================================
//=========================================================

void CAK47::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ak47"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_ak47.mdl");
	m_iId = WEAPON_AK47;

	m_iDefaultAmmo = AK47_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CAK47::Precache( void )
{
	PRECACHE_MODEL("models/v_ak47.mdl");
	PRECACHE_MODEL("models/w_ak47.mdl");
	PRECACHE_MODEL("models/p_ak47.mdl");

	m_iShell = PRECACHE_MODEL ("models/rshell.mdl");// brass shellTE_MODEL

	//PRECACHE_MODEL("models/grenade.mdl");	// grenade

	//PRECACHE_MODEL("models/w_ak47.mdl");
	//PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("weapons/ak47_clipin.wav");
	PRECACHE_SOUND("weapons/ak47_clipin.wav");

	PRECACHE_MODEL ("models/w_ak47clip.mdl");

	PRECACHE_SOUND ("weapons/ak47-1.wav");// A to the K
	PRECACHE_SOUND ("weapons/ak47-2.wav");// A to the K
	//PRECACHE_SOUND ("weapons/hks3.wav");// H to the K

	//PRECACHE_SOUND( "weapons/glauncher.wav" );
	//PRECACHE_SOUND( "weapons/glauncher2.wav" );

	PRECACHE_SOUND ("weapons/ak47_boltpull.wav");

	m_usAK47 = PRECACHE_EVENT( 1, "events/ak47.sc" );
	//m_usMP52 = PRECACHE_EVENT( 1, "events/mp52.sc" );
}

int CAK47::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ak47";
	p->iMaxAmmo1 = AK47_MAX_CARRY;
	//p->pszAmmo2 = "ARgrenades";
	//p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = AK47_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AK47;
	p->iWeight = AK47_WEIGHT;

	return 1;
}

int CAK47::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CAK47::Deploy( )
{
	return DefaultDeploy( "models/v_ak47.mdl", "models/p_ak47.mdl", AK47_DRAW, "mp5" );
}


void CAK47::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
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

#if CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_AK47, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usAK47, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CAK47::Reload( void )
{
	DefaultReload( AK47_MAX_CLIP, AK47_RELOAD, 3.0 );
}


void CAK47::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	iAnim = AK47_IDLE1;

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CAK47AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_ak47clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_ak47clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_AK47_GIVE, "ak47", AK47_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_ak47, CAK47AmmoClip );
//LINK_ENTITY_TO_CLASS( ammo_ak47, CAk47AmmoClip );
