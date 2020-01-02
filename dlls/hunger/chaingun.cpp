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
#include "gamerules.h"

enum chaingun_e 
{
	CHAINGUN_IDLE = 0,
	CHAINGUN_IDLE2,
	CHAINGUN_SPINUP,
	CHAINGUN_SPINDOWN,
	CHAINGUN_FIRE,
	CHAINGUN_DRAW,
	CHAINGUN_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_hornetgun, CWeaponEinarChaingun )
LINK_ENTITY_TO_CLASS( weapon_th_chaingun, CWeaponEinarChaingun )

void CWeaponEinarChaingun::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_th_chaingun" );
	Precache();
	m_iId = WEAPON_CHAINGUN;
	SET_MODEL( ENT( pev ), "models/w_tfac.mdl" );

	m_iDefaultAmmo = CHAINGUN_DEFAULT_GIVE;

	m_fInAttack = 0;
	m_fInSpecialReload = 0;

	FallInit();// get ready to fall down.
}

void CWeaponEinarChaingun::Precache()
{
	PRECACHE_MODEL( "models/v_tfac.mdl" );
	PRECACHE_MODEL( "models/w_tfac.mdl" );
	PRECACHE_MODEL( "models/p_tfac.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );

	PRECACHE_SOUND( "weapons/asscan1.wav" );
	PRECACHE_SOUND( "weapons/asscan2.wav" );
	PRECACHE_SOUND( "weapons/asscan3.wav" );
	PRECACHE_SOUND( "weapons/asscan4.wav" );

	m_usFireChaingun1 = PRECACHE_EVENT( 1, "events/chaingun1.sc" );
	m_usFireChaingun2 = PRECACHE_EVENT( 1, "events/chaingun2.sc" );
}

int CWeaponEinarChaingun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CHAINGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CHAINGUN;
	p->iWeight = CHAINGUN_WEIGHT;

	return 1;
}

BOOL CWeaponEinarChaingun::Deploy()
{
	return DefaultDeploy( "models/v_tfac.mdl", "models/p_tfac.mdl", CHAINGUN_DRAW, "egon" );
}

void CWeaponEinarChaingun::Holster( int skiplocal /*= 0*/ )
{
	m_fInSpecialReload = 0;
	if( m_fInAttack )
		SpinDown();
	else
		SendWeaponAnim( CHAINGUN_HOLSTER );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CWeaponEinarChaingun::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0 )
	{
		if( m_fInAttack != 0 )
		{
			// spin down
			SpinDown();
		}
		else
		{
			PlayEmptySound();
			m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		}
		return;
	}

	if( !m_fInAttack )
	{
		// Spin up
		SpinUp();
	}
	else
	{
		if( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
		{	
#ifdef CLIENT_DLL
			if( !bIsMultiplayer() )
#else
			if( !g_pGameRules->IsMultiplayer() )
#endif
			{
				// single player spread
				Fire( 0.1, 0.1, FALSE );
			}
			else
			{
				// optimized multiplayer. Widened to make it easier to hit a moving player
				Fire( 0.15, 0.15, FALSE );
			}
		}
	}
}

void CWeaponEinarChaingun::SecondaryAttack()
{
	if( m_fInAttack )
		SpinDown();
}

void CWeaponEinarChaingun::SpinUp()
{
	// spin up
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

	// Slowdown player.
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireChaingun1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, TRUE, 0 );

	m_fInAttack = 1;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;

}

void CWeaponEinarChaingun::SpinDown()
{
	// Restore player speed.
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireChaingun1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, FALSE, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.2f;

	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
}

void CWeaponEinarChaingun::Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	int iClipNotEmpty = 0;
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;
	if( m_iClip )
	{
		m_iClip--;
		iClipNotEmpty = 1;
	}
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecDir = m_pPlayer->FireBulletsPlayer( iClipNotEmpty ? 4 : 2, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_MP5, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireChaingun2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, iClipNotEmpty, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.1f;
}

void CWeaponEinarChaingun::Reload()
{
	if( m_fInAttack )
	{
		SpinDown();
	}
	else if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == CHAINGUN_MAX_CLIP )
	{
		return;
	}
	else if( DefaultReload( CHAINGUN_MAX_CLIP, CHAINGUN_HOLSTER, 0.5 ) )
	{
		m_fInSpecialReload = 2;

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 5, 10 );
	}
}

void CWeaponEinarChaingun::WeaponIdle()
{
	ResetEmptySound();

	if( m_pPlayer->m_flNextAttack < UTIL_WeaponTimeBase() )
	{
		if( m_fInSpecialReload == 3 )
		{
			SendWeaponAnim( CHAINGUN_DRAW, 0 );
			m_fInSpecialReload = 1;
		}
		else if( m_fInSpecialReload == 2 )
		{
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 0x1f ) );
			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
			m_fInSpecialReload = 3;
		}
	}

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack != 0 )
	{
		// Spin down
		SpinDown();
	}
	else
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand <= 0.5f )
		{
			iAnim = CHAINGUN_IDLE;
		}
		else
		{
			iAnim = CHAINGUN_IDLE2;
		}
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		SendWeaponAnim( iAnim, 1 );
	}
}
