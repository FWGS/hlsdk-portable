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


#define	SWORT_BODYHIT_VOLUME 128
#define	SWORT_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_swort, CSwort );



enum swort_e 
{
	SWORT_IDLE = 0,
	SWORT_DRAW,
	SWORT_HOLSTER,
	SWORT_ATTACK1HIT,
	SWORT_ATTACK1MISS,
	SWORT_ATTACK2MISS,
	SWORT_ATTACK2HIT,
	SWORT_ATTACK3MISS,
#ifndef CROWBAR_IDLE_ANIM
        SWORT_ATTACK3HIT
#else
        SWORT_ATTACK3HIT,
        SWORT_IDLE2,
        SWORT_IDLE3
#endif
};


void CSwort::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SWORT;
	SET_MODEL(ENT(pev), "models/w_swort.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CSwort::Precache( void )
{
	PRECACHE_MODEL("models/v_swort.mdl");
	PRECACHE_MODEL("models/w_swort.mdl");
	PRECACHE_MODEL("models/p_swort.mdl");
	PRECACHE_SOUND("weapons/swort_hit1.wav");
	PRECACHE_SOUND("weapons/swort_hit2.wav");
	PRECACHE_SOUND("weapons/swort_hitbod1.wav");
	PRECACHE_SOUND("weapons/swort_hitbod2.wav");
	PRECACHE_SOUND("weapons/swort_hitbod3.wav");
	PRECACHE_SOUND("weapons/swort_miss1.wav");

	m_usSwort = PRECACHE_EVENT ( 1, "events/swort.sc" );
}

int CSwort::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_SWORT;
	p->iWeight = SWORT_WEIGHT;
	p->weaponName = "Swort";
	return 1;
}


int CSwort::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CSwort::Deploy( )
{
	return DefaultDeploy( "models/v_swort.mdl", "models/p_swort.mdl", SWORT_DRAW, "swort" );
}

void CSwort::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SWORT_HOLSTER );
}

extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr1, float *mins, float *maxs, edict_t *pEntity );
/*void FindHull2Intersection( const Vector &vecSrc, TraceResult &tr1, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHull2End = tr1.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHull2End = vecSrc + ((vecHull2End - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHull2End, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr1 = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHull2End.x + minmaxs[i][0];
				vecEnd.y = vecHull2End.y + minmaxs[j][1];
				vecEnd.z = vecHull2End.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr1 = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}
*/

void CSwort::PrimaryAttack()
{
	if (! Swing( 1 ))
	{
		SetThink(&CSwort:: SwingAgain );
		SetNextThink( 0.1 );
	}
}


void CSwort::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}


void CSwort::SwingAgain( void )
{
	Swing( 0 );
}


int CSwort::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection ( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if( fFirst )
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usSwort, 
		0.0, g_vecZero, g_vecZero, 0, 0, 0,
		0.0, 0, 0.0 );
	}

	if ( tr.flFraction >= 1.0 )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
#ifdef CROWBAR_IDLE_ANIM
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( ((m_iSwing++) % 2) + 1 )
		{
		case 0:
			SendWeaponAnim( SWORT_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( SWORT_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( SWORT_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage();
			// If building with the clientside weapon prediction system,
                        // UTIL_WeaponTimeBase() is always 0 and m_flNextPrimaryAttack is >= -1.0f, thus making
                        // m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() always evaluate to false.
#ifdef CLIENT_WEAPONS
                        if( ( m_flNextPrimaryAttack + 1 == UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#else
                        if( ( m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
#endif
			{
				// first swing does full damage
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSwort, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgSwort / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}	
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/swort_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/swort_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/swort_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = SWORT_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
					m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25; //LRC: corrected half-life bug
					return TRUE;
				}
				else
					  flVol = 0.1;

				fHitWorld = FALSE;
			}
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
		{
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_CROWBAR);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play crowbar strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/swort_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/swort_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * SWORT_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
		
		SetThink(&CSwort:: Smack );
		SetNextThink( 0.2 );

		
	}
#ifdef CROWBAR_IDLE_ANIM
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
#endif
	return fDidHit;
}

#ifdef CROWBAR_IDLE_ANIM
void CSwort::WeaponIdle( void )
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
