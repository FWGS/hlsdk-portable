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
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"
#include "flame.h"

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#define EGON_SWITCH_NARROW_TIME			0.75f			// Time it takes to switch fire modes
#define EGON_SWITCH_WIDE_TIME			1.5f

enum egon_e {
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_egon, CEgon )

void CEgon::Spawn()
{
	Precache();
	m_iId = WEAPON_EGON;
	SET_MODEL( ENT( pev ), "models/w_egon.mdl" );

	m_iDefaultAmmo = EGON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CEgon::Precache( void )
{
	PRECACHE_MODEL( "models/w_egon.mdl" );
	PRECACHE_MODEL( "models/v_egon.mdl" );
	PRECACHE_MODEL( "models/p_egon.mdl" );

	PRECACHE_MODEL( "models/w_9mmclip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( EGON_SOUND_OFF );
	PRECACHE_SOUND( EGON_SOUND_RUN );
	PRECACHE_SOUND( EGON_SOUND_STARTUP );

	PRECACHE_MODEL( EGON_BEAM_SPRITE );
	PRECACHE_MODEL( EGON_FLARE_SPRITE );

	PRECACHE_SOUND( "weapons/357_cock1.wav" );

	PRECACHE_SOUND( "weapons/flmfire2.wav" );

	UTIL_PrecacheOther( "einar_flame" );
}

BOOL CEgon::Deploy( void )
{
	return DefaultDeploy( "models/v_egon.mdl", "models/p_egon.mdl", EGON_DRAW, "egon" );
}

int CEgon::AddToPlayer( CBasePlayer *pPlayer )
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

void CEgon::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( EGON_HOLSTER );

	EndAttack();
}

int CEgon::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "gas";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_EGON;
	p->iFlags = 0;
	p->iWeight = EGON_WEIGHT;

	return 1;
}

BOOL CEgon::HasAmmo( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return FALSE;

	return TRUE;
}

void CEgon::UseAmmo( int count )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
	else
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;
}

void CEgon::PrimaryAttack( void )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
		return;
	}

	if( !HasAmmo() )
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
		PlayEmptySound();
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = EGON_PRIMARY_VOLUME;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UseAmmo( 1 );

	SendWeaponAnim( EGON_FIRE1 + RANDOM_LONG( 0, 3 ), 0 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/flmfire2.wav", 1.0, ATTN_NORM, 0, 93 + RANDOM_LONG( 0, 15 ) );

#ifndef CLIENT_DLL
	Vector	position, velocity;
	Vector forward, right, up;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	forward = gpGlobals->v_forward;
	right = gpGlobals->v_right;
	up = gpGlobals->v_up;

	position = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	position = position + forward * 20;
	position = position + right * 5;
	position = position + up * 3;

	CEinarFlameRocket::FlameCreate( position, m_pPlayer->pev->v_angle, ENT( m_pPlayer->pev ) );
#endif
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
}

void CEgon::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;

	float flRand = RANDOM_FLOAT( 0.0f, 1.0f );

	if( flRand <= 0.5f )
	{
		iAnim = EGON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
	}
	else 
	{
		iAnim = EGON_FIDGET1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	SendWeaponAnim( iAnim );
}

void CEgon::EndAttack( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, "weapons/egon_run3.wav", 0, 0, SND_STOP, PITCH_NORM );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/egon_off1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

class CEgonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_gas.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_gas.mdl" );
		PRECACHE_SOUND( "player/pl_slosh1.wav" );
	}

	BOOL AddAmmo( CBaseEntity *pOther )
	{
		if( pOther->GiveAmmo( AMMO_GAS_GIVE, "gas", URANIUM_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "player/pl_slosh1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_egonclip, CEgonAmmo )
#endif
