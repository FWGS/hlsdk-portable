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
#include "nail.h"

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3,
	MP5_DEPLOY_EMPTY,
	MP5_LONGIDLE_EMPTY,
	MP5_IDLE1_EMPTY
};

LINK_ENTITY_TO_CLASS( weapon_nailgun, CMP5 )

//=========================================================
//=========================================================
void CMP5::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_nailgun.mdl" );
	m_iId = WEAPON_MP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CMP5::Precache( void )
{
	PRECACHE_MODEL( "models/v_nailgun.mdl" );
	PRECACHE_MODEL( "models/w_nailgun.mdl" );
	PRECACHE_MODEL( "models/p_nailgun.mdl" );

	PRECACHE_MODEL( "models/w_nailround.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );

	PRECACHE_SOUND( "weapons/nailgun.wav" );

	UTIL_PrecacheOther( "nailgun_nail" );
}

int CMP5::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

int CMP5::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMP5::Deploy()
{
	return DefaultDeploy( "models/v_nailgun.mdl", "models/p_nailgun.mdl", m_iClip ? MP5_DEPLOY : MP5_DEPLOY_EMPTY, "mp5" );
}

void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	m_iClip--;

	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/nailgun.wav", 1.0, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );

	SendWeaponAnim( MP5_FIRE1 + RANDOM_LONG( 0, 2 ) );

	// m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecAngles = m_pPlayer->pev->punchangle + m_pPlayer->pev->v_angle;
	UTIL_MakeVectors( vecAngles );
	vecAngles.x = -vecAngles.x;

	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 3 + gpGlobals->v_right * 3;
	float flSpread = 0.0349;
                        
	float x, y;
	do{      
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
	}
	while( x * x + y * y > 1.0f );

	Vector vecSpread = x * flSpread * gpGlobals->v_up + y * flSpread * gpGlobals->v_right + gpGlobals->v_forward;
	Vector vecDest = vecSrc + vecSpread * 2048.0f;

	UTIL_MakeTracer( vecSrc, vecDest );

	CNailGunNail *pNail = CNailGunNail::NailCreate( FALSE );
	pNail->pev->origin = vecSrc;
	pNail->pev->angles = vecAngles;
	pNail->pev->owner = m_pPlayer->edict();
	pNail->pev->velocity = vecSpread * 1600.0f;
	pNail->pev->speed = 1600.0f;
	pNail->pev->avelocity.z = 30;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	if( !m_iClip )
		SendWeaponAnim( MP5_LONGIDLE_EMPTY );
	m_pPlayer->pev->punchangle.x -= 1.0f;
}

void CMP5::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == MP5_MAX_CLIP )
		return;

	DefaultReload( MP5_MAX_CLIP, MP5_RELOAD, 1.5f );
}

void CMP5::WeaponIdle( void )
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = m_iClip ? MP5_LONGIDLE : MP5_LONGIDLE_EMPTY;	
		break;
	default:
	case 1:
		iAnim = m_iClip ? MP5_IDLE1 : MP5_IDLE1_EMPTY;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_nailround.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_nailround.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = ( pOther->GiveAmmo( AMMO_MP5CLIP_GIVE, "9mm", _9MM_MAX_CARRY ) != -1 );
		if( bResult )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_nailround, CMP5AmmoClip )
