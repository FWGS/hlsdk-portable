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

enum beretta_e {
	BERETTA_IDLE1 = 0,
	BERETTA_IDLE2,
	BERETTA_IDLE3,
	BERETTA_SHOOT,
	BERETTA_SHOOT_EMPTY,
	BERETTA_RELOAD,
	BERETTA_RELOAD_NOT_EMPTY,
	BERETTA_DRAW,
	BERETTA_HOLSTER,
	BERETTA_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_beretta, CBeretta );



void CBeretta::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_beretta"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_BERETTA;
	SET_MODEL(ENT(pev), "models/w_9mmberetta.mdl");

	m_iDefaultAmmo = BERETTA_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CBeretta::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmberetta.mdl");
	PRECACHE_MODEL("models/w_9mmberetta.mdl");
	PRECACHE_MODEL("models/p_9mmberetta.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	/* PRECACHE_SOUND ("weapons/pl_gun1.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun2.wav");//silenced handgun
	PRECACHE_SOUND ("weapons/pl_gun3.wav");//handgun */
	PRECACHE_SOUND ("weapons/beretta_fire1.wav");//single fire Beretta

	m_usFireBeretta1 = PRECACHE_EVENT( 1, "events/Beretta1.sc" );
	m_usFireBeretta2 = PRECACHE_EVENT( 1, "events/Beretta2.sc" );
}

int CBeretta::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BERETTA_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BERETTA;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CBeretta::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CBeretta::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_9mmberetta.mdl", "models/p_9mmberetta.mdl", BERETTA_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CBeretta::Holster( int skiplocal /* = 0 */ )
{
	DefaultHolster( BERETTA_HOLSTER, 1.2 );
}

void CBeretta::SecondaryAttack( void )
{
	BerettaFire( 0.1, 0.2, FALSE );
}

void CBeretta::PrimaryAttack( void )
{
	BerettaFire( 0.01, 0.3, TRUE );
}

void CBeretta::BerettaFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	}
	else
	{
		// non-silenced
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_BERETTA, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireBeretta1 : m_usFireBeretta2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CBeretta::Reload( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == BERETTA_MAX_CLIP )
		return;

	int iResult;

	if (m_iClip == 0)
		iResult = DefaultReload( BERETTA_MAX_CLIP, BERETTA_RELOAD, 1.5 );
	else
		iResult = DefaultReload( BERETTA_MAX_CLIP, BERETTA_RELOAD_NOT_EMPTY, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CBeretta::WeaponIdle( void )
{
	if( m_pPlayer->m_bIsHolster )
        {
                if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
                {
                        m_pPlayer->m_bIsHolster = FALSE;
                        Deploy();
                }
		return;
        }

	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3f)
		{
			iAnim = BERETTA_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
		}
		else if (flRand <= 0.6f)
		{
			iAnim = BERETTA_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
		}
		else
		{
			iAnim = BERETTA_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

