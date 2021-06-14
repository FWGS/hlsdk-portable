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

LINK_ENTITY_TO_CLASS( weapon_kmedkit, CKMedKit )

enum kmedkit_e
{
	KMEDKIT_IDLE1 = 0,
	KMEDKIT_IDLE2,
	KMEDKIT_LONGIDLE,
	KMEDKIT_LONGUSE,
	KMEDKIT_SHORTUSE,
	KMEDKIT_HOLSTER,
	KMEDKIT_DEPLOY
};

void CKMedKit::Spawn()
{
	Precache();
	m_iId = WEAPON_KMEDKIT;
	SET_MODEL( ENT( pev ), "models/w_kmedkit.mdl" );
	m_iDefaultAmmo = KMEDKIT_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CKMedKit::Precache( void )
{
	PRECACHE_MODEL( "models/v_kmedkit.mdl" );
	PRECACHE_MODEL( "models/w_kmedkit.mdl" );
	PRECACHE_MODEL( "models/p_kmedkit.mdl" );
	PRECACHE_SOUND( "weapons/kmedkit_miss.wav" );
	PRECACHE_SOUND( "weapons/kmedkit_heal.wav" );
}

int CKMedKit::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "katemedickit";
	p->iMaxAmmo1 = KMEDKIT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_KMEDKIT;
	p->iWeight = KMEDKIT_WEIGHT;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_NOAUTORELOAD | ITEM_FLAG_NOAUTOSWITCHEMPTY;
	return 1;
}

int CKMedKit::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CKMedKit::Deploy()
{
	return DefaultDeploy( "models/v_kmedkit.mdl", "models/p_kmedkit.mdl", KMEDKIT_DEPLOY, "kmedkit" );
}

void CKMedKit::Holster( int skiplocal /* = 0 */ )
{
	DefaultHolster( KMEDKIT_HOLSTER, 0.5 );
}

extern void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity );
/*
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
*/

void CKMedKit::PrimaryAttack()
{
	if( m_pPlayer->m_bIsHolster )
	{
		WeaponIdle();
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		UseMedkit( 1 );
	}
	else
	{
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/kmedkit_miss.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
		m_flNextPrimaryAttack = gpGlobals->time + 0.5f;
		m_flNextSecondaryAttack = gpGlobals->time + 0.1f;
	}
}

void CKMedKit::SecondaryAttack()
{
	if( m_pPlayer->m_bIsHolster )
        {
                WeaponIdle();
                return;
        }

	if( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() )
		UseMedkit( 2 );
}

void CKMedKit::UseMedkit( int fMode )
{
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

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

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	if( tr.flFraction < 1.0f )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity )
		{
			if( FClassnameIs( ENT( pEntity->pev ), "monster_kate" ) )
			{
				if( fMode == 2 )
				{
					if( !m_bIsStateChanged )
					{
						m_bIsStateChanged = TRUE;
						m_iState = 0;
						m_iKateHealth = pEntity->pev->health;

						SendWeaponAnim( KMEDKIT_LONGUSE, 0 );

						SetThink( &CKMedKit::SayKateHealth );
						pev->nextthink = gpGlobals->time + 0.1f;

						m_flTimeWeaponIdle = gpGlobals->time + 2.4f;
					}
				}
				else
				{
					if( pEntity->pev->health == gSkillData.kateHealth || pEntity->pev->dmg )
						goto miss;

					SendWeaponAnim( KMEDKIT_SHORTUSE, 0 );
					m_flTimeWeaponIdle = gpGlobals->time + 1.1f;

					pEntity->pev->health = gSkillData.healthkitCapacity + pEntity->pev->health;
					if( pEntity->pev->health > gSkillData.kateHealth )
						pEntity->pev->health = gSkillData.kateHealth;

					SetThink( &CKMedKit::SayKateHealth );
					pev->nextthink = gpGlobals->time + 1.5f;

					m_bIsStateChanged = TRUE;
					m_iState = 1;
					m_iKateHealth = pEntity->pev->health;

					--m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType];
					EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/kmedkit_heal.wav", 1, ATTN_NORM );
				}
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 100.0f;
			}
			return;
		}
	}
miss:
	// miss
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.5f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	SendWeaponAnim( KMEDKIT_IDLE1, 0 );
	EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/kmedkit_miss.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
}

void CKMedKit::SayKateHealth()
{
	if( m_bIsStateChanged )
	{
		char szSentence[64];
		const char *pszSentence;

		switch( m_iState )
		{
			case 0:
				pszSentence = "!KA_HEALTHST1";
				++m_iState;
				pev->nextthink = gpGlobals->time + 1.8f;
				break;
			case 1:
				pszSentence = "!KA_HEALTHST2";
				++m_iState;
				pev->nextthink = gpGlobals->time + 2.0f;
				break;
			case 2:
				UTIL_ShowKateHealth( m_iKateHealth );
				if( m_iKateHealth < 100 )
				{
					if( m_iKateHealth >= 20 )
					{
						sprintf( szSentence, "!KA_HEALTH%d0", m_iKateHealth / 10 );
						pszSentence = szSentence;
						pev->nextthink = gpGlobals->time + 1.5f;
						++m_iState;
						if( m_iKateHealth % 10 == 0 )
							++m_iState;
					}
					else
					{
						sprintf( szSentence, "!KA_HEALTH%d", m_iKateHealth );
						pszSentence = szSentence;
						pev->nextthink = gpGlobals->time + 1.6f;
						m_iState += 2;
					}
				}
				else
				{
					pszSentence = "!KA_HEALTH00";
					pev->nextthink = gpGlobals->time + 1.2f;
					m_iState += 2;
				}
				break;
			case 3:
				sprintf( szSentence, "!KA_HEALTH%d", m_iKateHealth % 10 );
				pszSentence = szSentence;
				pev->nextthink = gpGlobals->time + 1.0f;
				++m_iState;
				break;
			case 4:
				pszSentence = "!KA_HEALTHED";
				pev->nextthink = gpGlobals->time + 1.0f;
				++m_iState;
				break;
			case 5:
				m_bIsStateChanged = FALSE;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = gpGlobals->time + 0.1f;
			default:
				return;
		}
		EMIT_SOUND_SUIT( m_pPlayer->edict(), pszSentence );
	}
}

void CKMedKit::WeaponIdle()
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
		int iAnim, iRand = RANDOM_LONG( 0, 2 );
		if( iRand == 0 )
		{
			iAnim = KMEDKIT_LONGIDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.7f;
		}
		else if( iRand == 1 )
		{
			iAnim = KMEDKIT_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3f;
		}
		else
		{
			iAnim = KMEDKIT_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		SendWeaponAnim( iAnim );
	}
}
