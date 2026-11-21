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

===== triggers.cpp ========================================================

  spawn and use functions for editor-placed triggers              

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"			// trigger_camera has train functionality
#include "gamerules.h"
#include "triggers.h"
#include "cdll_dll.h"
#include "sequence.h"
#include "czeror/cz_seqapi.h"
#include "czeror/cz_music.h"
#include "monsters.h"
#include "soundent.h"

#define	SF_TRIGGER_PUSH_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.

extern DLL_GLOBAL BOOL		g_fGameOver;

extern void SetMovedir(entvars_t* pev);
extern Vector VecBModelOrigin( entvars_t* pevBModel );

class CFrictionModifier : public CBaseEntity
{
public:
	void		Spawn( void );
	void		KeyValue( KeyValueData *pkvd );
	void EXPORT	ChangeFriction( CBaseEntity *pOther );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	static	TYPEDESCRIPTION m_SaveData[];

	float		m_frictionFraction;		// Sorry, couldn't resist this name :)
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( func_friction, CFrictionModifier );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] = 
{
	DEFINE_FIELD( CFrictionModifier, m_frictionFraction, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CFrictionModifier,CBaseEntity);


// Modify an entity's friction
void CFrictionModifier :: Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch( &CFrictionModifier::ChangeFriction );
}


// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: ChangeFriction( CBaseEntity *pOther )
{
	if ( pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE && pOther->pev->movetype != MOVETYPE_BOUNCE )
		pOther->pev->friction = m_frictionFraction;
}



// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "modifier"))
	{
		m_frictionFraction = atof(pkvd->szValue) / 100.0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE		0x0001

class CAutoTrigger : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
	void Think( void );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int			m_globalstate;
	USE_TYPE	triggerType;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_auto, CAutoTrigger );

TYPEDESCRIPTION	CAutoTrigger::m_SaveData[] = 
{
	DEFINE_FIELD( CAutoTrigger, m_globalstate, FIELD_STRING ),
	DEFINE_FIELD( CAutoTrigger, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CAutoTrigger,CBaseDelay);

void CAutoTrigger::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "globalstate"))
	{
		m_globalstate = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CAutoTrigger::Spawn( void )
{
	Precache();
}


void CAutoTrigger::Precache( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
}


void CAutoTrigger::Think( void )
{
	if ( !m_globalstate || gGlobalState.EntityGetState( m_globalstate ) == GLOBAL_ON )
	{
		SUB_UseTargets( this, triggerType, 0 );
		if ( pev->spawnflags & SF_AUTO_FIREONCE )
			UTIL_Remove( this );
	}
}



#define SF_RELAY_FIREONCE		0x0001

class CTriggerRelay : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	USE_TYPE	triggerType;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_relay, CTriggerRelay );

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerRelay, triggerType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE(CTriggerRelay,CBaseDelay);

void CTriggerRelay::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi( pkvd->szValue );
		switch( type )
		{
		case 0:
			triggerType = USE_OFF;
			break;
		case 2:
			triggerType = USE_TOGGLE;
			break;
		default:
			triggerType = USE_ON;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}


void CTriggerRelay::Spawn( void )
{
}




void CTriggerRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SUB_UseTargets( this, triggerType, 0 );
	if ( pev->spawnflags & SF_RELAY_FIREONCE )
		UTIL_Remove( this );
}


//**********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets 
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001

class CMultiManager : public CBaseToggle
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn ( void );
	void EXPORT ManagerThink ( void );
	void EXPORT ManagerUse   ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );


	BOOL		HasTarget( string_t targetname );
	
	int ObjectCaps( void ) { return CBaseToggle::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	int		m_cTargets;	// the total number of targets in this manager's fire list.
	int		m_index;	// Current target
	float	m_startTime;// Time we started firing
	int		m_iTargetName	[ MAX_MULTI_TARGETS ];// list if indexes into global string array
	float	m_flTargetDelay [ MAX_MULTI_TARGETS ];// delay (in seconds) from time of manager fire to target fire
private:
	inline BOOL IsClone( void ) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
	inline BOOL ShouldClone( void ) 
	{ 
		if ( IsClone() )
			return FALSE;

		return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE; 
	}

	CMultiManager *Clone( void );
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( multi_manager, CMultiManager );

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] = 
{
	DEFINE_FIELD( CMultiManager, m_cTargets, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_index, FIELD_INTEGER ),
	DEFINE_FIELD( CMultiManager, m_startTime, FIELD_TIME ),
	DEFINE_ARRAY( CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS ),
	DEFINE_ARRAY( CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS ),
};

IMPLEMENT_SAVERESTORE(CMultiManager,CBaseToggle);

void CMultiManager :: KeyValue( KeyValueData *pkvd )
{
	// UNDONE: Maybe this should do something like this:
	//CBaseToggle::KeyValue( pkvd );
	// if ( !pkvd->fHandled )
	// ... etc.

	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else // add this field to the target list
	{
		// this assumes that additional fields are targetnames and their values are delay values.
		if ( m_cTargets < MAX_MULTI_TARGETS )
		{
			char tmp[128];

			UTIL_StripToken( pkvd->szKeyName, tmp, sizeof( tmp ) );
			m_iTargetName [ m_cTargets ] = ALLOC_STRING( tmp );
			m_flTargetDelay [ m_cTargets ] = atof (pkvd->szValue);
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}


void CMultiManager :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	SetUse ( &CMultiManager::ManagerUse );
	SetThink ( &CMultiManager::ManagerThink);

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;

	while ( swapped )
	{
		swapped = 0;
		for ( int i = 1; i < m_cTargets; i++ )
		{
			if ( m_flTargetDelay[i] < m_flTargetDelay[i-1] )
			{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i-1];
				m_flTargetDelay[i] = m_flTargetDelay[i-1];
				m_iTargetName[i-1] = name;
				m_flTargetDelay[i-1] = delay;
				swapped = 1;
			}
		}
	}
}


BOOL CMultiManager::HasTarget( string_t targetname )
{ 
	for ( int i = 0; i < m_cTargets; i++ )
		if ( FStrEq(STRING(targetname), STRING(m_iTargetName[i])) )
			return TRUE;
	
	return FALSE;
}


// Designers were using this to fire targets that may or may not exist -- 
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager :: ManagerThink ( void )
{
	float	time;

	time = gpGlobals->time - m_startTime;
	while ( m_index < m_cTargets && m_flTargetDelay[ m_index ] <= time )
	{
		FireTargets( STRING( m_iTargetName[ m_index ] ), m_hActivator, this, USE_TOGGLE, 0 );
		m_index++;
	}

	if ( m_index >= m_cTargets )// have we fired all targets?
	{
		SetThink( NULL );
		if ( IsClone() )
		{
			UTIL_Remove( this );
			return;
		}
		SetUse ( &CMultiManager::ManagerUse );// allow manager re-use 
	}
	else
		pev->nextthink = m_startTime + m_flTargetDelay[ m_index ];
}

CMultiManager *CMultiManager::Clone( void )
{
	CMultiManager *pMulti = GetClassPtr( (CMultiManager *)NULL );

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy( pMulti->pev, pev, sizeof(*pev) );
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy( pMulti->m_iTargetName, m_iTargetName, sizeof( m_iTargetName ) );
	memcpy( pMulti->m_flTargetDelay, m_flTargetDelay, sizeof( m_flTargetDelay ) );

	return pMulti;
}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager :: ManagerUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if ( ShouldClone() )
	{
		CMultiManager *pClone = Clone();
		pClone->ManagerUse( pActivator, pCaller, useType, value );
		return;
	}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse( NULL );// disable use until all targets have fired

	SetThink ( &CMultiManager::ManagerThink );
	pev->nextthink = gpGlobals->time;
}


//***********************************************************


//
// Render parameters trigger
//
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered.
//


// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX	(1<<0)
#define SF_RENDER_MASKAMT	(1<<1)
#define SF_RENDER_MASKMODE	(1<<2)
#define SF_RENDER_MASKCOLOR	(1<<3)

class CRenderFxManager : public CBaseEntity
{
public:
	void Spawn( void );
	void Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_render, CRenderFxManager );


void CRenderFxManager :: Spawn ( void )
{
	pev->solid = SOLID_NOT;
}

void CRenderFxManager :: Use ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!FStringNull(pev->target))
	{
		edict_t* pentTarget	= NULL;
		while ( 1 )
		{
			pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, STRING(pev->target));
			if (FNullEnt(pentTarget))
				break;

			entvars_t *pevTarget = VARS( pentTarget );
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKFX ) )
				pevTarget->renderfx = pev->renderfx;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKAMT ) )
				pevTarget->renderamt = pev->renderamt;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKMODE ) )
				pevTarget->rendermode = pev->rendermode;
			if ( !FBitSet( pev->spawnflags, SF_RENDER_MASKCOLOR ) )
				pevTarget->rendercolor = pev->rendercolor;
		}
	}
}



LINK_ENTITY_TO_CLASS( trigger, CBaseTrigger );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CBaseTrigger::CBaseTrigger()
{
	m_iQueryTarget = 0;
	m_iViewTargetName = 0;
	m_fViewConeDegrees = 30;
}

int CBaseTrigger::ObjectCaps()
{
	return 0;
}

TYPEDESCRIPTION CBaseTrigger::m_SaveData[] =
{
	DEFINE_FIELD( CBaseTrigger, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CBaseTrigger, m_iKey, FIELD_STRING, 16 ),
	DEFINE_ARRAY( CBaseTrigger, m_iValue, FIELD_STRING, 16 ),
	DEFINE_FIELD( CBaseTrigger, m_iQueryTarget, FIELD_STRING ),
	DEFINE_FIELD( CBaseTrigger, m_iViewTargetName, FIELD_STRING ),
	DEFINE_FIELD( CBaseTrigger, m_fViewConeDegrees, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBaseTrigger, CBaseToggle );

/*
================
InitTrigger
================
*/
void CBaseTrigger::InitTrigger( )
{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir(pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
		SetBits( pev->effects, EF_NODRAW );
}


//
// Cache user-entity-field values until spawn is called.
//

void CBaseTrigger :: KeyValue( KeyValueData *pkvd )
{
	if ( m_cTargets < 32 )
	{
		char token[128];
		UTIL_StripToken( pkvd->szKeyName, token, sizeof( token ) );
		m_iKey[m_cTargets] = ALLOC_STRING( token );
		UTIL_StripToken( pkvd->szValue, token, sizeof( token ) );
		m_iValue[m_cTargets] = ALLOC_STRING( token );
		++m_cTargets;
	}
	if ( FStrEq( pkvd->szKeyName, "querytarget" ) )
	{
		m_iQueryTarget = ALLOC_STRING( pkvd->szValue );
	}
	if ( FStrEq( pkvd->szKeyName, "viewtarget" ) )
	{
		m_iViewTargetName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "viewconedegrees" ) )
	{
		float degrees = atof( pkvd->szValue );
		m_fViewConeDegrees = degrees >= 1 ? degrees : 1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damage"))
	{
		pev->dmg = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "count"))
	{
		m_cTriggersLeft = (int) atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "damagetype"))
	{
		m_bitsDamageInflict = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

CBaseEntity* CBaseTrigger::GetQueryTarget( CBaseEntity* activator )
{
	const char* target = STRING( m_iQueryTarget );
	if ( FStrEq( target, "activator" ) )
	{
		return activator;
	}
	if ( FStrEq( target, "player" ) )
	{
		return CBaseEntity::Instance( INDEXENT( 1 ) );
	}
	return UTIL_FindEntityByTargetname( NULL, target );
}

BOOL CBaseTrigger::CheckConditions( CBaseEntity* activator )
{
	if ( !m_iQueryTarget )
	{
		return TRUE;
	}
	CBaseEntity* target = GetQueryTarget( activator );
	if ( !target )
	{
		ALERT( at_console, "Could not check conditions for triggering %s because the entity is not found", STRING( activator->pev->targetname ) );
		return FALSE;
	}
	CBaseMonster* monster = target->MyMonsterPointer();
	entvars_t* targetVars = target->pev;
	for ( int i = 0; i < m_cTargets; ++i )
	{
		char key[128];
		char value[128];
		strncpy( key, STRING( m_iKey[i] ), sizeof( key ) - 1 );
		key[sizeof( key ) - 1] = 0;
		strncpy( value, STRING( m_iValue[i] ), sizeof( value ) - 1 );
		value[sizeof( value ) - 1] = 0;
		int number = atoi( value );
		if ( ( FStrEq( key, "healthover" ) && number > targetVars->health ) ||
			( FStrEq( key, "healthunder" ) && targetVars->health > number ) ||
			( FStrEq( key, "healthequal" ) && targetVars->health != number ) ||
			( FStrEq( key, "isdead" ) && targetVars->deadflag == DEAD_NO ) ||
			( FStrEq( key, "hasclassname" ) && !FStrEq( STRING( targetVars->classname ), value ) ) )
		{
			return FALSE;
		}
		if ( FStrEq( key, "usingweapon" ) && number != ( target->IsPlayer() ? targetVars->weaponmodel : targetVars->weapons ) )
		{
			return FALSE;
		}
		if ( !monster )
		{
			continue;
		}
		if ( FStrEq( key, "takingdamage" ) && !( monster->m_afConditions & ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) ) )
		{
			return FALSE;
		}
		const int hearWorld = bits_COND_HEAR_SOUND | bits_SOUND_WORLD;
		const int hearCombat = bits_COND_HEAR_SOUND | bits_SOUND_COMBAT;
		if ( ( FStrEq( key, "hearworld" ) && ( monster->m_afConditions & hearWorld ) != hearWorld ) ||
			( FStrEq( key, "hearcombat" ) && ( monster->m_afConditions & hearCombat ) != hearCombat ) )
		{
			return FALSE;
		}
		if ( target->IsPlayer() )
		{
			continue;
		}
		if ( FStrEq( key, "enemyisplayer" ) && ( !monster->m_hEnemy || !monster->m_hEnemy->IsPlayer() ) )
		{
			return FALSE;
		}
		if ( FStrEq( key, "seeplayer" ) && !( monster->m_afConditions & bits_COND_SEE_CLIENT ) )
		{
			return FALSE;
		}
		const int hearPlayer = bits_COND_HEAR_SOUND | bits_SOUND_PLAYER;
		if ( FStrEq( key, "hearplayer" ) && ( monster->m_afConditions & hearPlayer ) != hearPlayer )
		{
			return FALSE;
		}
	}
	return TRUE;
}

BOOL CBaseTrigger::IsViewObjectVisible( CBaseEntity* activator )
{
	if ( !m_iViewTargetName )
	{
		return TRUE;
	}
	if ( !activator->IsPlayer() )
	{
		return FALSE;
	}
	CBaseEntity* target = UTIL_FindEntityByTargetname( NULL, STRING( m_iViewTargetName ) );
	return ( (CBasePlayer*)activator )->FInViewCone( target, m_fViewConeDegrees ) != FALSE;
}

class CTriggerHurt : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT RadiationThink( void );
};

LINK_ENTITY_TO_CLASS( trigger_hurt, CTriggerHurt );

//
// trigger_monsterjump
//
class CTriggerMonsterJump : public CBaseTrigger
{
public:
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void Think( void );
};

LINK_ENTITY_TO_CLASS( trigger_monsterjump, CTriggerMonsterJump );


void CTriggerMonsterJump :: Spawn ( void )
{
	SetMovedir ( pev );
	
	InitTrigger ();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if ( !FStringNull ( pev->targetname ) )
	{// if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin( pev, pev->origin ); // Unlink from trigger list
		SetUse( &CTriggerMonsterJump::ToggleUse );
	}
}


void CTriggerMonsterJump :: Think( void )
{
	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
	UTIL_SetOrigin( pev, pev->origin ); // Unlink from trigger list
	SetThink( NULL );
}

void CTriggerMonsterJump :: Touch( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if ( !FBitSet ( pevOther->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;
	
	if ( FBitSet ( pevOther->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
}


//=====================================
//
// trigger_cdaudio - starts/stops cd audio tracks
//
class CTriggerCDAudio : public CBaseTrigger
{
public:
	CTriggerCDAudio();
	void Spawn() override;
	void Restart() override;
	void KeyValue(KeyValueData *pkvd) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void PlayTrack();
	void EXPORT PlayThink();
	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_isMP3;
	BOOL m_loop;
	int m_action;
	string_t m_trackname;
	BOOL m_restarting;
};

LINK_ENTITY_TO_CLASS( trigger_cdaudio, CTriggerCDAudio );

TYPEDESCRIPTION CTriggerCDAudio::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerCDAudio, m_trackname, FIELD_STRING),
	DEFINE_FIELD(CTriggerCDAudio, m_loop, FIELD_BOOLEAN),
	DEFINE_FIELD(CTriggerCDAudio, m_action, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerCDAudio, m_isMP3, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerCDAudio, CBaseTrigger);

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CTriggerCDAudio::CTriggerCDAudio()
{
	m_isMP3 = FALSE;
	m_loop = FALSE;
	m_action = 0;
}

void CTriggerCDAudio::Restart()
{
}

void CTriggerCDAudio::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "trackname"))
	{
		m_trackname = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
		m_isMP3 = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "loop"))
	{
		m_loop = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
}

void CTriggerCDAudio :: Spawn( void )
{
	InitTrigger();
	SetThink(&CTriggerCDAudio::PlayThink);
	pev->nextthink = gpGlobals->time + 1;
}

void CTriggerCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!m_restarting)
	{
		PlayTrack();
	}
}

void PlayCDTrack( int iTrack )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;

	if ( iTrack < -1 || iTrack > 30 )
	{
		ALERT ( at_console, "TriggerCDAudio - Track %d out of range\n" );
		return;
	}

	if ( iTrack == -1 )
	{
		CLIENT_COMMAND ( pClient, "cd stop\n");
	}
	else
	{
		char string [ 64 ];

		sprintf( string, "cd play %3d\n", iTrack );
		CLIENT_COMMAND ( pClient, string);
	}
}

void PlayMP3Track(string_t trackName, int action)
{
	edict_t *pPlayer = INDEXENT(1);
	if (pPlayer)
	{
		char command[256];
		if (action == 1)
			_snprintf(command, sizeof(command), "mp3 play %s\n", STRING(trackName));
		else if (action == 2)
			_snprintf(command, sizeof(command), "mp3 loop %s\n", STRING(trackName));
		else
			sprintf(command, "mp3 stop\n");
		command[sizeof(command) - 1] = 0;
		CZ_MusicCommand(pPlayer, command);
	}
}


// only plays for ONE client, so only use in single play!
void CTriggerCDAudio :: PlayTrack( void )
{
	if (!m_isMP3)
	{
		PlayCDTrack((int)pev->health);
		return;
	}
	if (m_action)
		m_action = 0;
	else if (m_loop)
		m_action = 2;
	else
		m_action = 1;
	PlayMP3Track(m_trackname, m_action);
}

void CTriggerCDAudio::PlayThink()
{
	if (m_restarting)
	{
		PlayMP3Track(m_trackname, m_action);
		m_restarting = FALSE;
	}
	pev->nextthink = gpGlobals->time + 2;
}


// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
{
public:
	void			Spawn( void );
	void			KeyValue( KeyValueData *pkvd );

	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void			Think( void );
	void			Play( void );
};

LINK_ENTITY_TO_CLASS( target_cdaudio, CTargetCDAudio );

void CTargetCDAudio :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "radius"))
	{
		pev->scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CTargetCDAudio :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if ( pev->scale > 0 )
		pev->nextthink = gpGlobals->time + 1.0;
}

void CTargetCDAudio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Play();
}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think( void )
{
	edict_t *pClient;
	
	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	
	// Can't play if the client is not connected!
	if ( !pClient )
		return;
	
	pev->nextthink = gpGlobals->time + 0.5;

	if ( (pClient->v.origin - pev->origin).Length() <= pev->scale )
		Play();

}

void CTargetCDAudio::Play( void ) 
{ 
	PlayCDTrack( (int)pev->health );
	UTIL_Remove(this); 
}

//=====================================

//
// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
//
//int gfToggleState = 0; // used to determine when all radiation trigger hurts have called 'RadiationThink'

void CTriggerHurt :: Spawn( void )
{
	InitTrigger();
	SetTouch ( &CTriggerHurt::HurtTouch );

	if ( !FStringNull ( pev->targetname ) )
	{
		SetUse ( &CTriggerHurt::ToggleUse );
	}
	else
	{
		SetUse ( NULL );
	}

	if (m_bitsDamageInflict & DMG_RADIATION)
	{
		SetThink ( &CTriggerHurt::RadiationThink );
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.0, 0.5); 
	}

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_HURT_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger

void CTriggerHurt :: RadiationThink( void )
{

	edict_t *pentPlayer;
	CBasePlayer *pPlayer = NULL;
	float flRange;
	entvars_t *pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue	

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS(edict());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin

	if (!FNullEnt(pentPlayer))
	{
 
		pPlayer = GetClassPtr( (CBasePlayer *)VARS(pentPlayer));

		pevTarget = VARS(pentPlayer);

		// get range to player;

		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;
		
		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range 

		if (pPlayer->m_flgeigerRange >= flRange)
			pPlayer->m_flgeigerRange = flRange;
	}

	pev->nextthink = gpGlobals->time + 0.25;
}

//
// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
//
void CBaseTrigger :: ToggleUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!CheckConditions(pActivator) || !IsViewObjectVisible(pActivator))
		return;

	if (pev->solid == SOLID_NOT)
	{// if the trigger is off, turn it on
		pev->solid = SOLID_TRIGGER;
		
		// Force retouch
		gpGlobals->force_retouch++;
	}
	else
	{// turn the trigger off
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( pev, pev->origin );
}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger :: HurtTouch ( CBaseEntity *pOther )
{
	float fldmg;
	if (!CheckConditions(pOther) || !IsViewObjectVisible(pOther))
	{
		return;
	}

	if ( !pOther->pev->takedamage )
		return;

	if ( (pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if ( (pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( pev->dmgtime > gpGlobals->time )
		{
			if ( gpGlobals->time != pev->pain_finished )
			{// too early to hurt again, and not same frame with a different entity
				if ( pOther->IsPlayer() )
				{
					int playerMask = 1 << (pOther->entindex() - 1);

					// If I've already touched this player (this time), then bail out
					if ( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
			{
				int playerMask = 1 << (pOther->entindex() - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if ( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{// too early to hurt again, and not same frame with a different entity
			return;
		}
	}



	// If this is time_based damage (poison, radiation), override the pev->dmg with a 
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	fldmg = pev->dmg * 0.5;	// 0.5 seconds worth of damage, pev->dmg is damage/second


	if ( fldmg < 0 )
	{
		pOther->TakeHealth( -fldmg, m_bitsDamageInflict );
	}
	else
		pOther->TakeDamage( pev, pev, fldmg, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again

  
	
	if ( pev->target )
	{
		// trigger has a target it wants to fire. 
		if ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if ( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if ( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}


/*QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
*/
class CTriggerMultiple : public CBaseTrigger
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_multiple, CTriggerMultiple );


void CTriggerMultiple :: Spawn( void )
{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger();
	SetTouch( &CTriggerMultiple::MultiTouch );
}


/*QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target" to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.  Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
*/
class CTriggerOnce : public CTriggerMultiple
{
public:
	void Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_once, CTriggerOnce );
void CTriggerOnce::Spawn( void )
{
	m_flWait = -1;
	
	CTriggerMultiple :: Spawn();
}



void CBaseTrigger :: MultiTouch( CBaseEntity *pOther )
{
	entvars_t	*pevToucher;

	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if ( ((pevToucher->flags & FL_CLIENT) && !(pev->spawnflags & SF_TRIGGER_NOCLIENTS)) ||
		 ((pevToucher->flags & FL_MONSTER) && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS)) ||
		 (pev->spawnflags & SF_TRIGGER_PUSHABLES) && FClassnameIs(pevToucher,"func_pushable") )
	{
		ActivateMultiTrigger( pOther );
	}
}


//
// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
//
void CBaseTrigger :: ActivateMultiTrigger( CBaseEntity *pActivator )
{
	if (pev->nextthink > gpGlobals->time)
		return;         // still waiting for reset time

	if ( HasGlobalState() ? !IsGlobalStateTriggered() : !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
		return;
	if ( !CheckConditions( pActivator ) || !IsViewObjectVisible( pActivator ) )
	{
		return;
	}

	if (FClassnameIs(pev, "trigger_secret"))
	{
		if ( pev->enemy == NULL || !FClassnameIs(pev->enemy, "player"))
			return;
		gpGlobals->found_secrets++;
	}

	if (!FStringNull(pev->noise))
		EMIT_SOUND(ENT(pev), CHAN_VOICE, (char*)STRING(pev->noise), 1, ATTN_NORM);

// don't trigger again until reset
// pev->takedamage = DAMAGE_NO;

	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );

	if ( pev->message && pActivator && pActivator->IsPlayer() )
	{
		UTIL_ShowMessage( STRING(pev->message), pActivator );
//		CLIENT_PRINTF( ENT( pActivator->pev ), print_center, STRING(pev->message) );
	}

	if (m_flWait > 0)
	{
		SetThink( &CBaseTrigger::MultiWaitOver );
		pev->nextthink = gpGlobals->time + m_flWait;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink(  &CBaseTrigger::SUB_Remove );
	}
}


// the wait time has passed, so set back up for another activation
void CBaseTrigger :: MultiWaitOver( void )
{
//	if (pev->max_health)
//		{
//		pev->health		= pev->max_health;
//		pev->takedamage	= DAMAGE_YES;
//		pev->solid		= SOLID_BBOX;
//		}
	SetThink( NULL );
}


// ========================= COUNTING TRIGGER =====================================

//
// GLOBALS ASSUMED SET:  g_eoActivator
//
void CBaseTrigger::CounterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!CheckConditions(pActivator) || !IsViewObjectVisible(pActivator))
	{
		return;
	}
	m_cTriggersLeft--;
	m_hActivator = pActivator;
	if (m_cTriggersLeft == 0)
	{
		ActivateMultiTrigger(m_hActivator);
	}
}


/*QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
*/
class CTriggerCounter : public CBaseTrigger
{
public:
	void Spawn( void );
};
LINK_ENTITY_TO_CLASS( trigger_counter, CTriggerCounter );

void CTriggerCounter :: Spawn( void )
{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse( &CTriggerCounter::CounterUse );
}

// ====================== TRIGGER_CHANGELEVEL ================================

class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
{
public:
	void		Spawn( void );
};

LINK_ENTITY_TO_CLASS( trigger_transition, CTriggerVolume );

// Define space that travels across a level transition
void CTriggerVolume :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->model = NULL;
	pev->modelindex = 0;
}


// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
{
public:
	void Spawn( void );
	void Precache( void );
	void Think( void );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() | FCAP_FORCE_TRANSITION; }	// Always go across transitions
};
LINK_ENTITY_TO_CLASS( fireanddie, CFireAndDie );

void CFireAndDie::Spawn( void )
{
	pev->classname = MAKE_STRING("fireanddie");
	// Don't call Precache() - it should be called on restore
}


void CFireAndDie::Precache( void )
{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
}


void CFireAndDie::Think( void )
{
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_Remove( this );
}


#define SF_CHANGELEVEL_USEONLY		0x0002
class CChangeLevel : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TriggerChangeLevel( void );
	void EXPORT ExecuteChangeLevel( void );
	void EXPORT TouchChangeLevel( CBaseEntity *pOther );
	void ChangeLevelNow( CBaseEntity *pActivator );

	static edict_t *FindLandmark( const char *pLandmarkName );
	static int ChangeList( LEVELLIST *pLevelList, int maxList );
	static int AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark );
	static int InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
	char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
	int		m_changeTarget;
	float	m_changeTargetDelay;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_changelevel, CChangeLevel );

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] = 
{
	DEFINE_ARRAY( CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_ARRAY( CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost ),
	DEFINE_FIELD( CChangeLevel, m_changeTarget, FIELD_STRING ),
	DEFINE_FIELD( CChangeLevel, m_changeTargetDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE(CChangeLevel,CBaseTrigger);

//
// Cache user-entity-field values until spawn is called.
//

void CChangeLevel :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "map"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue );
		strncpy(m_szMapName, pkvd->szValue, cchMapNameMost - 1);
		m_szMapName[cchMapNameMost - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "landmark"))
	{
		if (strlen(pkvd->szValue) >= cchMapNameMost)
			ALERT( at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue );
		strncpy(m_szLandmarkName, pkvd->szValue, cchMapNameMost - 1);
		m_szLandmarkName[cchMapNameMost - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changetarget"))
	{
		m_changeTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "changedelay"))
	{
		m_changeTargetDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable.  Unless the NO_INTERMISSION flag is set, the view will go to the info_intermission spot and display stats.
*/

void CChangeLevel :: Spawn( void )
{
	if ( FStrEq( m_szMapName, "" ) )
		ALERT( at_console, "a trigger_changelevel doesn't have a map" );

	if ( FStrEq( m_szLandmarkName, "" ) )
		ALERT( at_console, "trigger_changelevel to %s doesn't have a landmark", m_szMapName );

	if (!FStringNull ( pev->targetname ) )
	{
		SetUse ( &CChangeLevel::UseChangeLevel );
	}
	InitTrigger();
	if ( !(pev->spawnflags & SF_CHANGELEVEL_USEONLY) )
		SetTouch( &CChangeLevel::TouchChangeLevel );
//	ALERT( at_console, "TRANSITION: %s (%s)\n", m_szMapName, m_szLandmarkName );
}


void CChangeLevel :: ExecuteChangeLevel( void )
{
	MESSAGE_BEGIN( MSG_ALL, SVC_CDTRACK );
		WRITE_BYTE( 3 );
		WRITE_BYTE( 3 );
	MESSAGE_END();

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();
}


FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel :: FindLandmark( const char *pLandmarkName )
{
	edict_t	*pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING( NULL, "targetname", pLandmarkName );
	while ( !FNullEnt( pentLandmark ) )
	{
		// Found the landmark
		if ( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING( pentLandmark, "targetname", pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}


//=========================================================
// CChangeLevel :: Use - allows level transitions to be 
// triggered by buttons, etc.
//
//=========================================================
void CChangeLevel :: UseChangeLevel ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	ChangeLevelNow( pActivator );
}

void CChangeLevel :: ChangeLevelNow( CBaseEntity *pActivator )
{
	edict_t	*pentLandmark;
	LEVELLIST	levels[16];


	// Don't work in deathmatch
	if ( g_pGameRules->IsDeathmatch() )
		return;

	// Some people are firing these multiple times in a frame, disable
	if ( gpGlobals->time == pev->dmgtime )
		return;

	pev->dmgtime = gpGlobals->time;


	CBaseEntity *pPlayer = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );
	if ( !InTransitionVolume( pPlayer, m_szLandmarkName ) )
	{
		ALERT( at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName );
		return;
	}

	// Create an entity to fire the changetarget
	if ( m_changeTarget )
	{
		CFireAndDie *pFireAndDie = GetClassPtr( (CFireAndDie *)NULL );
		if ( pFireAndDie )
		{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn( pFireAndDie->edict() );
		}
	}
	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy(st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	st_szNextSpot[0] = 0;	// Init landmark to NULL

	// look for a landmark entity		
	pentLandmark = FindLandmark( m_szLandmarkName );
	if ( !FNullEnt( pentLandmark ) )
	{
		strcpy(st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS(pentLandmark)->origin;
	}
//	ALERT( at_console, "Level touches %d levels\n", ChangeList( levels, 16 ) );
	ALERT( at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot );
	CHANGE_LEVEL( st_szNextMap, st_szNextSpot );
}

//
// GLOBALS ASSUMED SET:  st_szNextMap
//
void CChangeLevel :: TouchChangeLevel( CBaseEntity *pOther )
{
	if (!FClassnameIs(pOther->pev, "player"))
		return;

	ChangeLevelNow( pOther );
}


// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList( LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark )
{
	int i;

	if ( !pLevelList || !pMapName || !pLandmarkName || !pentLandmark )
		return 0;

	for ( i = 0; i < listCount; i++ )
	{
		if ( pLevelList[i].pentLandmark == pentLandmark && strcmp( pLevelList[i].mapName, pMapName ) == 0 )
			return 0;
	}
	strcpy( pLevelList[listCount].mapName, pMapName );
	strcpy( pLevelList[listCount].landmarkName, pLandmarkName );
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS(pentLandmark)->origin;

	return 1;
}

int BuildChangeList( LEVELLIST *pLevelList, int maxList )
{
	return CChangeLevel::ChangeList( pLevelList, maxList );
}


int CChangeLevel::InTransitionVolume( CBaseEntity *pEntity, char *pVolumeName )
{
	edict_t	*pentVolume;


	if ( pEntity->ObjectCaps() & FCAP_FORCE_TRANSITION )
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if ( pEntity->pev->movetype == MOVETYPE_FOLLOW )
	{
		if ( pEntity->pev->aiment != NULL )
			pEntity = CBaseEntity::Instance( pEntity->pev->aiment );
	}

	int inVolume = 1;	// Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME( NULL, pVolumeName );
	while ( !FNullEnt( pentVolume ) )
	{
		CBaseEntity *pVolume = CBaseEntity::Instance( pentVolume );
		
		if ( pVolume && FClassnameIs( pVolume->pev, "trigger_transition" ) )
		{
			if ( pVolume->Intersects( pEntity ) )	// It touches one, it's in the volume
				return 1;
			else
				inVolume = 0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
		}
		pentVolume = FIND_ENTITY_BY_TARGETNAME( pentVolume, pVolumeName );
	}

	return inVolume;
}


// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList( LEVELLIST *pLevelList, int maxList )
{
	edict_t	*pentChangelevel, *pentLandmark;
	int			i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING( NULL, "classname", "trigger_changelevel" );
	if ( FNullEnt( pentChangelevel ) )
		return 0;
	while ( !FNullEnt( pentChangelevel ) )
	{
		CChangeLevel *pTrigger;
		
		pTrigger = GetClassPtr((CChangeLevel *)VARS(pentChangelevel));
		if ( pTrigger )
		{
			// Find the corresponding landmark
			pentLandmark = FindLandmark( pTrigger->m_szLandmarkName );
			if ( pentLandmark )
			{
				// Build a list of unique transitions
				if ( AddTransitionToList( pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark ) )
				{
					count++;
					if ( count >= maxList )		// FULL!!
						break;
				}
			}
		}
		pentChangelevel = FIND_ENTITY_BY_STRING( pentChangelevel, "classname", "trigger_changelevel" );
	}

	if ( gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable )
	{
		CSave saveHelper( (SAVERESTOREDATA *)gpGlobals->pSaveData );

		for ( i = 0; i < count; i++ )
		{
			int j, entityCount = 0;
			CBaseEntity *pEntList[ MAX_ENTITY ];
			int			 entityFlags[ MAX_ENTITY ];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS( pLevelList[i].pentLandmark );

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while ( !FNullEnt( pent ) )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance(pent);
				if ( pEntity )
				{
//					ALERT( at_console, "Trying %s\n", STRING(pEntity->pev->classname) );
					int caps = pEntity->ObjectCaps();
					if ( !(caps & FCAP_DONT_SAVE) )
					{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if ( caps & FCAP_ACROSS_TRANSITION )
							flags |= FENTTABLE_MOVEABLE;
						if ( pEntity->pev->globalname && !pEntity->IsDormant() )
							flags |= FENTTABLE_GLOBAL;
						if ( flags )
						{
							pEntList[ entityCount ] = pEntity;
							entityFlags[ entityCount ] = flags;
							entityCount++;
							if ( entityCount > MAX_ENTITY )
								ALERT( at_error, "Too many entities across a transition!" );
						}
//						else
//							ALERT( at_console, "Failed %s\n", STRING(pEntity->pev->classname) );
					}
//					else
//						ALERT( at_console, "DON'T SAVE %s\n", STRING(pEntity->pev->classname) );
				}
				pent = pent->v.chain;
			}

			for ( j = 0; j < entityCount; j++ )
			{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if ( entityFlags[j] && InTransitionVolume( pEntList[j], pLevelList[i].landmarkName ) )
				{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex( pEntList[j] );
					// Flag it with the level number
					saveHelper.EntityFlagsSet( index, entityFlags[j] | (1<<i) );
				}
//				else
//					ALERT( at_console, "Screened out %s\n", STRING(pEntList[j]->pev->classname) );

			}
		}
	}

	return count;
}

/*
go to the next level for deathmatch
only called if a time or frag limit has expired
*/
void NextLevel( void )
{
	edict_t* pent;
	CChangeLevel *pChange;
	
	// find a trigger_changelevel
	pent = FIND_ENTITY_BY_CLASSNAME(NULL, "trigger_changelevel");
	
	// go back to start if no trigger_changelevel
	if (FNullEnt(pent))
	{
		gpGlobals->mapname = ALLOC_STRING("start");
		pChange = GetClassPtr( (CChangeLevel *)NULL );
		strcpy(pChange->m_szMapName, "start");
	}
	else
		pChange = GetClassPtr( (CChangeLevel *)VARS(pent));
	
	strcpy(st_szNextMap, pChange->m_szMapName);
	g_fGameOver = TRUE;
	
	if (pChange->pev->nextthink < gpGlobals->time)
	{
		pChange->SetThink( &CChangeLevel::ExecuteChangeLevel );
		pChange->pev->nextthink = gpGlobals->time + 0.1;
	}
}


// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Precache( void );
};
LINK_ENTITY_TO_CLASS( func_ladder, CLadder );


void CLadder :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


//=========================================================
// func_ladder - makes an area vertically negotiable
//=========================================================
void CLadder :: Precache( void )
{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if ( CVAR_GET_FLOAT("showtriggers") == 0 )
	{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
	}
	pev->effects &= ~EF_NODRAW;
}


void CLadder :: Spawn( void )
{
	Precache();

	SET_MODEL(ENT(pev), STRING(pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
}


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Touch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_push, CTriggerPush );


void CTriggerPush :: KeyValue( KeyValueData *pkvd )
{
	CBaseTrigger::KeyValue( pkvd );
}


/*QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
Pushes the player
*/

void CTriggerPush :: Spawn( )
{
	if ( pev->angles == g_vecZero )
		pev->angles.y = 360;
	InitTrigger();

	if (pev->speed == 0)
		pev->speed = 100;

	if ( FBitSet (pev->spawnflags, SF_TRIGGER_PUSH_START_OFF) )// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse( &CTriggerPush::ToggleUse );

	UTIL_SetOrigin( pev, pev->origin );		// Link into the list
}


void CTriggerPush :: Touch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch( pevToucher->movetype )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
	case MOVETYPE_FOLLOW:
		return;
	}
	if (!IsViewObjectVisible(pOther))
	{
		return;
	}

	if (pevToucher->solid != SOLID_NOT && (pevToucher->solid != SOLID_BSP ||
		(FClassnameIs(pevToucher, "func_pushable") && (pev->spawnflags & 4))))
	{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet(pev->spawnflags, SF_TRIG_PUSH_ONCE))
		{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if ( pevToucher->velocity.z > 0 )
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove( this );
		}
		else
		{	// Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if ( pevToucher->flags & FL_BASEVELOCITY )
				vecPush = vecPush +  pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;

			pevToucher->flags |= FL_BASEVELOCITY;
//			ALERT( at_console, "Vel %f, base %f\n", pevToucher->velocity.z, pevToucher->basevelocity.z );
		}
	}
}


//======================================
// teleport trigger
//
//

void CBaseTrigger :: TeleportTouch( CBaseEntity *pOther )
{
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;
	if (!CheckConditions(pOther) || !IsViewObjectVisible(pOther))
	{
		return;
	}

	// Only teleport monsters or clients
	if ( !FBitSet( pevToucher->flags, FL_CLIENT|FL_MONSTER ) )
		return;
    
	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;
 	
	if ( !( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) )
	{// no monsters allowed!
		if ( FBitSet( pevToucher->flags, FL_MONSTER ) )
		{
			return;
		}
	}

	if ( ( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) )
	{// no clients allowed
		if ( pOther->IsPlayer() )
		{
			return;
		}
	}
	
	pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, STRING(pev->target) );
	if (FNullEnt(pentTarget))
	   return;	
	
	Vector tmp = VARS( pentTarget )->origin;

	if ( pOther->IsPlayer() )
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;
	
	UTIL_SetOrigin( pevToucher, tmp );

	pevToucher->angles = pentTarget->v.angles;

	if ( pOther->IsPlayer() )
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
}


class CTriggerTeleport : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT TeleportUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};
LINK_ENTITY_TO_CLASS( trigger_teleport, CTriggerTeleport );

void CTriggerTeleport :: Spawn( void )
{
	InitTrigger();

	SetTouch( &CTriggerTeleport::TeleportTouch );
	SetUse(&CTriggerTeleport::TeleportUse);
}

void CTriggerTeleport::TeleportUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pPlayer = CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer)
	{
		TeleportTouch(pPlayer);
	}
}


LINK_ENTITY_TO_CLASS( info_teleport_destination, CPointEntity );



class CTriggerSave : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT SaveTouch( CBaseEntity *pOther );
	void EXPORT SaveUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};
LINK_ENTITY_TO_CLASS( trigger_autosave, CTriggerSave );

void CTriggerSave::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();
	SetUse(&CTriggerSave::SaveUse);
	SetTouch((pev->spawnflags & 1) ? NULL : &CTriggerSave::SaveTouch);
}

void CTriggerSave::SaveUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer && !pPlayer->m_endingMission)
	{
		SERVER_COMMAND("autosave\n");
	}
}

void CTriggerSave::SaveTouch( CBaseEntity *pOther )
{
	if ( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;

	// Only save on clients
	if ( !pOther->IsPlayer() || ((CBasePlayer *)pOther)->m_endingMission )
		return;
    
	SetTouch( NULL );
	UTIL_Remove( this );
	SERVER_COMMAND( "autosave\n" );
}

#define SF_ENDSECTION_USEONLY		0x0001

class CTriggerEndSection : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT EndSectionTouch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};
LINK_ENTITY_TO_CLASS( trigger_endsection, CTriggerEndSection );


// the engine leaves the game with "disconnect", and with developer on it stays in the console;
// the end of the game (cz_end) must bring the main menu, so open it right after
static void CZ_EndSection( const char *section )
{
	g_engfuncs.pfnEndSection( section );

	if( stricmp( section, "oem_end_credits" ))
		SERVER_COMMAND( "escape\n" );
}

void CTriggerEndSection::EndSectionUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Only save on clients
	if ( pActivator && !pActivator->IsNetClient() )
		return;

	SetUse( NULL );

	if ( pev->message )
	{
		CZ_EndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection::Spawn( void )
{
	if ( g_pGameRules->IsDeathmatch() )
	{
		REMOVE_ENTITY( ENT(pev) );
		return;
	}

	InitTrigger();

	SetUse ( &CTriggerEndSection::EndSectionUse );
	// If it is a "use only" trigger, then don't set the touch function.
	if ( ! (pev->spawnflags & SF_ENDSECTION_USEONLY) )
		SetTouch( &CTriggerEndSection::EndSectionTouch );
}

void CTriggerEndSection::EndSectionTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsNetClient() )
		return;
    
	SetTouch( NULL );

	if (pev->message)
	{
		CZ_EndSection(STRING(pev->message));
	}
	UTIL_Remove( this );
}

void CTriggerEndSection :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "section"))
	{
//		m_iszSectionName = ALLOC_STRING( pkvd->szValue );
		// Store this in message so we don't have to write save/restore for this ent
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseTrigger::KeyValue( pkvd );
}


class CTriggerGravity : public CBaseTrigger
{
public:
	void Spawn( void );
	void EXPORT GravityTouch( CBaseEntity *pOther );
};
LINK_ENTITY_TO_CLASS( trigger_gravity, CTriggerGravity );

void CTriggerGravity::Spawn( void )
{
	InitTrigger();
	SetTouch( &CTriggerGravity::GravityTouch );
}

void CTriggerGravity::GravityTouch( CBaseEntity *pOther )
{
	// Only save on clients
	if ( !pOther->IsPlayer() )
		return;

	pOther->pev->gravity = pev->gravity;
}







// this is a really bad idea.
class CTriggerChangeTarget : public CBaseDelay
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

private:
	int		m_iszNewTarget;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( trigger_changetarget, CTriggerChangeTarget );

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeTarget,CBaseDelay);

void CTriggerChangeTarget::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszNewTarget"))
	{
		m_iszNewTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

void CTriggerChangeTarget::Spawn( void )
{
}


void CTriggerChangeTarget::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pTarget = UTIL_FindEntityByString( NULL, "targetname", STRING( pev->target ) );

	if (pTarget)
	{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer( );
		if (pMonster)
		{
			pMonster->m_pGoalEnt = NULL;
		}
	}
}




#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4
#define SF_CAMERA_ALLOW_EXIT		8
#define SF_CAMERA_FIXED_ANGLES	16

LINK_ENTITY_TO_CLASS( trigger_camera, CTriggerCamera );

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] = 
{
	DEFINE_FIELD( CTriggerCamera, m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CTriggerCamera, m_pentPath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTriggerCamera, m_sPath, FIELD_STRING ),
	DEFINE_FIELD( CTriggerCamera, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_flReturnTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_flStopTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerCamera, m_moveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_targetSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_initialSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_acceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_deceleration, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerCamera, m_state, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCamera, m_useOverlay, FIELD_INTEGER ),
	DEFINE_FIELD( CTriggerCamera, m_immediate, FIELD_BOOLEAN ),
	DEFINE_FIELD( CTriggerCamera, m_flMinimumTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE(CTriggerCamera,CBaseDelay);

int CTriggerCamera::ObjectCaps()
{
	return 0;
}

BOOL CTriggerCamera::MinimumTimeMet()
{
	return m_flMinimumTime <= gpGlobals->time;
}

void CTriggerCamera::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;
	if ( m_acceleration == 0 )
		m_acceleration = 500;
	if ( m_deceleration == 0 )
		m_deceleration = 500;
}


void CTriggerCamera :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_flWait = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "useoverlay"))
	{
		m_useOverlay = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "moveto"))
	{
		m_sPath = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "acceleration"))
	{
		m_acceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "deceleration"))
	{
		m_deceleration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "immediate"))
	{
		m_immediate = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}



void CTriggerCamera::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_state ) )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));

	// Toggle state
	m_state = !m_state;
	if (m_state == 0)
	{
		m_flReturnTime = gpGlobals->time;
		return;
	}
	if ( !pActivator || !pActivator->IsPlayer() )
	{
		pActivator = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex( 1 ));
	}
		
	m_hPlayer = pActivator;

	m_flReturnTime = gpGlobals->time + m_flWait;
	m_flMinimumTime = gpGlobals->time + 0.3;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TARGET ) )
	{
		m_hTarget = m_hPlayer;
	}
	else
	{
		m_hTarget = GetNextTarget();
	}

	// Nothing to look at!
	if ( m_hTarget == NULL )
	{
		return;
	}


	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL ) )
	{
		((CBasePlayer *)pActivator)->EnableControl(FALSE);
	}

	if ( m_sPath )
	{
		m_pentPath = Instance( FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(m_sPath)) );
	}
	else
	{
		m_pentPath = NULL;
	}

	m_flStopTime = gpGlobals->time;
	if ( m_pentPath )
	{
		if ( m_pentPath->pev->speed != 0 )
			m_targetSpeed = m_pentPath->pev->speed;
		
		m_flStopTime += m_pentPath->GetDelay();
	}

	// copy over player information
	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_POSITION ) )
	{
		UTIL_SetOrigin( pev, pActivator->pev->origin + pActivator->pev->view_ofs );
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
	}
	else
	{
		pev->velocity = Vector( 0, 0, 0 );
	}

	if (m_useOverlay)
	{
		pPlayer->SetSecurityCamera(TRUE, (pev->spawnflags & SF_CAMERA_ALLOW_EXIT) ? this : NULL);
	}
	SET_VIEW( pActivator->edict(), edict() );

	SET_MODEL(ENT(pev), STRING(pActivator->pev->model) );

	// follow the player down
	SetThink( &CTriggerCamera::FollowTarget );
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move();
}


void CTriggerCamera::FollowTarget( )
{
	if (m_hPlayer == NULL)
		return;

	if (m_hTarget == NULL || m_flReturnTime < gpGlobals->time)
	{
		if (m_hPlayer->IsAlive( ))
		{
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl(TRUE);
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		pev->avelocity = Vector( 0, 0, 0 );
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
		if (m_useOverlay)
		{
			pPlayer->SetSecurityCamera(FALSE, NULL);
		}
		m_state = 0;
		return;
	}

	Vector vecGoal = UTIL_VecToAngles( m_hTarget->pev->origin - pev->origin );
	vecGoal.x = -vecGoal.x;

	if (pev->spawnflags & SF_CAMERA_FIXED_ANGLES)
	{
		pev->angles = vecGoal;
		pev->avelocity = g_vecZero;
		pev->velocity = g_vecZero;
	}
	else
	{
		if (pev->angles.y > 360)
			pev->angles.y -= 360;

		if (pev->angles.y < 0)
			pev->angles.y += 360;

		float dx = vecGoal.x - pev->angles.x;
		float dy = vecGoal.y - pev->angles.y;

		if (dx < -180)
			dx += 360;
		if (dx > 180)
			dx = dx - 360;
	
		if (dy < -180)
			dy += 360;
		if (dy > 180)
			dy = dy - 360;

		pev->avelocity.x = dx * 40 * gpGlobals->frametime;
		pev->avelocity.y = dy * 40 * gpGlobals->frametime;
	}


	if (!(FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))	
	{
		pev->velocity = pev->velocity * 0.8;
		if (pev->velocity.Length( ) < 10.0)
			pev->velocity = g_vecZero;
	}

	pev->nextthink = gpGlobals->time;

	Move();
}

void CTriggerCamera::Move()
{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if ( m_moveDistance <= 0 )
	{
		// Fire the passtarget if there is one
		if ( m_pentPath->pev->message )
		{
			FireTargets( STRING(m_pentPath->pev->message), this, this, USE_TOGGLE, 0 );
			if ( FBitSet( m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE ) )
				m_pentPath->pev->message = 0;
		}
		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget();

		// Set up next corner
		if ( !m_pentPath )
		{
			pev->velocity = g_vecZero;
		}
		else 
		{
			if ( m_pentPath->pev->speed != 0 )
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length();
			pev->movedir = delta.Normalize();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay();
		}
	}

	if ( m_flStopTime > gpGlobals->time )
		pev->speed = UTIL_Approach( 0, pev->speed, m_deceleration * gpGlobals->frametime );
	else
		pev->speed = UTIL_Approach( m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime );

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1-fraction));
}

LINK_ENTITY_TO_CLASS(env_fog, CClientFog);

TYPEDESCRIPTION CClientFog::m_SaveData[] =
{
	DEFINE_FIELD(CClientFog, m_flDensity, FIELD_FLOAT),
	DEFINE_FIELD(CClientFog, m_fStartDistance, FIELD_FLOAT),
	DEFINE_FIELD(CClientFog, m_fStopDistance, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CClientFog, CBaseEntity);

CClientFog::CClientFog()
{
	m_flDensity = 0;
	m_fStartDistance = 1500;
	m_fStopDistance = 2000;
}

void CClientFog::Spawn()
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
}

void CClientFog::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "density"))
	{
		m_flDensity = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fogStartDistance"))
	{
		m_fStartDistance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "fogStopDistance"))
	{
		m_fStopDistance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		pkvd->fHandled = FALSE;
	}
}

LINK_ENTITY_TO_CLASS(trigger_objective, CTriggerObjective);

CTriggerObjective::CTriggerObjective()
{
	objectiveState = 0;
	objectiveText = 0;
	transition = TRUE;
}

TYPEDESCRIPTION CTriggerObjective::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerObjective, objectiveState, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerObjective, objectiveText, FIELD_STRING),
	DEFINE_FIELD(CTriggerObjective, transition, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerObjective, CBaseTrigger);

void CTriggerObjective::Spawn()
{
	SetUse(&CTriggerObjective::ObjectiveUse);
}

int CTriggerObjective::ObjectCaps()
{
	return transition ? FCAP_ACROSS_TRANSITION : 0;
}

void CTriggerObjective::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "objective"))
	{
		objectiveText = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "state"))
	{
		objectiveState = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "notransition"))
	{
		transition = atoi(pkvd->szValue) == 0;
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
	}
}

void CTriggerObjective::ObjectiveUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		if (pev->target)
		{
			SUB_UseTargets(pActivator, useType, value);
		}
		objectiveState = objectiveState == 0 ? 1 : 2;
	}
}

BOOL CTriggerObjective::DisplayObjective(int lineCount)
{
	if (!objectiveState)
	{
		return FALSE;
	}
	hudtextparms_t textParms;
	textParms.effect = 0;
	textParms.x = 0.1;
	textParms.y = lineCount * 0.04f + 0.25f;
	textParms.r1 = textParms.g1 = textParms.b1 = textParms.a1 = 255;
	textParms.r2 = textParms.g2 = textParms.b2 = textParms.a2 = 255;
	textParms.fadeinTime = 0.5;
	textParms.fadeoutTime = 0.5;
	textParms.holdTime = 5;
	textParms.fxTime = 0;
	textParms.channel = lineCount;
	char text[255];
	if (objectiveState == 2)
	{
		_snprintf(text, sizeof(text), "%s_COMPLETE", STRING(objectiveText));
		textParms.r1 = textParms.b1 = 0;
	}
	else
	{
		strncpy(text, STRING(objectiveText), sizeof(text) - 1);
	}
	text[sizeof(text) - 1] = 0;
	UTIL_HudMessageAll(textParms, text);
	return TRUE;
}

LINK_ENTITY_TO_CLASS(trigger_usetool, CTriggerUseTool);

CTriggerUseTool::CTriggerUseTool()
{
	m_iToolName = 0;
	m_iToolTarget = 0;
	m_iToolSet = 0;
	m_iDefuseTime = 6;
	m_iRCBombTarget = 0;
	m_bombPlanted = FALSE;
}

TYPEDESCRIPTION CTriggerUseTool::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerUseTool, m_iToolName, FIELD_STRING),
	DEFINE_FIELD(CTriggerUseTool, m_iToolTarget, FIELD_STRING),
	DEFINE_FIELD(CTriggerUseTool, m_iToolSet, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerUseTool, m_iDefuseTime, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerUseTool, m_iRCBombTarget, FIELD_STRING),
	DEFINE_FIELD(CTriggerUseTool, m_bombPlanted, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerUseTool, CBaseTrigger);

void CTriggerUseTool::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerUseTool::TouchTriggerUseTool);
	SetUse(&CTriggerUseTool::FireUseTarget);
}

void CTriggerUseTool::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "toolname"))
	{
		m_iToolName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "tooltarget"))
	{
		m_iToolTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rcbombtarget"))
	{
		m_iRCBombTarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "toolset"))
	{
		m_iToolSet = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bombdefusetime"))
	{
		m_iDefuseTime = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
	}
}

void CTriggerUseTool::PreKillTarget()
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer->m_toolTrigger == this)
	{
		pPlayer->m_toolTrigger = NULL;
	}
}

void CTriggerUseTool::FireUseTarget(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		return;
	}
	if (pev->target)
	{
		SUB_UseTargets(pActivator, useType, value);
	}
	if (pev->spawnflags & 1)
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
		pPlayer->m_toolTrigger = NULL;
		SUB_Remove();
	}
}

BOOL CTriggerUseTool::IsPlayerInTrigger()
{
	edict_t *pPlayer = INDEXENT(1);
	return pPlayer && pPlayer->v.absmin.x <= pev->absmax.x && pPlayer->v.absmin.y <= pev->absmax.y &&
		pPlayer->v.absmin.z <= pev->absmax.z && pev->absmin.x <= pPlayer->v.absmax.x &&
		pev->absmin.y <= pPlayer->v.absmax.y && pev->absmin.z <= pPlayer->v.absmax.z;
}

BOOL CTriggerUseTool::IsPlayerInPosition()
{
	if (!IsPlayerInTrigger())
	{
		return FALSE;
	}
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (!pPlayer)
	{
		return FALSE;
	}
	if (!m_iToolTarget)
	{
		return TRUE;
	}
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_iToolTarget));
	return pTarget && pPlayer->FInViewCone(pTarget, 30);
}

BOOL CTriggerUseTool::IsPlayerInPositionUsingProperTool()
{
	return IsPlayerUsingProperTool() && IsPlayerInPosition();
}

BOOL CTriggerUseTool::IsBombDefuseTrigger()
{
	return FStrEq(STRING(m_iToolName), "bomb_defuse");
}

CBaseEntity *CTriggerUseTool::GetRCBombTarget()
{
	if (!m_iRCBombTarget)
	{
		return NULL;
	}
	CBaseEntity *pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_iRCBombTarget));
	m_bombPlanted = TRUE;
	return pTarget;
}

BOOL CTriggerUseTool::IsPlayerUsingProperTool()
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (!pPlayer)
	{
		return 0;
	}
	char weapon[255];
	strcpy(weapon, STRING(pPlayer->GetActiveWeaponClassname()));
	int tool;
	if (FStrEq(weapon, "bomb_defuse"))
	{
		tool = 128;
	}
	else if (FStrEq(weapon, "weapon_blowtorch"))
	{
		tool = 1;
	}
	else if (FStrEq(weapon, "weapon_briefcase"))
	{
		tool = 2;
	}
	else if (FStrEq(weapon, "weapon_camera"))
	{
		tool = 4;
	}
	else if (FStrEq(weapon, "weapon_fiberopticcamera"))
	{
		tool = 8;
	}
	else if (FStrEq(weapon, "weapon_radio"))
	{
		tool = 16;
	}
	else if (FStrEq(weapon, "weapon_radiocontrolledbomb"))
	{
		tool = 32;
	}
	else
	{
		return (FStrEq(weapon, "weapon_syringe") || FStrEq(STRING(m_iToolName), weapon)) &&
			(m_iToolSet & 64) ? 64 : 0;
	}
	return (m_iToolSet & tool) || FStrEq(STRING(m_iToolName), weapon) ? tool : 0;
}

void CTriggerUseTool::TouchTriggerUseTool(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer() || !UTIL_IsMasterTriggered(m_sMaster, pOther))
	{
		return;
	}
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (pPlayer->m_toolTrigger != this && g_iSkillLevel == 1)
	{
		DisplayToolMessage();
	}
	pPlayer->m_toolTrigger = this;
	BOOL inPosition = IsPlayerInPosition();
	int properTool = IsPlayerUsingProperTool();
	if (pPlayer->m_iHideHUD & HIDEHUD_WEAPONS)
	{
		return;
	}
	char weapon[255];
	strcpy(weapon, STRING(pPlayer->GetActiveWeaponClassname()));
	if (FStrEq(STRING(m_iToolName), "weapon_blowtorch") || (m_iToolSet & 1))
	{
		pPlayer->m_iHudIcons |= 1;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 1;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "bomb_defuse") || (m_iToolSet & 128))
	{
		pPlayer->m_iHudIcons |= 512;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 512;
		pPlayer->m_iHudIconsCorrectWeapon |= 512;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_briefcase") || (m_iToolSet & 2))
	{
		pPlayer->m_iHudIcons |= 2;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 2;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_camera") || (m_iToolSet & 4))
	{
		pPlayer->m_iHudIcons |= 4;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 4;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_fiberopticcamera") || (m_iToolSet & 8))
	{
		pPlayer->m_iHudIcons |= 8;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 8;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_radio") || (m_iToolSet & 16))
	{
		pPlayer->m_iHudIcons |= 16;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 16;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_radiocontrolledbomb") || (m_iToolSet & 32))
	{
		pPlayer->m_iHudIcons |= 32;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 32;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
	if (FStrEq(STRING(m_iToolName), "weapon_syringe") || (m_iToolSet & 64))
	{
		pPlayer->m_iHudIcons |= 128;
		if (inPosition)
			pPlayer->m_iHudIconsInPosition |= 128;
		pPlayer->m_iHudIconsCorrectWeapon |= properTool;
	}
}

void CTriggerUseTool::DisplayToolMessage()
{
	if (g_iSkillLevel != 1)
	{
		return;
	}
	hudtextparms_t textParms;
	textParms.x = -1;
	textParms.y = 0.01;
	textParms.effect = 0;
	textParms.r1 = textParms.g1 = 0;
	textParms.b1 = textParms.a1 = 255;
	textParms.r2 = textParms.g2 = textParms.b2 = textParms.a2 = 255;
	textParms.fadeinTime = textParms.fadeoutTime = 0.05;
	textParms.holdTime = 1;
	textParms.fxTime = 0;
	textParms.channel = 0;
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	char weapon[255];
	strcpy(weapon, STRING(pPlayer->GetActiveWeaponClassname()));
	if (FStrEq(STRING(m_iToolName), "weapon_blowtorch") || (m_iToolSet & 1))
		UTIL_HudMessageAll(textParms, "#BLOWTORCH_ZONE");
	if (FStrEq(STRING(m_iToolName), "bomb_defuse") || (m_iToolSet & 128))
		UTIL_HudMessageAll(textParms, "#BOMBDEFUSE_ZONE");
	if (FStrEq(STRING(m_iToolName), "weapon_briefcase") || (m_iToolSet & 2))
		UTIL_HudMessageAll(textParms, "#BRIEFCASE_ZONE");
	if (FStrEq(STRING(m_iToolName), "weapon_camera") || (m_iToolSet & 4))
		UTIL_HudMessageAll(textParms, "#CAMERA_ZONE");
	if (FStrEq(STRING(m_iToolName), "weapon_fiberopticcamera") || (m_iToolSet & 8))
		UTIL_HudMessageAll(textParms, "#FIBERCAMERA_ZONE");
	if (FStrEq(STRING(m_iToolName), "weapon_radio") || (m_iToolSet & 16))
		UTIL_HudMessageAll(textParms, "#RADIO_ZONE");
	if (FStrEq(STRING(m_iToolName), "weapon_radiocontrolledbomb") || (m_iToolSet & 32))
		UTIL_HudMessageAll(textParms, "#RCBOMB_ZONE");
}

class CRainZone : public CBaseTrigger
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(env_rain, CRainZone);
LINK_ENTITY_TO_CLASS(func_rain, CRainZone);

void CRainZone::Spawn()
{
	InitTrigger();
}

class CSnowZone : public CBaseTrigger
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS(env_snow, CSnowZone);
LINK_ENTITY_TO_CLASS(func_snow, CSnowZone);

void CSnowZone::Spawn()
{
	InitTrigger();
}

class CRandomWeather : public CPointEntity
{
public:
	void Spawn() override;
	void EXPORT WeatherToggle();
	float m_flNextSwitchTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(env_randomweather, CRandomWeather);

void CRandomWeather::Spawn()
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	m_flNextSwitchTime = gpGlobals->time + RANDOM_FLOAT(20, 25);
	SetThink(&CRandomWeather::WeatherToggle);
	pev->nextthink = m_flNextSwitchTime;
}

void CRandomWeather::WeatherToggle()
{
	CBaseEntity *pZone = NULL;
	while ((pZone = UTIL_FindEntityByClassname(pZone, "env_snow")) != NULL)
	{
		pZone->pev->iuser2 = !pZone->pev->iuser2;
	}
	while ((pZone = UTIL_FindEntityByClassname(pZone, "env_rain")) != NULL)
	{
		pZone->pev->iuser2 = !pZone->pev->iuser2;
	}
	m_flNextSwitchTime = gpGlobals->time + RANDOM_FLOAT(20, 25);
	SetThink(&CRandomWeather::WeatherToggle);
	pev->nextthink = m_flNextSwitchTime;
}

class CTriggerIncognito : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT IncognitoUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(trigger_incognito, CTriggerIncognito);

void CTriggerIncognito::Spawn()
{
	InitTrigger();
	SetUse(&CTriggerIncognito::IncognitoUse);
}

void CTriggerIncognito::IncognitoUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (CheckConditions(pActivator))
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
		pPlayer->m_iIncognito = !pPlayer->m_iIncognito;
	}
}

class CTriggerFreezePlayer : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT FreezeUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(trigger_freezeplayer, CTriggerFreezePlayer);

void CTriggerFreezePlayer::Spawn()
{
	InitTrigger();
	SetUse(&CTriggerFreezePlayer::FreezeUse);
}

void CTriggerFreezePlayer::FreezeUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CheckConditions(pActivator))
	{
		return;
	}
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pev->solid != SOLID_NOT)
	{
		pev->solid = SOLID_NOT;
		pPlayer->EnableControl(FALSE);
	}
	else
	{
		pev->solid = SOLID_TRIGGER;
		pPlayer->EnableControl(TRUE);
		gpGlobals->force_retouch++;
	}
	UTIL_SetOrigin(pev, pev->origin);
}

class CTriggerHud : public CBaseTrigger
{
public:
	CTriggerHud() { m_display = TRUE; }
	void Spawn() override;
	void KeyValue(KeyValueData *pkvd) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void EXPORT TriggerHudUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	static TYPEDESCRIPTION m_SaveData[];
	BOOL m_display;
};

LINK_ENTITY_TO_CLASS(trigger_hud, CTriggerHud);

TYPEDESCRIPTION CTriggerHud::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerHud, m_display, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerHud, CBaseTrigger);

void CTriggerHud::Spawn()
{
	InitTrigger();
	SetUse(&CTriggerHud::TriggerHudUse);
}

void CTriggerHud::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "display"))
	{
		m_display = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
	}
}

void CTriggerHud::TriggerHudUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CheckConditions(pActivator))
	{
		return;
	}
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (m_display)
	{
		pPlayer->m_iHideHUD = pPlayer->savedHudBits;
		pPlayer->m_iHudIcons = pPlayer->savedIconBits;
		gpGlobals->force_retouch++;
	}
	else
	{
		pPlayer->savedHudBits = pPlayer->m_iHideHUD;
		pPlayer->savedIconBits = pPlayer->m_iHudIcons;
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH |
			HIDEHUD_TIMER | HIDEHUD_CROSSHAIR | HIDEHUD_NIGHTVISION;
		pPlayer->m_iHudIcons = 0;
	}
	UTIL_SetOrigin(pev, pev->origin);
}

class CTriggerKillTarget : public CBaseTrigger
{
public:
	void Spawn() override;
	void EXPORT KillEmAll(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(trigger_killtarget, CTriggerKillTarget);

void CTriggerKillTarget::Spawn()
{
	InitTrigger();
	SetTouch(&CBaseTrigger::HurtTouch);
	if (pev->targetname)
	{
		SetUse(&CTriggerKillTarget::KillEmAll);
	}
}

void CTriggerKillTarget::KillEmAll(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!CheckConditions(pActivator))
	{
		return;
	}
	float extraDamage = 10;
	if (pev->spawnflags & 1)
		extraDamage = 50;
	if (FStrEq(STRING(pev->target), "player"))
	{
		CBaseEntity *pPlayer = CBaseEntity::Instance(INDEXENT(1));
		if (pPlayer)
		{
			pPlayer->TakeDamage(pev, pev, pPlayer->pev->health + 10, DMG_FALL);
		}
	}
	else
	{
		CBaseEntity *pTarget = NULL;
		while ((pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(pev->target))) != NULL)
		{
			int oldDamage = (int)pTarget->pev->takedamage;
			pTarget->pev->takedamage = DAMAGE_YES;
			CBaseMonster *pMonster = pTarget->MyMonsterPointer();
			if (pMonster)
			{
				pMonster->m_unkillable = FALSE;
			}
			pTarget->TakeDamage(pev, pev, extraDamage + pTarget->pev->health, DMG_GENERIC);
			pTarget->pev->takedamage = oldDamage;
		}
		if (pev->spawnflags & 2)
		{
			UTIL_Remove(this);
		}
	}
}

LINK_ENTITY_TO_CLASS(trigger_stealth, CTriggerStealth);

TYPEDESCRIPTION CTriggerStealth::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerStealth, m_flCover, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerStealth, m_flCoverTime, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerStealth, m_bPlayerHidden, FIELD_BOOLEAN),
	DEFINE_FIELD(CTriggerStealth, m_showIcon, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTriggerStealth, CBaseTrigger);

CTriggerStealth::CTriggerStealth()
{
	m_flCover = 0;
	m_flCoverTime = 0;
	m_bPlayerHidden = FALSE;
	m_showIcon = TRUE;
}

CTriggerStealth::~CTriggerStealth()
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer && pPlayer->m_stealthTrigger == this)
	{
		pPlayer->m_stealthTrigger = NULL;
	}
}

void CTriggerStealth::Spawn()
{
	InitTrigger();
	SetTouch(&CTriggerStealth::TouchTriggerStealth);
}

void CTriggerStealth::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "cover"))
	{
		m_flCover = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "showicon"))
	{
		m_showIcon = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
	}
}

void CTriggerStealth::TouchTriggerStealth(CBaseEntity *pOther)
{
	if (!pOther->IsPlayer())
	{ // only clients may trigger these events
		return;
	}
	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	if (HasGlobalState() ? !IsGlobalStateTriggered() : !UTIL_IsMasterTriggered(m_sMaster, pOther))
	{
		pPlayer->m_stealthTrigger = NULL;
		return;
	}
	pPlayer->m_stealthTrigger = this;
	if (!(pPlayer->m_iHideHUD & HIDEHUD_WEAPONS) && m_showIcon)
	{
		pPlayer->m_iHudIcons |= 64;
	}
}

BOOL CTriggerStealth::IsPlayerHidden()
{
	edict_t *pPlayer = INDEXENT(1);
	if (!pPlayer || pPlayer->v.absmin.x > pev->absmax.x || pPlayer->v.absmin.y > pev->absmax.y ||
		pPlayer->v.absmin.z > pev->absmax.z || pev->absmin.x > pPlayer->v.absmax.x ||
		pev->absmin.y > pPlayer->v.absmax.y || pev->absmin.z > pPlayer->v.absmax.z)
	{
		return FALSE;
	}
	if (gpGlobals->time > m_flCoverTime)
	{
		m_flCoverTime = gpGlobals->time + RANDOM_FLOAT(1, 2.5);
		m_bPlayerHidden = !(m_flCover < RANDOM_FLOAT(0, 100));
	}
	return m_bPlayerHidden;
}

class CTriggerZipline : public CBaseTrigger
{
public:
	void Spawn() override;
	void KeyValue(KeyValueData *pkvd) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void EXPORT ZiplineUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT ZiplineTouch(CBaseEntity *pActivator);
	CBaseEntity *FindZiplineTarget();
	void TeleportPlayer(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	void DoZipline(CBaseEntity *pActivator, CBasePlayer *pPlayer);
	static TYPEDESCRIPTION m_SaveData[];
	string_t m_ziplinetarget;
	BOOL m_location;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(trigger_zipline, CTriggerZipline);

TYPEDESCRIPTION CTriggerZipline::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerZipline, m_location, FIELD_BOOLEAN),
	DEFINE_FIELD(CTriggerZipline, m_ziplinetarget, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerZipline, CBaseTrigger);

void CTriggerZipline::Spawn()
{
	InitTrigger();
	SetUse(&CTriggerZipline::ZiplineUse);
	SetTouch(&CTriggerZipline::ZiplineTouch);
}

void CTriggerZipline::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "end"))
	{
		m_location = atoi(pkvd->szValue) == 1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "start"))
	{
		m_location = atoi(pkvd->szValue) != 1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ziplinetarget"))
	{
		m_ziplinetarget = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
	}
}

CBaseEntity *CTriggerZipline::FindZiplineTarget()
{
	return m_ziplinetarget ? UTIL_FindEntityByTargetname(NULL, STRING(m_ziplinetarget)) : NULL;
}

void CTriggerZipline::TeleportPlayer(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
	edict_t *pDestination = FIND_ENTITY_BY_TARGETNAME(NULL, STRING(m_ziplinetarget));
	if (FNullEnt(pDestination))
	{
		return;
	}
	entvars_t *pPlayerVars = pPlayer->pev;
	Vector origin = pDestination->v.origin;
	if (pPlayer->IsPlayer())
	{
		origin.z -= pPlayerVars->mins.z;
	}
	pPlayerVars->flags &= ~FL_ONGROUND;
	origin.z++;
	UTIL_SetOrigin(pPlayerVars, origin);
	pPlayerVars->angles = pDestination->v.angles;
	if (pPlayer->IsPlayer())
	{
		pPlayerVars->v_angle = pDestination->v.angles;
	}
	pPlayerVars->fixangle = TRUE;
	pPlayerVars->basevelocity = pPlayerVars->velocity = g_vecZero;
}

void CTriggerZipline::DoZipline(CBaseEntity *pActivator, CBasePlayer *pPlayer)
{
	if (m_location)
	{
		pPlayer->m_iHideHUD = pPlayer->savedHudBits;
		pPlayer->m_iHudIcons = pPlayer->savedIconBits;
		pPlayer->AltFireWeapon();
	}
	else
	{
		pPlayer->savedHudBits = pPlayer->m_iHideHUD;
		pPlayer->savedIconBits = pPlayer->m_iHudIcons;
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_FLASHLIGHT | HIDEHUD_HEALTH |
			HIDEHUD_TIMER | HIDEHUD_CROSSHAIR | HIDEHUD_NIGHTVISION;
		pPlayer->m_iHudIcons = 0;
		pPlayer->GiveNamedItem("weapon_zipline");
		CBaseEntity *pTarget = FindZiplineTarget();
		if (pTarget)
		{
			TeleportPlayer(pPlayer, pTarget);
		}
	}
	SUB_UseTargets(this, USE_TOGGLE, 0);
	if (pev->spawnflags & 1)
	{
		UTIL_Remove(this);
	}
}

void CTriggerZipline::ZiplineTouch(CBaseEntity *pActivator)
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer && UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		SetTouch(NULL);
		DoZipline(pActivator, pPlayer);
	}
}

void CTriggerZipline::ZiplineUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer && UTIL_IsMasterTriggered(m_sMaster, pActivator))
	{
		SetUse(NULL);
		DoZipline(pActivator, pPlayer);
	}
}

class CTriggerChangeKeyValue : public CBaseDelay
{
public:
	void Spawn() override;
	int ObjectCaps() override;
	void KeyValue(KeyValueData *pkvd) override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	static TYPEDESCRIPTION m_SaveData[];
private:
	string_t m_changeTargetName;
	int m_cTargets;
	int m_iKey[16];
	int m_iValue[16];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(trigger_changekeyvalue, CTriggerChangeKeyValue);

TYPEDESCRIPTION CTriggerChangeKeyValue::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerChangeKeyValue, m_cTargets, FIELD_INTEGER),
	DEFINE_ARRAY(CTriggerChangeKeyValue, m_iKey, FIELD_STRING, 16),
	DEFINE_ARRAY(CTriggerChangeKeyValue, m_iValue, FIELD_STRING, 16),
	DEFINE_FIELD(CTriggerChangeKeyValue, m_changeTargetName, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerChangeKeyValue, CBaseDelay);

void CTriggerChangeKeyValue::Spawn()
{
}

int CTriggerChangeKeyValue::ObjectCaps()
{
	return 0;
}

void CTriggerChangeKeyValue::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "origin") || FStrEq(pkvd->szKeyName, "target") ||
		FStrEq(pkvd->szKeyName, "targetname") || FStrEq(pkvd->szKeyName, "classname"))
	{
		CBaseDelay::KeyValue(pkvd);
	}
	else
	{
		if (FStrEq(pkvd->szKeyName, "changetarget"))
		{
			m_changeTargetName = ALLOC_STRING(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		if (m_cTargets < 16)
		{
			char token[128];
			UTIL_StripToken(pkvd->szKeyName, token, sizeof(token));
			m_iKey[m_cTargets] = ALLOC_STRING(token);
			UTIL_StripToken(pkvd->szValue, token, sizeof(token));
			m_iValue[m_cTargets] = ALLOC_STRING(token);
			m_cTargets++;
			pkvd->fHandled = TRUE;
		}
	}
}

void CTriggerChangeKeyValue::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity *pTarget = NULL;
	while ((pTarget = UTIL_FindEntityByString(pTarget, "targetname", STRING(pev->target))) != NULL)
	{
		for (int i = 0; i < m_cTargets; i++)
		{
			KeyValueData *pkvd = new KeyValueData;
			char key[128], data[128];
			strncpy(key, STRING(m_iKey[i]), sizeof(key) - 1);
			key[sizeof(key) - 1] = 0;
			strncpy(data, STRING(m_iValue[i]), sizeof(data) - 1);
			data[sizeof(data) - 1] = 0;
			pkvd->szKeyName = key;
			pkvd->szValue = data;
			pkvd->fHandled = FALSE;
			pkvd->szClassName = (char *)STRING(pTarget->pev->classname);
			DispatchKeyValue(pTarget->edict(), pkvd);
			delete pkvd;
		}
		if (m_changeTargetName)
		{
			pTarget->pev->target = m_changeTargetName;
		}
	}
}

class CTriggerEndMission : public CBaseTrigger
{
public:
	CTriggerEndMission();
	void Spawn() override;
	void KeyValue(KeyValueData *pkvd) override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void EXPORT EndMissionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT EndMissionTouch(CBaseEntity *pOther);
	void EXPORT ChangeLevelThink();
	void UpdateLevelLocks(const char *levelName);
	static TYPEDESCRIPTION m_SaveData[];
	float duration;
	float holdtime;
	float alphaamt;
	float r, g, b;
	int mp3Name;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(trigger_endmission, CTriggerEndMission);

CTriggerEndMission::CTriggerEndMission()
{
	duration = 5;
	holdtime = 3;
	alphaamt = 255;
	r = 0;
	g = 0;
	b = 0;
	mp3Name = 0;
}

TYPEDESCRIPTION CTriggerEndMission::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerEndMission, duration, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, holdtime, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, alphaamt, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, r, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, g, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, b, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerEndMission, mp3Name, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CTriggerEndMission, CBaseTrigger);

void CTriggerEndMission::Spawn()
{
	if (g_pGameRules->IsDeathmatch())
	{
		REMOVE_ENTITY(edict());
		return;
	}
	InitTrigger();
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	pev->dmg_take = duration;
	pev->dmg_save = holdtime;
	pev->renderamt = alphaamt;
	pev->rendercolor = Vector(r, g, b);
	SetUse(&CTriggerEndMission::EndMissionUse);
	if (!(pev->spawnflags & 1))
	{
		SetTouch(&CTriggerEndMission::EndMissionTouch);
	}
}

void CTriggerEndMission::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "nextmap"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "music"))
	{
		mp3Name = ALLOC_STRING(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		duration = atof(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "holdtime"))
	{
		holdtime = atof(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "fade_red"))
	{
		r = atof(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "fade_green"))
	{
		g = atof(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "fade_blue"))
	{
		b = atof(pkvd->szValue);
	}
	else if (FStrEq(pkvd->szKeyName, "fade_alpha"))
	{
		alphaamt = atof(pkvd->szValue);
	}
	else
	{
		CBaseTrigger::KeyValue(pkvd);
		return;
	}
	pkvd->fHandled = TRUE;
}

void CTriggerEndMission::EndMissionUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetUse(NULL);
	if (mp3Name)
	{
		edict_t *pClient = INDEXENT(1);
		if (pClient)
		{
			char command[256];
			_snprintf(command, sizeof(command), "mp3 play %s\n", STRING(mp3Name));
			command[sizeof(command) - 1] = 0;
			CLIENT_COMMAND(pClient, command);
		}
	}
	UTIL_ScreenFadeAll(pev->rendercolor, pev->dmg_take, pev->dmg_save, (int)pev->renderamt, FFADE_OUT);
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer)
	{
		pPlayer->m_endingMission = TRUE;
		unsigned short event = PRECACHE_EVENT(1, "events/blowtorchholster.sc");
		PLAYBACK_EVENT_FULL(0, pPlayer->edict(), event, 0, (float *)&g_vecZero.x, (float *)&g_vecZero.x, 0, 0,
			(int)(pPlayer->pev->punchangle.x * 100), (int)(pPlayer->pev->punchangle.y * 100), 0, 0);
	}
	SetThink(&CTriggerEndMission::ChangeLevelThink);
	pev->nextthink = gpGlobals->time + pev->dmg_save + pev->dmg_take - 0.1f;
}

void CTriggerEndMission::EndMissionTouch(CBaseEntity *pOther)
{
	if (!pOther->IsNetClient())
	{
		return;
	}
	SetTouch(NULL);
	if (mp3Name)
	{
		edict_t *pClient = INDEXENT(1);
		if (pClient)
		{
			char command[256];
			_snprintf(command, sizeof(command), "mp3 play %s\n", STRING(mp3Name));
			command[sizeof(command) - 1] = 0;
			CLIENT_COMMAND(pClient, command);
		}
	}
	UTIL_ScreenFadeAll(pev->rendercolor, pev->dmg_take, pev->dmg_save, (int)pev->renderamt, FFADE_OUT);
	if (pOther->IsNetClient())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
		if (pPlayer)
		{
			pPlayer->m_endingMission = TRUE;
			unsigned short event = PRECACHE_EVENT(1, "events/blowtorchholster.sc");
			PLAYBACK_EVENT_FULL(0, pPlayer->edict(), event, 0, (float *)&g_vecZero.x, (float *)&g_vecZero.x, 0, 0,
				(int)(pPlayer->pev->punchangle.x * 100), (int)(pPlayer->pev->punchangle.y * 100), 0, 0);
		}
		SetThink(&CTriggerEndMission::ChangeLevelThink);
		pev->nextthink = gpGlobals->time + pev->dmg_save + pev->dmg_take - 0.1f;
	}
}

void CTriggerEndMission::UpdateLevelLocks(const char *levelName)
{
	if (strstr(levelName, "cz_train"))
		CBasePlayer::gLevelLocks |= 2;
	else if (strstr(levelName, "cz_recoil"))
		CBasePlayer::gLevelLocks |= 4;
	else if (strstr(levelName, "cz_lostcause"))
		CBasePlayer::gLevelLocks |= 8;
	else if (strstr(levelName, "cz_silo"))
		CBasePlayer::gLevelLocks |= 16;
	else if (strstr(levelName, "cz_brecon"))
		CBasePlayer::gLevelLocks |= 32;
	else if (strstr(levelName, "cz_druglab"))
		CBasePlayer::gLevelLocks |= 64;
	else if (strstr(levelName, "cz_motor"))
		CBasePlayer::gLevelLocks |= 128;
	else if (strstr(levelName, "cz_thinice"))
		CBasePlayer::gLevelLocks |= 256;
	else if (strstr(levelName, "cz_downed"))
		CBasePlayer::gLevelLocks |= 512;
	else if (strstr(levelName, "cz_hankagai"))
		CBasePlayer::gLevelLocks |= 1024;
	else if (strstr(levelName, "cz_turncrank"))
		CBasePlayer::gLevelLocks |= 2048;
	else if (strstr(levelName, "alamo"))
		CBasePlayer::gLevelLocks |= 4096;
	else if (strstr(levelName, "cz_hr"))
		CBasePlayer::gLevelLocks |= 8192;
	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
	if (pPlayer)
	{
		pPlayer->m_LevelLocks = CBasePlayer::gLevelLocks;
	}
}

void CTriggerEndMission::ChangeLevelThink()
{
	if (pev->message)
	{
		UpdateLevelLocks(STRING(pev->message));
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(INDEXENT(1));
		if (pPlayer)
		{
			pPlayer->m_bHasNightVision = FALSE;
			pPlayer->m_endingMission = FALSE;
		}
		CHANGE_LEVEL((char *)STRING(pev->message), NULL);
	}
	UTIL_Remove(this);
}

class CTriggerSequence : public CBaseDelay
{
public:
	CTriggerSequence();
	void Spawn() override;
	void Precache() override;
	void Restart() override;
	void Think() override;
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) override;
	void KeyValue(KeyValueData *pkvd) override;
	int ObjectCaps() override;
	int Save(CSave &save) override;
	int Restore(CRestore &restore) override;
	void StopSequence();
	void GoToFirstCommand();
	void FastForward();
	void ExecuteFireTargets(char *targetNames);
	void ExecuteKillTargets(char *targetNames);
	void ExecuteTextMessage(client_textmessage_t *message, int textChannel);
	void ExecuteSound(char *soundName);
	void ExecuteSpeech(char *soundName, char *speakerName, char *listenerName, float duration);
	void ExecuteSoundOrSpeech(char *soundName, char *speakerName, char *listenerName, float duration);
	void AdvanceAndCheckForRepeat(int repeatCount);
	void ExecuteSequenceCommand(sequenceCommandLine_s *command);
	static TYPEDESCRIPTION m_SaveData[];
	char m_sequenceEntryName[256];
	float m_sequenceStartTime;
	sequenceEntry_s *m_sequenceEntry;
	sequenceCommandLine_s *m_nextCommand;
	CBaseEntity *m_activator;
	CBaseEntity *m_caller;
	USE_TYPE m_useType;
	float m_value;
	BOOL m_isBusy;
	BOOL m_isDisabled;
	BOOL m_fastForward;
};

LINK_ENTITY_TO_CLASS(trigger_sequence, CTriggerSequence);

TYPEDESCRIPTION CTriggerSequence::m_SaveData[] =
{
	DEFINE_ARRAY(CTriggerSequence, m_sequenceEntryName, FIELD_CHARACTER, 256),
	DEFINE_FIELD(CTriggerSequence, m_sequenceStartTime, FIELD_TIME),
	DEFINE_FIELD(CTriggerSequence, m_activator, FIELD_CLASSPTR),
	DEFINE_FIELD(CTriggerSequence, m_useType, FIELD_INTEGER),
	DEFINE_FIELD(CTriggerSequence, m_value, FIELD_FLOAT),
	DEFINE_FIELD(CTriggerSequence, m_isBusy, FIELD_BOOLEAN),
	DEFINE_FIELD(CTriggerSequence, m_isDisabled, FIELD_BOOLEAN),
};

int CTriggerSequence::Save(CSave &save)
{
	if (!CBaseDelay::Save(save))
		return 0;
	return save.WriteFields("CTriggerSequence", this, m_SaveData, ARRAYSIZE(m_SaveData));
}

int CTriggerSequence::Restore(CRestore &restore)
{
	if (!CBaseDelay::Restore(restore))
		return 0;

	int status = restore.ReadFields("CTriggerSequence", this, m_SaveData, ARRAYSIZE(m_SaveData));

	// czeror: the entry is built (and its sounds precached) here, while the saved game
	// loads and precaching is allowed; Restart() after ServerActivate finds it then
	m_sequenceEntry = CZ_SequenceGet(NULL, m_sequenceEntryName);

	return status;
}

CTriggerSequence::CTriggerSequence()
{
	m_sequenceEntryName[0] = 0;
	m_sequenceEntry = NULL;
	StopSequence();
	m_isDisabled = FALSE;
	m_fastForward = FALSE;
}

int CTriggerSequence::ObjectCaps()
{
	return 0;
}

void CTriggerSequence::Precache()
{
}

void CTriggerSequence::Spawn()
{
	m_sequenceEntry = CZ_SequenceGet(NULL, m_sequenceEntryName);
}

void CTriggerSequence::Restart()
{
	m_nextCommand = NULL;
	m_sequenceEntry = CZ_SequenceGet(NULL, m_sequenceEntryName);
	m_fastForward = m_isBusy;
}

void CTriggerSequence::KeyValue(KeyValueData *pkvd)
{
	if (!stricmp(pkvd->szKeyName, "sequence_id"))
	{
		size_t length = strlen(pkvd->szValue);
		if (length > sizeof(m_sequenceEntryName) - 1)
		{
			ALERT(at_error, "Sequence entry label '%s' too long (was %d chars, max is %d)\n",
				pkvd->szValue, (int)length, (int)sizeof(m_sequenceEntryName) - 1);
		}
		strncpy(m_sequenceEntryName, pkvd->szValue, sizeof(m_sequenceEntryName) - 1);
		m_sequenceEntryName[sizeof(m_sequenceEntryName) - 1] = 0;
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue(pkvd);
	}
}

void CTriggerSequence::StopSequence()
{
	m_nextCommand = NULL;
	m_activator = NULL;
	m_caller = NULL;
	m_sequenceStartTime = 0;
	m_useType = USE_OFF;
	m_value = 0;
	m_isBusy = FALSE;
	if (pev)
	{
		pev->nextthink = 0;
	}
}

void CTriggerSequence::GoToFirstCommand()
{
	m_sequenceStartTime = gpGlobals->time;
	m_nextCommand = m_sequenceEntry->firstCommand;
}

void CTriggerSequence::FastForward()
{
	m_nextCommand = m_sequenceEntry->firstCommand;
	float commandTime = m_sequenceStartTime;
	while (gpGlobals->time > commandTime + 0.0001f && m_nextCommand)
	{
		commandTime += m_nextCommand->delay;
		m_nextCommand = m_nextCommand->nextCommandLine;
	}
}

void CTriggerSequence::ExecuteFireTargets(char *targetNames)
{
	if (targetNames)
	{
		FireTargets(targetNames, m_activator, this, m_useType, m_value);
	}
}

void CTriggerSequence::ExecuteKillTargets(char *targetNames)
{
	if (!targetNames)
	{
		return;
	}
	edict_t *pTarget = NULL;
	while (!FNullEnt(pTarget = FIND_ENTITY_BY_TARGETNAME(pTarget, targetNames)))
	{
		CBaseEntity *pEntity = (CBaseEntity *)pTarget->pvPrivateData;
		pEntity->PreKillTarget();
		UTIL_Remove(pEntity);
	}
}

void CTriggerSequence::ExecuteTextMessage(client_textmessage_t *message, int textChannel)
{
	if (!message || !message->pMessage)
	{
		return;
	}
	hudtextparms_t textParms;
	textParms.effect = message->effect;
	textParms.x = message->x;
	textParms.y = message->y;
	textParms.r1 = message->r1;
	textParms.g1 = message->g1;
	textParms.b1 = message->b1;
	textParms.a1 = message->a1;
	textParms.r2 = message->r2;
	textParms.g2 = message->g2;
	textParms.b2 = message->b2;
	textParms.a2 = message->a2;
	textParms.fadeinTime = message->fadein;
	textParms.fadeoutTime = message->fadeout;
	textParms.holdTime = message->holdtime;
	textParms.fxTime = message->fxtime;
	textParms.channel = textChannel;
	if (message->holdtime == -1)
	{
		textParms.holdTime = strlen(message->pMessage) * 0.05f + 1;
	}
	UTIL_HudMessageAll(textParms, message->pMessage);
}

void CTriggerSequence::ExecuteSound(char *soundName)
{
	if (soundName)
	{
		SENTENCEG_PlayRndSz(edict(), soundName, 1, ATTN_NONE, 0, PITCH_NORM);
	}
}

void CTriggerSequence::ExecuteSpeech(char *soundName, char *speakerName, char *listenerName, float duration)
{
	if (!soundName || !speakerName)
	{
		return;
	}
	CBaseEntity *pSpeaker = UTIL_FindEntityByTargetname(NULL, speakerName);
	if (pSpeaker)
	{
		CBaseMonster *pMonster = pSpeaker->MyMonsterPointer();
		if (pMonster && pev)
		{
			CBaseEntity *pListener = UTIL_FindEntityGeneric(listenerName, pev->origin, 4096);
			pMonster->PlayScriptedSentence(soundName, duration, 1, ATTN_NORM, TRUE, pListener);
		}
	}
}

void CTriggerSequence::ExecuteSoundOrSpeech(char *soundName, char *speakerName, char *listenerName, float duration)
{
	if (!soundName)
	{
		return;
	}
	if (speakerName && !FStrEq(speakerName, "none"))
	{
		ExecuteSpeech(soundName, speakerName, listenerName, duration);
	}
	else
	{
		ExecuteSound(soundName);
	}
}

void CTriggerSequence::AdvanceAndCheckForRepeat(int repeatCount)
{
	if (repeatCount)
	{
		GoToFirstCommand();
	}
	else
	{
		m_nextCommand = m_nextCommand->nextCommandLine;
	}
}

void CTriggerSequence::ExecuteSequenceCommand(sequenceCommandLine_s *command)
{
	if (command)
	{
		ExecuteFireTargets(command->fireTargetNames);
		ExecuteKillTargets(command->killTargetNames);
		ExecuteTextMessage(&command->clientMessage, command->textChannel);
		ExecuteSoundOrSpeech(command->soundFileName, command->speakerName, command->listenerName, command->delay);
		AdvanceAndCheckForRepeat(command->repeatCount);
	}
}

void CTriggerSequence::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!m_sequenceEntry)
	{
		StopSequence();
		UTIL_LogPrintf("CTriggerSequence: unknown sequence \"%s\"; must be specified \n", m_sequenceEntryName);
		return;
	}
	if (m_isDisabled)
	{
		UTIL_LogPrintf("CTriggerSequence: sequence \"%s\" is disabled, not firing.\n", m_sequenceEntryName);
		return;
	}
	if (pev->spawnflags & 1)
	{
		m_isDisabled = TRUE;
	}
	StopSequence();
	if (m_sequenceEntry->firstCommand)
	{
		m_activator = pActivator;
		m_caller = pCaller;
		m_useType = useType;
		m_value = value;
		GoToFirstCommand();
		m_isBusy = TRUE;
		Think();
	}
	else
	{
		UTIL_LogPrintf("CTriggerSequence: sequence \"%s\" (in file \"%s\") was empty!\n",
			m_sequenceEntryName, m_sequenceEntry->fileName);
	}
}

void CTriggerSequence::Think()
{
	float currentTime = gpGlobals->time;
	float nextTime = currentTime;
	if (m_fastForward)
	{
		FastForward();
		m_fastForward = FALSE;
	}
	while (m_nextCommand && nextTime <= currentTime)
	{
		sequenceCommandLine_s *command = m_nextCommand;
		nextTime += command->delay;
		ExecuteSequenceCommand(command);
	}
	if (m_nextCommand && pev)
	{
		pev->nextthink = nextTime;
	}
	else
	{
		StopSequence();
	}
}
