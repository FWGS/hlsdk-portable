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

===== lights.cpp ========================================================

  spawn and think functions for editor-placed lights

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class CLight : public CPointEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Spawn() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

protected:
	int m_iStyle;
	int m_iszPattern;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( light, CLight );

//
// shut up spawn functions for new spotlights
//
LINK_ENTITY_TO_CLASS( light_spot, CLight );

TYPEDESCRIPTION CLight::m_SaveData[] =
{
	DEFINE_FIELD( CLight, m_iStyle, FIELD_INTEGER ),
	DEFINE_FIELD( CLight, m_iszPattern, FIELD_STRING )
};

IMPLEMENT_SAVERESTORE( CLight, CBaseEntity );

void CLight::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "style" ) )
	{
		m_iStyle = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "pitch" ) )
	{
		pev->angles.x = atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "pattern" ) )
	{
		m_iszPattern = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else
		data->fHandled = FALSE;
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) LIGHT_START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/

void CLight::Spawn()
{
	if ( !pev->targetname )
	{
		REMOVE_ENTITY( edict() );
		return;
	}
	if ( m_iStyle >= 32 )
	{
		if ( pev->spawnflags & SF_LIGHT_START_OFF )
			LIGHT_STYLE( m_iStyle, "a" );
		else if ( m_iszPattern )
			LIGHT_STYLE( m_iStyle, (char*)STRING( m_iszPattern ) );
		else
			LIGHT_STYLE( m_iStyle, "m" );
	}
}

void CLight::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( m_iStyle <= 31 || !ShouldToggle( useType, !( pev->spawnflags & SF_LIGHT_START_OFF ) ) )
		return;
	if ( pev->spawnflags & SF_LIGHT_START_OFF )
	{
		if ( m_iszPattern )
			LIGHT_STYLE( m_iStyle, (char*)STRING( m_iszPattern ) );
		else
			LIGHT_STYLE( m_iStyle, "m" );
		pev->spawnflags &= ~SF_LIGHT_START_OFF;
	}
	else
	{
		LIGHT_STYLE( m_iStyle, "a" );
		pev->spawnflags |= SF_LIGHT_START_OFF;
	}
}

class CEnvLight : public CLight
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Spawn() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	float m_skycolor_r;
	float m_skycolor_g;
	float m_skycolor_b;
	float m_skyvec_x;
	float m_skyvec_y;
	float m_skyvec_z;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( light_environment, CEnvLight );

TYPEDESCRIPTION CEnvLight::m_SaveData[] =
{
	DEFINE_FIELD( CEnvLight, m_skycolor_r, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvLight, m_skycolor_g, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvLight, m_skycolor_b, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvLight, m_skyvec_x, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvLight, m_skyvec_y, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvLight, m_skyvec_z, FIELD_FLOAT )
};

int CEnvLight::Save( CSave& save )
{
	if ( !CLight::Save( save ) )
		return 0;
	return save.WriteFields( "CEnvLight", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

static int EnvLightPrintedColor( float color )
{
	double promoted = color;
	int lowDword;
	memcpy( &lowDword, &promoted, sizeof( lowDword ) );
	return lowDword;
}

int CEnvLight::Restore( CRestore& restore )
{
	if ( !CLight::Restore( restore ) )
		return 0;
	int result = restore.ReadFields( "CEnvLight", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	char buffer[64];
	sprintf( buffer, "%d", EnvLightPrintedColor( m_skycolor_r ) );
	CVAR_SET_STRING( "sv_skycolor_r", buffer );
	sprintf( buffer, "%d", EnvLightPrintedColor( m_skycolor_g ) );
	CVAR_SET_STRING( "sv_skycolor_g", buffer );
	sprintf( buffer, "%d", EnvLightPrintedColor( m_skycolor_b ) );
	CVAR_SET_STRING( "sv_skycolor_b", buffer );
	sprintf( buffer, "%f", m_skyvec_x );
	CVAR_SET_STRING( "sv_skyvec_x", buffer );
	sprintf( buffer, "%f", m_skyvec_y );
	CVAR_SET_STRING( "sv_skyvec_y", buffer );
	sprintf( buffer, "%f", m_skyvec_z );
	CVAR_SET_STRING( "sv_skyvec_z", buffer );
	return result;
}

#pragma warning( push )
#pragma warning( disable : 4701 )
void CEnvLight::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "_light" ) )
	{
		int r, g, b, brightness;
		if ( sscanf( data->szValue, "%d %d %d %d\n", &r, &g, &b, &brightness ) == 1 )
			g = b = r;
		m_skycolor_r = pow( r / 255.0, 0.6 ) * 255;
		m_skycolor_g = pow( g / 255.0, 0.6 ) * 255;
		m_skycolor_b = pow( b / 255.0, 0.6 ) * 255;
		data->fHandled = TRUE;
		char buffer[64];
		sprintf( buffer, "%g", m_skycolor_r );
		CVAR_SET_STRING( "sv_skycolor_r", buffer );
		sprintf( buffer, "%g", m_skycolor_g );
		CVAR_SET_STRING( "sv_skycolor_g", buffer );
		sprintf( buffer, "%g", m_skycolor_b );
		CVAR_SET_STRING( "sv_skycolor_b", buffer );
	}
	else
		CLight::KeyValue( data );
}
#pragma warning( pop )

void CEnvLight::Spawn()
{
	UTIL_MakeAimVectors( pev->angles );
	m_skyvec_x = gpGlobals->v_forward.x;
	m_skyvec_y = gpGlobals->v_forward.y;
	m_skyvec_z = gpGlobals->v_forward.z;
	char buffer[64];
	sprintf( buffer, "%f", m_skyvec_x );
	CVAR_SET_STRING( "sv_skyvec_x", buffer );
	sprintf( buffer, "%f", m_skyvec_y );
	CVAR_SET_STRING( "sv_skyvec_y", buffer );
	sprintf( buffer, "%f", m_skyvec_z );
	CVAR_SET_STRING( "sv_skyvec_z", buffer );
	CLight::Spawn();
}
