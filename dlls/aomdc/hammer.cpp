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

LINK_ENTITY_TO_CLASS( weapon_hammer, CHammer );



enum hammer_e {
	HAMMER_IDLE = 0,
	HAMMER_WHACK,
	HAMMER_DRAW
};


void CHammer::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HAMMER;
	SET_MODEL(ENT(pev), "models/w_hammer.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CHammer::Precache( void )
{
	PRECACHE_MODEL("models/v_hammer.mdl");
	PRECACHE_MODEL("models/w_hammer.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/hammer_hit.wav");
	PRECACHE_SOUND("weapons/hammer_hitbody.wav");
	PRECACHE_SOUND("weapons/hammer_swing.wav");

	m_usHammer = PRECACHE_EVENT ( 1, "events/hammer.sc" );
}

int CHammer::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CHammer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_HAMMER;
	p->iWeight = HAMMER_WEIGHT;
	return 1;
}

BOOL CHammer::Deploy( )
{
	return DefaultDeploy( "models/v_hammer.mdl", "models/p_crowbar.mdl", HAMMER_DRAW, "Hammer" );
}

void FindHullIntersection(const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity);
/*
void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
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

void CHammer::PrimaryAttack()
{
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.7f;
	SendWeaponAnim( HAMMER_WHACK );
#ifndef CLIENT_DLL
	SetThink( &CHammer::BigWhackThink );
	pev->nextthink = gpGlobals->time + 0.45f;
#endif
}

void CHammer::BigWhackThink()
{
	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

#ifndef CLIENT_DLL
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
	PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_usHammer, 
	0.0, g_vecZero, g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );

	if ( tr.flFraction < 1.0f )
	{
		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		
#ifndef CLIENT_DLL
		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		UTIL_ScreenShake( m_pPlayer->pev->origin, 5.0, 1.0, 0.7, 0.25 );

		if (pEntity)
		{
			ClearMultiDamage();
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgHammer, gpGlobals->v_forward, &tr, DMG_CRUSH );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/hammer_hitbody.wav", 1, ATTN_NORM);
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
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

			// also play hammer strike
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/hammer_hit.wav", fvolbar, ATTN_NORM);

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );	
	}
}
