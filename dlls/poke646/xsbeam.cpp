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


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"xsbeam.h"

LINK_ENTITY_TO_CLASS( xs_beam, CXSBeam )

TYPEDESCRIPTION	CXSBeam::m_SaveData[] =
{
	DEFINE_FIELD( CXSBeam, m_iTrail, FIELD_INTEGER ),
	DEFINE_FIELD( CXSBeam, m_iBeamCount, FIELD_INTEGER ),
	DEFINE_FIELD( CXSBeam, m_flDeflectionDist, FIELD_FLOAT ),
	DEFINE_FIELD( CXSBeam, m_flDmg, FIELD_FLOAT ),
	DEFINE_ARRAY( CXSBeam, m_flDeflectionDot, FIELD_FLOAT, XENSQUASHER_MAX_BEAMS ),
	DEFINE_FIELD( CXSBeam, m_vecOldOrigin, FIELD_VECTOR ),
	DEFINE_ARRAY( CXSBeam, m_pBeam, FIELD_CLASSPTR, XENSQUASHER_MAX_BEAMS ),
};

IMPLEMENT_SAVERESTORE( CXSBeam, CBaseEntity )

void CXSBeam::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5f;

	SET_MODEL( ENT( pev ), "models/grenade.mdl" );

	pev->effects |= EF_NODRAW;

	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	if( m_flDmg >= 150.0f )
	{
		m_flDeflectionDist = 22.0f;
	}
	else if( m_flDmg >= 100.0f )
	{
		m_flDeflectionDist = 17.0f;
	}
	else if( m_flDmg >= 50.0f )
	{
		m_flDeflectionDist = 10.0f;
	}
	else
	{
		m_flDeflectionDist = 5.0f;
	}

	SetTouch( &CXSBeam::BeamTouch );
	SetThink( &CXSBeam::FlyThink );
	pev->nextthink = gpGlobals->time + 0.01f;
}

void CXSBeam::Precache()
{
	PRECACHE_MODEL( "sprites/hotglow.spr" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
}

CXSBeam* CXSBeam::CXSBeamCreate( float flDamage )
{
	CXSBeam *pBeam = GetClassPtr( (CXSBeam *)NULL );
	pBeam->pev->classname = MAKE_STRING( "xs_beam" );
	pBeam->m_flDmg = flDamage;
	pBeam->m_iBeamCount = Q_min( static_cast<int>( ( flDamage + 50.0f ) * 0.02f ), XENSQUASHER_MAX_BEAMS );
	pBeam->Spawn();

	return pBeam;
}

void CXSBeam::Init()
{
	int i;
	for( i = 0; i < m_iBeamCount; i++ )
	{
		m_pBeam[i] = CSprite::SpriteCreate( "sprites/hotglow.spr", pev->origin, FALSE );
		if( m_pBeam[i] )
		{
			m_pBeam[i]->pev->scale = 0.5;
			m_pBeam[i]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			UTIL_SetOrigin( m_pBeam[i]->pev, pev->origin );

			m_pBeam[i]->pev->spawnflags |= SF_SPRITE_TEMPORARY;

			// rocket trail
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BEAMFOLLOW );
				WRITE_SHORT( m_pBeam[i]->entindex() );      // entity
				WRITE_SHORT( m_iTrail );        // model
				WRITE_BYTE( 5 ); // life
				WRITE_BYTE( 4 );	// width
				WRITE_BYTE( 192 );	// r
				WRITE_BYTE( 224 );	// g
				WRITE_BYTE( 0 );	// b
				WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();
		}
	}

	if( m_iBeamCount == 3 )
	{
		m_flDeflectionDot[0] = 0.0f;
		m_flDeflectionDot[1] = 0.33f;
		m_flDeflectionDot[2] = 0.66f;
	}
	else
	{
		m_flDeflectionDot[0] = 0.25f;
		m_flDeflectionDot[1] = 0.75f;
		m_flDeflectionDot[2] = 0.0f;
		m_flDeflectionDot[3] = 0.5f;
	}
}

void CXSBeam::BeamTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	TraceResult tr = UTIL_GetGlobalTrace();

	UTIL_DecalTrace( &tr, DECAL_SMALLSCORCH1 + RANDOM_LONG( 0, 2 ) );

	float flRadius = m_flDmg * 0.08 + 8;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( pev->origin.x );	// X
		WRITE_COORD( pev->origin.y );	// Y
		WRITE_COORD( pev->origin.z );	// Z
		WRITE_BYTE( flRadius );		// radius * 0.1
		WRITE_BYTE( 192 );	// r
		WRITE_BYTE( 224 );	// g
		WRITE_BYTE( 0 );		// b
		WRITE_BYTE( flRadius * 2.5f );		// time * 10
		WRITE_BYTE( 4 );		// decay * 0.1
	MESSAGE_END();

	RadiusDamage( pev->origin, pev, pev, m_flDmg, 10 * flRadius, CLASS_NONE, DMG_POISON | DMG_ALWAYSGIB );

	for( ; m_iBeamCount > 1; --m_iBeamCount )
	{
		UTIL_Remove( m_pBeam[m_iBeamCount - 1] );
	}

	if( FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		m_pBeam[0]->pev->origin = tr.vecEndPos;

		float flQuarterRadius = flRadius / 4.0f;
		m_pBeam[0]->Expand(0, 255.0f / flQuarterRadius );

		SetThink( &CXSBeam::RemoveThink );
		pev->nextthink = gpGlobals->time + flQuarterRadius;
	}
	else
	{
		UTIL_Remove( m_pBeam[0] );
		Killed( pev, GIB_NEVER );
	}
}

void CXSBeam::FlyThink()
{
	int i;
	float flCenter;

	UTIL_MakeVectors( pev->angles );

	flCenter = ( pev->origin - m_vecOldOrigin ).Length() * 0.01f;

	for( i = 0; i < m_iBeamCount; i++ )
	{
		float flDist = m_flDeflectionDot[i] * 360.0f * ( M_PI / 180.0f ) + flCenter;
		Vector vecSin = sin( flDist ) * m_flDeflectionDist * gpGlobals->v_up;
		Vector vecCos = cos( flDist ) * m_flDeflectionDist * gpGlobals->v_right;
		m_pBeam[i]->pev->origin = pev->origin + vecSin + vecCos;
	}
	pev->nextthink = gpGlobals->time + 0.05f;
}

void CXSBeam::RemoveThink()
{
	SetThink( NULL );
	Killed( pev, GIB_NEVER );
}
