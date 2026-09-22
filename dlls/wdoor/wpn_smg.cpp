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

enum smg_e
{
	SMG_LONGIDLE = 0,
	SMG_RELOAD,
	SMG_DEPLOY,
	SMG_FIRE1,
	SMG_FIRE2,
	SMG_FIRE3,
	SMG_HOLSTER,
};

LINK_ENTITY_TO_CLASS( weapon_smg, CSMG );
void CSMG::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 14;
	m_iId = WEAPON_SMG;

	m_iDefaultAmmo = 30;

	pev->classname = MAKE_STRING("weapon_smg"); // hack to allow for old names

	FallInit();// get ready to fall down.
}


void CSMG::Precache( void )
{
	PRECACHE_MODEL("models/v_mp5.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("weapons/mp5_clipin.wav");
	PRECACHE_SOUND("weapons/mp5_clipout.wav");

	PRECACHE_SOUND ("weapons/mp5_fire-1.wav");
	PRECACHE_SOUND ("weapons/mp5_fire-2.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usSMG = PRECACHE_EVENT( 1, "events/smg.sc" );
}

int CSMG::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SMG;
	p->iWeight = SMG_WEIGHT;

	return 1;
}

int CSMG::AddToPlayer( CBasePlayer *pPlayer )
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

void CSMG::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;
	m_pPlayer->m_newcross_active = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SMG_HOLSTER );
}


BOOL CSMG::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	return DefaultDeploy( "models/v_mp5.mdl", 0, SMG_DEPLOY, "mp5" );
}


void CSMG::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.1;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.1;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
	{	
		if ( m_pPlayer->pev->flags & FL_DUCKING ) {
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.027,0.027,0.027), 3000, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else if ( m_pPlayer->pev->velocity.Length2D() >= 200)
		{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.033,0.033,0.033), 3000, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.03,0.03,0.03), 3000, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}
	else{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( 0.036, 0.036, 0.036 ), 3000, BULLET_PLAYER_9MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	
  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSMG, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.085;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x -= 0.5;

	if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		KickBack(0.9, 0.475, 0.35, 0.0425, 5.0, 3.0, 6);
	else if (m_pPlayer->pev->velocity.Length2D() > 200)
		KickBack(0.5, 0.275, 0.2, 0.03, 3.0, 2.0, 10);
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		KickBack(0.225, 0.15, 0.1, 0.015, 2.0, 1.0, 10);
	else
		KickBack(0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10);
}

void CSMG::Reload( void )
{
	if ( m_pPlayer->ammo_9mm <= 0 )
		return;

	DefaultReload( 30, SMG_RELOAD, 1.8 );
}


void CSMG::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( SMG_LONGIDLE );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CSMGAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 13;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 30, "9mm", _9MM_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_smgclip, CSMGAmmoClip );
