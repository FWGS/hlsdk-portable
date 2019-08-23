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
#include "grapple_tonguetip.h"

LINK_ENTITY_TO_CLASS( grapple_tip, CBarnacleGrappleTip )

void CBarnacleGrappleTip::Precache()
{
	PRECACHE_MODEL( "models/shock_effect.mdl" );
}

void CBarnacleGrappleTip::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT(pev), "models/shock_effect.mdl" );

	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );

	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CBarnacleGrappleTip::FlyThink );
	SetTouch( &CBarnacleGrappleTip::TongueTouch );

	Vector vecAngles = pev->angles;

	vecAngles.x -= 30.0;

	pev->angles = vecAngles;

	UTIL_MakeVectors( pev->angles );

	vecAngles.x = -( 30.0 + vecAngles.x );

	pev->velocity = g_vecZero;

	pev->gravity = 1.0;

	pev->nextthink = gpGlobals->time + 0.02;

	m_bIsStuck = FALSE;
	m_bMissed = FALSE;
}

void CBarnacleGrappleTip::FlyThink()
{
	UTIL_MakeAimVectors( pev->angles );

	pev->angles = UTIL_VecToAngles( gpGlobals->v_forward );

	const float flNewVel = ( ( pev->velocity.Length() * 0.8 ) + 400.0 );

	pev->velocity = pev->velocity * 0.2 + ( flNewVel * gpGlobals->v_forward );

	if( !g_pGameRules->IsMultiplayer() )
	{
		//Note: the old grapple had a maximum velocity of 1600. - Solokiller
		if( pev->velocity.Length() > 750.0 )
		{
			pev->velocity = pev->velocity.Normalize() * 750.0;
		}
	}
	else
	{
		//TODO: should probably clamp at sv_maxvelocity to prevent the tip from going off course. - Solokiller
		if( pev->velocity.Length() > 2000.0 )
		{
			pev->velocity = pev->velocity.Normalize() * 2000.0;
		}
	}

	pev->nextthink = gpGlobals->time + 0.02;
}

void CBarnacleGrappleTip::OffsetThink()
{
	//Nothing
}

void CBarnacleGrappleTip::TongueTouch( CBaseEntity* pOther )
{
	if( !pOther )
	{
		targetClass = GRAPPLE_NOT_A_TARGET;
		m_bMissed = TRUE;
	}
	else
	{
		if( pOther->IsPlayer() )
		{
			targetClass = GRAPPLE_MEDIUM;

			m_hGrappleTarget = pOther;

			m_bIsStuck = TRUE;
		}
		else
		{
			targetClass = CheckTarget( pOther );

			if( targetClass != GRAPPLE_NOT_A_TARGET )
			{
				m_bIsStuck = TRUE;
			}
			else
			{
				m_bMissed = TRUE;
			}
		}
	}

	pev->velocity = g_vecZero;

	m_GrappleType = targetClass;

	SetThink( &CBarnacleGrappleTip::OffsetThink );
	pev->nextthink = gpGlobals->time + 0.02;

	SetTouch( NULL );
}

int CBarnacleGrappleTip::CheckTarget( CBaseEntity* pTarget )
{
	if( !pTarget )
		return GRAPPLE_NOT_A_TARGET;

	if( pTarget->IsPlayer() )
	{
		m_hGrappleTarget = pTarget;

		return pTarget->SizeForGrapple();
	}

	Vector vecStart = pev->origin;
	Vector vecEnd = pev->origin + pev->velocity * 1024.0;

	TraceResult tr;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &tr );

	CBaseEntity* pHit = Instance( tr.pHit );

/*	if( !pHit )
		pHit = CWorld::GetInstance();*/

	float rgfl1[3];
	float rgfl2[3];
	const char *pTexture;

	vecStart.CopyToArray(rgfl1);
	vecEnd.CopyToArray(rgfl2);

	if (pHit)
		pTexture = TRACE_TEXTURE(ENT(pHit->pev), rgfl1, rgfl2);
	else
		pTexture = TRACE_TEXTURE(ENT(0), rgfl1, rgfl2);

	bool bIsFixed = false;

	if( pTexture && strnicmp( pTexture, "xeno_grapple", 12 ) == 0 )
	{
		bIsFixed = true;
	}
	else if (pTarget->SizeForGrapple() != GRAPPLE_NOT_A_TARGET)
	{
		if (pTarget->SizeForGrapple() == GRAPPLE_FIXED) {
			bIsFixed = true;
		} else {
			m_hGrappleTarget = pTarget;
			m_vecOriginOffset = pev->origin - pTarget->pev->origin;
			return pTarget->SizeForGrapple();
		}
	}

	if( bIsFixed )
	{
		m_hGrappleTarget = pTarget;
		m_vecOriginOffset = g_vecZero;

		return GRAPPLE_FIXED;
	}

	return GRAPPLE_NOT_A_TARGET;
}

void CBarnacleGrappleTip::SetPosition( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner )
{
	UTIL_SetOrigin( pev, vecOrigin );
	pev->angles = vecAngles;
	pev->owner = pOwner->edict();
}
