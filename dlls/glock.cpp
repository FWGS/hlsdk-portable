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

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_DRAW_EMPTY,
	GLOCK_HOLSTER,
	GLOCK_HOLSTER_EMPTY,
};

LINK_ENTITY_TO_CLASS( weapon_glock, CGlock )
LINK_ENTITY_TO_CLASS( weapon_9mmhandgun, CGlock )

void CGlock::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_9mmhandgun" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL( ENT( pev ), "models/w_9mmhandgun.mdl" );

	m_iDefaultAmmo = RANDOM_LONG(7, 15);

	FallInit();// get ready to fall down.
}

void CGlock::Precache( void )
{
	PRECACHE_MODEL( "models/v_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/w_9mmhandgun.mdl" );
	PRECACHE_MODEL( "models/p_9mmhandgun.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );
	PRECACHE_SOUND("items/deploy_357.wav");
	PRECACHE_SOUND("items/deploy_default1.wav");

	PRECACHE_SOUND( "weapons/pl_gun1.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun2.wav" );//silenced handgun
	PRECACHE_SOUND( "weapons/pl_gun3.wav" );//handgun
	PRECACHE_SOUND("weapons/common_hand1.wav");
	PRECACHE_SOUND("weapons/glock_slideback1.wav");

	PRECACHE_SOUND("items/9mmclip_insert.wav");
	PRECACHE_SOUND("items/9mmclip_release.wav");
	PRECACHE_SOUND("items/9mmclip_slide.wav");
	PRECACHE_SOUND("items/9mmclip_pickup.wav");


	PRECACHE_SOUND("fvox/ammo_low.wav");

	m_usFireGlock1 = PRECACHE_EVENT( 1, "events/glock1.sc" );
	m_usFireGlock2 = PRECACHE_EVENT( 1, "events/glock2.sc" );

	m_flBurstTime = 0;
}

int CGlock::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

int CGlock::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGlock::Deploy()
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);

	m_flBurstTime = 0;
	m_fBurstShot = 0;
	m_fBurstFire = FALSE;

	if (m_iClip <= 0)
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.45;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.45;
		return DefaultDeploy("models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW_EMPTY, "onehanded", /*UseDecrement() ? 1 : 0*/ 0);
	}
	else
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.95;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.95;
		return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
	}

	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 ); 
}

void CGlock::Holster( int skiplocal /* = 0 */ )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.49;
	if (m_iClip <= 0)
	{
		SendWeaponAnim( GLOCK_HOLSTER_EMPTY );
	}
	else
	{
		SendWeaponAnim( GLOCK_HOLSTER );
	}
}

void CGlock::SecondaryAttack( void )
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_fBurstShot = 0;
		m_fBurstFire = FALSE;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;

		return;
	}

	if (m_fBurstShot == 0)
		m_fBurstFire = TRUE;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.32;
}

void CGlock::PrimaryAttack( void )
{
	m_fBurstShot = 0;
	m_fBurstFire = FALSE;
	GlockFire( 0.01f, 0.3f, TRUE );
}

void CGlock::GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	if( m_iClip <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}
		m_fBurstShot = 0;
		m_fBurstFire = FALSE;

		return;
	}

	m_iClip--;

	if (m_iClip == 6)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	#ifndef CLIENT_DLL
	UTIL_ScreenShake( pev->origin, 1, 5.0, 0.25, 120 ); 
	#endif
	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	// silenced
	if( pev->body == 1 )
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

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming;

	if( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), fUseAutoAim ? m_usFireGlock1 : m_usFireGlock2, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGlock::ItemPostFrame()
{
	if ((m_fBurstShot <= 2) && (m_flBurstTime < gpGlobals->time) && m_fBurstFire) 
	{
		m_flBurstTime = 0;
		m_fBurstShot++;
		m_fBurstFire = TRUE;

		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.32;

		m_flBurstTime = gpGlobals->time + 0.085;
		GlockFire( 0.03, 0.1, FALSE );

		if (m_fBurstShot == 3)
		{
			m_fBurstFire = FALSE;
			m_fBurstShot = 0;

			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.32;
		}
		if (m_iClip <= 0)
			PlayEmptySound();
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CGlock::Reload( void )
{
	int iResult;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GLOCK_MAX_CLIP)
	{
		return;
	}
	else
	{
		if( m_iClip == 0 )
		{
			iResult = DefaultReload(17, GLOCK_RELOAD, 1.3);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.1;
		}
		else
		{
			iResult = DefaultReload(17, GLOCK_RELOAD_NOT_EMPTY, 1.3);
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 1.7;
		}
	}

	if( iResult )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}

	m_fBurstFire = FALSE;
	m_fBurstShot = 0;
}

void CGlock::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if( m_iClip != 0 )
	{
		int iAnim;
		iAnim = GLOCK_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 52.0 / 16.0;
		SendWeaponAnim( iAnim, 1 );
	}
}

class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND("items/9mmclip_pickup.wav");
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip_pickup.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_glockclip, CGlockAmmo )
LINK_ENTITY_TO_CLASS( ammo_9mmclip, CGlockAmmo )
