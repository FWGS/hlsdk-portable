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

#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_heaterpipe, CCrowbar )

enum crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
#ifndef CROWBAR_IDLE_ANIM	
	CROWBAR_ATTACK3HIT
#else
	CROWBAR_ATTACK3HIT,
	CROWBAR_IDLE2,
	CROWBAR_IDLE3
#endif
};

void CCrowbar::Spawn()
{
	Precache();
	m_iId = WEAPON_CROWBAR;
	SET_MODEL( ENT( pev ), "models/w_heaterpipe.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CCrowbar::Precache( void )
{
	PRECACHE_MODEL( "models/v_heaterpipe.mdl" );
	PRECACHE_MODEL( "models/w_heaterpipe.mdl" );
	PRECACHE_MODEL( "models/p_heaterpipe.mdl" );
	PRECACHE_SOUND( "weapons/pipe_hit1.wav" );
	PRECACHE_SOUND( "weapons/pipe_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/pipe_miss.wav" );
	PRECACHE_SOUND( "player/pl_breathe.wav" );
}

int CCrowbar::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

int CCrowbar::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CCrowbar::Deploy()
{
	return DefaultDeploy( "models/v_heaterpipe.mdl", "models/p_heaterpipe.mdl", CROWBAR_DRAW, "crowbar" );
}

void CCrowbar::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( CROWBAR_HOLSTER );
}

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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
	if( tmpTrace.flFraction < 1.0 )
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
				if( tmpTrace.flFraction < 1.0 )
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
}

void CCrowbar::PrimaryAttack()
{
	if( m_flNextPrimaryAttack + 1 <= UTIL_WeaponTimeBase() )
	{
		m_iSwing = Q_max( 0, m_iSwing - static_cast<int>( ( UTIL_WeaponTimeBase() - m_flNextPrimaryAttack ) * 5.0f ) );
	}

	Swing();

	++m_iSwing;
}

void CCrowbar::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CCrowbar::Swing()
{
	TraceResult tr;
	float flVol;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if( tr.flFraction < 1.0 )
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
	if( m_iSwing > 4 && m_flNextBreathSoundTime <= UTIL_WeaponTimeBase() )
	{
		flVol = Q_min( 1.0f, static_cast<float>( m_iSwing - 3 ) * 0.2 );
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_BODY, "player/pl_breathe.wav", flVol, ATTN_NORM );
		m_flNextBreathSoundTime = UTIL_WeaponTimeBase() + 1.2f;
	}

	if( tr.flFraction >= 1.0 )
	{
		// miss
		switch( ( m_iSwing++ ) % 3 )
		{
		case 0:
			SendWeaponAnim( CROWBAR_ATTACK1MISS );
			break;
		case 1:
			SendWeaponAnim( CROWBAR_ATTACK2MISS );
			break;
		case 2:
			SendWeaponAnim( CROWBAR_ATTACK3MISS );
			break;
		}
		m_flNextPrimaryAttack = ( static_cast<float>( m_iSwing ) * 0.03f ) + UTIL_WeaponTimeBase() + 0.5f;
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/pipe_miss.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
#ifdef CROWBAR_IDLE_ANIM
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
	else
	{
		switch( ( m_iSwing++ ) % 3 )
		{
		case 0:
			SendWeaponAnim( CROWBAR_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( CROWBAR_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( CROWBAR_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL
		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
		flVol = 1.0;
		int fHitWorld = TRUE;

		m_flNextPrimaryAttack = ( static_cast<float>( m_iSwing ) * 0.03f ) + UTIL_WeaponTimeBase() + 0.3f;

		if( pEntity )
		{
			ClearMultiDamage();

			float flDmg = Q_max( 1.0f, gSkillData.plrDmgCrowbar - ( static_cast<float>( m_iSwing ) * 0.5f ) );
			pEntity->TraceAttack( m_pPlayer->pev, flDmg, gpGlobals->v_forward, &tr, DMG_CLUB ); 

			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 2 ) )
				{
				case 0:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM );
					break;
				}
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
					return;
				else
					flVol = 0.1;

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
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pipe_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) ); 
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/pipe_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * CROWBAR_WALLHIT_VOLUME );

		SetThink( &CCrowbar::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.1f;
#endif
	}
#ifdef CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
}

#ifdef CROWBAR_IDLE_ANIM
void CCrowbar::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9 )
		{
			iAnim = CROWBAR_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
		}
		else
		{
			if( flRand > 0.5 )
			{
				iAnim = CROWBAR_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 30.0;
			}
			else
			{
				iAnim = CROWBAR_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
