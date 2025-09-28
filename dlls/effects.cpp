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
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "animation.h"
#include "gamerules.h"
//#include "monstermaker.h"

#define	SF_GIBSHOOTER_REPEATABLE		1 // allows a gibshooter to be refired

#define SF_FUNNEL_REVERSE			1 // funnel effect repels particles instead of attracting them.

#define SF_MIRROR_FINAL				0x0001 // final part of the lasermirror chain
#define SF_MIRROR_FIREONCE			0x0002 // if global state is on, then fire our target only once

extern int gmsgLensFlare;

// Lightning target, just alias landmark
LINK_ENTITY_TO_CLASS( info_target, CPointEntity )

//==============================================================================
//		Decay's displacer's teleport target entity
//==============================================================================

LINK_ENTITY_TO_CLASS( info_displacer_xen_target, CDisplacerTarget );

void CDisplacerTarget::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "player_index"))
	{
		m_iPlayerIndex = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

//==============================================================================
//		Decay's spawnflags setter helper entity
//==============================================================================

class CFlagHelper : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( info_flaghelper, CFlagHelper );

void CFlagHelper::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( FStringNull( pev->target ) )
	  return;

	CBaseEntity *pEnt = NULL;
	pEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
	if ( pEnt )
		pEnt->pev->spawnflags = this->pev->spawnflags;
}

//==============================================================================
//		Decay's cheat helper entity
//==============================================================================

class CCheatHelper : public CPointEntity
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	KeyValue( KeyValueData *pkvd );
	void	CheckCode();

	// consists of "udlr stcx" (up down left right  square triangle circle (x)cross )
	string_t	m_sCheat1, target1;
	string_t	m_sCheat2, target2;
	char m_sListener[16];
};
LINK_ENTITY_TO_CLASS( info_cheathelper, CCheatHelper );

void CCheatHelper::CheckCode()
{
	int i = 0;
	while ( ( i < 16 ) && ( m_sListener[i] == 0 ) )
		i++;

	if ( i == 16 )
		return;

	char m_sCurrentCheat[16];
	memset( &m_sCurrentCheat, 0, sizeof(m_sCurrentCheat) );
	memmove( m_sCurrentCheat, m_sListener+i, 16-i );
	ALERT( at_console, "Current code: '%s'\n", m_sCurrentCheat );

	char m_sCurrentCompareCheat[16];


	//memset( &m_sCurrentCompareCheat, 0, sizeof(m_sCurrentCompareCheat) );
	//strncpy( m_sCurrentCompareCheat, m_sCurrentCheat, strlen( STRING( m_sCheat1 ) ) );

	if ( strstr( m_sCurrentCheat, STRING( m_sCheat1 ) ) )
	// if ( strcmp( m_sCurrentCompareCheat, STRING( m_sCheat1 ) ) == 0 )
	{
		// cheat_bonus entered
		if ( !FStringNull( target1 ) )
			FireTargets( STRING( target1 ), this, this, USE_TOGGLE, 0.0 );
		memset( &m_sListener, 0, sizeof(m_sListener) );
	}

	//memset( &m_sCurrentCompareCheat, 0, sizeof(m_sCurrentCompareCheat) );
	//strncpy( m_sCurrentCompareCheat, m_sCurrentCheat, strlen( STRING( m_sCheat2 ) ) );
	if ( strstr( m_sCurrentCheat, STRING( m_sCheat2 ) ) )
	//if ( strcmp( m_sCurrentCompareCheat, STRING( m_sCheat2 ) ) == 0 )
	{
		// cheat_alien entered
		CDecayRules *g_pDecayRules;
		g_pDecayRules = (CDecayRules*)g_pGameRules;
		g_pDecayRules->unlockMissions( true );
		g_pDecayRules->statsSave();

		if ( !FStringNull( target1 ) )
			FireTargets( STRING( target2 ), this, this, USE_TOGGLE, 0.0 );
		memset( &m_sListener, 0, sizeof(m_sListener) );
	}
}

void CCheatHelper::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( FStringNull( pCaller->pev->targetname ) )
	  return;

	static char sCallersCheat[32];

	strcpy( sCallersCheat, STRING( pCaller->pev->targetname ));

	// move all starting from 2 char to 1 char
	memmove (m_sListener+1, m_sListener+2, 15);
	m_sListener[15] = sCallersCheat[0];

	CheckCode();
}

void CCheatHelper::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "cheat_bonus"))
	{
		m_sCheat1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	} else
	if (FStrEq(pkvd->szKeyName, "cheat_alien"))
	{
		m_sCheat2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	} else
	if (FStrEq(pkvd->szKeyName, "target1"))
	{
		target1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	} else
	if (FStrEq(pkvd->szKeyName, "target2"))
	{
		target2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

//==============================================================================
//		Bubbling entity
//==============================================================================

class CBubbling : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	void EXPORT FizzThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	virtual int ObjectCaps( void ) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static TYPEDESCRIPTION m_SaveData[];

	int m_density;
	int m_frequency;
	int m_bubbleModel;
	int m_state;
};

LINK_ENTITY_TO_CLASS( env_bubbles, CBubbling )

TYPEDESCRIPTION	CBubbling::m_SaveData[] =
{
	DEFINE_FIELD( CBubbling, m_density, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_frequency, FIELD_INTEGER ),
	DEFINE_FIELD( CBubbling, m_state, FIELD_INTEGER ),
	// Let spawn restore this!
	//DEFINE_FIELD( CBubbling, m_bubbleModel, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBubbling, CBaseEntity )

#define SF_BUBBLES_STARTOFF		0x0001

void CBubbling::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );		// Set size

	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;
	int speed = fabs( pev->speed );

	// HACKHACK!!! - Speed in rendercolor
	pev->rendercolor.x = speed >> 8;
	pev->rendercolor.y = speed & 255;
	pev->rendercolor.z = ( pev->speed < 0 ) ? 1 : 0;

	if( !( pev->spawnflags & SF_BUBBLES_STARTOFF ) )
	{
		SetThink( &CBubbling::FizzThink );
		pev->nextthink = gpGlobals->time + 2.0f;
		m_state = 1;
	}
	else 
		m_state = 0;
}

void CBubbling::Precache( void )
{
	m_bubbleModel = PRECACHE_MODEL( "sprites/bubble.spr" );			// Precache bubble sprite
}

void CBubbling::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( ShouldToggle( useType, m_state ) )
		m_state = !m_state;

	if( m_state )
	{
		SetThink( &CBubbling::FizzThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		SetThink( NULL );
		pev->nextthink = 0;
	}
}

void CBubbling::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "density" ) )
	{
		m_density = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		m_frequency = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "current" ) )
	{
		pev->speed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CBubbling::FizzThink( void )
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, VecBModelOrigin( pev ) );
		WRITE_BYTE( TE_FIZZ );
		WRITE_SHORT( (short)ENTINDEX( edict() ) );
		WRITE_SHORT( (short)m_bubbleModel );
		WRITE_BYTE( m_density );
	MESSAGE_END();

	if( m_frequency > 19 )
		pev->nextthink = gpGlobals->time + 0.5f;
	else
		pev->nextthink = gpGlobals->time + 2.5f - ( 0.1f * m_frequency );
}

// --------------------------------------------------
// 
// Beams
//
// --------------------------------------------------

LINK_ENTITY_TO_CLASS( beam, CBeam )

void CBeam::Spawn( void )
{
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();
}

void CBeam::Precache( void )
{
	if( pev->owner )
		SetStartEntity( ENTINDEX( pev->owner ) );
	if( pev->aiment )
		SetEndEntity( ENTINDEX( pev->aiment ) );
}

void CBeam::SetStartEntity( int entityIndex )
{ 
	pev->sequence = ( entityIndex & 0x0FFF ) | ( pev->sequence & 0xF000 ); 
	pev->owner = g_engfuncs.pfnPEntityOfEntIndex( entityIndex );
}

void CBeam::SetEndEntity( int entityIndex )
{ 
	pev->skin = ( entityIndex & 0x0FFF ) | ( pev->skin & 0xF000 ); 
	pev->aiment = g_engfuncs.pfnPEntityOfEntIndex( entityIndex );
}

// These don't take attachments into account
const Vector &CBeam::GetStartPos( void )
{
	if( GetType() == BEAM_ENTS )
	{
		edict_t *pent =  g_engfuncs.pfnPEntityOfEntIndex( GetStartEntity() );
		return pent->v.origin;
	}
	return pev->origin;
}

const Vector &CBeam::GetEndPos( void )
{
	int type = GetType();
	if( type == BEAM_POINTS || type == BEAM_HOSE )
	{
		return pev->angles;
	}

	edict_t *pent =  g_engfuncs.pfnPEntityOfEntIndex( GetEndEntity() );
	if( pent )
		return pent->v.origin;
	return pev->angles;
}

CBeam *CBeam::BeamCreate( const char *pSpriteName, int width )
{
	// Create a new entity with CBeam private data
	CBeam *pBeam = GetClassPtr( (CBeam *)NULL );
	pBeam->pev->classname = MAKE_STRING( "beam" );

	pBeam->BeamInit( pSpriteName, width );

	return pBeam;
}

void CBeam::BeamInit( const char *pSpriteName, int width )
{
	pev->flags |= FL_CUSTOMENTITY;
	SetColor( 255, 255, 255 );
	SetBrightness( 255 );
	SetNoise( 0 );
	SetFrame( 0 );
	SetScrollRate( 0 );
	pev->model = MAKE_STRING( pSpriteName );
	SetTexture( PRECACHE_MODEL( pSpriteName ) );
	SetWidth( width );
	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
}

void CBeam::PointsInit( const Vector &start, const Vector &end )
{
	SetType( BEAM_POINTS );
	SetStartPos( start );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::HoseInit( const Vector &start, const Vector &direction )
{
	SetType( BEAM_HOSE );
	SetStartPos( start );
	SetEndPos( direction );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::PointEntInit( const Vector &start, int endIndex )
{
	SetType( BEAM_ENTPOINT );
	SetStartPos( start );
	SetEndEntity( endIndex );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::EntsInit( int startIndex, int endIndex )
{
	SetType( BEAM_ENTS );
	SetStartEntity( startIndex );
	SetEndEntity( endIndex );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CBeam::RelinkBeam( void )
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->mins.x = Q_min( startPos.x, endPos.x );
	pev->mins.y = Q_min( startPos.y, endPos.y );
	pev->mins.z = Q_min( startPos.z, endPos.z );
	pev->maxs.x = Q_max( startPos.x, endPos.x );
	pev->maxs.y = Q_max( startPos.y, endPos.y );
	pev->maxs.z = Q_max( startPos.z, endPos.z );
	pev->mins = pev->mins - pev->origin;
	pev->maxs = pev->maxs - pev->origin;

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
}

#if 0
void CBeam::SetObjectCollisionBox( void )
{
	const Vector &startPos = GetStartPos(), &endPos = GetEndPos();

	pev->absmin.x = min( startPos.x, endPos.x );
	pev->absmin.y = min( startPos.y, endPos.y );
	pev->absmin.z = min( startPos.z, endPos.z );
	pev->absmax.x = max( startPos.x, endPos.x );
	pev->absmax.y = max( startPos.y, endPos.y );
	pev->absmax.z = max( startPos.z, endPos.z );
}
#endif

void CBeam::TriggerTouch( CBaseEntity *pOther )
{
	if( pOther->pev->flags & ( FL_CLIENT | FL_MONSTER ) )
	{
		if( pev->owner )
		{
			CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
			pOwner->Use( pOther, this, USE_TOGGLE, 0 );
		}
		ALERT( at_console, "Firing targets!!!\n" );
	}
}

CBaseEntity *CBeam::RandomTargetname( const char *szName )
{
	int total = 0;

	CBaseEntity *pEntity = NULL;
	CBaseEntity *pNewEntity = NULL;
	while( ( pNewEntity = UTIL_FindEntityByTargetname( pNewEntity, szName ) ) != NULL )
	{
		total++;
		if( RANDOM_LONG( 0, total - 1 ) < 1 )
			pEntity = pNewEntity;
	}
	return pEntity;
}

void CBeam::DoSparks( const Vector &start, const Vector &end )
{
	if( pev->spawnflags & ( SF_BEAM_SPARKSTART | SF_BEAM_SPARKEND ) )
	{
		if( pev->spawnflags & SF_BEAM_SPARKSTART )
		{
			UTIL_Sparks( start );
		}
		if( pev->spawnflags & SF_BEAM_SPARKEND )
		{
			UTIL_Sparks( end );
		}
	}
}

class CLightning : public CBeam
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );

	void EXPORT StrikeThink( void );
	void EXPORT DamageThink( void );
	void RandomArea( void );
	void RandomPoint( Vector &vecSrc );
	void Zap( const Vector &vecSrc, const Vector &vecDest );
	void EXPORT StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	inline BOOL ServerSide( void )
	{
		if( m_life == 0 && !( pev->spawnflags & SF_BEAM_RING ) )
			return TRUE;
		return FALSE;
	}

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void BeamUpdateVars( void );

	int	m_active;
	string_t	m_iszStartEntity;
	string_t	m_iszEndEntity;
	float	m_life;
	int	m_boltWidth;
	int	m_noiseAmplitude;
	int	m_brightness;
	int	m_speed;
	float	m_restrike;
	int	m_spriteTexture;
	string_t	m_iszSpriteName;
	int	m_frameStart;

	float	m_radius;
};

LINK_ENTITY_TO_CLASS( env_lightning, CLightning )
LINK_ENTITY_TO_CLASS( env_beam, CLightning )

// UNDONE: Jay -- This is only a test
#if _DEBUG
class CTripBeam : public CLightning
{
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trip_beam, CTripBeam )

void CTripBeam::Spawn( void )
{
	CLightning::Spawn();
	SetTouch( &CLightning::TriggerTouch );
	pev->solid = SOLID_TRIGGER;
	RelinkBeam();
}
#endif

TYPEDESCRIPTION	CLightning::m_SaveData[] =
{
	DEFINE_FIELD( CLightning, m_active, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszStartEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_iszEndEntity, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_life, FIELD_FLOAT ),
	DEFINE_FIELD( CLightning, m_boltWidth, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_noiseAmplitude, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_brightness, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_speed, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_restrike, FIELD_FLOAT ),
	DEFINE_FIELD( CLightning, m_spriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLightning, m_frameStart, FIELD_INTEGER ),
	DEFINE_FIELD( CLightning, m_radius, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CLightning, CBeam )

void CLightning::Spawn( void )
{
	if( FStringNull( m_iszSpriteName ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();

	pev->dmgtime = gpGlobals->time;

	if( ServerSide() )
	{
		SetThink( NULL );
		if( pev->dmg > 0 )
		{
			SetThink( &CLightning::DamageThink );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
		if( pev->targetname )
		{
			if( !( pev->spawnflags & SF_BEAM_STARTON ) )
			{
				pev->effects = EF_NODRAW;
				m_active = 0;
				pev->nextthink = 0;
			}
			else
				m_active = 1;
		
			SetUse( &CLightning::ToggleUse );
		}
	}
	else
	{
		m_active = 0;
		if( !FStringNull( pev->targetname ) )
		{
			SetUse( &CLightning::StrikeUse );
		}
		if( FStringNull( pev->targetname ) || FBitSet( pev->spawnflags, SF_BEAM_STARTON ) )
		{
			SetThink( &CLightning::StrikeThink );
			pev->nextthink = gpGlobals->time + 1.0f;
		}
	}
}

void CLightning::Precache( void )
{
	m_spriteTexture = PRECACHE_MODEL( STRING( m_iszSpriteName ) );
	CBeam::Precache();
}

void CLightning::Activate( void )
{
	if( ServerSide() )
		BeamUpdateVars();
}

void CLightning::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LightningStart" ) )
	{
		m_iszStartEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "LightningEnd" ) )
	{
		m_iszEndEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "life" ) )
	{
		m_life = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "BoltWidth" ) )
	{
		m_boltWidth = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		m_noiseAmplitude = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		m_speed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "StrikeTime" ) )
	{
		m_restrike = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		m_iszSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		m_frameStart = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "Radius" ) )
	{
		m_radius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

void CLightning::ToggleUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_active ) )
		return;
	if( m_active )
	{
		m_active = 0;
		pev->effects |= EF_NODRAW;
		pev->nextthink = 0;
	}
	else
	{
		m_active = 1;
		pev->effects &= ~EF_NODRAW;
		DoSparks( GetStartPos(), GetEndPos() );
		if( pev->dmg > 0 )
		{
			pev->nextthink = gpGlobals->time;
			pev->dmgtime = gpGlobals->time;
		}
	}
}

void CLightning::StrikeUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !ShouldToggle( useType, m_active ) )
		return;

	if( m_active )
	{
		m_active = 0;
		SetThink( NULL );
	}
	else
	{
		SetThink( &CLightning::StrikeThink );
		pev->nextthink = gpGlobals->time + 0.1f;
	}

	if( !FBitSet( pev->spawnflags, SF_BEAM_TOGGLE ) )
		SetUse( NULL );
}

int IsPointEntity( CBaseEntity *pEnt )
{
	if( !pEnt->pev->modelindex )
		return 1;
	if( FClassnameIs( pEnt->pev, "info_target" ) || FClassnameIs( pEnt->pev, "info_landmark" ) || FClassnameIs( pEnt->pev, "path_corner" ) )
		return 1;

	return 0;
}

void CLightning::StrikeThink( void )
{
	if( m_life != 0 )
	{
		if( pev->spawnflags & SF_BEAM_RANDOM )
			pev->nextthink = gpGlobals->time + m_life + RANDOM_FLOAT( 0.0f, m_restrike );
		else
			pev->nextthink = gpGlobals->time + m_life + m_restrike;
	}
	m_active = 1;

	if( FStringNull( m_iszEndEntity ) )
	{
		if( FStringNull( m_iszStartEntity ) )
		{
			RandomArea();
		}
		else
		{
			CBaseEntity *pStart = RandomTargetname( STRING( m_iszStartEntity ) );
			if( pStart != NULL )
				RandomPoint( pStart->pev->origin );
			else
				ALERT( at_console, "env_beam: unknown entity \"%s\"\n", STRING( m_iszStartEntity ) );
		}
		return;
	}

	CBaseEntity *pStart = RandomTargetname( STRING( m_iszStartEntity ) );
	CBaseEntity *pEnd = RandomTargetname( STRING( m_iszEndEntity ) );

	if( pStart != NULL && pEnd != NULL )
	{
		if( IsPointEntity( pStart ) || IsPointEntity( pEnd ) )
		{
			if( pev->spawnflags & SF_BEAM_RING )
			{
				// don't work
				return;
			}
		}

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			if( IsPointEntity( pStart ) || IsPointEntity( pEnd ) )
			{
				if( !IsPointEntity( pEnd ) )	// One point entity must be in pEnd
				{
					CBaseEntity *pTemp;
					pTemp = pStart;
					pStart = pEnd;
					pEnd = pTemp;
				}
				if( !IsPointEntity( pStart ) )	// One sided
				{
					WRITE_BYTE( TE_BEAMENTPOINT );
					WRITE_SHORT( pStart->entindex() );
					WRITE_COORD( pEnd->pev->origin.x );
					WRITE_COORD( pEnd->pev->origin.y );
					WRITE_COORD( pEnd->pev->origin.z );
				}
				else
				{
					WRITE_BYTE( TE_BEAMPOINTS );
					WRITE_COORD( pStart->pev->origin.x );
					WRITE_COORD( pStart->pev->origin.y );
					WRITE_COORD( pStart->pev->origin.z );
					WRITE_COORD( pEnd->pev->origin.x );
					WRITE_COORD( pEnd->pev->origin.y );
					WRITE_COORD( pEnd->pev->origin.z );
				}

			}
			else
			{
				if( pev->spawnflags & SF_BEAM_RING )
					WRITE_BYTE( TE_BEAMRING );
				else
					WRITE_BYTE( TE_BEAMENTS );
				WRITE_SHORT( pStart->entindex() );
				WRITE_SHORT( pEnd->entindex() );
			}

			WRITE_SHORT( m_spriteTexture );
			WRITE_BYTE( m_frameStart ); // framestart
			WRITE_BYTE( (int)pev->framerate ); // framerate
			WRITE_BYTE( (int)( m_life * 10.0f ) ); // life
			WRITE_BYTE( m_boltWidth );  // width
			WRITE_BYTE( m_noiseAmplitude );   // noise
			WRITE_BYTE( (int)pev->rendercolor.x );   // r, g, b
			WRITE_BYTE( (int)pev->rendercolor.y );   // r, g, b
			WRITE_BYTE( (int)pev->rendercolor.z );   // r, g, b
			WRITE_BYTE( (int)pev->renderamt );	// brightness
			WRITE_BYTE( m_speed );		// speed
		MESSAGE_END();
		DoSparks( pStart->pev->origin, pEnd->pev->origin );
		if( pev->dmg > 0 )
		{
			TraceResult tr;
			UTIL_TraceLine( pStart->pev->origin, pEnd->pev->origin, dont_ignore_monsters, NULL, &tr );
			BeamDamageInstant( &tr, pev->dmg );
		}
	}
}

void CBeam::BeamDamage( TraceResult *ptr )
{
	RelinkBeam();
	if( ptr->flFraction != 1.0f && ptr->pHit != NULL )
	{
		CBaseEntity *pHit = CBaseEntity::Instance( ptr->pHit );
		if( pHit )
		{
			ClearMultiDamage();
			pHit->TraceAttack( pev, pev->dmg * ( gpGlobals->time - pev->dmgtime ), ( ptr->vecEndPos - pev->origin ).Normalize(), ptr, DMG_ENERGYBEAM );
			ApplyMultiDamage( pev, pev );
			if( pev->spawnflags & SF_BEAM_DECALS )
			{
				if( pHit->IsBSPModel() )
					UTIL_DecalTrace( ptr, DECAL_BIGSHOT1 + RANDOM_LONG( 0, 4 ) );
			}
		}
	}
	pev->dmgtime = gpGlobals->time;
}

void CLightning::DamageThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
	TraceResult tr;
	UTIL_TraceLine( GetStartPos(), GetEndPos(), dont_ignore_monsters, NULL, &tr );
	BeamDamage( &tr );
}

void CLightning::Zap( const Vector &vecSrc, const Vector &vecDest )
{
#if 1
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMPOINTS );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDest.x );
		WRITE_COORD( vecDest.y );
		WRITE_COORD( vecDest.z );
		WRITE_SHORT( m_spriteTexture );
		WRITE_BYTE( m_frameStart ); // framestart
		WRITE_BYTE( (int)pev->framerate ); // framerate
		WRITE_BYTE( (int)( m_life * 10.0f ) ); // life
		WRITE_BYTE( m_boltWidth );  // width
		WRITE_BYTE( m_noiseAmplitude );   // noise
		WRITE_BYTE( (int)pev->rendercolor.x );   // r, g, b
		WRITE_BYTE( (int)pev->rendercolor.y );   // r, g, b
		WRITE_BYTE( (int)pev->rendercolor.z );   // r, g, b
		WRITE_BYTE( (int)pev->renderamt );	// brightness
		WRITE_BYTE( m_speed );		// speed
	MESSAGE_END();
#else
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LIGHTNING );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_COORD( vecDest.x );
		WRITE_COORD( vecDest.y );
		WRITE_COORD( vecDest.z );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 50 );
		WRITE_BYTE( 40 );
		WRITE_SHORT( m_spriteTexture );
	MESSAGE_END();
#endif
	DoSparks( vecSrc, vecDest );
}

void CLightning::RandomArea( void )
{
	int iLoops;

	for( iLoops = 0; iLoops < 10; iLoops++ )
	{
		Vector vecSrc = pev->origin;

		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( tr1.flFraction == 1.0f )
			continue;

		Vector vecDir2;
		do
		{
			vecDir2 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		} while( DotProduct( vecDir1, vecDir2 ) > 0.0f );
		vecDir2 = vecDir2.Normalize();
		TraceResult tr2;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir2 * m_radius, ignore_monsters, ENT( pev ), &tr2 );

		if( tr2.flFraction == 1.0f )
			continue;

		if( ( tr1.vecEndPos - tr2.vecEndPos ).Length() < m_radius * 0.1f )
			continue;

		UTIL_TraceLine( tr1.vecEndPos, tr2.vecEndPos, ignore_monsters, ENT( pev ), &tr2 );

		if( tr2.flFraction != 1.0f )
			continue;

		Zap( tr1.vecEndPos, tr2.vecEndPos );

		break;
	}
}

void CLightning::RandomPoint( Vector &vecSrc )
{
	int iLoops;

	for( iLoops = 0; iLoops < 10; iLoops++ )
	{
		Vector vecDir1 = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir1 = vecDir1.Normalize();
		TraceResult tr1;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir1 * m_radius, ignore_monsters, ENT( pev ), &tr1 );

		if( ( tr1.vecEndPos - vecSrc ).Length() < m_radius * 0.1f )
			continue;

		if( tr1.flFraction == 1.0f )
			continue;

		Zap( vecSrc, tr1.vecEndPos );
		break;
	}
}

void CLightning::BeamUpdateVars( void )
{
	int beamType;
	int pointStart, pointEnd;

	edict_t *pStart = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszStartEntity ) );
	edict_t *pEnd = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING( m_iszEndEntity ) );
	pointStart = IsPointEntity( CBaseEntity::Instance( pStart ) );
	pointEnd = IsPointEntity( CBaseEntity::Instance( pEnd ) );

	pev->skin = 0;
	pev->sequence = 0;
	pev->rendermode = 0;
	pev->flags |= FL_CUSTOMENTITY;
	pev->model = m_iszSpriteName;
	SetTexture( m_spriteTexture );

	beamType = BEAM_ENTS;
	if( pointStart || pointEnd )
	{
		if( !pointStart )	// One point entity must be in pStart
		{
			edict_t *pTemp;
			// Swap start & end
			pTemp = pStart;
			pStart = pEnd;
			pEnd = pTemp;
			int swap = pointStart;
			pointStart = pointEnd;
			pointEnd = swap;
		}
		if( !pointEnd )
			beamType = BEAM_ENTPOINT;
		else
			beamType = BEAM_POINTS;
	}

	SetType( beamType );
	if( beamType == BEAM_POINTS || beamType == BEAM_ENTPOINT || beamType == BEAM_HOSE )
	{
		SetStartPos( pStart->v.origin );
		if( beamType == BEAM_POINTS || beamType == BEAM_HOSE )
			SetEndPos( pEnd->v.origin );
		else
			SetEndEntity( ENTINDEX( pEnd ) );
	}
	else
	{
		SetStartEntity( ENTINDEX( pStart ) );
		SetEndEntity( ENTINDEX( pEnd ) );
	}

	RelinkBeam();

	SetWidth( m_boltWidth );
	SetNoise( m_noiseAmplitude );
	SetFrame( m_frameStart );
	SetScrollRate( m_speed );
	if( pev->spawnflags & SF_BEAM_SHADEIN )
		SetFlags( BEAM_FSHADEIN );
	else if( pev->spawnflags & SF_BEAM_SHADEOUT )
		SetFlags( BEAM_FSHADEOUT );
}

LINK_ENTITY_TO_CLASS( env_laser, CLaser )

TYPEDESCRIPTION	CLaser::m_SaveData[] =
{
	DEFINE_FIELD( CLaser, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( CLaser, m_iszSpriteName, FIELD_STRING ),
	DEFINE_FIELD( CLaser, m_firePosition, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CLaser, CBeam )

void CLaser::Spawn( void )
{
	if( FStringNull( pev->model ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;							// Remove model & collisions
	Precache();

	SetThink( &CLaser::StrikeThink );
	pev->flags |= FL_CUSTOMENTITY;

	PointsInit( pev->origin, pev->origin );

	if( !m_pSprite && m_iszSpriteName )
		m_pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteName ), pev->origin, TRUE );
	else
		m_pSprite = NULL;

	if( m_pSprite )
		m_pSprite->SetTransparency( kRenderGlow, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, (int)pev->renderamt, (int)pev->renderfx );

	if( pev->targetname && !( pev->spawnflags & SF_BEAM_STARTON ) )
		TurnOff();
	else
		TurnOn();
}

void CLaser::Precache( void )
{
	pev->modelindex = PRECACHE_MODEL( STRING( pev->model ) );
	if( m_iszSpriteName )
		PRECACHE_MODEL( STRING( m_iszSpriteName ) );
}

void CLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "LaserTarget" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "width" ) )
	{
		SetWidth( (int)atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "NoiseAmplitude" ) )
	{
		SetNoise( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "TextureScroll" ) )
	{
		SetScrollRate( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "texture" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "EndSprite" ) )
	{
		m_iszSpriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "framestart" ) )
	{
		pev->frame = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "damage" ) )
	{
		pev->dmg = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBeam::KeyValue( pkvd );
}

int CLaser::IsOn( void )
{
	if( pev->effects & EF_NODRAW )
		return 0;
	return 1;
}

void CLaser::TurnOff( void )
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = 0;
	if( m_pSprite )
		m_pSprite->TurnOff();
}

void CLaser::TurnOn( void )
{
	pev->effects &= ~EF_NODRAW;
	if( m_pSprite )
		m_pSprite->TurnOn();
	pev->dmgtime = gpGlobals->time;
	pev->nextthink = gpGlobals->time;
}

void CLaser::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int active = IsOn();

	if( !ShouldToggle( useType, active ) )
		return;
	if( active )
	{
		TurnOff();
	}
	else
	{
		TurnOn();
	}
}

void CLaser::FireAtPoint( TraceResult &tr )
{
	SetEndPos( tr.vecEndPos );
	if( m_pSprite )
		UTIL_SetOrigin( m_pSprite->pev, tr.vecEndPos );

	BeamDamage( &tr );
	DoSparks( GetStartPos(), tr.vecEndPos );
}

void CLaser::StrikeThink( void )
{
	CBaseEntity *pEnd = RandomTargetname( STRING( pev->message ) );

	if( pEnd )
		m_firePosition = pEnd->pev->origin;

	TraceResult tr;

	UTIL_TraceLine( pev->origin, m_firePosition, dont_ignore_monsters, NULL, &tr );
	FireAtPoint( tr );
	pev->nextthink = gpGlobals->time + 0.1f;
}

class CGlow : public CPointEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Animate( float frames );
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_lastTime;
	float m_maxFrame;
};

LINK_ENTITY_TO_CLASS( env_glow, CGlow )

TYPEDESCRIPTION	CGlow::m_SaveData[] =
{
	DEFINE_FIELD( CGlow, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CGlow, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGlow, CPointEntity )

void CGlow::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	PRECACHE_MODEL( STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	if( m_maxFrame > 1.0f && pev->framerate != 0 )
		pev->nextthink	= gpGlobals->time + 0.1f;

	m_lastTime = gpGlobals->time;
}

void CGlow::Think( void )
{
	Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CGlow::Animate( float frames )
{ 
	if( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame + frames, m_maxFrame );
}

LINK_ENTITY_TO_CLASS( env_sprite, CSprite )

TYPEDESCRIPTION	CSprite::m_SaveData[] =
{
	DEFINE_FIELD( CSprite, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CSprite, m_maxFrame, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSprite, CPointEntity )

void CSprite::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	if( pev->targetname && !( pev->spawnflags & SF_SPRITE_STARTON ) )
		TurnOff();
	else
		TurnOn();

	// Worldcraft only sets y rotation, copy to Z
	if( pev->angles.y != 0 && pev->angles.z == 0 )
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
}

void CSprite::Precache( void )
{
	PRECACHE_MODEL( STRING( pev->model ) );

	// Reset attachment after save/restore
	if( pev->aiment )
		SetAttachment( pev->aiment, pev->body );
	else
	{
		// Clear attachment
		pev->skin = 0;
		pev->body = 0;
	}
}

void CSprite::SpriteInit( const char *pSpriteName, const Vector &origin )
{
	pev->model = MAKE_STRING( pSpriteName );
	pev->origin = origin;
	Spawn();
}

CSprite *CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, BOOL animate )
{
	CSprite *pSprite = GetClassPtr( (CSprite *)NULL );
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->pev->classname = MAKE_STRING( "env_sprite" );
	pSprite->pev->solid = SOLID_NOT;
	pSprite->pev->movetype = MOVETYPE_NOCLIP;
	if( animate )
		pSprite->TurnOn();

	return pSprite;
}

void CSprite::AnimateThink( void )
{
	Animate( pev->framerate * ( gpGlobals->time - m_lastTime ) );

	pev->nextthink = gpGlobals->time + 0.1f;
	m_lastTime = gpGlobals->time;
}

void CSprite::AnimateUntilDead( void )
{
	if( gpGlobals->time > pev->dmgtime )
		UTIL_Remove( this );
	else
	{
		AnimateThink();
		pev->nextthink = gpGlobals->time;
	}
}

void CSprite::Expand( float scaleSpeed, float fadeSpeed )
{
	pev->speed = scaleSpeed;
	pev->health = fadeSpeed;
	SetThink( &CSprite::ExpandThink );

	pev->nextthink	= gpGlobals->time;
	m_lastTime = gpGlobals->time;
}

void CSprite::ExpandThink( void )
{
	float frametime = gpGlobals->time - m_lastTime;
	pev->scale += pev->speed * frametime;
	pev->renderamt -= pev->health * frametime;
	if( pev->renderamt <= 0 )
	{
		pev->renderamt = 0;
		UTIL_Remove( this );
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1f;
		m_lastTime = gpGlobals->time;
	}
}

void CSprite::Animate( float frames )
{ 
	pev->frame += frames;
	if( pev->frame > m_maxFrame )
	{
		if( pev->spawnflags & SF_SPRITE_ONCE )
		{
			TurnOff();
		}
		else
		{
			if( m_maxFrame > 0 )
				pev->frame = fmod( pev->frame, m_maxFrame );
		}
	}
}

void CSprite::TurnOff( void )
{
	pev->effects = EF_NODRAW;
	pev->nextthink = 0;
}

void CSprite::TurnOn( void )
{
	pev->effects = 0;
	if( ( pev->framerate && m_maxFrame > 1.0f ) || ( pev->spawnflags & SF_SPRITE_ONCE ) )
	{
		SetThink( &CSprite::AnimateThink );
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}

void CSprite::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int on = pev->effects != EF_NODRAW;
	if( ShouldToggle( useType, on ) )
	{
		if( on )
		{
			TurnOff();
		}
		else
		{
			TurnOn();
		}
	}
}

class CGibShooter : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ShootThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual CGib *CreateGib( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibs;
	int m_iGibCapacity;
	int m_iGibMaterial;
	int m_iGibModelIndex;
	float m_flGibVelocity;
	float m_flVariance;
	float m_flGibLife;
};

TYPEDESCRIPTION CGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( CGibShooter, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_iGibModelIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CGibShooter, m_flGibVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flVariance, FIELD_FLOAT ),
	DEFINE_FIELD( CGibShooter, m_flGibLife, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CGibShooter, CBaseDelay )
LINK_ENTITY_TO_CLASS( gibshooter, CGibShooter )

void CGibShooter::Precache( void )
{
	m_iGibModelIndex = PRECACHE_MODEL( "models/hgibs.mdl" );
}

void CGibShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_iGibs" ) )
	{
		m_iGibs = m_iGibCapacity = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flGibVelocity = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flVariance" ) )
	{
		m_flVariance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_flGibLife" ) )
	{
		m_flGibLife = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CGibShooter::ShootThink );
	pev->nextthink = gpGlobals->time;
}

void CGibShooter::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	if( m_flGibLife == 0 )
	{
		m_flGibLife = 25;
	}

	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

CGib *CGibShooter::CreateGib( void )
{
	if( CVAR_GET_FLOAT( "violence_hgibs" ) == 0 )
		return NULL;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	pGib->Spawn( "models/hgibs.mdl" );
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if( pev->body <= 1 )
	{
		ALERT( at_aiconsole, "GibShooter Body is <= 1!\n" );
	}

	pGib->pev->body = RANDOM_LONG( 1, pev->body - 1 );// avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}

void CGibShooter::ShootThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;

	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT( -1.0f, 1.0f ) * m_flVariance;;

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
		if( pev->spawnflags & SF_GIBSHOOTER_REPEATABLE )
		{
			m_iGibs = m_iGibCapacity;
			SetThink( NULL );
			pev->nextthink = gpGlobals->time;
		}
		else
		{
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time;
		}
	}
}

class CEnvShooter : public CGibShooter
{
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );

	CGib *CreateGib( void );
};

LINK_ENTITY_TO_CLASS( env_shooter, CEnvShooter )

void CEnvShooter::KeyValue( KeyValueData *pkvd )
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
		CGibShooter::KeyValue( pkvd );
	}
}

void CEnvShooter::Precache( void )
{
	m_iGibModelIndex = PRECACHE_MODEL( STRING( pev->model ) );
	CBreakable::MaterialSoundPrecache( (Materials)m_iGibMaterial );
}

CGib *CEnvShooter::CreateGib( void )
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

class CTestEffect : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	// void	KeyValue( KeyValueData *pkvd );
	void EXPORT TestThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iLoop;
	int m_iBeam;
	CBeam *m_pBeam[24];
	float m_flBeamTime[24];
	float m_flStartTime;
};

LINK_ENTITY_TO_CLASS( test_effect, CTestEffect )

void CTestEffect::Spawn( void )
{
	Precache();
}

void CTestEffect::Precache( void )
{
	PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CTestEffect::TestThink( void )
{
	int i;
	float t = gpGlobals->time - m_flStartTime;

	if( m_iBeam < 24 )
	{
		CBeam *pbeam = CBeam::BeamCreate( "sprites/lgtning.spr", 100 );

		TraceResult tr;

		Vector vecSrc = pev->origin;
		Vector vecDir = Vector( RANDOM_FLOAT( -1.0f, 1.0f ), RANDOM_FLOAT( -1.0f, 1.0f ),RANDOM_FLOAT( -1.0f, 1.0f ) );
		vecDir = vecDir.Normalize();
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 128, ignore_monsters, ENT( pev ), &tr );

		pbeam->PointsInit( vecSrc, tr.vecEndPos );
		// pbeam->SetColor( 80, 100, 255 );
		pbeam->SetColor( 255, 180, 100 );
		pbeam->SetWidth( 100 );
		pbeam->SetScrollRate( 12 );
		
		m_flBeamTime[m_iBeam] = gpGlobals->time;
		m_pBeam[m_iBeam] = pbeam;
		m_iBeam++;
#if 0
		Vector vecMid = ( vecSrc + tr.vecEndPos ) * 0.5f;
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_DLIGHT );
			WRITE_COORD( vecMid.x );	// X
			WRITE_COORD( vecMid.y );	// Y
			WRITE_COORD( vecMid.z );	// Z
			WRITE_BYTE( 20 );		// radius * 0.1
			WRITE_BYTE( 255 );		// r
			WRITE_BYTE( 180 );		// g
			WRITE_BYTE( 100 );		// b
			WRITE_BYTE( 20 );		// time * 10
			WRITE_BYTE( 0 );		// decay * 0.1
		MESSAGE_END( );
#endif
	}

	if( t < 3.0f )
	{
		for( i = 0; i < m_iBeam; i++ )
		{
			t = ( gpGlobals->time - m_flBeamTime[i] ) / ( 3.0f + m_flStartTime - m_flBeamTime[i] );
			m_pBeam[i]->SetBrightness( (int)( 255.0f * t ) );
			// m_pBeam[i]->SetScrollRate( 20 * t );
		}
		pev->nextthink = gpGlobals->time + 0.1f;
	}
	else
	{
		for( i = 0; i < m_iBeam; i++ )
		{
			UTIL_Remove( m_pBeam[i] );
		}
		m_flStartTime = gpGlobals->time;
		m_iBeam = 0;
		// pev->nextthink = gpGlobals->time;
		SetThink( NULL );
	}
}

void CTestEffect::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CTestEffect::TestThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_flStartTime = gpGlobals->time;
}

// Blood effects
class CBlood : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline int Color( void )
	{
		return pev->impulse;
	}

	inline float BloodAmount( void )
	{
		return pev->dmg;
	}

	inline void SetColor( int color )
	{
		pev->impulse = color;
	}
	
	inline void SetBloodAmount( float amount )
	{
		pev->dmg = amount;
	}
	
	Vector Direction( void );
	Vector BloodPosition( CBaseEntity *pActivator );
private:
};

LINK_ENTITY_TO_CLASS( env_blood, CBlood )

#define SF_BLOOD_RANDOM		0x0001
#define SF_BLOOD_STREAM		0x0002
#define SF_BLOOD_PLAYER		0x0004
#define SF_BLOOD_DECAL		0x0008

void CBlood::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	SetMovedir( pev );
}

void CBlood::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "color" ) )
	{
		int color = atoi( pkvd->szValue );
		switch( color )
		{
		case 1:
			SetColor( BLOOD_COLOR_YELLOW );
			break;
		default:
			SetColor( BLOOD_COLOR_RED );
			break;
		}

		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "amount" ) )
	{
		SetBloodAmount( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

Vector CBlood::Direction( void )
{
	if( pev->spawnflags & SF_BLOOD_RANDOM )
		return UTIL_RandomBloodVector();

	return pev->movedir;
}

Vector CBlood::BloodPosition( CBaseEntity *pActivator )
{
	if( pev->spawnflags & SF_BLOOD_PLAYER )
	{
		edict_t *pPlayer;

		if( pActivator && pActivator->IsPlayer() )
		{
			pPlayer = pActivator->edict();
		}
		else
			pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
		if( pPlayer )
			return( pPlayer->v.origin + pPlayer->v.view_ofs ) + Vector( RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ), RANDOM_FLOAT( -10.0f, 10.0f ) );
	}

	return pev->origin;
}

void CBlood::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_BLOOD_STREAM )
		UTIL_BloodStream( BloodPosition( pActivator ), Direction(), ( Color() == BLOOD_COLOR_RED ) ? 70 : Color(), (int)BloodAmount() );
	else
		UTIL_BloodDrips( BloodPosition( pActivator ), Direction(), Color(), (int)BloodAmount() );

	if( pev->spawnflags & SF_BLOOD_DECAL )
	{
		Vector forward = Direction();
		Vector start = BloodPosition( pActivator );
		TraceResult tr;

		UTIL_TraceLine( start, start + forward * BloodAmount() * 2, ignore_monsters, NULL, &tr );
		if( tr.flFraction != 1.0f )
			UTIL_BloodDecalTrace( &tr, Color() );
	}
}

// Screen shake
class CShake : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline float Amplitude( void )
	{
		return pev->scale;
	}

	inline float Frequency( void )
	{
		return pev->dmg_save;
	}

	inline float Duration( void )
	{
		return pev->dmg_take;
	}

	inline float Radius( void )
	{
		return pev->dmg;
	}

	inline void SetAmplitude( float amplitude )
	{
		pev->scale = amplitude;
	}

	inline void SetFrequency( float frequency )
	{
		pev->dmg_save = frequency;
	}

	inline void SetDuration( float duration )
	{
		pev->dmg_take = duration;
	}

	inline void SetRadius( float radius )
	{
		pev->dmg = radius;
	}
private:
};

LINK_ENTITY_TO_CLASS( env_shake, CShake )

// pev->scale is amplitude
// pev->dmg_save is frequency
// pev->dmg_take is duration
// pev->dmg is radius
// radius of 0 means all players
// NOTE: UTIL_ScreenShake() will only shake players who are on the ground

#define SF_SHAKE_EVERYONE	0x0001		// Don't check radius
// UNDONE: These don't work yet
#define SF_SHAKE_DISRUPT	0x0002		// Disrupt controls
#define SF_SHAKE_INAIR		0x0004		// Shake players in air

void CShake::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;

	if( pev->spawnflags & SF_SHAKE_EVERYONE )
		pev->dmg = 0;
}

void CShake::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "amplitude" ) )
	{
		SetAmplitude( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		SetFrequency( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		SetRadius( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CShake::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	UTIL_ScreenShake( pev->origin, Amplitude(), Frequency(), Duration(), Radius() );
}


class CFade : public CPointEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );

	inline float Duration( void )
	{
		return pev->dmg_take;
	}

	inline float HoldTime( void )
	{
		return pev->dmg_save;
	}

	inline void SetDuration( float duration )
	{
		pev->dmg_take = duration;
	}

	inline void SetHoldTime( float hold )
	{
		pev->dmg_save = hold;
	}
private:
};

LINK_ENTITY_TO_CLASS( env_fade, CFade )

// pev->dmg_take is duration
// pev->dmg_save is hold duration
#define SF_FADE_IN			0x0001		// Fade in, not out
#define SF_FADE_MODULATE		0x0002		// Modulate, don't blend
#define SF_FADE_ONLYONE			0x0004

void CFade::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

void CFade::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "duration" ) )
	{
		SetDuration( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "holdtime" ) )
	{
		SetHoldTime( atof( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CFade::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int fadeFlags = 0;
	
	if( !( pev->spawnflags & SF_FADE_IN ) )
		fadeFlags |= FFADE_OUT;

	if( pev->spawnflags & SF_FADE_MODULATE )
		fadeFlags |= FFADE_MODULATE;

	if( pev->spawnflags & SF_FADE_ONLYONE )
	{
		if( pActivator->IsNetClient() )
		{
			UTIL_ScreenFade( pActivator, pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
		}
	}
	else
	{
		UTIL_ScreenFadeAll( pev->rendercolor, Duration(), HoldTime(), (int)pev->renderamt, fadeFlags );
	}
	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

class CMessage : public CPointEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData *pkvd );
private:
};

LINK_ENTITY_TO_CLASS( env_message, CMessage )

void CMessage::Spawn( void )
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	switch( pev->impulse )
	{
	case 1:
		// Medium radius
		pev->speed = ATTN_STATIC;
		break;
	case 2:
		// Large radius
		pev->speed = ATTN_NORM;
		break;
	case 3:
		//EVERYWHERE
		pev->speed = ATTN_NONE;
		break;
	default:
	case 0: // Small radius
		pev->speed = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if( pev->scale <= 0.0f )
		pev->scale = 1.0f;
}

void CMessage::Precache( void )
{
	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
}

void CMessage::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "messagesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "messagevolume" ) )
	{
		pev->scale = atof( pkvd->szValue ) * 0.1;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "messageattenuation" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CMessage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pPlayer = NULL;

	if( pev->spawnflags & SF_MESSAGE_ALL )
		UTIL_ShowMessageAll( STRING( pev->message ) );
	else
	{
		if( pActivator && pActivator->IsPlayer() )
			pPlayer = pActivator;
		else
			pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

		if( pPlayer )
			UTIL_ShowMessage( STRING( pev->message ), pPlayer );
	}

	if( pev->noise )
		EMIT_SOUND( edict(), CHAN_BODY, STRING( pev->noise ), pev->scale, pev->speed );

	if( pev->spawnflags & SF_MESSAGE_ONCE )
		UTIL_Remove( this );

	SUB_UseTargets( this, USE_TOGGLE, 0 );
}

//=========================================================
// FunnelEffect
//=========================================================
class CEnvFunnel : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int m_iSprite;	// Don't save, precache
};

void CEnvFunnel::Precache( void )
{
	m_iSprite = PRECACHE_MODEL( "sprites/flare6.spr" );
}

LINK_ENTITY_TO_CLASS( env_funnel, CEnvFunnel )

void CEnvFunnel::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_LARGEFUNNEL );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iSprite );

		if( pev->spawnflags & SF_FUNNEL_REVERSE )// funnel flows in reverse?
		{
			WRITE_SHORT( 1 );
		}
		else
		{
			WRITE_SHORT( 0 );
		}

	MESSAGE_END();

	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CEnvFunnel::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
}

//=========================================================
// Beverage Dispenser
// overloaded pev->frags, is now a flag for whether or not a can is stuck in the dispenser. 
// overloaded pev->health, is now how many cans remain in the machine.
//=========================================================
class CEnvBeverage : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CEnvBeverage::Precache( void )
{
	PRECACHE_MODEL( "models/can.mdl" );
	PRECACHE_SOUND( "weapons/g_bounce3.wav" );
}

LINK_ENTITY_TO_CLASS( env_beverage, CEnvBeverage )

void CEnvBeverage::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->frags != 0 || pev->health <= 0 )
	{
		// no more cans while one is waiting in the dispenser, or if I'm out of cans.
		return;
	}

	CBaseEntity *pCan = CBaseEntity::Create( "item_sodacan", pev->origin, pev->angles, edict() );	

	if( pev->skin == 6 )
	{
		// random
		pCan->pev->skin = RANDOM_LONG( 0, 5 );
	}
	else
	{
		pCan->pev->skin = pev->skin;
	}

	pev->frags = 1;
	pev->health--;

	//SetThink( &SUB_Remove );
	//pev->nextthink = gpGlobals->time;
}

void CEnvBeverage::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->frags = 0;

	if( pev->health == 0 )
	{
		pev->health = 10;
	}
}

//=========================================================
// Soda can
//=========================================================
class CItemSoda : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT CanThink( void );
	void EXPORT CanTouch( CBaseEntity *pOther );
};

void CItemSoda::Precache( void )
{
	PRECACHE_MODEL( "models/can.mdl" );
	PRECACHE_SOUND( "weapons/g_bounce3.wav" );
}

LINK_ENTITY_TO_CLASS( item_sodacan, CItemSoda )

void CItemSoda::Spawn( void )
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;

	SET_MODEL( ENT( pev ), "models/can.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	
	SetThink( &CItemSoda::CanThink );
	pev->nextthink = gpGlobals->time + 0.5f;
}

void CItemSoda::CanThink( void )
{
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/g_bounce3.wav", 1, ATTN_NORM );

	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 8 ) );
	SetThink( NULL );
	SetTouch( &CItemSoda::CanTouch );
}

void CItemSoda::CanTouch( CBaseEntity *pOther )
{
	if( !pOther->IsPlayer() )
	{
		return;
	}

	// spoit sound here
	pOther->TakeHealth( 1, DMG_GENERIC );// a bit of health.

	if( !FNullEnt( pev->owner ) )
	{
		// tell the machine the can was taken
		pev->owner->v.frags = 0;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	SetTouch( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Entities added for Decay are below
//=========================================================

//=========================================================
// object_model
//=========================================================

/*
model file name  
  body index - object

tech_crategibs.mdl
  7  - pc wax
  8  - screwdriver (yellow)
  9  - cd
  10 - chip1
  11 - floppy (red)
  12 - floppy (grass green)
  13 - hard drive
  14 - screwdriver (blue)
  15 - ampermeter
  16 - wrench (low poly)
  17 - wrench (high poly)
  18 - erm...chip with wires I think ;-)

med_crategibs.mdl
  7  - syringe
  8  - med pack
  9  - medic erm...stamp
  10 - same as 9, but twice bigger
  11 - same as 10, but twice bigger

garbagegibs.mdl
  5  - glass
  6  - hamburger
  7  - apple
  8  - banana
  9  - news paper
  10 - envelope package
  12 - sprite can

bookgibs.mdl
  1-4 books
  5  - opened book (small)
  6  - --//-- (big)
  15 - upside down book

office_gibs.mdl
  1-7 papers
  8  - journal (blue)
  9  - --//--  (orange)
  10 - --//-- green, opened
*/

LINK_ENTITY_TO_CLASS( env_model, CObjModel );

void CObjModel::KeyValue(KeyValueData *pkvd)
{
		if (FStrEq(pkvd->szKeyName, "skin"))
        {
                m_iSkin = atof(pkvd->szValue);
                pkvd->fHandled = TRUE;
        }
        else if (FStrEq(pkvd->szKeyName, "scale"))
        {
                m_iScale = atof(pkvd->szValue);
                pkvd->fHandled = TRUE;
        }
		else if (FStrEq(pkvd->szKeyName, "body"))
        {
                m_iBody = atoi(pkvd->szValue);
                pkvd->fHandled = TRUE;
        }
		else if (FStrEq(pkvd->szKeyName, "bodygroup")) // THIS IS THE ONLY PARAMETER, WHICH NEEDS TO BE READ
		{
			     m_iBodyGroup = atoi(pkvd->szValue);
				 pkvd->fHandled = TRUE;
		}
		else
                CBaseEntity::KeyValue( pkvd );
}

void CObjModel::ObjModelInit( const char *pModelName, int skin, int scale, int body, int bodygroup )
{
	/*m_iSkin = skin;
	m_iScale = scale;
	m_iBody = body;*/
	m_iBodyGroup = bodygroup;
	//m_iSequence = sequence;

	pev->model = MAKE_STRING( pModelName );
	Spawn();
}

CObjModel *CObjModel::ObjModelCreate( const char *pModelName, int skin, int scale, int body, int bodygroup )
{
	CObjModel *pObjModel = GetClassPtr( (CObjModel *)NULL );
	pObjModel->Spawn();
    //pObjModel->ObjModelInit( pModelName, skin, scale, body, bodygroup );
	return pObjModel;
}

float CObjModel::SetBoneController(int iController, float flValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return SetController( pmodel, pev, iController, flValue );
}

void CObjModel::Precache( void )
{
    PRECACHE_MODEL( (char *)STRING(pev->model) );
}

void CObjModel::Spawn( void )
{
	Precache();
	SET_MODEL(ENT(pev), STRING(pev->model));

    //pev->skin = m_iSkin;
	//pev->scale = m_iScale;
	SetBodygroup(GET_MODEL_PTR( ENT(pev) ), pev, m_iBodyGroup, m_iBody);//-1
}

//=========================================================
// Dynamic light entity
//=========================================================

#define SF_DLIGHT_ONLYONCE 1
#define SF_DLIGHT_STARTON  2

class CEnvDLight : public CPointEntity
{
public:
	void	Spawn( void );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	Think( void );
	void	DesiredAction( void );
	virtual void	MakeLight( int iTime );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	bool	GetState( void )
	{
		if (pev->health == 0 && pev->nextthink > 0) // if we're thinking, and in switchable mode, then we're on
			return true; //on
		else
			return false;//off
	}

	Vector m_vecPos;
};

LINK_ENTITY_TO_CLASS( env_dlight, CEnvDLight );

TYPEDESCRIPTION	CEnvDLight::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvDLight, m_vecPos, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CEnvDLight, CPointEntity );

void CEnvDLight::Spawn( void )
{
	if (FStringNull(pev->targetname) || pev->spawnflags & SF_DLIGHT_STARTON)
	{
		DesiredAction();
	}
}

void CEnvDLight::DesiredAction( void )
{
	Use(this, this, USE_ON, 0);
}

void CEnvDLight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType, GetState()))
	{
		return;
	}
	if (GetState() == true)
	{
		// turn off
		//DontThink();
		return;
	}

	int iTime;
	m_vecPos = pev->origin;

	if (pev->health == 0)
	{
		iTime = 10; // 1 second
		pev->nextthink = 1.0;
	}
	else if (pev->health > 25)
	{
		iTime = 250;
		pev->takedamage = 25;
		pev->nextthink = 25.0;
	}
	else
	{
		iTime = pev->health*10;
	}

	MakeLight(iTime);

	if (pev->spawnflags & SF_DLIGHT_ONLYONCE)
	{
		SetThink( &CEnvDLight::SUB_Remove );
		pev->nextthink = 0;
	}
}

void CEnvDLight::MakeLight( int iTime)
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( m_vecPos.x );		// X
		WRITE_COORD( m_vecPos.y );		// Y
		WRITE_COORD( m_vecPos.z );		// Z
		WRITE_BYTE( pev->renderamt );		// radius * 0.1
		WRITE_BYTE( pev->rendercolor.x );	// r
		WRITE_BYTE( pev->rendercolor.y );	// g
		WRITE_BYTE( pev->rendercolor.z );	// b
		WRITE_BYTE( iTime );				// time * 10
		WRITE_BYTE( pev->frags );			// decay * 0.1
	MESSAGE_END( );
}

void CEnvDLight::Think( void )
{
	int iTime;
	if (pev->health == 0)
	{
		iTime = 10;
		pev->nextthink = 1.0;
	}
	else
	{
		pev->takedamage += 25;
		if (pev->health > pev->takedamage)
		{
			iTime = 25;
			pev->nextthink = 25;
		}
		else
		{
			// finished, just do the leftover bit
			iTime = (pev->health - pev->takedamage)*10;
			pev->takedamage = 0;
		}
	}

	MakeLight( iTime );
}

//==================================================================
class CEnvELight : public CEnvDLight
{
public:
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	MakeLight(int iTime);
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hAttach;
};

LINK_ENTITY_TO_CLASS( env_elight, CEnvELight );

TYPEDESCRIPTION	CEnvELight::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvELight, m_hAttach, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CEnvELight, CEnvDLight );

void CEnvELight::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (pev->target)
	{
		m_hAttach = UTIL_FindEntityByTargetname( NULL, STRING(pev->target));
		if (m_hAttach == NULL)
		{
			ALERT(at_console, "env_elight \"%s\" can't find target %s\n", STRING(pev->targetname), STRING(pev->target));
			return; // error?
		}
	}
	else
	{
		m_hAttach = this;
	}

	CEnvDLight::Use(pActivator, pCaller, useType, value);
}

void CEnvELight::MakeLight(int iTime)
{
	if (m_hAttach == NULL)
	{
		//DontThink();
		pev->takedamage = 0;
		return;
	}

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( m_hAttach->entindex( ) + 0x1000 * pev->impulse );		// entity, attachment
		WRITE_COORD( m_vecPos.x );		// X
		WRITE_COORD( m_vecPos.y );		// Y
		WRITE_COORD( m_vecPos.z );		// Z
		WRITE_COORD( pev->renderamt );		// radius * 0.1
		WRITE_BYTE( pev->rendercolor.x );	// r
		WRITE_BYTE( pev->rendercolor.y );	// g
		WRITE_BYTE( pev->rendercolor.z );	// b
		WRITE_BYTE( iTime );				// time * 10
		WRITE_COORD( pev->frags );			// decay * 0.1
	MESSAGE_END( );
}

//=========================================================
// Lens flare spawn point entity
//=========================================================

class CEnvLensFlare : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DIRECTIONAL_USE; }
  	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    void Precache( void );
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( env_lensflare, CEnvLensFlare);

void CEnvLensFlare::Precache(void)
{
    PRECACHE_MODEL( "models/uplant1.mdl");
}

void CEnvLensFlare::Spawn(void)
{
    Precache();
    SET_MODEL(ENT(pev), "models/uplant1.mdl");

	pev->scale = 1.0;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 1; //make model invisible
}

void CEnvLensFlare::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// char szMes[256];
	// sprintf(szMes,"Server: <Origin - X: %f,Y: %f,Z: %f Index: %d>",pev->origin.x,pev->origin.y,pev->origin.z, entindex());
	// ALERT( at_console, szMes );
	
	MESSAGE_BEGIN( MSG_ALL, gmsgLensFlare);
	  WRITE_BYTE( entindex() );  // send this entity number as part of the message (for callback)
	MESSAGE_END();
}

//==================================================================
// Xen monsters' warp-in effect
//==================================================================

LINK_ENTITY_TO_CLASS( effect_warpball, CEnvWarpBall );

void CEnvWarpBall :: KeyValue( KeyValueData *pkvd )
{
/*	
	if ( FStrEq(pkvd->szKeyName, "monstercount") )
	{
		m_cNumMonsters = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "m_imaxlivechildren") )
	{
		m_iMaxLiveChildren = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "monstertype") )
	{
		m_iszMonsterClassname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else	*/

	if ( FStrEq(pkvd->szKeyName, "warp_target") )
	{
		m_iszWarpTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvWarpBall::Precache( void )
{
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/Fexplo1.spr" );
	PRECACHE_MODEL( "sprites/XFlare1.spr" );
	PRECACHE_SOUND( "debris/beamstart2.wav" );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
	//if (m_iszMonsterClassname != NULL)
	//	UTIL_PrecacheOther( STRING(m_iszMonsterClassname) );
}

void CEnvWarpBall::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBeam *pBeam[15];
	TraceResult tr;
    Vector vecDest, Origin;
	
	CBaseEntity	*m_pGoalEnt;
	m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_iszWarpTarget ) );
	if (m_pGoalEnt)
	{
		Origin = m_pGoalEnt->pev->origin;
		ALERT( at_console, "effect_warpball: playing at entity \"%s\" (%f %f %f)\n", STRING(m_iszWarpTarget), Origin.x, Origin.y, Origin.z );
	} else
		Origin = pev->origin;
	
	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart7.wav", 1, ATTN_NORM );
	
	int i;
	for (i=1;i<15;i++)
	{
		vecDest = 500 * (Vector(RANDOM_FLOAT(-2,2), RANDOM_FLOAT(-2,2), RANDOM_FLOAT(-2,2)).Normalize());//better use -5,5
		UTIL_TraceLine( Origin, Origin + vecDest, ignore_monsters, NULL, &tr);
		if (tr.flFraction != 1.0)
		{
			// we hit something.
			pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr",200);
			pBeam[i]->pev->origin = Origin;
			pBeam[i]->PointsInit( Origin, tr.vecEndPos );
			pBeam[i]->SetColor( 0, 255, 0 ); //Blue-Shift style
			pBeam[i]->SetNoise( 65 );
			pBeam[i]->SetBrightness( 150 );
			pBeam[i]->SetWidth( 18 );
			pBeam[i]->SetScrollRate( 35 );
			pBeam[i]->SetThink( &CBeam::SUB_Remove );
			pBeam[i]->pev->nextthink = gpGlobals->time + 1; //was 0.1
		}
	}
	EMIT_SOUND( edict(), CHAN_BODY, "debris/beamstart2.wav", 1, ATTN_NORM );
	UTIL_ScreenShake( Origin, 4.0, 3.0, 1.0, 750 );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(Origin.x);	// X
		WRITE_COORD(Origin.y);	// Y
		WRITE_COORD(Origin.z);	// Z
		WRITE_BYTE( 8 );	    	// radius * 0.1
		WRITE_BYTE( 255 );		    // r
		WRITE_BYTE( 180 );	    	// g
		WRITE_BYTE( 96 );    		// b
		WRITE_BYTE( 10 );	    	// time * 10
		WRITE_BYTE( 0 );	    	// decay * 0.1
	MESSAGE_END( );

	CSprite *pSpr = CSprite::SpriteCreate( "sprites/Fexplo1.spr", Origin, TRUE );
	pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  77, 210, 130,  255, kRenderFxNoDissipation);
	
	pSpr = CSprite::SpriteCreate( "sprites/XFlare1.spr", Origin, TRUE );
	pSpr->AnimateAndDie( 10 );
	pSpr->SetTransparency(kRenderGlow,  184, 250, 214,  255, kRenderFxNoDissipation);
    /*
	if (m_iszMonsterClassname != NULL )
	{
		CMonsterMaker *pMonsterMaker = CMonsterMaker::MonsterMakerCreate( STRING(m_iszMonsterClassname), 1, 1); // "monster_headcrab"
		pMonsterMaker->pev->origin = pev->origin;
		pMonsterMaker->pev->angles = pev->angles;
		SetBits( pMonsterMaker->pev->spawnflags, SF_MONSTERMAKER_FIREONCE );
		pMonsterMaker->Use( this, this, USE_ON, 1);
	}
	*/
	//	SUB_UseTargets(this,USE_TOGGLE,1);
	pev->nextthink = 2;
}

void CEnvWarpBall::Think( void )
{
	EMIT_SOUND( edict(), CHAN_ITEM, "debris/beamstart7.wav", 1, ATTN_NORM );
	SUB_UseTargets( this, USE_TOGGLE, 0);
	
	if ( pev->spawnflags & SF_AUTO_FIREONCE )
		UTIL_Remove( this );
}

CEnvWarpBall *CEnvWarpBall::WarpBallCreate()
{
	CEnvWarpBall *pWarpBall = GetClassPtr( (CEnvWarpBall *)NULL );
	pWarpBall->Spawn();
	return pWarpBall;
}

//==================================================================
// Selection Frame bounding box object
//==================================================================
LINK_ENTITY_TO_CLASS( func_frame, CFuncFrame );

void CFuncFrame :: Spawn( void )
{
	pev->angles		= g_vecZero;
	pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid		= SOLID_NOT;
	SET_MODEL( ENT(pev), STRING(pev->model) );
	
	// If it can't move/go away, it's really part of the world
	pev->flags |= FL_WORLDBRUSH;
	pev->effects |= EF_NODRAW;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	pev->nextthink = 0.1;
}

void CFuncFrame::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "kind"))
	{
		m_iKind = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

//==================================================================
//LRC- Shockwave effect, like when a Houndeye attacks.
//==================================================================
#define SF_SHOCKWAVE_CENTERED 1
#define SF_SHOCKWAVE_REPEATABLE 2

class CEnvShockwave : public CBaseEntity
{
public:
	void	Precache( void );
	void	Spawn( void ) { Precache(); }
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	void	KeyValue( KeyValueData *pkvd );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iTime;
	int m_iRadius;
	int	m_iHeight;
	int m_iScrollRate;
	int m_iNoise;
	int m_iFrameRate;
	int m_iStartFrame;
	int m_iSpriteTexture;
};

LINK_ENTITY_TO_CLASS( env_shockwave, CEnvShockwave );

TYPEDESCRIPTION	CEnvShockwave::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvShockwave, m_iHeight, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iTime, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iRadius, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iScrollRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iNoise, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iFrameRate, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iStartFrame, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvShockwave, m_iSpriteTexture, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CEnvShockwave, CBaseEntity );

void CEnvShockwave::Precache( void )
{
	m_iSpriteTexture = PRECACHE_MODEL( (char *)STRING(pev->netname) );
}

void CEnvShockwave::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iTime"))
	{
		m_iTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRadius"))
	{
		m_iRadius = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iHeight"))
	{
		m_iHeight = atoi(pkvd->szValue)/2; //LRC- the actual height is doubled when drawn
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iScrollRate"))
	{
		m_iScrollRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iNoise"))
	{
		m_iNoise = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFrameRate"))
	{
		m_iFrameRate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iStartFrame"))
	{
		m_iStartFrame = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvShockwave::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	int posz;
	if (pev->spawnflags & SF_SHOCKWAVE_CENTERED)
		posz = pev->origin.z;
	else
		posz = pev->origin.z + m_iHeight;
	// blast circle
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_BEAMCYLINDER );
	WRITE_COORD( pev->origin.x );// coord coord coord (center position)
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( posz );
	WRITE_COORD( pev->origin.x );// coord coord coord (axis and radius)
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( posz + m_iRadius );
	WRITE_SHORT( m_iSpriteTexture ); // short (sprite index)
	WRITE_BYTE( m_iStartFrame ); // byte (starting frame)
	WRITE_BYTE( m_iFrameRate ); // byte (frame rate in 0.1's)
	WRITE_BYTE( m_iTime ); // byte (life in 0.1's)
	WRITE_BYTE( m_iHeight );  // byte (line width in 0.1's)
	WRITE_BYTE( m_iNoise );   // byte (noise amplitude in 0.01's)
	WRITE_BYTE( pev->rendercolor.x );   // byte,byte,byte (color)
	WRITE_BYTE( pev->rendercolor.y );
	WRITE_BYTE( pev->rendercolor.z );
	WRITE_BYTE( pev->renderamt );  // byte (brightness)
	WRITE_BYTE( m_iScrollRate );	// byte (scroll speed in 0.1's)
	MESSAGE_END();
	
	if (!(pev->spawnflags & SF_SHOCKWAVE_REPEATABLE))
	{
		SetThink( &CEnvShockwave::SUB_Remove );
		pev->nextthink = 0;
	}
}

//=========================================================
// env_rtcamera - real-timer camera effect
//=========================================================
class CEnvRTCamera : public CBaseEntity
{
public:
	bool bActive;
	//void Activate( void );
	void ThinkOn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
/*
void CEnvRTCamera :: Activate ( void )
{
	pev->effects |= EF_NODRAW;
	pev->nextthink = gpGlobals->time + 1.0;
}
*/
extern int gmsgCamera;

void CEnvRTCamera :: ThinkOn ()
{
	ALERT( at_console, "env_rtcamera called ThinkOn\n");
	MESSAGE_BEGIN(MSG_BROADCAST, gmsgCamera, NULL);
		WRITE_BYTE(1); // mode
		WRITE_COORD(pev->origin.x); // view position
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
	MESSAGE_END();
	pev->nextthink = gpGlobals->time + 1.0;
}

void CEnvRTCamera :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    if (useType == USE_TOGGLE )
	{
		bActive = !bActive;
		if (bActive)
		{
			SetThink( &CEnvRTCamera::ThinkOn );
			pev->nextthink = gpGlobals->time + 1.0;
		} else
		{
			MESSAGE_BEGIN(MSG_BROADCAST, gmsgCamera, NULL);
				WRITE_BYTE(0);
				WRITE_COORD(0);
				WRITE_COORD(0);
				WRITE_COORD(0);
			MESSAGE_END();
			SetThink( NULL );
		}
	}     
	ALERT( at_console, "env_rtcamera called Use\n");
}


LINK_ENTITY_TO_CLASS( env_rtcamera, CEnvRTCamera );

//=========================================================
// env_mirroredlaser
//=========================================================

TYPEDESCRIPTION	CEnvMirroredLaser::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvMirroredLaser, m_iSearchDistance, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvMirroredLaser, m_iPrimarySearchDistance, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvMirroredLaser, iMaxStep, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvMirroredLaser, iBeamCount, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvMirroredLaser, iMirrorCount, FIELD_INTEGER ),
	// all beams?
	// all mirrors?
};
IMPLEMENT_SAVERESTORE( CEnvMirroredLaser, CBaseEntity );

void CEnvMirroredLaser :: KeyValue( KeyValueData *pkvd )
{
	if ( FStrEq(pkvd->szKeyName, "searchdistance") )
	{
		m_iSearchDistance = atoi( pkvd->szValue );
		m_iPrimarySearchDistance = m_iSearchDistance;
		pkvd->fHandled = TRUE;
	} else
	if ( FStrEq(pkvd->szKeyName, "maxstep") )
	{
		iMaxStep = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	} else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvMirroredLaser :: Spawn()
{
	iBeamCount = -1;
	iMirrorCount = -1;
	
	if (!m_iSearchDistance)
		m_iSearchDistance = 1024;

	// a1ba: the only map using this entity is dy_lasers.bsp and it sets maxstep to 12
	// original code always set it 12, regardless of the set value, due to a possible typo
	if (!iMaxStep)
		iMaxStep = 12;
}

void CEnvMirroredLaser :: ReColorLasers()
{
	if (iBeamCount != -1)
	{
		// recolor beams
		for (int i = 0; i <= iBeamCount; i++)
		{
			pBeam[i]->SetColor( pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z );
			pBeam[i]->SetBrightness( pev->renderamt );
		}
	}
}

void CEnvMirroredLaser :: RebuildPath()
{
	Vector origStartAngle, StartAngle, StartOrigin, NewOrigin/*, Collector*/;
	CBaseEntity *LastHit = this;

	StartAngle = pev->angles;
	StartOrigin = pev->origin;
	m_iSearchDistance = m_iPrimarySearchDistance;

	if (iBeamCount != -1)
	{
		// remove previous beams
		for (int i = 0; i <= iBeamCount; i++)
		{
			UTIL_Remove( pBeam[i] );
			//pBeam[i]->SetThink( SUB_Remove );
			//pBeam[i]->pev->nextthink = gpGlobals->time;
		}
		// send off signal to mirrors used previously
		// DONE: iBeamCount doesn't mean there is the same amount of mirrors as beams!!!
		for (int i = 0; i <= iMirrorCount; i++)
			pMirrors[i]->CheckTargets(false);
	}
	iBeamCount = -1;
	iMirrorCount = -1;

	//CBaseEntity	*m_pGoalEnt;
	//m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, "collector" );
	//Collector = m_pGoalEnt->pev->origin;

	// TODO: on rebuild go through all pMirrors and call USE_OFF

	for (int i = 0; i < iMaxStep; i++)
	{
		TraceResult tr;
		tr.pHit = NULL;
		tr.vecEndPos = Vector(0, 0, 0);
		tr.vecPlaneNormal = Vector(0, 0, 0);
		tr.flFraction = 0;

		origStartAngle = StartAngle;
		UTIL_MakeVectors( StartAngle ); // converts angle into vector and puts into gpGlobals->v_forward
		StartAngle = gpGlobals->v_forward;
		//m_vecDir = StartAngle;
		m_vecEnd = StartOrigin + StartAngle * m_iSearchDistance; // 1024

		gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
		UTIL_TraceLine( StartOrigin, m_vecEnd, missile, ENT(LastHit->pev) /*ENT( pev )*/, &tr );
		float	m_flBeamLength = tr.flFraction;
 		Vector vecTmpEnd = StartOrigin + StartAngle * m_iSearchDistance * m_flBeamLength; // 1024

		iBeamCount++;

		//ALERT( at_console, "env_mirroredlaser: spawning a laser\n" );
		pBeam[iBeamCount] = CBeam::BeamCreate("sprites/laserbeam.spr", 32); //200
		pBeam[iBeamCount]->pev->origin = StartOrigin;
		//pBeam[iBeamCount]->PointsInit( StartOrigin, vecTmpEnd );
		pBeam[iBeamCount]->SetColor( pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z ); //0, 255, 0 Blue-Shift style
		pBeam[iBeamCount]->SetBrightness( pev->renderamt ); // 150
		pBeam[iBeamCount]->SetScrollRate( 35 );  // 35
		pBeam[iBeamCount]->SetNoise( 1 );

		if (tr.pHit)
		{
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			ALERT( at_console, "env_mirroredlaser hit: %s (%s)\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->targetname) );
			if ( FClassnameIs( pEntity->pev, "env_lasermirror" ) /*&& STRING(pEntity->pev->targetname) != ""*/)
			{
				NewOrigin = pEntity->pev->origin;
				pBeam[iBeamCount]->PointsInit( StartOrigin, NewOrigin/*vecTmpEnd*/ );
				StartOrigin = NewOrigin;
				StartAngle = pEntity->pev->angles;
				LastHit = pEntity;
				//pEntity->pev->origin = Collector; //Vector(0,0,0);
				//UTIL_Remove( pEntity );
				tr.pHit = NULL;

				CEnvLaserMirror* pMirror = (CEnvLaserMirror*)CBaseEntity::Instance(LastHit->edict());
				pMirror->CheckTargets(true);
				iMirrorCount++;
				pMirrors[iMirrorCount]=pMirror;

				if ( pMirror->pev->spawnflags & SF_MIRROR_FINAL )
					return;
				
				m_iSearchDistance = pMirror->m_iSearchDistance;

				//pEntity->SetThink( SUB_Remove );
				//pEntity->pev->nextthink = gpGlobals->time + 0.01;  
			} else
			{
				pBeam[iBeamCount]->PointsInit( StartOrigin, vecTmpEnd );

				// ******* NEW STUFF **************
				StartOrigin = vecTmpEnd;
				StartAngle = origStartAngle;
				LastHit = pEntity;
				tr.pHit = NULL;
				// ******* END OF NEW STUFF *******

				//return;
			}
		}else
		{
			pBeam[iBeamCount]->PointsInit( StartOrigin, vecTmpEnd );
			return;
		}
		// FORMULA: Vect2 = Vect1 - 2 * WallN * (WallN DOT Vect1)
		/*Vector Mirrored, WallN;
		WallN = tr.vecPlaneNormal;
		Mirrored = (StartOrigin+StartAngle) - 2 * WallN * (DotProduct(WallN, (StartOrigin+StartAngle)));

		StartAngle = Mirrored;
		StartOrigin = vecTmpEnd;
		ALERT( at_console, "normal: %f %f %f\n", tr.vecPlaneNormal.x, tr.vecPlaneNormal.y, tr.vecPlaneNormal.z );*/
	}
}

void CEnvMirroredLaser :: ThinkOn ()
{
	ALERT( at_console, "env_mirroredlaser called ThinkOn version 6\n");
	// do something      
	SetThink( NULL );
	pev->nextthink = gpGlobals->time + 1.0;  
}

void CEnvMirroredLaser :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    if (useType == USE_TOGGLE )
	{
		/*bActive = !bActive;
		if (bActive)
		{
			RebuildPath();
			SetThink( ThinkOn );
			pev->nextthink = gpGlobals->time + 1.0;
		} else
		{
			// do something - off
			SetThink( NULL );
		}*/
		bActive = true;
		useType = USE_ON;
	}
	if (useType == USE_ON)
	{
		RebuildPath();
		bActive = true;
	}
	if (useType == USE_OFF)
	{
		if (iBeamCount != -1)
		{
			// remove previous beams
			for (int i = 0; i <= iBeamCount; i++)
				UTIL_Remove( pBeam[i] );
			// send off signal to mirrors used previously
			for (int i = 0; i <= iMirrorCount; i++)
				pMirrors[i]->CheckTargets(false);
		}
		iBeamCount = -1;
		iMirrorCount = -1;
		bActive = false;
	}
	ALERT( at_console, "env_mirroredlaser called Use\n");
}

LINK_ENTITY_TO_CLASS( env_mirroredlaser, CEnvMirroredLaser );


//=========================================================
// Mirrored laser mirror lens object
//=========================================================

LINK_ENTITY_TO_CLASS( env_lasermirror, CEnvLaserMirror);

TYPEDESCRIPTION	CEnvLaserMirror::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvLaserMirror, m_iszTargetUnlocked, FIELD_STRING ),
	DEFINE_FIELD( CEnvLaserMirror, m_iszTargetLocked, FIELD_STRING ),
	DEFINE_FIELD( CEnvLaserMirror, bStateOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEnvLaserMirror, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CEnvLaserMirror, m_iUseStateMode, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvLaserMirror, m_iSearchDistance, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvLaserMirror, bUsedCount, FIELD_CHARACTER ),
};
IMPLEMENT_SAVERESTORE( CEnvLaserMirror, CBaseEntity );

void CEnvLaserMirror::Precache(void)
{
    PRECACHE_MODEL( "models/uplant1.mdl");
}

void CEnvLaserMirror::Spawn(void)
{
    Precache();
    SET_MODEL(ENT(pev), "models/uplant1.mdl");

	pev->scale = 1.0;
	UTIL_SetOrigin ( pev, pev->origin );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY; //MOVETYPE_FLY;
	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_NO;
	pev->nextthink		+= 1.0;

	// UTIL_SetSize( pev, Vector(-32, -32, -8), Vector(8, 32, 32) );
	UTIL_SetSize( pev, Vector(-24, -24, -8), Vector(8, 24, 24) );

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 1; //make model invisible
	bStateOn = true;
	bUsedCount = 0;
}

void CEnvLaserMirror::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "activated_target"))
	{
		m_iszTargetUnlocked = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "snoozed_target"))
	{
		m_iszTargetLocked = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "globalstate") )
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "searchdistance") )
	{
		m_iSearchDistance = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "useglobalstatewhen") )
	{
		m_iUseStateMode = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvLaserMirror::CheckTargets(bool DoActivate)
{
	//typedef enum { GLOBAL_OFF = 0, GLOBAL_ON = 1, GLOBAL_DEAD = 2 } GLOBALESTATE;

	// TODO:
	// add ability to turn off - e.g. turn on targetlocked after globalstate has been
	// turned off on lasers resync and doesn't hit our final mirror
	// Or turn off when global state is on but laser does not hit mirror anymore

	if ((m_iUseStateMode != -1) && (m_globalstate))
	{
		if ( gGlobalState.EntityGetState( m_globalstate ) != m_iUseStateMode )
		{
			if (DoActivate == false)
			{
				FireTargets( STRING(m_iszTargetLocked), this, this, USE_TOGGLE, 0 );
				return;
			}
		} else
		{
			if ((pev->spawnflags & SF_MIRROR_FIREONCE) && (bUsedCount >= 1))
				return;

			FireTargets( STRING(m_iszTargetUnlocked), this, this, USE_TOGGLE, 0 );
			bUsedCount++;
		}
	} else
	if (DoActivate == true)
	{
		FireTargets( STRING(m_iszTargetUnlocked), this, this, USE_TOGGLE, 0 );
	} else
	{
		FireTargets( STRING(m_iszTargetLocked), this, this, USE_TOGGLE, 0 );
	}
}

void CEnvLaserMirror::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//change state
	//state just just toggle solidness of object

	if (bStateOn)
	{ // if ON then turn OFF
		pev->solid			= SOLID_NOT;
		UTIL_SetSize( pev, Vector(-1, -1, -1), Vector(1, 1, 1) );
		bStateOn = false;
	} else
	{
		pev->solid			= SOLID_BBOX;
		// UTIL_SetSize( pev, Vector(-32, -32, -8), Vector(8, 32, 32) ); // -32 -32 -8 ___ 8 32 32
		UTIL_SetSize( pev, Vector(-24, -24, -8), Vector(8, 24, 24) );
		bStateOn = true;
	}
	pev->nextthink		= gpGlobals->time + 0.01;

	// char szMes[256];
	// sprintf(szMes,"Server: <Origin - X: %f,Y: %f,Z: %f Index: %d>",pev->origin.x,pev->origin.y,pev->origin.z, entindex());
	// ALERT( at_console, szMes );
}
