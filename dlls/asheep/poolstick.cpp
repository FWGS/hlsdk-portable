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


#define	POOLSTICK_BODYHIT_VOLUME 128
#define	POOLSTICK_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_poolstick, CPoolstick )


enum poolstick_e {
	POOLSTICK_IDLE = 0,
	POOLSTICK_DRAW,
	POOLSTICK_HOLSTER,
	POOLSTICK_ATTACK1HIT,
	POOLSTICK_ATTACK1MISS,
	POOLSTICK_ATTACK2MISS,
	POOLSTICK_ATTACK2HIT,
	POOLSTICK_ATTACK3MISS,
	POOLSTICK_ATTACK3HIT,
	POOLSTICK_IDLE2,
	POOLSTICK_IDLE3
};


void CPoolstick::Spawn( )
{
	Precache( );
	m_iId = WEAPON_POOLSTICK;
	SET_MODEL(ENT(pev), "models/w_poolstick.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CPoolstick::Precache( void )
{
	PRECACHE_MODEL("models/v_poolstick.mdl");
	PRECACHE_MODEL("models/w_poolstick.mdl");
	PRECACHE_MODEL("models/p_poolstick.mdl");
	PRECACHE_SOUND("weapons/pstk_hit1.wav");
	PRECACHE_SOUND("weapons/pstk_hit2.wav");
	PRECACHE_SOUND("weapons/pstk_hitbod1.wav");
	PRECACHE_SOUND("weapons/pstk_hitbod2.wav");
	PRECACHE_SOUND("weapons/pstk_hitbod3.wav");
	PRECACHE_SOUND("weapons/pstk_miss1.wav");
}

int CPoolstick::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_POOLSTICK;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

int CPoolstick::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CPoolstick::Deploy( )
{
	return DefaultDeploy( "models/v_poolstick.mdl", "models/p_poolstick.mdl", POOLSTICK_DRAW, "poolstick" );
}

void CPoolstick::Holster( int skiplocal /* = 0 */ )
{
	DefaultHolster( POOLSTICK_HOLSTER, 0.7 );
}

extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );
/*void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = {mins, maxs};
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0f )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0f )
				{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length();
					if ( thisDistance < distance )
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

void CPoolstick::PrimaryAttack()
{
	if( m_pPlayer->m_bIsHolster )
	{
		WeaponIdle();
		return;
	}

	if (! Swing( 1 ))
	{
		SetThink( &CPoolstick::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}


void CPoolstick::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_POOLSTICK );
}


void CPoolstick::SwingAgain( void )
{
	Swing( 0 );
}


int CPoolstick::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#if !CLIENT_DLL
	if ( tr.flFraction >= 1.0f )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0f )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if ( tr.flFraction >= 1.0f )
	{
		if (fFirst)
		{
			// miss
			switch( ( m_iSwing++ ) % 3 )
			{
			case 0:
				SendWeaponAnim( POOLSTICK_ATTACK1MISS );
				break;
			case 1:
				SendWeaponAnim( POOLSTICK_ATTACK2MISS );
				break;
			case 2:
				SendWeaponAnim( POOLSTICK_ATTACK3MISS );
				break;
			}
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/pstk_miss1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 0xF ) );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		switch( (m_iSwing++) % 3) 
		{
		case 0:
			SendWeaponAnim( POOLSTICK_ATTACK1HIT ); break;
		case 1:
			SendWeaponAnim( POOLSTICK_ATTACK2HIT ); break;
		case 2:
			SendWeaponAnim( POOLSTICK_ATTACK3HIT ); break;
		}

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		ClearMultiDamage( );

		if ( (m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase() ) || g_pGameRules->IsMultiplayer() )
		{
			// first swing does full damage
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgPoolstick, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}
		else
		{
			// subsequent swings do half
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgPoolstick / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
		}	
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
		{
			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pstk_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pstk_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pstk_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = POOLSTICK_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
#if CROWBAR_FIX_RAPID_CROWBAR
					  m_flNextPrimaryAttack = GetNextAttackDelay(0.25);
#endif
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
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_POOLSTICK);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play poolstick strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pstk_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pstk_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * POOLSTICK_WALLHIT_VOLUME;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25f;
		
		SetThink( &CPoolstick::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2f;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	return fDidHit;
}

void CPoolstick::WeaponIdle()
{
	if( m_pPlayer->m_bIsHolster )
	{
		if( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			m_pPlayer->m_bIsHolster = FALSE;
			Deploy();
		}
		return;
	}

	if( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand > 0.9f )
		{
			iAnim = POOLSTICK_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
		}
		else
		{
			if( flRand > 0.5f )
			{
				iAnim = POOLSTICK_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0f / 30.0f;
			}
			else
			{
				iAnim = POOLSTICK_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 160.0f / 30.0f;
			}
		}
		SendWeaponAnim( iAnim );
	}
}
