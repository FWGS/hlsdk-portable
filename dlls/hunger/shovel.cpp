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

#define	SHOVEL_BODYHIT_VOLUME 128
#define	SHOVEL_WALLHIT_VOLUME 512

extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );

LINK_ENTITY_TO_CLASS( weapon_th_shovel, CWeaponEinarShovel )

enum shovel_e
{
	SHOVEL_IDLE = 0,
	SHOVEL_DRAW,
	SHOVEL_HOLSTER,
	SHOVEL_ATTACK1HIT,
	SHOVEL_ATTACK1MISS,
	SHOVEL_ATTACK2MISS,
	SHOVEL_ATTACK2HIT,
	SHOVEL_ATTACK3MISS,
	SHOVEL_ATTACK3HIT,
	SHOVEL_IDLE2,
	SHOVEL_IDLE3
};

void CWeaponEinarShovel::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOVEL;
	SET_MODEL( ENT( pev ), "models/w_shovel.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CWeaponEinarShovel::Precache()
{
	PRECACHE_MODEL( "models/v_shovel.mdl" );
	PRECACHE_MODEL( "models/w_shovel.mdl" );
	PRECACHE_MODEL( "models/p_shovel.mdl" );
/*
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/cbar_hitbod3.wav" );
	PRECACHE_SOUND( "weapons/cbar_miss1.wav");

	PRECACHE_SOUND( "kelly/cbar_hitkelly1.wav" );
	PRECACHE_SOUND( "kelly/cbar_hitkelly2.wav" );
	PRECACHE_SOUND( "kelly/cbar_hitkelly3.wav" );
*/
}

int CWeaponEinarShovel::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_SHOVEL;
	p->iWeight = SHOVEL_WEIGHT;
	return 1;
}

int CWeaponEinarShovel::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarShovel::Deploy()
{
	return DefaultDeploy( "models/v_shovel.mdl", "models/p_shovel.mdl", SHOVEL_DRAW, "crowbar" );
}

void CWeaponEinarShovel::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SHOVEL_HOLSTER );
}

void CWeaponEinarShovel::PrimaryAttack()
{
	if( !Swing( 1 ) )
	{
		SetThink( &CWeaponEinarShovel::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CWeaponEinarShovel::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

void CWeaponEinarShovel::SwingAgain()
{
	Swing( 0 );
}

int CWeaponEinarShovel::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

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
	if( tr.flFraction >= 1.0 )
	{
		if( fFirst )
		{
			// miss
			if( RANDOM_FLOAT( 0.0, 1.0 ) <= 0.25 )
				++m_iSwing;
			switch( ( m_iSwing++ ) % 2 )
			{
			case 0:
				SendWeaponAnim( SHOVEL_ATTACK1MISS );
				break;
			case 1:
				SendWeaponAnim( SHOVEL_ATTACK2MISS );
				break;  
			/*case 2:
				SendWeaponAnim( SHOVEL_ATTACK3MISS );
				break;*/
                        }
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifdef CROWBAR_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
		}
	}
	else
	{
		// switch ( ( ( m_iSwing++ ) % 2 ) + 1 )
		switch( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SendWeaponAnim(	SHOVEL_ATTACK1HIT );
			break;
		case 1:
			SendWeaponAnim(	SHOVEL_ATTACK2HIT );
			break;
		case 2:
			SendWeaponAnim(	SHOVEL_ATTACK3HIT );
			break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if( pEntity )
		{
			float flDmg;
			ClearMultiDamage();

			if( g_pGameRules->IsMultiplayer() )
			{
				// more damage in multiplayer
				flDmg = gSkillData.plrDmgCrowbar * 1.6;
			}
			else if( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() )
			{
				// first swing does full damage
				flDmg = gSkillData.plrDmgCrowbar * 1.2;
			}
			else
			{
				// subsequent swings do half
				flDmg = gSkillData.plrDmgCrowbar * 0.6;
			}
			pEntity->TraceAttack( m_pPlayer->pev, flDmg, gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// Skeletons make different hit sounds.
				if( pEntity->Classify() == CLASS_SKELETON )
				{
					// play thwack or smack sound
					switch( RANDOM_LONG( 0, 2 ) )
					{
					case 0:
						EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "kelly/cbar_hitkelly1.wav", 1, ATTN_NORM );
						break;
					case 1:
						EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "kelly/cbar_hitkelly2.wav", 1, ATTN_NORM );
						break;
					case 2:
						EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "kelly/cbar_hitkelly3.wav", 1, ATTN_NORM );
						break;
					}
				}
				else
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
				}

				m_pPlayer->m_iWeaponVolume = SHOVEL_BODYHIT_VOLUME;
				if( !pEntity->IsAlive() )
					return TRUE;
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
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			case 1:
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * SHOVEL_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;

		SetThink( &CWeaponEinarShovel::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;
	}
#ifdef CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

#ifdef CROWBAR_IDLE_ANIM
void CWeaponEinarShovel::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9 )
		{
			iAnim = SHOVEL_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
		}
		else
		{
			if( flRand > 0.5 )
			{
				iAnim = SHOVEL_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 30.0;
			}
			else
			{
				iAnim = SHOVEL_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0 / 30.0;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
#endif
