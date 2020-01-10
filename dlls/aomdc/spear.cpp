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
#include "shake.h"

#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_Spear, CSpear );



enum spear_e {
	SPEAR_IDLE = 0,
	SPEAR_STAB_START,
	SPEAR_STAB_MISS,
	SPEAR_STAB,
	SPEAR_DRAW,
	SPEAR_ELECTROCUTE
};


void CSpear::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SPEAR;
	SET_MODEL(ENT(pev), "models/w_spear.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CSpear::Precache( void )
{
	PRECACHE_MODEL("models/v_spear.mdl");
	PRECACHE_MODEL("models/w_spear.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/spear_stab.wav");
	PRECACHE_SOUND("weapons/spear_hitwall.wav");
	PRECACHE_SOUND("weapons/spear_swing.wav");
	PRECACHE_SOUND("weapons/spear_electrocute.wav");

	m_usSpear = PRECACHE_EVENT ( 1, "events/null.sc" );
}

int CSpear::AddToPlayer( CBasePlayer *pPlayer )
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

int CSpear::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = WEAPON_SPEAR;
	p->iWeight = SPEAR_WEIGHT;
	return 1;
}



BOOL CSpear::Deploy( )
{
	return DefaultDeploy( "models/v_spear.mdl", "models/p_crowbar.mdl", SPEAR_DRAW, "Spear" );
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

void CSpear::PrimaryAttack()
{
	if( FBitSet( m_pPlayer->pev->flags, FL_INWATER ) )
	{
		SendWeaponAnim( SPEAR_ELECTROCUTE );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.34f;
#ifndef CLIENT_DLL
		UTIL_ScreenFade( m_pPlayer, Vector( 255, 255, 255 ), 0.5, 0.0, 100, FFADE_IN );
		m_pPlayer->TakeDamage(m_pPlayer->pev, m_pPlayer->pev, DAMAGE_AIM, DMG_GENERIC );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spear_electrocute.wav", 1, ATTN_NORM);
#endif
	}
	else
	{
		SendWeaponAnim( SPEAR_STAB_START );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;

		SetThink( &CSpear::BigSpearStab );
		pev->nextthink = gpGlobals->time + 0.3f;
	}
}

void CSpear::UnStab()
{
	m_pPlayer->EnableControl(TRUE);
}

void CSpear::BigSpearStab()
{
	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc	= m_pPlayer->GetGunPosition( );
	Vector vecEnd	= vecSrc + gpGlobals->v_forward * 72;
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

	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSpear, 
	0.0, g_vecZero, g_vecZero, 0, 0, 0,
	0.0, 0, 0.0 );

	if ( tr.flFraction < 1.0f )
	{
		SendWeaponAnim( SPEAR_STAB );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#ifndef CLIENT_DLL

		// hit
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		UTIL_ScreenShake( m_pPlayer->pev->origin, 5.0, 1.0, 0.7, 0.25 );

		m_pPlayer->EnableControl(FALSE);
		SetThink( &CSpear::UnStab );
		pev->nextthink = gpGlobals->time + 0.4f;

		if (pEntity)
		{
			ClearMultiDamage( );     
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgKnife * 2.2f, gpGlobals->v_forward, &tr, DMG_SPEAR );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spear_stab.wav", 1, ATTN_NORM);
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

			// also play spear strike
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/spear_hitwall.wav", fvolbar, ATTN_NORM); 

			// delay the decal a bit
			m_trHit = tr;
		}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );		
	}
}
