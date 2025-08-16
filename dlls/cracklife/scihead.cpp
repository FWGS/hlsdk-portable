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

#define	SCIHEAD_BODYHIT_VOLUME 128
#define	SCIHEAD_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_scihead, CScihead )

enum scihead_e
{
	SCIHEAD_IDLE = 0,
	SCIHEAD_DRAW,
	SCIHEAD_HOLSTER,
	SCIHEAD_ATTACK1HIT,
	SCIHEAD_ATTACK1MISS,
	SCIHEAD_ATTACK2MISS,
	SCIHEAD_ATTACK2HIT,
	SCIHEAD_ATTACK3MISS,
#if !CROWBAR_IDLE_ANIM	
	SCIHEAD_ATTACK3HIT
#else
	SCIHEAD_ATTACK3HIT,
	SCIHEAD_IDLE2,
	SCIHEAD_IDLE3
#endif
};

void CScihead::Spawn()
{
	Precache();
	m_iId = WEAPON_SCIHEAD;
	SET_MODEL( ENT( pev ), "models/w_crowbar.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CScihead::Precache( void )
{
	PRECACHE_MODEL( "models/v_scihead.mdl" );
	PRECACHE_MODEL( "models/w_crowbar.mdl" );
	PRECACHE_MODEL( "models/p_crowbar.mdl" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav" );

	m_usScihead = PRECACHE_EVENT( 1, "events/scihead.sc" );
}

int CScihead::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_SCIHEAD;
	p->iWeight = SCIHEAD_WEIGHT;
	return 1;
}

int CScihead::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CScihead::Deploy()
{
	return DefaultDeploy( "models/v_scihead.mdl", "models/p_crowbar.mdl", SCIHEAD_DRAW, "crowbar" );
}

void CScihead::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( SCIHEAD_HOLSTER );
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

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2.0f );
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
}
*/
void CScihead::PrimaryAttack()
{
	if( !Swing( 1 ) )
	{
#if !CLIENT_DLL
		SetThink( &CScihead::SwingAgain );
		pev->nextthink = gpGlobals->time + 1.0f;
#endif
	}
}

void CScihead::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CScihead::SwingAgain( void )
{
	Swing( 0 );
}

int CScihead::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32.0f;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
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
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usScihead, 
		0.0f, g_vecZero, g_vecZero, 0, 0, 0,
		0, 0, 0 );
	}

	if( tr.flFraction >= 1.0f )
	{
		if( fFirst )
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
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
			SendWeaponAnim( SCIHEAD_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim( SCIHEAD_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim( SCIHEAD_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

#if !CLIENT_DLL
		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
                float flVol = 1.0f;
                int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();
			// If building with the clientside weapon prediction system,
			// UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
			// m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
#if CLIENT_WEAPONS
			if( ( m_flNextPrimaryAttack + 1.0f == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
			if( ( m_flNextPrimaryAttack + 1.0f < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgScihead, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgScihead * 0.5f, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG( 0, 2 ) )
				{
				case 0:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1.0f, ATTN_NORM );
					break;
				case 1:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1.0f, ATTN_NORM );
					break;
				case 2:
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1.0f, ATTN_NORM );
					break;
				}

				m_pPlayer->m_iWeaponVolume = SCIHEAD_BODYHIT_VOLUME;

				if( !pEntity->IsAlive() )
				{
#if CROWBAR_FIX_RAPID_CROWBAR
					m_flNextPrimaryAttack = GetNextAttackDelay(1.0f);
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
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2.0f, BULLET_PLAYER_CROWBAR );

			if( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1.0f;
			}

			// also play crowbar strike
			switch( RANDOM_LONG( 0, 1 ) )
			{
			case 0:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) ); 
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = (int)( flVol * SCIHEAD_WALLHIT_VOLUME );

		SetThink( &CScihead::Smack );
		pev->nextthink = gpGlobals->time + 1.0f;
#endif
#if CROWBAR_DELAY_FIX
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
#else
		m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
#endif
	}
#if CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

#if CROWBAR_IDLE_ANIM
void CScihead::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9f )
		{
			iAnim = SCIHEAD_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
		}
		else
		{
			if( flRand > 0.5f )
			{
				iAnim = SCIHEAD_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 25.0f;
			}
			else
			{
				iAnim = SCIHEAD_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
