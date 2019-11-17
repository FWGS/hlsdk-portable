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
#include "nail.h"

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
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER,
	GLOCK_UPRIGHT_TO_TILT,
	GLOCK_TILT_TO_UPRIGHT,
	GLOCK_FASTSHOOT
};

LINK_ENTITY_TO_CLASS( weapon_bradnailer, CGlock )

void CGlock::Spawn()
{
	// pev->classname = MAKE_STRING( "weapon_9mmhandgun" ); // hack to allow for old names
	Precache();
	m_iId = WEAPON_GLOCK;
	SET_MODEL( ENT( pev ), "models/w_bradnailer.mdl" );

	m_fInAttack = 0;
	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CGlock::Precache( void )
{
	PRECACHE_MODEL( "models/v_bradnailer.mdl" );
	PRECACHE_MODEL( "models/w_bradnailer.mdl" );
	PRECACHE_MODEL( "models/p_bradnailer.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );
	PRECACHE_SOUND( "weapons/bradnailer.wav" );

	// UTIL_PrecacheOther( "nailgun_nail" );
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
	// pev->body = 1;
	return DefaultDeploy( "models/v_bradnailer.mdl", "models/p_bradnailer.mdl", GLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CGlock::SecondaryAttack( void )
{
	GlockFire( 0.2f, FALSE );
}

void CGlock::PrimaryAttack( void )
{
	GlockFire( 0.3f, TRUE );
}

void CGlock::GlockFire( float flCycleTime, BOOL fUseAutoAim )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( m_iClip <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		}
		return;
	}

	if( fUseAutoAim )
	{
		if( m_fInAttack )
		{
			SendWeaponAnim( GLOCK_TILT_TO_UPRIGHT );
			m_fInAttack = 0;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
			return;
		}

		SendWeaponAnim( GLOCK_SHOOT );
	}
	else
	{
		if( !m_fInAttack )
		{
			SendWeaponAnim( GLOCK_UPRIGHT_TO_TILT );
			m_fInAttack = 1;
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
			return;
		}

		SendWeaponAnim( GLOCK_FASTSHOOT );
	}
	m_iClip--;

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/bradnailer.wav", 1.0, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );

	// m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecAngles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( vecAngles );

	vecAngles.x = -vecAngles.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 3;
	float flSpread;

	if( fUseAutoAim )
	{
		vecSrc = vecSrc + gpGlobals->v_up + gpGlobals->v_right * 2;
		flSpread = 0.008732f;
	}
	else
	{
		flSpread = 0.06976f;
	}

	float x, y;
	do{
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
	}
	while( x * x + y * y > 1.0f );

	Vector vecSpread = x * flSpread * gpGlobals->v_up + y * flSpread * gpGlobals->v_right + gpGlobals->v_forward;
	Vector vecDest = vecSrc + vecSpread * 2048.0f;

	UTIL_MakeTracer( vecSrc, vecDest );

	CNailGunNail *pNail = CNailGunNail::NailCreate( TRUE );
	pNail->pev->origin = vecSrc;
	pNail->pev->angles = vecAngles;
	pNail->pev->owner = m_pPlayer->edict();
	pNail->pev->velocity = vecSpread * 1600.0f;
	pNail->pev->speed = 1600.0f;
	pNail->pev->avelocity.z = 10;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if( m_fInAttack )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	}
	else
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}

	m_pPlayer->pev->punchangle.x -= 2.0f;
}

void CGlock::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GLOCK_MAX_CLIP )
		return;

	if( m_fInAttack )
	{
		if( !m_fInSpecialReload )
		{
			SendWeaponAnim( GLOCK_TILT_TO_UPRIGHT );

			SetThink( &CGlock::ReloadWait );
			pev->nextthink = gpGlobals->time + 0.4f;
			m_fInSpecialReload = 1;
		}
	}
	else if( DefaultReload( GLOCK_MAX_CLIP, m_iClip ? GLOCK_RELOAD : GLOCK_RELOAD_NOT_EMPTY, 2.3 ) )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CGlock::ReloadWait()
{
	m_fInAttack = m_fInSpecialReload = 0;
	SetThink( NULL );
	Reload();
}

void CGlock::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack )
	{
		m_fInAttack = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.4f;
		SendWeaponAnim( GLOCK_TILT_TO_UPRIGHT );
	}
	else
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if( flRand <= 0.3f + 0 * 0.75f )
		{
			iAnim = GLOCK_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0f / 16.0f;
		}
		else if( flRand <= 0.6f + 0 * 0.875f )
		{
			iAnim = GLOCK_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0f / 16.0f;
		}
		else
		{
			iAnim = GLOCK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}

class CGlockAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_nailclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_nailclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_GLOCKCLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_nailclip, CGlockAmmo )
