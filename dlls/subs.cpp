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
/*

===== subs.cpp ========================================================

  frequently used global functions

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "nodes.h"
#include "doors.h"

extern CGraph WorldGraph;

extern BOOL FEntIsVisible( entvars_t *pev, entvars_t *pevTarget );

extern DLL_GLOBAL int g_iSkillLevel;

// Landmark class
void CPointEntity::Spawn( void )
{
	pev->solid = SOLID_NOT;
	//UTIL_SetSize( pev, g_vecZero, g_vecZero );
}

class CNullEntity : public CBaseEntity
{
public:
	void Spawn( void );
};

// Null Entity, remove on startup
void CNullEntity::Spawn( void )
{
	REMOVE_ENTITY( ENT( pev ) );
}

LINK_ENTITY_TO_CLASS( info_null, CNullEntity )

class CBaseDMStart : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	BOOL IsTriggered( CBaseEntity *pEntity );

private:
};

// These are the new entry points to entities. 
LINK_ENTITY_TO_CLASS( info_player_deathmatch, CBaseDMStart )
LINK_ENTITY_TO_CLASS( info_player_start, CPointEntity )
LINK_ENTITY_TO_CLASS( info_landmark, CPointEntity )

void CBaseDMStart::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

BOOL CBaseDMStart::IsTriggered( CBaseEntity *pEntity )
{
	BOOL master = UTIL_IsMasterTriggered( pev->netname, pEntity );

	return master;
}

// This updates global tables that need to know about entities being removed
void CBaseEntity::UpdateOnRemove( void )
{
	int i;

	if( FBitSet( pev->flags, FL_GRAPHED ) )
	{
		// this entity was a LinkEnt in the world node graph, so we must remove it from
		// the graph since we are removing it from the world.
		for( i = 0; i < WorldGraph.m_cLinks; i++ )
		{
			if( WorldGraph.m_pLinkPool[i].m_pLinkEnt == pev )
			{
				// if this link has a link ent which is the same ent that is removing itself, remove it!
				WorldGraph.m_pLinkPool[i].m_pLinkEnt = NULL;
			}
		}
	}

	if( pev->globalname )
		gGlobalState.EntitySetState( pev->globalname, GLOBAL_DEAD );

	// tell owner ( if any ) that we're dead.This is mostly for MonsterMaker functionality.
	//Killtarget didn't do this before, so the counter broke. - Solokiller
	if( CBaseEntity* pOwner = pev->owner ? Instance( pev->owner ) : 0 )
	{
		pOwner->DeathNotice( pev );
	}
}

// Convenient way to delay removing oneself
void CBaseEntity::SUB_Remove( void )
{
	UpdateOnRemove();
	if( pev->health > 0 )
	{
		// this situation can screw up monsters who can't tell their entity pointers are invalid.
		pev->health = 0;
		ALERT( at_aiconsole, "SUB_Remove called on entity with health > 0\n" );
	}

	REMOVE_ENTITY( ENT( pev ) );
}

// Convenient way to explicitly do nothing (passed to functions that require a method)
void CBaseEntity::SUB_DoNothing( void )
{
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseDelay::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDelay, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseDelay, m_iszKillTarget, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CBaseDelay, CBaseEntity )

void CBaseDelay::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		m_flDelay = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "killtarget" ) )
	{
		m_iszKillTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseEntity::KeyValue( pkvd );
	}
}

/*
==============================
SUB_UseTargets

If self.delay is set, a DelayedUse entity will be created that will actually
do the SUB_UseTargets after that many seconds have passed.

Removes all entities with a targetname that match self.killtarget,
and removes them, so some events can remove other triggers.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function (if they have one)

==============================
*/
void CBaseEntity::SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	//
	// fire targets
	//
	if( !FStringNull( pev->target ) )
	{
		FireTargets( STRING( pev->target ), pActivator, this, useType, value );
	}
}

void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	edict_t *pentTarget = NULL;
	if( !targetName )
		return;

	ALERT( at_aiconsole, "Firing: (%s)\n", targetName );

	for( ; ; )
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, targetName );
		if( FNullEnt( pentTarget ) )
			break;

		CBaseEntity *pTarget = CBaseEntity::Instance( pentTarget );
		if( pTarget && !( pTarget->pev->flags & FL_KILLME ) )	// Don't use dying ents
		{
			ALERT( at_aiconsole, "Found: %s, firing (%s)\n", STRING( pTarget->pev->classname ), targetName );
			pTarget->Use( pActivator, pCaller, useType, value );
		}
	}
}

LINK_ENTITY_TO_CLASS( DelayedUse, CBaseDelay )

void CBaseDelay::SUB_UseTargets( CBaseEntity *pActivator, USE_TYPE useType, float value )
{
	//
	// exit immediatly if we don't have a target or kill target
	//
	if( FStringNull( pev->target ) && !m_iszKillTarget )
		return;

	//
	// check for a delay
	//
	if( m_flDelay != 0 )
	{
		// create a temp object to fire at a later time
		CBaseDelay *pTemp = GetClassPtr( (CBaseDelay *)NULL );
		pTemp->pev->classname = MAKE_STRING( "DelayedUse" );

		pTemp->pev->nextthink = gpGlobals->time + m_flDelay;

		pTemp->SetThink( &CBaseDelay::DelayThink );

		// Save the useType
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->pev->target = pev->target;

		// HACKHACK
		// This wasn't in the release build of Half-Life.  We should have moved m_hActivator into this class
		// but changing member variable hierarchy would break save/restore without some ugly code.
		// This code is not as ugly as that code
		if( pActivator && pActivator->IsPlayer() )		// If a player activates, then save it
		{
			pTemp->pev->owner = pActivator->edict();
		}
		else
		{
			pTemp->pev->owner = NULL;
		}

		return;
	}

	//
	// kill the killtargets
	//
	if( m_iszKillTarget )
	{
		edict_t *pentKillTarget = NULL;

		ALERT( at_aiconsole, "KillTarget: %s\n", STRING( m_iszKillTarget ) );
		pentKillTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszKillTarget ) );
		while( !FNullEnt(pentKillTarget) )
		{
			UTIL_Remove( CBaseEntity::Instance( pentKillTarget ) );

			ALERT( at_aiconsole, "killing %s\n", STRING( pentKillTarget->v.classname ) );
			pentKillTarget = FIND_ENTITY_BY_TARGETNAME( pentKillTarget, STRING( m_iszKillTarget ) );
		}
	}

	//
	// fire targets
	//
	if( !FStringNull( pev->target ) )
	{
		FireTargets( STRING( pev->target ), pActivator, this, useType, value );
	}
}

/*
void CBaseDelay::SUB_UseTargetsEntMethod( void )
{
	SUB_UseTargets( pev );
}
*/

/*
QuakeEd only writes a single float for angles (bad idea), so up and down are
just constant angles.
*/
void SetMovedir( entvars_t *pev )
{
	if( pev->angles == Vector( 0, -1, 0 ) )
	{
		pev->movedir = Vector( 0, 0, 1 );
	}
	else if (pev->angles == Vector( 0, -2, 0 ) )
	{
		pev->movedir = Vector( 0, 0, -1 );
	}
	else
	{
		UTIL_MakeVectors( pev->angles );
		pev->movedir = gpGlobals->v_forward;
	}

	pev->angles = g_vecZero;
}

void CBaseDelay::DelayThink( void )
{
	CBaseEntity *pActivator = NULL;

	if( pev->owner != NULL )		// A player activated this on delay
	{
		pActivator = CBaseEntity::Instance( pev->owner );	
	}

	// The use type is cached (and stashed) in pev->button
	SUB_UseTargets( pActivator, (USE_TYPE)pev->button, 0 );
	REMOVE_ENTITY( ENT( pev ) );
}

// Global Savedata for Toggle
TYPEDESCRIPTION	CBaseToggle::m_SaveData[] =
{
	DEFINE_FIELD( CBaseToggle, m_toggle_state, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseToggle, m_flActivateFinished, FIELD_TIME ),
	DEFINE_FIELD( CBaseToggle, m_flMoveDistance, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flWait, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flLip, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTWidth, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flTLength, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecPosition2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecAngle1, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_vecAngle2, FIELD_VECTOR ),		// UNDONE: Position could go through transition, but also angle?
	DEFINE_FIELD( CBaseToggle, m_cTriggersLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseToggle, m_flHeight, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalAngle, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_sMaster, FIELD_STRING),
	DEFINE_FIELD( CBaseToggle, m_bitsDamageInflict, FIELD_INTEGER ),	// damage type inflicted
};

IMPLEMENT_SAVERESTORE( CBaseToggle, CBaseAnimating )

void CBaseToggle::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq(pkvd->szKeyName, "lip" ) )
	{
		m_flLip = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_sMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "distance" ) )
	{
		m_flMoveDistance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

/*
=============
LinearMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
===============
*/
void CBaseToggle::LinearMove( Vector vecDest, float flSpeed )
{
	ASSERTSZ( flSpeed != 0, "LinearMove:  no speed is defined!" );
	//ASSERTSZ( m_pfnCallWhenMoveDone != NULL, "LinearMove: no post-move function defined" );

	m_vecFinalDest = vecDest;

	// Already there?
	if( vecDest == pev->origin )
	{
		LinearMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CBaseToggle::LinearMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->velocity = vecDestDelta / flTravelTime;
}

/*
============
After moving, set origin to exact final destination, call "move done" function
============
*/
void CBaseToggle::LinearMoveDone( void )
{
	Vector delta = m_vecFinalDest - pev->origin;
	float error = delta.Length();
	if( error > 0.03125 )
	{
		LinearMove( m_vecFinalDest, 100 );
		return;
	}

	UTIL_SetOrigin( pev, m_vecFinalDest );
	pev->velocity = g_vecZero;
	pev->nextthink = -1;
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

BOOL CBaseToggle::IsLockedByMaster( void )
{
	if( m_sMaster && !UTIL_IsMasterTriggered( m_sMaster, m_hActivator ) )
		return TRUE;
	else
		return FALSE;
}

/*
=============
AngularMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
Just like LinearMove, but rotational.
===============
*/
void CBaseToggle::AngularMove( Vector vecDestAngle, float flSpeed )
{
	ASSERTSZ( flSpeed != 0, "AngularMove:  no speed is defined!" );
	//ASSERTSZ( m_pfnCallWhenMoveDone != NULL, "AngularMove: no post-move function defined" );

	m_vecFinalAngle = vecDestAngle;

	// Already there?
	if( vecDestAngle == pev->angles )
	{
		AngularMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - pev->angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	pev->nextthink = pev->ltime + flTravelTime;
	SetThink( &CBaseToggle::AngularMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	pev->avelocity = vecDestDelta / flTravelTime;
}

/*
============
After rotating, set angle to exact final angle, call "move done" function
============
*/
void CBaseToggle::AngularMoveDone( void )
{
	pev->angles = m_vecFinalAngle;
	pev->avelocity = g_vecZero;
	pev->nextthink = -1;
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

float CBaseToggle::AxisValue( int flags, const Vector &angles )
{
	if( FBitSet( flags, SF_DOOR_ROTATE_Z ) )
		return angles.z;
	if( FBitSet( flags, SF_DOOR_ROTATE_X ) )
		return angles.x;

	return angles.y;
}

void CBaseToggle::AxisDir( entvars_t *pev )
{
	if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_Z ) )
		pev->movedir = Vector( 0, 0, 1 );	// around z-axis
	else if( FBitSet( pev->spawnflags, SF_DOOR_ROTATE_X ) )
		pev->movedir = Vector( 1, 0, 0 );	// around x-axis
	else
		pev->movedir = Vector( 0, 1, 0 );		// around y-axis
}

float CBaseToggle::AxisDelta( int flags, const Vector &angle1, const Vector &angle2 )
{
	if( FBitSet( flags, SF_DOOR_ROTATE_Z ) )
		return angle1.z - angle2.z;

	if( FBitSet( flags, SF_DOOR_ROTATE_X ) )
		return angle1.x - angle2.x;

	return angle1.y - angle2.y;
}

/*
=============
FEntIsVisible

returns TRUE if the passed entity is visible to caller, even if not infront ()
=============
*/
BOOL FEntIsVisible( entvars_t *pev, entvars_t *pevTarget)
{
	Vector vecSpot1 = pev->origin + pev->view_ofs;
	Vector vecSpot2 = pevTarget->origin + pevTarget->view_ofs;
	TraceResult tr;

	UTIL_TraceLine( vecSpot1, vecSpot2, ignore_monsters, ENT( pev ), &tr );

	if( tr.fInOpen && tr.fInWater )
		return FALSE;                   // sight line crossed contents

	if( tr.flFraction == 1 )
		return TRUE;

	return FALSE;
}

void CBaseEntity::tfgoal_timer_tick()
{
}

void CBaseEntity::ReturnItem()
{
}

void CBaseEntity::item_tfgoal_touch( CBaseEntity *pOther )
{
}

void CBaseEntity::tfgoal_touch( CBaseEntity *pOther )
{
}

void CBaseEntity::DelayedResult()
{
}

void CBaseEntity::DoDrop( Vector *p_vecOrigin )
{
}

void CBaseEntity::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "goal_no" ) )
	{
		goal_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "mdl" ) )
	{
		pev->model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "group_no" ) )
	{
		group_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "team_no" ) )
	{
		team_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "playerclass" ) )
	{
		playerclass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "items_allowed" ) )
	{
		items_allowed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "goal_state" ) )
	{
		goal_state = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "items" ) )
	{
		items = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "owned_by" ) )
	{
		owned_by = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "armorclass" ) )
	{
		armorclass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_activation" )
	    || FStrEq( pkvd->szKeyName, "g_a" ) )
	{
		goal_activation = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_effects" )
	    || FStrEq( pkvd->szKeyName, "g_e" ))
	{
		goal_effects = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_result" ) )
	{
		goal_result = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_group" ) )
	{
		goal_group = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "else_goal" ) )
	{
		else_goal = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_goal_is_active" ) )
	{
		if_goal_is_active = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_goal_is_inactive" ) )
	{
		if_goal_is_inactive = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_goal_is_removed" ) )
	{
		if_goal_is_removed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_group_is_active" ) )
	{
		if_group_is_active = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_group_is_inactive" ) )
	{
		if_group_is_inactive = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_group_is_removed" ) )
	{
		if_group_is_removed = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "search_time" ) )
	{
		search_time = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "t_length" ) )
	{
		t_length = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "activate_goal_no" ) )
	{
		activate_goal_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "inactivate_goal_no" ) )
	{
		inactivate_goal_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "remove_goal_no" )
	    || FStrEq( pkvd->szKeyName, "rv_g" ) )
	{
		remove_goal_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "restore_goal_no" )
	    || FStrEq( pkvd->szKeyName, "rs_g" ) )
	{
		restore_goal_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "activate_group_no" ) )
	{
		activate_group_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "inactivate_group_no" ) )
	{
		inactivate_group_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "remove_group_no" )
	    || FStrEq( pkvd->szKeyName, "rv_gr" ) )
	{
		remove_group_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "restore_group_no" )
	    || FStrEq( pkvd->szKeyName, "rs_gr" ) )
	{
		restore_group_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_min" ) )
	{
		UTIL_StringToVector( goal_min, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "goal_max" ) )
	{
		UTIL_StringToVector( goal_max, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "has_item_from_group" )
	    || FStrEq( pkvd->szKeyName, "h_i_g" ) )
	{
		has_item_from_group = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "hasnt_item_from_group" )
	    || FStrEq( pkvd->szKeyName, "hn_i_g" ) )
	{
		hasnt_item_from_group = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "remove_item_group" )
	    || FStrEq( pkvd->szKeyName, "r_i_g" ) )
	{
		remove_item_group = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "return_item_no" ) )
	{
		return_item_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_item_has_moved" ) )
	{
		if_item_has_moved = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "if_item_hasnt_moved" ) )
	{
		if_item_hasnt_moved = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "remove_spawnpoint" ) )
	{
		remove_spawnpoint = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "restore_spawnpoint" ) )
	{
		restore_spawnpoint = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "remove_spawngroup" )
	    || FStrEq( pkvd->szKeyName, "rv_s_h" ) )
	{
		remove_spawngroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "restore_spawngroup" )
	    || FStrEq( pkvd->szKeyName, "rs_s_h" ) )
	{
		restore_spawngroup = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "display_item_status1" ) )
	{
		display_item_status[0] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "display_item_status2" ) )
	{
		display_item_status[1] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "display_item_status3" ) )
	{
		display_item_status[2] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "display_item_status4" ) )
	{
		display_item_status[3] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_str_home" )
	    || FStrEq( pkvd->szKeyName, "t_s_g" ) )
	{
		team_str_home = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_str_moved" )
	    || FStrEq( pkvd->szKeyName, "t_s_m" ) )
	{
		team_str_moved = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_str_carried" )
	    || FStrEq( pkvd->szKeyName, "t_s_c" ) )
	{
		team_str_carried = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_team_str_home" )
	    || FStrEq( pkvd->szKeyName, "t_s_h" ) )
	{
		non_team_str_home = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_team_str_moved" ) )
	{
		non_team_str_moved = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_team_str_carried" )
	    || FStrEq( pkvd->szKeyName, "n_s_c" ) )
	{
		non_team_str_carried = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ex_skill_min" ) )
	{
		ex_skill_min = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ex_skill_max" ) )
	{
		ex_skill_max = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team1" ) )
	{
		increase_team[0] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team2" ) )
	{
		increase_team[1] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team3" ) )
	{
		increase_team[2] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team4" ) )
	{
		increase_team[3] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		KeyValuePartTwo( pkvd );
}

void CBaseEntity::KeyValuePartTwo( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		wait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team" ) )
	{
		count = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "axhitme" ) )
	{
		axhitme = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pausetime" )
	    || FStrEq( pkvd->szKeyName, "delay" ) )
	{
		drop_time = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "attack_finished" ) )
	{
		attack_finished = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "axhitme" ) )
	{
		axhitme = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "distance" ) )
	{
		distance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "killtarget" ) )
	{
		killtarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "noise4" ) )
	{
		noise4 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "broadcast" )
	    || FStrEq( pkvd->szKeyName, "b_b" ) )
	{
		broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "b_t" ) )
	{
		axhitme = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "speak" ) )
	{
		speak = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_speak" ) )
	{
		team_speak = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "owners_team_speak" ) )
	{
		owners_team_speak = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_owners_team_speak" ) )
	{
		non_owners_team_speak = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "b_n" ) )
	{
		non_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_team_speak" ) )
	{
		non_team_speak = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "owners_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "b_o" ) )
	{
		owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "non_owners_team_broadcast" ) )
	{
		non_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_broadcast" )
	    || FStrEq( pkvd->szKeyName, "n_b" ) )
	{
		netname_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "n_t" ) )
	{
		netname_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_non_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "n_n" ) )
	{
		netname_non_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_owners_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "n_o" ) )
	{
		netname_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team_drop" )
	    || FStrEq( pkvd->szKeyName, "d_t" ) )
        {
                team_drop = ALLOC_STRING( pkvd->szValue );
                pkvd->fHandled = TRUE;
        }
	else if( FStrEq( pkvd->szKeyName, "non_team_drop" )
	    || FStrEq( pkvd->szKeyName, "d_n" ) )
	{
		non_team_drop = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_team_drop" )
	    || FStrEq( pkvd->szKeyName, "d_n_t" ) )
	{
		netname_team_drop = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "netname_non_team_drop" )
	    || FStrEq( pkvd->szKeyName, "d_n_n" ) )
	{
		netname_non_team_drop = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "all_active" ) )
	{
		all_active = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "delay_time" ) )
	{
		delay_time = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_shells" )
	    || FStrEq( pkvd->szKeyName, "a_s" ) )
	{
		ammo_shells = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_nails" )
	    || FStrEq( pkvd->szKeyName, "a_n" ) )
	{
		ammo_nails = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_rockets" )
	    || FStrEq( pkvd->szKeyName, "a_r" ) )
	{
		ammo_rockets = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_cells" )
	    || FStrEq( pkvd->szKeyName, "a_c" ) )
	{
		ammo_cells = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_medikit" ) )
	{
		ammo_medikit = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ammo_detpack" ) )
	{
		ammo_detpack = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "no_grenades_1" ) )
	{
		no_grenades_1 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "no_grenades_2" ) )
	{
		no_grenades_2 = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_shells" ) )
	{
		maxammo_shells = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_nails" ) )
	{
		maxammo_nails = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_rockets" ) )
	{
		maxammo_rockets = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_cells" ) )
	{
		maxammo_cells = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_medikit" ) )
	{
		maxammo_medikit = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxammo_detpack" ) )
	{
		maxammo_detpack = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team1_allies" ) )
	{
		teamallies[1] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team2_allies" ) )
	{
		teamallies[2] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team3_allies" ) )
	{
		teamallies[3] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team4_allies" ) )
	{
		teamallies[4] = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "invincible_finished" ) )
	{
		invincible_finished = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "invisible_finished" ) )
	{
		invisible_finished = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "super_damage_finished" ) )
	{
		super_damage_finished = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "radsuit_finished" ) )
	{
		radsuit_finished = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "deathtype" ) )
	{
		deathtype = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "speed_reduction" ) )
	{
		speed_reduction = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_time" ) )
	{
		m_flEndRoundTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_team1" ) )
	{
		m_iszEndRoundMsg_Team1 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_team2" ) )
	{
		m_iszEndRoundMsg_Team2 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_team3" ) )
	{
		m_iszEndRoundMsg_Team3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_team4" ) )
	{
		m_iszEndRoundMsg_Team4 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_win_team1" ) )
	{
		m_iszEndRoundMsg_Team1_Win = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_win_team2" ) )
	{
		m_iszEndRoundMsg_Team2_Win = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_win_team3" ) )
	{
		m_iszEndRoundMsg_Team3_Win = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_win_team4" ) )
	{
		m_iszEndRoundMsg_Team4_Win = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_lose_team1" ) )
	{
		m_iszEndRoundMsg_Team1_Lose = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_lose_team2" ) )
	{
		m_iszEndRoundMsg_Team2_Lose = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_lose_team3" ) )
	{
		m_iszEndRoundMsg_Team3_Lose = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_lose_team4" ) )
	{
		m_iszEndRoundMsg_Team4_Lose = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "teamcheck" ) )
	{
		teamcheck = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "owned_by_teamcheck" ) )
	{
		owned_by_teamcheck = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "team1_name" )
	    || FStrEq( pkvd->szKeyName, "team2_name" )
	    || FStrEq( pkvd->szKeyName, "team3_name" )
	    || FStrEq( pkvd->szKeyName, "team4_name" )
	    || FStrEq( pkvd->szKeyName, "number_of_teams" )
	    || FStrEq( pkvd->szKeyName, "last_impulse" ) )
	{
		last_impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		KeyValuePartThree( pkvd );
}

void CBaseEntity::KeyValuePartThree( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "endround_owned_by" ) )
	{
		m_iszEndRoundMsg_OwnedBy = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "endround_non_owned_by" ) )
	{
		m_iszEndRoundMsg_NonOwnedBy = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_broadcast" )
	    || FStrEq( pkvd->szKeyName, "org_b_b" ) )
	{
		org_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "org_b_t" ) )
	{
		org_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_message" ) )
	{
		org_message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_owners_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "org_b_o" ) )
	{
		org_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_non_owners_team_broadcast" ) )
	{
		org_non_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_non_team_broadcast" )
	    || FStrEq( pkvd->szKeyName, "org_b_n" ) )
	{
		org_non_team_broadcast = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_noise3" ) )
	{
		org_noise3 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_noise4" ) )
	{
		org_noise4 = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_team_drop" )
	    || FStrEq( pkvd->szKeyName, "org_d_t" ) )
	{
		org_team_drop = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "org_non_team_drop" )
	    || FStrEq( pkvd->szKeyName, "org_d_n" ) )
	{
		org_non_team_drop = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "increase_team_owned_by" ) )
	{
		increase_team_owned_by = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "replacement_model" ) )
	{
		replacement_model = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "replacement_model_body" ) )
	{
		replacement_model_body = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "replacement_model_skin" ) )
	{
		replacement_model_skin = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "replacement_model_flags" ) )
	{
		replacement_model_flags = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

BOOL CBaseEntity::CheckExistence()
{
	return TRUE;
}

void CBaseEntity::DoRespawn()
{
}

void CBaseEntity::tfgoalitem_dropthink()
{
}

void CBaseEntity::DoDrop( Vector vecOrigin )
{
}

void CBaseEntity::tfgoalitem_remove()
{
}

void CBaseEntity::EndRoundEnd()
{
}

void CBaseEntity::tfgoalitem_droptouch()
{
}

BOOL CBaseEntity::IsTeammate( CBaseEntity *pOther )
{
	return TRUE;
}

BOOL CBaseEntity::IsAlly( int iTeamNo )
{
	return TRUE;
}

BOOL CBaseEntity::IsAlly( CBaseEntity *pOther )
{
	return TRUE;
}

CBaseEntity *CBaseEntity::FindTeamSpawnPoint()
{
	return 0;
}

void CBaseEntity::Timer_AutokickThink()
{
}

void CBaseEntity::Timer_CeaseFireThink()
{
}

void CBaseEntity::Timer_SpyUndercoverThink()
{
}

void CBaseEntity::Timer_FinishedBuilding()
{
}

void CBaseEntity::Timer_CheckBuildDistance()
{
}

void CBaseEntity::Timer_DetpackDisarm()
{
}

void CBaseEntity::Timer_DetpackSet()
{
}

void CBaseEntity::Timer_Birthday()
{
}

void CBaseEntity::Timer_Regeneration()
{
}

void CBaseEntity::Timer_Tranquilisation()
{
}

void CBaseEntity::Timer_Hallucination()
{
}

void CBaseEntity::Timer_HealthRot()
{
}

CBaseEntity *CBaseEntity::CreateTimer( int iTimerType )
{
	return 0;
}

CBaseEntity *CBaseEntity::FindTimer( int iTimerType )
{
	return 0;
}

void CBaseEntity::Timer_Infection()
{
}

void CBaseEntity::CheckBelowBuilding( int iDist )
{
}

int CBaseEntity::CheckArea( CBaseEntity *pIgnore )
{
	return 0;
}

void CBaseEntity::TeamFortress_CalcEMPDmgRad( float dmg, float rad )
{
}

void CBaseEntity::TeamFortress_EMPExplode( entvars_t *pevGren, float damage, float radius )
{
}
