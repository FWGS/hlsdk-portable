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
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_TYPE_SMALL	0x0001
#define SF_SHAKE_TYPE_MEDIUM	0x0002
#define SF_SHAKE_TYPE_HUGE	0x0004

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Entity: env_random_shake. An entity that randomly shakes
//  the screen globally across the map, includes sounds effects.
//  Purpose is to allow multiple shakes without the need of many
//  entitys and triggers. The intensity of the shake is random,
//  the duration is random and the delay between fires is also
//  random.

class CRandomShake : public CBaseDelay
{
public:
	void	Spawn();
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );

	void	Shake();

	inline	float	Amplitude() { return pev->scale; }
	inline	float	Frequency() { return pev->dmg_save; }
	inline	float	Duration() { return pev->dmg_take; }
	inline	float	Radius() { return pev->dmg; }

	inline	void	SetAmplitude( float amplitude ) { pev->scale = amplitude; }
	inline	void	SetFrequency( float frequency ) { pev->dmg_save = frequency; }
	inline	void	SetDuration( float duration ) { pev->dmg_take = duration; }
	inline	void	SetRadius( float radius ) { pev->dmg = radius; }

	void	EXPORT ShootThink();

	int	m_iShakeCount;
};

LINK_ENTITY_TO_CLASS( env_random_shake, CRandomShake )

void CRandomShake::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	SetRadius( 0 );
	SetDuration( RANDOM_LONG( 2, 8 ) );

	m_iShakeCount = 10;

	if( m_flDelay == 0.0f )
		m_flDelay = 0.1f;

	SetAmplitude( 4.0f );

	if( pev->spawnflags & SF_SHAKE_TYPE_SMALL )
		SetAmplitude( RANDOM_FLOAT( 1.0f, 5.0f ) );

	if( pev->spawnflags & SF_SHAKE_TYPE_MEDIUM )
		SetAmplitude( RANDOM_FLOAT( 4.0f, 10.0f ) );

	if( pev->spawnflags & SF_SHAKE_TYPE_HUGE )
		SetAmplitude( RANDOM_FLOAT( 8.0f, 16.0f ) );
}

void CRandomShake::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CRandomShake::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CRandomShake::ShootThink );

	pev->nextthink = gpGlobals->time;
}

void CRandomShake::Shake()
{
	UTIL_ScreenShake( pev->origin, Amplitude(), Frequency(), Duration(), Radius() );

	EMIT_GROUPNAME_SUIT( ENT( pev ), "FRAGILE" );
}

void CRandomShake::ShootThink()
{
	m_flDelay = RANDOM_FLOAT( 9.0f, 24.0f );

	pev->nextthink = gpGlobals->time + m_flDelay;

	Shake();

	if( --m_iShakeCount <= 0 )
	{
		m_iShakeCount += 10;

		pev->nextthink = gpGlobals->time;
	}
}

