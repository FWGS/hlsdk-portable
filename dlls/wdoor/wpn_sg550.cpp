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


LINK_ENTITY_TO_CLASS( weapon_sg550, CSg550 );
LINK_ENTITY_TO_CLASS( weapon_sg550_drop, CSg550 );

void CSg550::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SG550;
	SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
	pev->body = 3;

	m_iDefaultAmmo = 20;

	#ifndef CLIENT_DLL
	if ( FClassnameIs( pev, "weapon_sg550_drop" ) ){
	m_iDefaultAmmo = 10;
	}
	#endif

	pev->classname = MAKE_STRING("weapon_sg550"); // hack to allow for old names

	FallInit();// get ready to fall down.
}

int CSg550::AddToPlayer( CBasePlayer *pPlayer )
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

void CSg550::Precache( void )
{
	PRECACHE_MODEL("models/v_sg550.mdl");
	PRECACHE_SOUND("weapons/sg550-1.wav");
	PRECACHE_SOUND("weapons/sg550_boltpull.wav");
	PRECACHE_SOUND("weapons/sg550_clipin.wav");
	PRECACHE_SOUND("weapons/sg550_clipout.wav");

	m_usSg550 = PRECACHE_EVENT( 1, "events/sg550.sc" );
}


int CSg550::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556nato";
	p->iMaxAmmo1 = M16_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 5;
	p->iPosition = 2;
	p->iId = WEAPON_SG550;
	p->iFlags = 0;
	p->iWeight = SG550_WEIGHT;
	return 1;
}


BOOL CSg550::Deploy( )
{
	m_pPlayer->m_newcross_active = 0;

	return DefaultDeploy( "models/v_sg550.mdl", 0, 4, "mp5",0,114 );
}

void CSg550::Holster( int skiplocal /* = 0 */ )
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

void CSg550::PrimaryAttack( void )
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;


		if ( m_pPlayer->pev->flags & FL_ONGROUND ) 
		{	
			if ( m_pPlayer->pev->velocity.Length2D() >= 140)
			{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.03,0.03,0.03), 7500, BULLET_556, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
			else{
				vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.01,0.01,0.01), 7500, BULLET_556, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
			}
		}
		else{
			vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.05,0.05,0.05), 7500, BULLET_556, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSg550, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->punchangle.x -= UTIL_SharedRandomFloat(m_pPlayer->random_seed + 4, 1.5, 1.75) + m_pPlayer->pev->punchangle.x * 0.25;
	m_pPlayer->pev->punchangle.y += UTIL_SharedRandomFloat(m_pPlayer->random_seed + 5, -1.0, 1.0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

void CSg550::SecondaryAttack()
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


void CSg550::Reload( void )
{
	if ( m_pPlayer->ammo_556nato <= 0 )
		return;

	if ( m_pPlayer->pev->fov != 0 )
	{
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0; // 0 means reset to default fov
		m_fInZoom = 0;
	}

	if ( DefaultReload( 20, 3, 3.5 ) )
	{
	//	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/xbow_reload1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CSg550::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		SendWeaponAnim( 0 );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 30.0;
	}
}



class CSg550Ammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items4.mdl");
		pev->body = 17;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 20, "556nato", M16_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_sg550, CSg550Ammo );



#endif