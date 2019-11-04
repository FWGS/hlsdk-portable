/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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

enum tommygun_e
{
	TOMMYGUN_IDLE = 0,
	TOMMYGUN_RELOAD,
	TOMMYGUN_DRAW,
	TOMMYGUN_FIRE1,
	TOMMYGUN_FIRE2,
	TOMMYGUN_EMPTY_IDLE,
};

#include "TommyGun.h"

LINK_ENTITY_TO_CLASS( weapon_tommygun, CTommyGun );


//=========================================================
//=========================================================

void CTommyGun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_tommygun"); // hack to allow for old names
	Precache( );
	SET_MODEL(ENT(pev), "models/w_tommygun.mdl");
	m_iId = WEAPON_TOMMYGUN;

	m_iDefaultAmmo = TOMMYGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CTommyGun::Precache( void )
{
	PRECACHE_MODEL("models/v_tommygun.mdl");
	PRECACHE_MODEL("models/w_tommygun.mdl");
	PRECACHE_MODEL("models/p_tommygun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/tommy_ammo.mdl");

	PRECACHE_SOUND("weapons/tommy_draw_slideback.wav");              
	PRECACHE_SOUND("weapons/tommy_reload_clipin.wav");
	PRECACHE_SOUND("weapons/tommy_reload_clipout.wav");
	PRECACHE_SOUND("weapons/tommy_shoot1.wav");
	PRECACHE_SOUND("weapons/tommy_shoot2.wav");

//	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usTommyGun = PRECACHE_EVENT( 1, "events/tommygun.sc" );
}

int CTommyGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Tommy Gun";
	p->iMaxAmmo1 = TOMMYGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = TOMMYGUN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_TOMMYGUN;
	p->iWeight = TOMMYGUN_WEIGHT;

	return 1;
}

int CTommyGun::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CTommyGun::Deploy( )
{
	return DefaultDeploy( "models/v_tommygun.mdl", "models/p_tommygun.mdl", TOMMYGUN_DRAW, "tommygun" );
}

void CTommyGun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usTommyGun );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	
	if ( g_pGameRules->IsDeathmatch() )
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_TOMMYGUN, 2 );
	}
	else
	{
		// single player spread
		m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_TOMMYGUN, 2 );
	}

	//if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
	//	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = m_flNextPrimaryAttack + 0.1;
	if (m_flNextPrimaryAttack < gpGlobals->time)
		m_flNextPrimaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}

void CTommyGun::SecondaryAttack( void )
{
	PrimaryAttack();
}

void CTommyGun::Reload( void )
{
	DefaultReload( TOMMYGUN_MAX_CLIP, TOMMYGUN_RELOAD, 177.0/38.0 );
}

void CTommyGun::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	SendWeaponAnim( TOMMYGUN_IDLE );

	//m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );// how long till we do this again.
	m_flTimeWeaponIdle = gpGlobals->time + 40.0/12.0 + RANDOM_FLOAT ( 1, 4 );
}

////////////////////////////////////////////////////////////////////////////

void CTommyGunAmmoClip::Spawn( void )
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/tommy_ammo.mdl");
	CBasePlayerAmmo::Spawn( );
}

void CTommyGunAmmoClip::Precache( void )
{
	PRECACHE_MODEL ("models/tommy_ammo.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CTommyGunAmmoClip::AddAmmo( CBaseEntity *pOther ) 
{ 
	int bResult = (pOther->GiveAmmo( AMMO_TOMMYGUNCLIP_GIVE, "Tommy Gun", TOMMYGUN_MAX_CARRY) != -1);
	if (bResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
	}
	return bResult;
}


LINK_ENTITY_TO_CLASS( ammo_tommygun, CTommyGunAmmoClip );




