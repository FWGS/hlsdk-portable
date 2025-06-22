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
#include "gamerules.h"

#define	KATANA_BODYHIT_VOLUME 128
#define	KATANA_WALLHIT_VOLUME 512

enum katana_e
{
	KATANA_IDLE = 0,
	KATANA_DRAW,
	KATANA_HOLSTER,
	KATANA_ATTACK1HIT,
	KATANA_ATTACK1MISS,
	KATANA_ATTACK2MISS,
	KATANA_ATTACK2HIT,
	KATANA_ATTACK3MISS,
#if !CROWBAR_IDLE_ANIM
	KATANA_ATTACK3HIT
#else
	KATANA_ATTACK3HIT,
	KATANA_IDLE2,
	KATANA_IDLE3
#endif
};

LINK_ENTITY_TO_CLASS( weapon_katana, CKatana )

void CKatana::Spawn( )
{
	Precache();
	m_iId = WEAPON_KATANA;
	SET_MODEL( ENT( pev ), "models/w_katana.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CKatana::Precache( void )
{
	PRECACHE_MODEL( "models/fedora.mdl" );
	PRECACHE_MODEL( "models/v_katana.mdl" );
	PRECACHE_MODEL( "models/w_katana.mdl" );
	PRECACHE_MODEL( "models/p_katana.mdl" );
	PRECACHE_SOUND( "weapons/katana_draw.wav" );
	PRECACHE_SOUND( "weapons/katana_hit1.wav" );
	PRECACHE_SOUND( "weapons/katana_hit2.wav" );
	PRECACHE_SOUND( "weapons/katana_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/katana_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/katana_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/katana_tip.wav" );
	PRECACHE_SOUND( "weapons/katana_miss1.wav" );

	m_usKatana = PRECACHE_EVENT( 1, "events/katana.sc" );
}

int CKatana::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_KATANA;
	p->iWeight = KATANA_WEIGHT;

	return 1;
}

int CKatana::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CKatana::Deploy()
{
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_draw.wav", 1, ATTN_NORM );
	return DefaultDeploy( "models/v_katana.mdl", "models/p_katana.mdl", KATANA_DRAW, "katana" );
}
 
void CKatana::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( KATANA_HOLSTER );
}

extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );
/*void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int		i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult	tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if( tmpTrace.flFraction < 1.0f )
	{
		tr = tmpTrace;
		return;
	}

	for( i = 0; i < 2; i++ )
	{
		for( j = 0; j < 2; j++ )
		{
			for( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if( tmpTrace.flFraction < 1.0f )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}*/

void CKatana::PrimaryAttack()
{
	if( !Swing( 1 ) )
	{
#if !CLIENT_DLL
		SetThink( &CKatana::SwingAgain );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.6f;
#endif
	}
}

void CKatana::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CKatana::SwingAgain( void )
{
	Swing( 0 );
}

int CKatana::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif
	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usKatana, 
		0.0, g_vecZero, g_vecZero, 0, 0, 0,
		0.0, 0, 0.0 );
	}

	if( tr.flFraction >= 1.0 )
	{
		if( fFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.6f );

#if CROWBAR_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ( ( m_iSwing++ ) % 2 ) + 1 )
		{
		case 0:
			SendWeaponAnim( KATANA_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( KATANA_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( KATANA_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
		float flVol = 1.0f;
		int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();

			// swing does full damage
			pEntity->TraceAttack( m_pPlayer->pev, 85, gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 2 ) )
				{
				case 0:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_hitbod1.wav", 1, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_hitbod2.wav", 1, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_hitbod3.wav", 1, ATTN_NORM );
					break;
				}
				m_pPlayer->m_iWeaponVolume = KATANA_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
				{
#if CROWBAR_FIX_RAPID_CROWBAR
					m_flNextPrimaryAttack = GetNextAttackDelay( 0.6f );
#endif
					return TRUE;
				}
				else
					flVol = 0.1f;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) ); 
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * KATANA_WALLHIT_VOLUME;

		SetThink( &CKatana::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.6f;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.6f;
	}
#if CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

void CKatana::SecondaryAttack( void )
{
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/katana_tip.wav", 1, ATTN_NORM );
#if !CLIENT_DLL
	CGrenadeFedora::ShootContact( m_pPlayer->pev, m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 15, gpGlobals->v_forward * 512 );
#endif
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.0f;
}

#if CROWBAR_IDLE_ANIM
void CKatana::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9f )
		{
			iAnim = KATANA_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
		}
		else
		{
			if( flRand > 0.5f )
			{
				iAnim = KATANA_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 25.0f;
			}
			else
			{
				iAnim = KATANA_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
