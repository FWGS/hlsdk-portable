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
#include "movewith.h"
#include "player.h"

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
LINK_ENTITY_TO_CLASS(info_null,CNullEntity);
LINK_ENTITY_TO_CLASS(info_texlights,CNullEntity); // don't complain about Merl's new info entities
LINK_ENTITY_TO_CLASS(info_compile_parameters,CNullEntity);

class CBaseDMStart : public CPointEntity
{
public:
	void KeyValue( KeyValueData *pkvd );
	STATE		GetState( CBaseEntity *pEntity );

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

STATE CBaseDMStart::GetState( CBaseEntity *pEntity )
{
	if (UTIL_IsMasterTriggered( pev->netname, pEntity ))
		return STATE_ON;
	else
		return STATE_OFF;
}

// This updates global tables that need to know about entities being removed
void CBaseEntity::UpdateOnRemove( void )
{
	int i;
	CBaseEntity* pTemp;

	if (!g_pWorld)
	{
		ALERT(at_console, "UpdateOnRemove has no AssistList!\n");
		return;
	}

	//LRC - remove this from the AssistList.
	for (pTemp = g_pWorld; pTemp->m_pAssistLink != NULL; pTemp = pTemp->m_pAssistLink)
	{
		if (this == pTemp->m_pAssistLink)
		{
//			ALERT(at_console,"REMOVE: %s removed from the Assist List.\n", STRING(pev->classname));
			pTemp->m_pAssistLink = this->m_pAssistLink;
			this->m_pAssistLink = NULL;
			break;
		}
	}

	//LRC
	if (m_pMoveWith)
	{
		// if I'm moving with another entity, take me out of the list. (otherwise things crash!)
		pTemp = m_pMoveWith->m_pChildMoveWith;
		if (pTemp == this)
		{
			m_pMoveWith->m_pChildMoveWith = this->m_pSiblingMoveWith;
		}
		else
		{
			while (pTemp->m_pSiblingMoveWith)
			{
				if (pTemp->m_pSiblingMoveWith == this)
				{
					pTemp->m_pSiblingMoveWith = this->m_pSiblingMoveWith;
					break;
				}
				pTemp = pTemp->m_pSiblingMoveWith;
			}

		}
//		ALERT(at_console,"REMOVE: %s removed from the %s ChildMoveWith list.\n", STRING(pev->classname), STRING(m_pMoveWith->pev->targetname));
	}

	//LRC - do the same thing if another entity is moving with _me_.
	if (m_pChildMoveWith)
	{
		CBaseEntity* pCur = m_pChildMoveWith;
		CBaseEntity* pNext;
		while (pCur != NULL)
		{
			pNext = pCur->m_pSiblingMoveWith;
			// bring children to a stop
			UTIL_SetMoveWithVelocity(pCur, g_vecZero, 100);
			UTIL_SetMoveWithAvelocity(pCur, g_vecZero, 100);
			pCur->m_pMoveWith = NULL;
			pCur->m_pSiblingMoveWith = NULL;
			pCur = pNext;
		}
	}

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
//	if (pev->ltime)
//		ALERT(at_console, "Doing Nothing %f\n", pev->ltime);
//	else
//		ALERT(at_console, "Doing Nothing %f\n", gpGlobals->time);
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseDelay::m_SaveData[] =
{
	DEFINE_FIELD( CBaseDelay, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseDelay, m_iszKillTarget, FIELD_STRING ),
	DEFINE_FIELD( CBaseDelay, m_hActivator, FIELD_EHANDLE ), //LRC
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
	const char *inputTargetName = targetName;
	CBaseEntity *inputActivator = pActivator;
	CBaseEntity *pTarget = NULL;
	int i,j, found = false;
	char szBuf[80];

	if( !targetName )
		return;
	if (useType == USE_NOT)
		return;

	//LRC - allow changing of usetype
	if (targetName[0] == '+')
	{
		targetName++;
		useType = USE_ON;
	}
	else if (targetName[0] == '-')
	{
		targetName++;
		useType = USE_OFF;
	}

	ALERT( at_aiconsole, "Firing: (%s)\n", targetName );

	pTarget = UTIL_FindEntityByTargetname(pTarget, targetName, pActivator);
	if( !pTarget )
	{
		// it's not an entity name; check for a locus specifier, e.g: "fadein(mywall)"
		for (i = 0; targetName[i]; i++)
		{
			if (targetName[i] == '(')
			{
				i++;
				for (j = i; targetName[j]; j++)
				{
					if (targetName[j] == ')')
					{
						strncpy(szBuf, targetName+i, j-i);
						szBuf[j-i] = 0;
						pActivator = UTIL_FindEntityByTargetname(NULL, szBuf, inputActivator);
						if (!pActivator)
	{
							//ALERT(at_console, "Missing activator \"%s\"\n", szBuf);
							return; // it's a locus specifier, but the locus is invalid.
						}
						//ALERT(at_console, "Found activator \"%s\"\n", STRING(pActivator->pev->targetname));
						found = true;
			break;
					}
				}
				if (!found) ALERT(at_error, "Missing ')' in target value \"%s\"", inputTargetName);
			break;
			}
		}
		if (!found) return; // no, it's not a locus specifier.

		strncpy(szBuf, targetName, i-1);
		szBuf[i-1] = 0;
		targetName = szBuf;
		pTarget = UTIL_FindEntityByTargetname(NULL, targetName, inputActivator);

		if (!pTarget) return; // it's a locus specifier all right, but the target's invalid.
	}

	do // start firing targets
	{
		if ( !(pTarget->pev->flags & FL_KILLME) )	// Don't use dying ents
		{
			if (useType == USE_KILL)
			{
				ALERT( at_aiconsole, "Use_kill on %s\n", STRING( pTarget->pev->classname ) );
				UTIL_Remove( pTarget );
			}
			else
		{
			ALERT( at_aiconsole, "Found: %s, firing (%s)\n", STRING( pTarget->pev->classname ), targetName );
			pTarget->Use( pActivator, pCaller, useType, value );
		}
	}
		pTarget = UTIL_FindEntityByTargetname(pTarget, targetName, inputActivator);
	} while (pTarget);

	//LRC- Firing has finished, aliases can now reflect their new values.
	UTIL_FlushAliases();
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

		pTemp->SetNextThink( m_flDelay );

		pTemp->SetThink( &CBaseDelay::DelayThink );

		// Save the useType
		pTemp->pev->button = (int)useType;
		pTemp->m_iszKillTarget = m_iszKillTarget;
		pTemp->m_flDelay = 0; // prevent "recursion"
		pTemp->pev->target = pev->target;

		//LRC - Valve had a hacked thing here to avoid breaking
		// save/restore. In Spirit that's not a problem.
		// I've moved m_hActivator into this class, for the "elegant" fix.
		pTemp->m_hActivator = pActivator;

		return;
	}

	//
	// kill the killtargets
	//
	if( m_iszKillTarget )
	{
		edict_t *pentKillTarget = NULL;

		ALERT( at_aiconsole, "KillTarget: %s\n", STRING( m_iszKillTarget ) );
		//LRC- now just USE_KILLs its killtarget, for consistency.
		FireTargets( STRING(m_iszKillTarget), pActivator, this, USE_KILL, 0);
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
	pev->movedir = GetMovedir(pev->angles);
	pev->angles = g_vecZero;
	}

Vector GetMovedir( Vector vecAngles )
	{
	if (vecAngles == Vector(0, -1, 0))
	{
		return Vector(0, 0, 1);
	}
	else if (vecAngles == Vector(0, -2, 0))
	{
		return Vector(0, 0, -1);
	}
	else
	{
		UTIL_MakeVectors(vecAngles);
		return gpGlobals->v_forward;
	}
	}



void CBaseDelay::DelayThink( void )
{
	CBaseEntity *pActivator = NULL;

	// The use type is cached (and stashed) in pev->button
	//LRC - now using m_hActivator.
	SUB_UseTargets( m_hActivator, (USE_TYPE)pev->button, 0 );
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
//	DEFINE_FIELD( CBaseToggle, m_hActivator, FIELD_EHANDLE ),
	DEFINE_FIELD( CBaseToggle, m_pfnCallWhenMoveDone, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseToggle, m_vecFinalDest, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseToggle, m_flLinearMoveSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseToggle, m_flAngularMoveSpeed, FIELD_FLOAT ), //LRC
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
//void CBaseToggle ::  LinearMove( Vector	vecInput, float flSpeed)
//{
//	LinearMove(vecInput, flSpeed);
//}


=============
LinearMove

calculate pev->velocity and pev->nextthink to reach vecDest from
pev->origin traveling at flSpeed
===============
*/
void CBaseToggle ::  LinearMove( Vector	vecInput, float flSpeed )//, BOOL bNow )
{
	ASSERTSZ( flSpeed != 0, "LinearMove:  no speed is defined!" );
	//ASSERTSZ( m_pfnCallWhenMoveDone != NULL, "LinearMove: no post-move function defined" );

	m_flLinearMoveSpeed = flSpeed;
	m_vecFinalDest = vecInput;

//	if ((m_pMoveWith || m_pChildMoveWith))// && !bNow)
//	{
//		ALERT(at_console,"Setting LinearMoveNow to happen after %f\n",gpGlobals->time);
		SetThink(&CBaseToggle :: LinearMoveNow );
		UTIL_DesiredThink( this );
		//pev->nextthink = pev->ltime + 0.01;
//	}
//	else
//	{
//		LinearMoveNow(); // starring Martin Sheen and Marlon Brando
//	}
}

void CBaseToggle :: LinearMoveNow( void )
{
//	ALERT(at_console, "LMNow %s\n", STRING(pev->targetname));

	Vector vecDest;
//	if (m_pMoveWith || m_pChildMoveWith )
//		ALERT(at_console,"THINK: LinearMoveNow happens at %f, speed %f\n",gpGlobals->time, m_flLinearMoveSpeed);

	if (m_pMoveWith)
	{
	    vecDest = m_vecFinalDest + m_pMoveWith->pev->origin;
	}
	else
	    vecDest = m_vecFinalDest;

//	ALERT(at_console,"LinearMoveNow: Destination is (%f %f %f), finalDest was (%f %f %f)\n",
//		vecDest.x,vecDest.y,vecDest.z,
//		m_vecFinalDest.x,m_vecFinalDest.y,m_vecFinalDest.z
//	);

	// Already there?
	if( vecDest == pev->origin )
	{
		LinearMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDest - pev->origin;

	// divide vector length by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / m_flLinearMoveSpeed;

	// set nextthink to trigger a call to LinearMoveDone when dest is reached
	SetNextThink( flTravelTime, TRUE );
	SetThink( &CBaseToggle::LinearMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
//	pev->velocity = vecDestDelta / flTravelTime;
	UTIL_SetVelocity( this, vecDestDelta / flTravelTime );

//	ALERT(at_console, "LMNow \"%s\": Vel %f %f %f, think %f\n", STRING(pev->targetname), pev->velocity.x, pev->velocity.y, pev->velocity.z, pev->nextthink);
}

/*
============
After moving, set origin to exact final destination, call "move done" function
============
*/
/*void CBaseToggle :: LinearMoveDone( void )
{
	Vector vecDiff;
	if (m_pMoveWith)
		vecDiff = (m_vecFinalDest + m_pMoveWith->pev->origin) - pev->origin;
	else
		vecDiff = m_vecFinalDest - pev->origin;
	if (vecDiff.Length() > 0.05) //pev->velocity.Length())
	{
		// HACK: not there yet, try waiting one more frame.
		ALERT(at_console,"Rejecting difference %f\n",vecDiff.Length());
		SetThink(&CBaseToggle ::LinearMoveFinalDone);
		pev->nextthink = gpGlobals->time + 0.01;
	}
	else
	{
		LinearMoveFinalDone();
	}
}*/

void CBaseToggle::LinearMoveDone( void )
{
	SetThink(&CBaseToggle ::LinearMoveDoneNow);
//	ALERT(at_console, "LMD: desiredThink %s\n", STRING(pev->targetname));
	UTIL_DesiredThink( this );
}

void CBaseToggle :: LinearMoveDoneNow( void )
{
	Vector vecDest;

//	ALERT(at_console, "LMDone %s\n", STRING(pev->targetname));

	UTIL_SetVelocity(this, g_vecZero);//, TRUE);
//	pev->velocity = g_vecZero;
	if (m_pMoveWith)
	{
		vecDest = m_vecFinalDest + m_pMoveWith->pev->origin;
//		ALERT(at_console, "LMDone %s: p.origin = %f %f %f, origin = %f %f %f. Set it to %f %f %f\n", STRING(pev->targetname), m_pMoveWith->pev->origin.x,  m_pMoveWith->pev->origin.y,  m_pMoveWith->pev->origin.z, pev->origin.x, pev->origin.y, pev->origin.z, vecDest.x, vecDest.y, vecDest.z);
	}
	else
	{
		vecDest = m_vecFinalDest;
//		ALERT(at_console, "LMDone %s: origin = %f %f %f. Set it to %f %f %f\n", STRING(pev->targetname), pev->origin.x, pev->origin.y, pev->origin.z, vecDest.x, vecDest.y, vecDest.z);
	}
	UTIL_AssignOrigin(this, vecDest);
	DontThink(); //LRC
	//pev->nextthink = -1;
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

BOOL CBaseToggle::IsLockedByMaster( void )
{
	if (UTIL_IsMasterTriggered(m_sMaster, m_hActivator))
		return FALSE;
	else
		return TRUE;
}

//LRC- mapping toggle-states to global states
STATE CBaseToggle :: GetState ( void )
{
	switch (m_toggle_state)
	{
		case TS_AT_TOP:		return STATE_ON;
		case TS_AT_BOTTOM:	return STATE_OFF;
		case TS_GOING_UP:	return STATE_TURN_ON;
		case TS_GOING_DOWN:	return STATE_TURN_OFF;
		default:			return STATE_OFF; // This should never happen.
	}
};

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
	m_flAngularMoveSpeed = flSpeed;

//	if ((m_pMoveWith || m_pChildMoveWith))// && !bNow)
//	{
//		ALERT(at_console,"Setting AngularMoveNow to happen after %f\n",gpGlobals->time);
	SetThink(&CBaseToggle :: AngularMoveNow );
	UTIL_DesiredThink( this );
//	ExternalThink( 0.01 );
//		pev->nextthink = pev->ltime + 0.01;
//	}
//	else
//	{
//		AngularMoveNow(); // starring Martin Sheen and Marlon Brando
//	}
}

void CBaseToggle :: AngularMoveNow()
{
//	ALERT(at_console, "AngularMoveNow %f\n", pev->ltime);
	Vector vecDestAngle;

	if (m_pMoveWith)
	    vecDestAngle = m_vecFinalAngle + m_pMoveWith->pev->angles;
	else
	    vecDestAngle = m_vecFinalAngle;

	// Already there?
	if( vecDestAngle == pev->angles )
	{
		AngularMoveDone();
		return;
	}

	// set destdelta to the vector needed to move
	Vector vecDestDelta = vecDestAngle - pev->angles;

	// divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / m_flAngularMoveSpeed;

	// set nextthink to trigger a call to AngularMoveDone when dest is reached
	SetNextThink( flTravelTime, TRUE );
	SetThink( &CBaseToggle::AngularMoveDone );

	// scale the destdelta vector by the time spent traveling to get velocity
	UTIL_SetAvelocity(this, vecDestDelta / flTravelTime );
}

void CBaseToggle :: AngularMoveDone( void )
{
	SetThink(&CBaseToggle ::AngularMoveDoneNow);
//	ALERT(at_console, "LMD: desiredThink %s\n", STRING(pev->targetname));
	UTIL_DesiredThink( this );
}

/*
============
After rotating, set angle to exact final angle, call "move done" function
============
*/
void CBaseToggle :: AngularMoveDoneNow( void )
{
//	ALERT(at_console, "AngularMoveDone %f\n", pev->ltime);
	UTIL_SetAvelocity(this, g_vecZero);
	if (m_pMoveWith)
	{
		UTIL_SetAngles(this, m_vecFinalAngle + m_pMoveWith->pev->angles);
	}
	else
{
		UTIL_SetAngles(this, m_vecFinalAngle);
	}
	DontThink();
	if( m_pfnCallWhenMoveDone )
		( this->*m_pfnCallWhenMoveDone )();
}

// this isn't currently used. Otherwise I'd fix it to use movedir the way it should...
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
	if ( pev->movedir != g_vecZero) //LRC
		return;

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


//=========================================================
// LRC - info_movewith, the first entity I've made which
//       truly doesn't fit ANY preexisting category.
//=========================================================
#define SF_IMW_INACTIVE 1
#define SF_IMW_BLOCKABLE 2

class CInfoMoveWith : public CBaseEntity
{
public:
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	STATE GetState() { return (pev->spawnflags & SF_IMW_INACTIVE)?STATE_OFF:STATE_ON; }
};

LINK_ENTITY_TO_CLASS(info_movewith, CInfoMoveWith);

void CInfoMoveWith :: Spawn( void )
{
	if (pev->spawnflags & SF_IMW_INACTIVE)
		m_MoveWith = pev->netname;
	else
		m_MoveWith = pev->target;

	if (pev->spawnflags & SF_IMW_BLOCKABLE)
	{
		pev->solid = SOLID_SLIDEBOX;
		pev->movetype = MOVETYPE_FLY;
	}
	// and allow InitMoveWith to set things up as usual.
}

void CInfoMoveWith :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pSibling;

	if (!ShouldToggle(useType)) return;
	
	if (m_pMoveWith)
	{
		// remove this from the old parent's list of children
		pSibling = m_pMoveWith->m_pChildMoveWith;
		if (pSibling == this)
			m_pMoveWith->m_pChildMoveWith = this->m_pSiblingMoveWith;
		else
		{
			while (pSibling->m_pSiblingMoveWith && pSibling->m_pSiblingMoveWith != this)
			{ pSibling = pSibling->m_pSiblingMoveWith; }

			if (pSibling->m_pSiblingMoveWith == this)
			{
				pSibling->m_pSiblingMoveWith = this->m_pSiblingMoveWith;
			}
			else
			{
				// failed to find myself in the list, complain
				ALERT(at_error, "info_movewith can't find itself\n");
				return;
			}
		}
		m_pMoveWith = NULL;
		m_pSiblingMoveWith = NULL;
	}

	if (pev->spawnflags & SF_IMW_INACTIVE)
	{
		pev->spawnflags &= ~SF_IMW_INACTIVE;
		m_MoveWith = pev->target;
	}
	else
	{
		pev->spawnflags |= SF_IMW_INACTIVE;
		m_MoveWith = pev->netname;
	}

	// set things up for the new m_MoveWith value
	if (!m_MoveWith)
	{
		UTIL_SetVelocity(this, g_vecZero); // come to a stop
		return;
	}

	m_pMoveWith = UTIL_FindEntityByTargetname(NULL, STRING(m_MoveWith));
	if (!m_pMoveWith)
	{
		ALERT(at_console,"Missing movewith entity %s\n", STRING(m_MoveWith));
		return;
	}

	pSibling = m_pMoveWith->m_pChildMoveWith;
	while (pSibling) // check that this entity isn't already in the list of children
	{
		if (pSibling == this) return;
		pSibling = pSibling->m_pSiblingMoveWith;
	}
	
	// add this entity to the list of children
	m_pSiblingMoveWith = m_pMoveWith->m_pChildMoveWith; // may be null: that's fine by me.
	m_pMoveWith->m_pChildMoveWith = this;
	m_vecMoveWithOffset = pev->origin - m_pMoveWith->pev->origin;
	UTIL_SetVelocity(this, g_vecZero); // match speed with the new entity
}
