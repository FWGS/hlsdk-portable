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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

enum sniper_e {
	SNIPER_IDLE1 = 0,	// full
	SNIPER_IDLE2,		// empty
	SNIPER_FIDGET1,	// full
	SNIPER_FIDGET2,	// empty
	SNIPER_FIRE1,		// full
	SNIPER_RELOAD1,		// reload
	SNIPER_FIRE2,		// empty
	SNIPER_RELOAD2,	// from empty
	SNIPER_DRAW1,		// full
	SNIPER_DRAW2,		// empty
	SNIPER_DRAW3,	// full
	SNIPER_DRAW4,	// empty
};

LINK_ENTITY_TO_CLASS( weapon_sniperrifle, CSniper );

void CSniper::Spawn( )
{
	 // hack to allow for old names

	Precache( );
	m_iId = WEAPON_M40A1;
	SET_MODEL(ENT(pev), "models/w_all_items3.mdl");
	pev->body = 1;

	m_iDefaultAmmo = 5;
	
	pev->classname = MAKE_STRING("weapon_sniperrifle");

	FallInit();// get ready to fall down.
}

int CSniper::AddToPlayer( CBasePlayer *pPlayer )
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

void CSniper::Precache( void )
{
	PRECACHE_MODEL("models/v_sniper.mdl");

	PRECACHE_SOUND("weapons/sniper_fire1.wav");
	PRECACHE_SOUND("weapons/sniper_zoom.wav");

	m_usSniper = PRECACHE_EVENT( 1, "events/sniper.sc" );
}


int CSniper::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "338mag";
	p->iMaxAmmo1 = SNIPER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 5;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_M40A1;
	p->iFlags = 0;
	p->iWeight = CROSSBOW_WEIGHT;
	return 1;
}


BOOL CSniper::Deploy( )
{
	m_pPlayer->m_newcross_active = 0;
	return DefaultDeploy( "models/v_sniper.mdl", 0, SNIPER_DRAW1, "shotgun",0,114 );
}

void CSniper::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		m_pPlayer->m_newcross_active = 0;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CSniper::PrimaryAttack( void )
{
	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);

			m_flNextPrimaryAttack = 1.5;

		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;


	if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
	{	
		if ( m_pPlayer->pev->velocity.Length2D() >= 140)
		{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.02,0.02,0.02), 16000, BULLET_338Magnum, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0,0,0), 16000, BULLET_338Magnum, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}
	}
	else{
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.04,0.04,0.04), 16000, BULLET_338Magnum, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

//  开火后保持开镜状态 Bug Fix 1.0
//	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
//	m_fInZoom = 0;
//	m_pPlayer->m_newcross_active = 0;

	#ifndef CLIENT_DLL
	UTIL_MakeVectors (m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	float flZVel = m_pPlayer->pev->velocity.z;
	m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 120;
	m_pPlayer->pev->velocity.z = flZVel;
	#endif

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.2;

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CSniper::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		m_pPlayer->m_newcross_active = 0;
	}
	else if ( m_pPlayer->pev->fov != 20 )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/sniper_zoom.wav", 1, ATTN_NORM);
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
		m_fInZoom = 1;
		m_pPlayer->m_newcross_active = 1;
	}
	
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
}


void CSniper::Reload( void )
{
	if ( m_pPlayer->ammo_338mag <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
		m_pPlayer->m_newcross_active = 0;
	}

	DefaultReload(5, SNIPER_RELOAD1, 3.1 );
}


void CSniper::WeaponIdle( void )
{
	TriggerReleased = TRUE;

	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75)
		{
			if (m_iClip)
			{
				SendWeaponAnim( SNIPER_IDLE1 );
			}
			else
			{
				SendWeaponAnim( SNIPER_IDLE2 );
			}
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			if (m_iClip)
			{
				SendWeaponAnim( SNIPER_FIDGET1 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
			}
			else
			{
				SendWeaponAnim( SNIPER_FIDGET2 );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 30.0;
			}
		}
	}
}

#endif

class CSniperAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items1.mdl");
		pev->body = 3;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 5, "338mag", SNIPER_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_338, CSniperAmmo );
