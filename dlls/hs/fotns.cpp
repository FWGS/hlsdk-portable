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


#define	FOTN_BODYHIT_VOLUME 128
#define	FOTN_WALLHIT_VOLUME 512

#define FOTN_DELAY			0.5f //1.5 Seconds

LINK_ENTITY_TO_CLASS( weapon_fotn, CFOTN );



enum gauss_e {
	FOTN_IDLE = 0,
	FOTN_IDLELONG,
	FOTN_LPUNCH,
	FOTN_RPUNCH,
	FOTN_DEPLOY,
	FOTN_HOLSTER,
	FOTN_BASEFIST
};


void CFOTN::Spawn( )
{
	Precache( );
	m_iId = WEAPON_FOTN;
	SET_MODEL(ENT(pev), "models/w_fotn.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CFOTN::Precache( void )
{
	PRECACHE_MODEL("models/v_fotn.mdl");
	PRECACHE_MODEL("models/w_fotn.mdl");
	PRECACHE_MODEL("models/p_fotn.mdl");
	PRECACHE_SOUND("weapons/fotn_hit1.wav");
	PRECACHE_SOUND("weapons/fotn_hit2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND("weapons/fotn_start.wav");
	PRECACHE_SOUND("weapons/fotn_wata.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("weapons/fotn_atat1.wav");
	PRECACHE_SOUND("weapons/fotn_atat2.wav");
	PRECACHE_SOUND("weapons/fotn_atat3.wav");
	PRECACHE_SOUND("weapons/fotn_atat4.wav");
	PRECACHE_SOUND("weapons/fotn_omae.wav");

	flAhDelay = gpGlobals->time; //Ah immedately.

	m_usFOTN = PRECACHE_EVENT ( 1, "events/fotns.sc" );
}

int CFOTN::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_FOTN;
	p->iWeight = BEAMKATANA_WEIGHT;
	return 1;
}



BOOL CFOTN::Deploy( )
{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_omae.wav", 1, ATTN_NORM);
	return DefaultDeploy( "models/v_fotn.mdl", "models/p_fotn.mdl", FOTN_DEPLOY, "crowbar" );
}

void CFOTN::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( FOTN_HOLSTER );
}


void FindGullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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


void CFOTN::WeaponIdle()
{
	if(!flWataDelay)
		return;

	if (flWataDelay < gpGlobals->time)
	{
		flWataDelay = 0;
		EndAttack();
	}
}

void CFOTN::EndAttack()
{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_wata.wav", 1, ATTN_NORM);
}

void CFOTN::PrimaryAttack()
{
	if (flAhDelay < gpGlobals->time)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_start.wav", 1, ATTN_NORM);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		flAhDelay = gpGlobals->time + FOTN_DELAY;
		return;
	}
	else
	{
	//Play Swing sound
	switch(RANDOM_LONG(0,3))
	{
	case 0: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_atat1.wav", 1, ATTN_NORM,0,RANDOM_LONG(90,100)); break;
	case 1: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_atat2.wav", 1, ATTN_NORM,0,RANDOM_LONG(90,100)); break;
	case 2: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_atat3.wav", 1, ATTN_NORM,0,RANDOM_LONG(90,100)); break;
	case 3: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_atat4.wav", 1, ATTN_NORM,0,RANDOM_LONG(90,100)); break;
	}
	}
	flAhDelay = gpGlobals->time + FOTN_DELAY;
	flWataDelay = flAhDelay;
	if (! Swing( 1 ))
	{
#if !CLIENT_DLL
		SetThink( &CFOTN::SwingAgain );
		pev->nextthink = gpGlobals->time + 0.12f;
#endif
	}
}

void CFOTN::SecondaryAttack()
{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_omae.wav", 1, ATTN_NORM);
	SendWeaponAnim( FOTN_IDLELONG );
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.5f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2.5f;
}


void CFOTN::Smack( )
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_FOTN );
}


void CFOTN::SwingAgain( void )
{
	Swing( 0 );
}


int CFOTN::Swing( int fFirst )
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
				FindGullIntersection( vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}
#endif

	if (fFirst)
	{
		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usFOTN, 
		0.0, g_vecZero, g_vecZero, 0, 0, 0,
		0.0, 0, 0.0 );
	}

	if ( tr.flFraction >= 1.0f )
	{
		if (fFirst)
		{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.12f;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		SendWeaponAnim( FOTN_LPUNCH );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#if !CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if( pEntity )
		{
			ClearMultiDamage( );

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
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgFOTN, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}
			else
			{
				// subsequent swings do half
				pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgFOTN / 2, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			}	
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", .3, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", .3, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", .3, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = FOTN_BODYHIT_VOLUME;
				if ( !pEntity->IsAlive() )
				{
#if CROWBAR_FIX_RAPID_CROWBAR
					m_flNextPrimaryAttack = GetNextAttackDelay(0.12);
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
			float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd-vecSrc)*2, BULLET_PLAYER_FOTN);

			if ( g_pGameRules->IsMultiplayer() )
			{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
			}

			// also play fotn strike
			switch( RANDOM_LONG(0,1) )
			{
			case 0:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/fotn_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0,3)); 
				break;
			}

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * FOTN_WALLHIT_VOLUME;

		SetThink( &CFOTN::Smack );
		pev->nextthink = gpGlobals->time + 0.18f;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.12f;
	}
	return fDidHit;
}

