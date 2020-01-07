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

enum deagle_e {
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_FIRE1,
	DEAGLE_FIRE2,
	DEAGLE_FIRE_LOAD,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_EMPTY,
	DEAGLE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDeagle );

void CDeagle::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DEAGLE;
	SET_MODEL(ENT(pev), "models/w_deagle.mdl");

	m_iDefaultAmmo = DEAGLE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CDeagle::Precache( void )
{
	PRECACHE_MODEL("models/v_deagle.mdl");
	PRECACHE_MODEL("models/w_deagle.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND ("weapons/deagle_cin2.wav");
	PRECACHE_SOUND ("weapons/deagle_cout.wav");
	PRECACHE_SOUND ("weapons/deagle_fire.wav");
	PRECACHE_SOUND ("weapons/deagle_slide.wav");

	m_usFireDeagle = PRECACHE_EVENT( 1, "events/deagle.sc" );
}

int CDeagle::AddToPlayer( CBasePlayer *pPlayer )
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

int CDeagle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "50AE";
	p->iMaxAmmo1 = DEAGLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = DEAGLE_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = DEAGLE_WEIGHT;

	return 1;
}

BOOL CDeagle::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_deagle.mdl", "models/p_9mmhandgun.mdl", DEAGLE_DRAW, "Deagle", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CDeagle::PrimaryAttack( void )
{
	float flSpread = 0.01;


	if ( m_iClip <= 0 || (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD) )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
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

	// non-silenced
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0, gSkillData.plrDmgDeagle, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireDeagle, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CDeagle::Reload( void )
{
	int iAnim;
	float fTime;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == DEAGLE_MAX_CLIP )
		 return;

	if( m_iClip )
        {
                iAnim = DEAGLE_RELOAD;
                fTime = 2.43;
        }
        else
        {
                iAnim = DEAGLE_RELOAD_EMPTY;
                fTime = 2.7;
        }
	DefaultReload( DEAGLE_MAX_CLIP, iAnim, fTime );
}



void CDeagle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( DEAGLE_IDLE1, 1 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CDeagleAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponclips/w_deagleclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponclips/w_deagleclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_DEAGLECLIP_GIVE, "50AE", DEAGLE_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_deagle, CDeagleAmmo );
