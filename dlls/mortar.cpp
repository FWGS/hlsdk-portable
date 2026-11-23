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
/*

===== mortar.cpp ========================================================

  the "LaBuznik" mortar device              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "soundent.h"

class CFuncMortarField : public CBaseToggle
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	int ObjectCaps() override { return 0; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void EXPORT FieldUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );

	static TYPEDESCRIPTION m_SaveData[];
	int		m_iszXController;
	int		m_iszYController;
	float	m_flSpread;
	float	m_flDelay;
	int		m_iCount;
	int		m_fControl;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_mortar_field, CFuncMortarField );

TYPEDESCRIPTION CFuncMortarField::m_SaveData[] =
{
	DEFINE_FIELD( CFuncMortarField, m_iszXController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_iszYController, FIELD_STRING ),
	DEFINE_FIELD( CFuncMortarField, m_flSpread, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncMortarField, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncMortarField, m_fControl, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CFuncMortarField, CBaseToggle );

void CFuncMortarField::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "m_iszXController" ) )
	{
		m_iszXController = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "m_iszYController" ) )
	{
		m_iszYController = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "m_flSpread" ) )
	{
		m_flSpread = atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "m_fControl" ) )
	{
		m_fControl = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "m_iCount" ) )
	{
		m_iCount = atoi( data->szValue );
		data->fHandled = TRUE;
	}
}


// Drop bombs from above
void CFuncMortarField::Spawn()
{
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), STRING( pev->model ) );    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SetUse( &CFuncMortarField::FieldUse );
	Precache();
}

void CFuncMortarField::Precache()
{
	PRECACHE_SOUND( "weapons/mortar.wav" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CFuncMortarField::FieldUse( CBaseEntity* activator, CBaseEntity*, USE_TYPE, float )
{
	Vector start;
	start.x = RANDOM_FLOAT( pev->mins.x, pev->maxs.x );
	start.y = RANDOM_FLOAT( pev->mins.y, pev->maxs.y );
	start.z = pev->maxs.z;
	if ( m_fControl == 1 && activator )
	{
		start.x = activator->pev->origin.x;
		start.y = activator->pev->origin.y;
	}
	else if ( m_fControl == 2 )
	{
		if ( m_iszXController )
		{
			CBaseEntity* controller = UTIL_FindEntityByTargetname( NULL, STRING( m_iszXController ) );
			if ( controller )
				start.x = controller->pev->ideal_yaw * pev->size.x + pev->mins.x;
		}
		if ( m_iszYController )
		{
			CBaseEntity* controller = UTIL_FindEntityByTargetname( NULL, STRING( m_iszYController ) );
			if ( controller )
				start.y = controller->pev->ideal_yaw * pev->size.y + pev->mins.y;
		}
	}
	int pitch = RANDOM_LONG( 95, 124 );
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/mortar.wav", 1, ATTN_NONE, 0, pitch );
	float delay = 2.5;
	for ( int i = 0; i < m_iCount; i++ )
	{
		Vector spot = start;
		spot.x += RANDOM_FLOAT( -m_flSpread, m_flSpread );
		spot.y += RANDOM_FLOAT( -m_flSpread, m_flSpread );
		TraceResult trace;
		UTIL_TraceLine( spot, spot + Vector( 0, 0, -4096 ), ignore_monsters, edict(), &trace );
		CBaseEntity* mortar = Create( "monster_mortar", trace.vecEndPos, g_vecZero,
			activator ? activator->edict() : NULL );
		mortar->pev->nextthink = gpGlobals->time + delay;
		delay += RANDOM_FLOAT( 0.2f, 0.5f );
		if ( i == 0 )
			CSoundEnt::InsertSound( bits_SOUND_DANGER, trace.vecEndPos, 400, 0.3f );
	}
}

class CMortar : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT MortarExplode();
	int m_spriteTexture;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_mortar, CMortar );

void CMortar::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->dmg = 200;
	SetThink( &CMortar::MortarExplode );
	pev->nextthink = 0;
	Precache();
}

void CMortar::Precache()
{
	m_spriteTexture = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CMortar::MortarExplode()
{
	// mortar beam
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z + 1024 );
		WRITE_SHORT( m_spriteTexture );
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 1 ); // life
		WRITE_BYTE( 40 );  // width
		WRITE_BYTE( 0 );   // noise
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 160 );   // r, g, b
		WRITE_BYTE( 100 );   // r, g, b
		WRITE_BYTE( 128 );	// brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();
	TraceResult trace;
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 1024 ), pev->origin - Vector( 0, 0, 1024 ),
		dont_ignore_monsters, edict(), &trace );
	Explode( &trace, DMG_BLAST | DMG_MORTAR );
	UTIL_ScreenShake( trace.vecEndPos, 25, 150, 1, 750 );
}
