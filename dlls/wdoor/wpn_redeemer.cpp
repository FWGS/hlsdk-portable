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
#include "shake.h"

LINK_ENTITY_TO_CLASS( weapon_redeemer, CRedeemer );

//=========================================================
//=========================================================

void CRedeemer::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 19;
	m_iId = WEAPON_REDEEMER;

	m_iDefaultAmmo = 5;

	FallInit();// get ready to fall down.
}


void CRedeemer::Precache( void )
{
	PRECACHE_MODEL("models/v_redeemer.mdl");

	PRECACHE_SOUND("weapons/redeemer_draw.wav");
	PRECACHE_SOUND("weapons/redeemer_fire.wav");
	PRECACHE_SOUND("weapons/redeemer_reload.wav");
}

int CRedeemer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "nuke";
	p->iMaxAmmo1 = 5;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_REDEEMER;
	p->iWeight = 40;

	return 1;
}

int CRedeemer::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CRedeemer::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	return DefaultDeploy( "models/v_redeemer.mdl", 0, 1, "gauss",0,810 );
}

void CRedeemer::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 1.0;

	SendWeaponAnim( 4 );
}

void CRedeemer::PrimaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] < 1 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}

	if ( m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] == 1 )
	{//最后一发
		SendWeaponAnim( 3 );
	}
	else{
		SendWeaponAnim( 2 );
	}

	#ifndef CLIENT_DLL	

	if(m_pPlayer->m_darkposion > 0){
	m_pPlayer->m_darkposion = 0;//隐身取消
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, g_vecZero, 16000, 628, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	EMIT_SOUND_DYN ( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/redeemer_fire.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
	m_pPlayer->pev->punchangle.x -= 15;
	#endif

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4;
}

void CRedeemer::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( 0 );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}







