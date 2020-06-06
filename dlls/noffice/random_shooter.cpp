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

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Entity: env_random_shooter. An updated version
//  of env_shooter only this fires repeatedly in bursts,
//  so one entity can be used instead of many env_shooters.

class CGibShooter2 : public CBaseDelay
{
public:
	void	Spawn();
	void	Precache();
	void	KeyValue( KeyValueData *pkvd );
	void EXPORT ShootThink();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual CGib *CreateGib();

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int     m_iGibs;
	int	m_iGibCapacity;
	int	m_iGibMaterial;
	int	m_iGibModelIndex;
	float	m_flGibVelocity;
	float	m_flVariance;
	float	m_flGibLife;
};

TYPEDESCRIPTION CGibShooter2::m_SaveData[] =
{
	DEFINE_FIELD( CGibShooter2, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter2, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter2, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter2, m_iGibModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter2, m_flGibVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter2, m_flVariance, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter2, m_flGibLife, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGibShooter2, CBaseDelay )

void CGibShooter2::Precache( void )
{
	m_iGibModelIndex = PRECACHE_MODEL( "models/hgibs.mdl" );
}

void CGibShooter2::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flGibVelocity = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CGibShooter2::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGibShooter2::ShootThink );
	pev->nextthink = gpGlobals->time;
}

void CGibShooter2::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	m_flGibLife = 6;
	m_iGibs = m_iGibCapacity = RANDOM_LONG( 2, 6 );

	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

CGib *CGibShooter2::CreateGib( void )
{
	if( CVAR_GET_FLOAT( "violence_hgibs" ) == 0 )
		return NULL;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	pGib->Spawn( "models/hgibs.mdl" );
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if( pev->body <= 1 )
	{
		ALERT( at_aiconsole, "GibShooter2 Body is <= 1!\n" );
	}

	pGib->pev->body = RANDOM_LONG( 1, pev->body - 1 );// avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}

void CGibShooter2::ShootThink( void )
{
	m_flDelay = RANDOM_FLOAT( 0.1f, 0.4f );

	pev->nextthink = gpGlobals->time + m_flDelay;

	m_flVariance = RANDOM_FLOAT( 0.1f, 0.3f );

	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;

	vecShootDir = vecShootDir.Normalize();
	CGib *pGib = CreateGib();
	
	if( pGib )
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = vecShootDir * m_flGibVelocity;

		pGib->pev->avelocity.x = RANDOM_FLOAT( 100.0f, 200.0f );
		pGib->pev->avelocity.y = RANDOM_FLOAT( 100.0f, 300.0f );

		float thinkTime = pGib->pev->nextthink - gpGlobals->time;

		pGib->m_lifeTime = ( m_flGibLife * RANDOM_FLOAT( 0.95f, 1.05f ) );	// +/- 5%
		if( pGib->m_lifeTime < thinkTime )
		{
			pGib->pev->nextthink = gpGlobals->time + pGib->m_lifeTime;
			pGib->m_lifeTime = 0;
		}
	}

	if( --m_iGibs <= 0 )
	{
		m_iGibs = m_iGibCapacity;
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 8.0f, 21.0f );
	}
}

class CEnvRandomShooter : public CGibShooter2
{
public:
	void	 Precache();
	void	 KeyValue( KeyValueData *pkvd );

	CGib	*CreateGib( void );
};

LINK_ENTITY_TO_CLASS( env_random_shooter, CEnvRandomShooter )

void CEnvRandomShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "shootmodel" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "shootsounds" ) )
	{
		int iNoise = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
		switch( iNoise )
		{
		case 0:
			m_iGibMaterial = matGlass;
			break;
		case 1:
			m_iGibMaterial = matWood;
			break;
		case 2:
			m_iGibMaterial = matMetal;
			break;
		case 3:
			m_iGibMaterial = matFlesh;
			break;
		case 4:
			m_iGibMaterial = matRocks;
			break;
		default:
		case -1:
			m_iGibMaterial = matNone;
			break;
		}
	}
	else
	{
		CGibShooter2::KeyValue( pkvd );
	}
}

void CEnvRandomShooter::Precache( void )
{
	m_iGibModelIndex = PRECACHE_MODEL( STRING( pev->model ) );
	CBreakable::MaterialSoundPrecache( (Materials)m_iGibMaterial );
}

CGib *CEnvRandomShooter::CreateGib( void )
{
	CGib *pGib = GetClassPtr( (CGib *)NULL );

	pGib->Spawn( STRING( pev->model ) );

	int bodyPart = 0;

	if( pev->body > 1 )
		bodyPart = RANDOM_LONG( 0, pev->body - 1 );

	pGib->pev->body = bodyPart;
	pGib->m_bloodColor = DONT_BLEED;
	pGib->m_material = m_iGibMaterial;

	pGib->pev->rendermode = pev->rendermode;
	pGib->pev->renderamt = pev->renderamt;
	pGib->pev->rendercolor = pev->rendercolor;
	pGib->pev->renderfx = pev->renderfx;
	pGib->pev->scale = pev->scale;
	pGib->pev->skin = pev->skin;

	return pGib;
}

