/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
/*


===== scripted.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"

#ifndef ANIMATION_H
#include "animation.h"
#endif

#ifndef SAVERESTORE_H
#include "saverestore.h"
#endif

#include "schedule.h"
#include "scripted.h"
#include "defaultai.h"
#include "movewith.h"



/*
classname "scripted_sequence"
targetname "me" - there can be more than one with the same name, and they act in concert
target "the_entity_I_want_to_start_playing" or "class entity_classname" will pick the closest inactive scientist
play "name_of_sequence"
idle "name of idle sequence to play before starting"
donetrigger "whatever" - can be any other triggerable entity such as another sequence, train, door, or a special case like "die" or "remove"
moveto - if set the monster first moves to this nodes position
range # - only search this far to find the target
spawnflags - (stop if blocked, stop if player seen)
*/


//
// Cache user-entity-field values until spawn is called.
//

void CCineMonster :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iszIdle"))
	{
		m_iszIdle = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszPlay"))
	{
		m_iszPlay = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEntity"))
	{
		m_iszEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszAttack"))
	{
		m_iszAttack = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszMoveTarget"))
	{
		m_iszMoveTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszFireOnBegin"))
	{
		m_iszFireOnBegin = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fMoveTo"))
	{
		m_fMoveTo = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fTurnType"))
	{
		m_fTurnType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fAction"))
	{
		m_fAction = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
// LRC	else if (FStrEq(pkvd->szKeyName, "m_flRepeat"))
//	{
//		m_flRepeat = atof( pkvd->szValue );
//		pkvd->fHandled = TRUE;
//	}
	else if (FStrEq(pkvd->szKeyName, "m_flRadius"))
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iRepeats"))
	{
		m_iRepeats = atoi( pkvd->szValue );
		m_iRepeatsLeft = m_iRepeats;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_fRepeatFrame"))
	{
		m_fRepeatFrame = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iFinishSchedule"))
	{
		m_iFinishSchedule = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iPriority"))
	{
		m_iPriority = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

TYPEDESCRIPTION	CCineMonster::m_SaveData[] = 
{
	DEFINE_FIELD( CCineMonster, m_iState, FIELD_INTEGER ), //LRC
	DEFINE_FIELD( CCineMonster, m_iszIdle, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszPlay, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_iszAttack, FIELD_STRING ), //LRC
	DEFINE_FIELD( CCineMonster, m_iszMoveTarget, FIELD_STRING ), //LRC
	DEFINE_FIELD( CCineMonster, m_iszFireOnBegin, FIELD_STRING ),
	DEFINE_FIELD( CCineMonster, m_fMoveTo, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_fTurnType, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_fAction, FIELD_INTEGER ),
//LRC- this is unused	DEFINE_FIELD( CCineMonster, m_flRepeat, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_flRadius, FIELD_FLOAT ),

	DEFINE_FIELD( CCineMonster, m_iDelay, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_startTime, FIELD_TIME ),

	DEFINE_FIELD( CCineMonster,	m_saved_movetype, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster,	m_saved_solid, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_saved_effects, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_iFinishSchedule, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_interruptable, FIELD_BOOLEAN ),

	//LRC
	DEFINE_FIELD( CCineMonster, m_iRepeats, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_iRepeatsLeft, FIELD_INTEGER ),
	DEFINE_FIELD( CCineMonster, m_fRepeatFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CCineMonster, m_iPriority, FIELD_INTEGER ),
};


IMPLEMENT_SAVERESTORE( CCineMonster, CBaseMonster );

LINK_ENTITY_TO_CLASS( scripted_sequence, CCineMonster );
LINK_ENTITY_TO_CLASS( scripted_action, CCineMonster ); //LRC

LINK_ENTITY_TO_CLASS( aiscripted_sequence, CCineMonster ); //LRC - aiscripted sequences don't need to be seperate


void CCineMonster :: Spawn( void )
{
	// pev->solid = SOLID_TRIGGER;
	// UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	pev->solid = SOLID_NOT;

	m_iState = STATE_OFF; //LRC

	if ( FStringNull(m_iszIdle) && FStringNull(pev->targetname) ) // if no targetname, start now
	{
		SetThink(&CCineMonster :: CineThink );
		SetNextThink( 1.0 );
	}
	else if ( m_iszIdle )
	{
		SetThink(&CCineMonster :: InitIdleThink );
		SetNextThink( 1.0 );
	}
	if ( pev->spawnflags & SF_SCRIPT_NOINTERRUPT )
		m_interruptable = FALSE;
	else
		m_interruptable = TRUE;

	//LRC - the only difference between AI and normal sequences
	if ( FClassnameIs(pev, "aiscripted_sequence") || pev->spawnflags & SF_SCRIPT_OVERRIDESTATE )
	{
		m_iPriority |= SS_INTERRUPT_ANYSTATE;
	}
}

//
// CineStart
//
void CCineMonster :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// do I already know who I should use
	CBaseEntity		*pEntity = m_hTargetEnt;
	CBaseMonster	*pTarget = NULL;

	if ( pEntity )
		pTarget = pEntity->MyMonsterPointer();

	if ( pTarget )
	{
//		ALERT(at_console, "Sequence \"%s\" triggered, already has a target.\n", STRING(pev->targetname));
		// am I already playing the script?
		if ( pTarget->m_scriptState == SCRIPT_PLAYING )
			return;

		m_startTime = gpGlobals->time + 0.05; //why the delay? -- LRC
	}
	else
	{
//		ALERT(at_console, "Sequence \"%s\" triggered, can't find target; searching\n", STRING(pev->targetname));
		m_hActivator = pActivator;
		// if not, try finding them
		SetThink(&CCineMonster :: CineThink );
//		SetNextThink( 0 );
		CineThink(); //LRC
	}
}


// This doesn't really make sense since only MOVETYPE_PUSH get 'Blocked' events
void CCineMonster :: Blocked( CBaseEntity *pOther )
{

}

void CCineMonster :: Touch( CBaseEntity *pOther )
{
/*
	ALERT( at_aiconsole, "Cine Touch\n" );
	if (m_pentTarget && OFFSET(pOther->pev) == OFFSET(m_pentTarget))
	{
		CBaseMonster *pTarget = GetClassPtr((CBaseMonster *)VARS(m_pentTarget));
		pTarget->m_monsterState == MONSTERSTATE_SCRIPT;
	}
*/
}


/*
	entvars_t *pevOther = VARS( gpGlobals->other );

	if ( !FBitSet ( pevOther->flags , FL_MONSTER ) ) 
	{// touched by a non-monster.
		return;
	}

	pevOther->origin.z += 1;
	
	if ( FBitSet ( pevOther->flags, FL_ONGROUND ) ) 
	{// clear the onground so physics don't bitch
		pevOther->flags -= FL_ONGROUND;
	}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;


	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
}
*/


//
// ********** Cinematic DIE **********
//
void CCineMonster :: Die( void )
{
	SetThink(&CCineMonster :: SUB_Remove );
}

//
// ********** Cinematic PAIN **********
//
void CCineMonster :: Pain( void )
{

}

//
// ********** Cinematic Think **********
//

//LRC: now redefined... find a viable entity with the given name, and return it (or NULL if not found).
CBaseMonster* CCineMonster :: FindEntity( const char* sName, CBaseEntity *pActivator )
{
	CBaseEntity *pEntity;

	pEntity = UTIL_FindEntityByTargetname(NULL, sName, pActivator);
	//m_hTargetEnt = NULL;
	CBaseMonster	*pMonster = NULL;

	while (pEntity)
	{
		if ( FBitSet( pEntity->pev->flags, FL_MONSTER ))
		{
			pMonster = pEntity->MyMonsterPointer( );
			if ( pMonster && pMonster->CanPlaySequence( m_iPriority | SS_INTERRUPT_ALERT ) )
			{
				return pMonster;
			}
			ALERT( at_debug, "Found %s, but can't play!\n", sName );
		}
		pEntity = UTIL_FindEntityByTargetname(pEntity, sName, pActivator);
		pMonster = NULL;
	}
	
	// couldn't find something with the given targetname; assume it's a classname instead.
	if ( !pMonster )
	{
		pEntity = NULL;
		while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, m_flRadius )) != NULL)
		{
			if (FClassnameIs( pEntity->pev, sName))
			{
				if ( FBitSet( pEntity->pev->flags, FL_MONSTER ))
				{
					pMonster = pEntity->MyMonsterPointer( );
					if ( pMonster && pMonster->CanPlaySequence( m_iPriority ) )
					{
						return pMonster;
					}
				}
			}
		}
	}
	return NULL;
}

// make the entity enter a scripted sequence
void CCineMonster :: PossessEntity( void )
{
	CBaseEntity		*pEntity = m_hTargetEnt;
	CBaseMonster	*pTarget = NULL;
	if ( pEntity )
		pTarget = pEntity->MyMonsterPointer();

//	ALERT( at_console, "Possess: pEntity %s, pTarget %s\n", STRING(pEntity->pev->targetname), STRING(pTarget->pev->targetname));

	if ( pTarget )
	{
		if (pTarget->m_pCine)
		{
			pTarget->m_pCine->CancelScript();
		}

		pTarget->m_pCine = this;
		if (m_iszAttack)
		{
			// anything with that name?
			pTarget->m_hTargetEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszAttack), m_hActivator);
			if ( pTarget->m_hTargetEnt == NULL )
			{	// nothing. Anything with that classname?
				while ((pTarget->m_hTargetEnt = UTIL_FindEntityInSphere( pTarget->m_hTargetEnt, pev->origin, m_flRadius )) != NULL)
				{
					if (FClassnameIs( pTarget->m_hTargetEnt->pev, STRING(m_iszAttack))) break;
				}
			}
			if (pTarget->m_hTargetEnt == NULL)
			{	// nothing. Oh well.
				ALERT(at_debug,"%s %s has a missing \"turn target\": %s\n",STRING(pev->classname),STRING(pev->targetname),STRING(m_iszAttack));
				pTarget->m_hTargetEnt = this;
			}
		}
		else
		{
			pTarget->m_hTargetEnt = this;
		}

		if (m_iszMoveTarget)
		{
			// anything with that name?
			pTarget->m_pGoalEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszMoveTarget), m_hActivator);
			if (pTarget->m_pGoalEnt == NULL)
			{	// nothing. Oh well.
				ALERT(at_debug,"%s %s has a missing \"move target\": %s\n",STRING(pev->classname),STRING(pev->targetname),STRING(m_iszMoveTarget));
				pTarget->m_pGoalEnt = this;
			}
		}
		else
		{
			pTarget->m_pGoalEnt = this;
		}
//		if (IsAction())
//		  pTarget->PushEnemy(this,pev->origin);

		m_saved_movetype = pTarget->pev->movetype;
		m_saved_solid = pTarget->pev->solid;
		m_saved_effects = pTarget->pev->effects;
		pTarget->pev->effects |= pev->effects;

//		ALERT(at_console, "script. IsAction = %d",IsAction());

		m_iState = STATE_ON; // LRC: assume we'll set it to 'on', unless proven otherwise...
		switch (m_fMoveTo)
		{
		case 1: 
		case 2: 
			DelayStart( 1 );
			m_iState = STATE_TURN_ON;
			// fall through...
		case 0: 
		case 4:
		case 5: 
		case 6: 
			pTarget->m_scriptState = SCRIPT_WAIT; 
			break;
		}
//		ALERT( at_aiconsole, "\"%s\" found and used (INT: %s)\n", STRING( pTarget->pev->targetname ), FBitSet(pev->spawnflags, SF_SCRIPT_NOINTERRUPT)?"No":"Yes" );

		pTarget->m_IdealMonsterState = MONSTERSTATE_SCRIPT;
//		if (m_iszIdle)
//		{
//			ALERT(at_console, "Possess: Play idle sequence\n");
//			StartSequence( pTarget, m_iszIdle, FALSE );
//			if (FStrEq( STRING(m_iszIdle), STRING(m_iszPlay)))
//			{
//				pTarget->pev->framerate = 0;
//			}
//		}
//		ALERT(at_console, "Finished PossessEntity, ms %d, ims %d\n", pTarget->m_MonsterState, pTarget->m_IdealMonsterState);
	}

}

// at the beginning of the level, set up the idle animation. --LRC
void CCineMonster :: InitIdleThink( void )
{
	if ((m_hTargetEnt = FindEntity(STRING(m_iszEntity), NULL)) != NULL)
	{
		PossessEntity( );
		m_startTime = gpGlobals->time + 1E6;
		ALERT( at_aiconsole, "script \"%s\" using monster \"%s\"\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
	}
	else
	{
		CancelScript( );
		ALERT( at_aiconsole, "script \"%s\" can't find monster \"%s\"\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
		SetNextThink( 1.0 );
	}
}

void CCineMonster :: CineThink( void )
{
//	ALERT(at_console, "Sequence think, activator %s\n", STRING(m_hActivator->pev->targetname));
	if ((m_hTargetEnt = FindEntity(STRING(m_iszEntity),m_hActivator)) != NULL)
	{
//		ALERT(at_console, "Sequence found %s \"%s\"\n", STRING(m_hTargetEnt->pev->classname), STRING(m_hTargetEnt->pev->targetname));
		PossessEntity( );
		ALERT( at_aiconsole, "script \"%s\" using monster \"%s\"\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
	}
	else
	{
//		ALERT(at_console, "Sequence found nothing called %s\n", STRING(m_iszEntity));
		CancelScript( );
		ALERT( at_aiconsole, "script \"%s\" can't find monster \"%s\"\n", STRING( pev->targetname ), STRING( m_iszEntity ) );
		SetNextThink( 1.0 );
	}
}


// lookup a sequence name and setup the target monster to play it
BOOL CCineMonster :: StartSequence( CBaseMonster *pTarget, int iszSeq, BOOL completeOnEmpty )
{
//	ALERT( at_console, "StartSequence %s \"%s\"\n", STRING(pev->classname), STRING(pev->targetname));

	if ( !iszSeq && completeOnEmpty )
	{
		SequenceDone( pTarget );
		return FALSE;
	}

	pTarget->pev->sequence = pTarget->LookupSequence( STRING( iszSeq ) );
	if (pTarget->pev->sequence == -1)
	{
		ALERT( at_error, "%s: unknown scripted sequence \"%s\"\n", STRING( pTarget->pev->targetname ), STRING( iszSeq) );
		pTarget->pev->sequence = 0;
		// return FALSE;
	}

#if 0
	char *s;
	if ( pev->spawnflags & SF_SCRIPT_NOINTERRUPT ) 
		s = "No";
	else
		s = "Yes";

	ALERT( at_debug, "%s (%s): started \"%s\":INT:%s\n", STRING( pTarget->pev->targetname ), STRING( pTarget->pev->classname ), STRING( iszSeq), s );
#endif

	pTarget->pev->frame = 0;
	pTarget->ResetSequenceInfo( );
	return TRUE;
}

//=========================================================
// SequenceDone - called when a scripted sequence animation
// sequence is done playing ( or when an AI Scripted Sequence
// doesn't supply an animation sequence to play ). Expects
// the CBaseMonster pointer to the monster that the sequence
// possesses. 
//=========================================================
void CCineMonster :: SequenceDone ( CBaseMonster *pMonster )
{
	m_iRepeatsLeft = m_iRepeats; //LRC - reset the repeater count
	m_iState = STATE_OFF; // we've finished.
//	ALERT( at_console, "Sequence %s finished\n", STRING(pev->targetname));//STRING( m_pCine->m_iszPlay ) );

	if ( !( pev->spawnflags & SF_SCRIPT_REPEATABLE ) )
	{
		SetThink(&CCineMonster :: SUB_Remove );
		SetNextThink( 0.1 );
	}
	
	// This is done so that another sequence can take over the monster when triggered by the first
	
	pMonster->CineCleanup();

	FixScriptMonsterSchedule( pMonster );
	
	// This may cause a sequence to attempt to grab this guy NOW, so we have to clear him out
	// of the existing sequence
	SUB_UseTargets( NULL, USE_TOGGLE, 0 );
}

//=========================================================
// When a monster finishes a scripted sequence, we have to 
// fix up its state and schedule for it to return to a 
// normal AI monster. 
//
// Scripted sequences just dirty the Schedule and drop the
// monster in Idle State.
//
// or select a specific AMBUSH schedule, regardless of state. //LRC
//=========================================================
void CCineMonster :: FixScriptMonsterSchedule( CBaseMonster *pMonster )
{
	if ( pMonster->m_IdealMonsterState != MONSTERSTATE_DEAD )
		pMonster->m_IdealMonsterState = MONSTERSTATE_IDLE;
//	pMonster->ClearSchedule();

	switch ( m_iFinishSchedule )
	{
		case SCRIPT_FINISHSCHED_DEFAULT:
			pMonster->ClearSchedule();
			break;
		case SCRIPT_FINISHSCHED_AMBUSH:
			pMonster->ChangeSchedule( pMonster->GetScheduleOfType( SCHED_AMBUSH ) );
			break;
		default:
			ALERT ( at_aiconsole, "FixScriptMonsterSchedule - no case!\n" );
			pMonster->ClearSchedule();
			break;
	}
}

BOOL CBaseMonster :: ExitScriptedSequence( )
{
	if ( pev->deadflag == DEAD_DYING )
	{
		// is this legal?
		// BUGBUG -- This doesn't call Killed()
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		return FALSE;
	}

	if (m_pCine)
	{
		m_pCine->CancelScript( );
	}

	return TRUE;
}


void CCineMonster::AllowInterrupt( BOOL fAllow )
{
	if ( pev->spawnflags & SF_SCRIPT_NOINTERRUPT )
		return;
	m_interruptable = fAllow;
}


BOOL CCineMonster::CanInterrupt( void )
{
	if ( !m_interruptable )
		return FALSE;

	CBaseEntity *pTarget = m_hTargetEnt;

	if ( pTarget != NULL && pTarget->pev->deadflag == DEAD_NO )
		return TRUE;

	return FALSE;
}


int	CCineMonster::IgnoreConditions( void )
{
	if ( CanInterrupt() )
		return 0;

	// Big fat BUG: This is an IgnoreConditions function - we need to return the conditions
	// that _shouldn't_ be able to break the script, instead of the conditions that _should_!!
	return SCRIPT_BREAK_CONDITIONS;
}


void ScriptEntityCancel( edict_t *pentCine )
{
	// make sure they are a scripted_sequence
	if (FClassnameIs( pentCine, "scripted_sequence" ) || FClassnameIs( pentCine, "scripted_action" ))
	{
		((CCineMonster *)VARS(pentCine))->m_iState = STATE_OFF;
		CCineMonster *pCineTarget = GetClassPtr((CCineMonster *)VARS(pentCine));
		// make sure they have a monster in mind for the script
		CBaseEntity		*pEntity = pCineTarget->m_hTargetEnt;
		CBaseMonster	*pTarget = NULL;
		if ( pEntity )
			pTarget = pEntity->MyMonsterPointer();
		
		if (pTarget)
		{
			// make sure their monster is actually playing a script
			if ( pTarget->m_MonsterState == MONSTERSTATE_SCRIPT )
			{
				// tell them do die
				pTarget->m_scriptState = CCineMonster::SCRIPT_CLEANUP;
				// do it now
				pTarget->CineCleanup( );
				//LRC - clean up so that if another script is starting immediately, the monster will notice it.
				pTarget->ClearSchedule( );
			}
		}
	}
}


// find all the cinematic entities with my targetname and stop them from playing
void CCineMonster :: CancelScript( void )
{	
	ALERT( at_aiconsole, "Cancelling script: %s\n", STRING(m_iszPlay) );
	
	if ( !pev->targetname )
	{
		ScriptEntityCancel( edict() );
		return;
	}

	CBaseEntity *pCineTarget = UTIL_FindEntityByTargetname(NULL, STRING(pev->targetname));

	while (pCineTarget)
	{
		ScriptEntityCancel( ENT(pCineTarget->pev) );
		pCineTarget = UTIL_FindEntityByTargetname(pCineTarget, STRING(pev->targetname));
	}
}


// find all the cinematic entities with my targetname and tell them whether to wait before starting
void CCineMonster :: DelayStart( int state )
{
	CBaseEntity *pCine = UTIL_FindEntityByTargetname(NULL, STRING(pev->targetname));

	while ( pCine )
	{
		if (FClassnameIs( pCine->pev, "scripted_sequence" ) || FClassnameIs( pCine->pev, "scripted_action" ))
		{
			CCineMonster *pTarget = GetClassPtr((CCineMonster *)(pCine->pev));
			if (state)
			{
//				ALERT(at_console, "Delaying start\n");
				pTarget->m_iDelay++;
			}
			else
			{
//				ALERT(at_console, "Undelaying start\n");
				pTarget->m_iDelay--;
				if (pTarget->m_iDelay <= 0)
				{
					pTarget->m_iState = STATE_ON; //LRC
					FireTargets(STRING(m_iszFireOnBegin), this, this, USE_TOGGLE, 0); //LRC
					pTarget->m_startTime = gpGlobals->time + 0.05; // why the delay? -- LRC
				}
			}
		}
		pCine = UTIL_FindEntityByTargetname(pCine, STRING(pev->targetname));
	}
}



// Find an entity that I'm interested in and precache the sounds he'll need in the sequence.
void CCineMonster :: Activate( void )
{
	CBaseEntity		*pEntity;
	CBaseMonster	*pTarget;

	// The entity name could be a target name or a classname
	// Check the targetname
	pEntity = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEntity));
	pTarget = NULL;

	while (!pTarget && pEntity)
	{
		if ( FBitSet( pEntity->pev->flags, FL_MONSTER ))
		{
			pTarget = pEntity->MyMonsterPointer( );
		}
		pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(m_iszEntity));
	}
	
	// If no entity with that targetname, check the classname
	if ( !pTarget )
	{
		pEntity = UTIL_FindEntityByClassname(NULL, STRING(m_iszEntity));
		while (!pTarget && pEntity)
		{
			pTarget = pEntity->MyMonsterPointer( );
			pEntity = UTIL_FindEntityByClassname(pEntity, STRING(m_iszEntity));
		}
	}
	// Found a compatible entity
	if ( pTarget )
	{
		void *pmodel;
		pmodel = GET_MODEL_PTR( pTarget->edict() );
		if ( pmodel )
		{
			// Look through the event list for stuff to precache
			SequencePrecache( pmodel, STRING( m_iszIdle ) );
			SequencePrecache( pmodel, STRING( m_iszPlay ) );
		}
	}

	CBaseMonster::Activate();
}

		
BOOL CBaseMonster :: CineCleanup( )
{
	CCineMonster *pOldCine = m_pCine;

	// am I linked to a cinematic?
	if (m_pCine)
	{
		// okay, reset me to what it thought I was before
		m_pCine->m_hTargetEnt = NULL;

		pev->movetype = m_pCine->m_saved_movetype;

// LRC - why mess around with this? Solidity isn't changed by sequences!
//		pev->solid = m_pCine->m_saved_solid;

		if (m_pCine->pev->spawnflags & SF_SCRIPT_STAYDEAD)
			pev->deadflag = DEAD_DYING;
	}
	else
	{
		// arg, punt
		pev->movetype = MOVETYPE_STEP;// this is evil
		pev->solid = SOLID_SLIDEBOX;
	}
	m_pCine = NULL;
	m_hTargetEnt = NULL;
	m_pGoalEnt = NULL;
	if (pev->deadflag == DEAD_DYING)
	{
		// last frame of death animation?
		pev->health			= 0;
		pev->framerate		= 0.0;
		pev->solid			= SOLID_NOT;
		SetState( MONSTERSTATE_DEAD );
		pev->deadflag = DEAD_DEAD;
		UTIL_SetSize( pev, pev->mins, Vector(pev->maxs.x, pev->maxs.y, pev->mins.z + 2) );

		if ( pOldCine && FBitSet( pOldCine->pev->spawnflags, SF_SCRIPT_LEAVECORPSE ) )
		{
			SetUse( NULL );		// BUGBUG -- This doesn't call Killed()
			SetThink( NULL );	// This will probably break some stuff
			SetTouch( NULL );
		}
		else
			SUB_StartFadeOut(); // SetThink( SUB_DoNothing );
		// This turns off animation & physics in case their origin ends up stuck in the world or something
		StopAnimation();
		pev->movetype = MOVETYPE_NONE;
		pev->effects |= EF_NOINTERP;	// Don't interpolate either, assume the corpse is positioned in its final resting place
		return FALSE;
	}

	// If we actually played a sequence
	if ( pOldCine && pOldCine->m_iszPlay )
	{
		if ( !(pOldCine->pev->spawnflags & SF_SCRIPT_NOSCRIPTMOVEMENT) )
		{
			// reset position
			Vector new_origin, new_angle;
			GetBonePosition( 0, new_origin, new_angle );

			// Figure out how far they have moved
			// We can't really solve this problem because we can't query the movement of the origin relative
			// to the sequence.  We can get the root bone's position as we do here, but there are
			// cases where the root bone is in a different relative position to the entity's origin
			// before/after the sequence plays.  So we are stuck doing this:

			// !!!HACKHACK: Float the origin up and drop to floor because some sequences have
			// irregular motion that can't be properly accounted for.

			// UNDONE: THIS SHOULD ONLY HAPPEN IF WE ACTUALLY PLAYED THE SEQUENCE.
			Vector oldOrigin = pev->origin;

			// UNDONE: ugly hack.  Don't move monster if they don't "seem" to move
			// this really needs to be done with the AX,AY,etc. flags, but that aren't consistantly
			// being set, so animations that really do move won't be caught.
			if ((oldOrigin - new_origin).Length2D() < 8.0)
				new_origin = oldOrigin;

			pev->origin.x = new_origin.x;
			pev->origin.y = new_origin.y;
			pev->origin.z += 1;

			pev->flags |= FL_ONGROUND;
			int drop = DROP_TO_FLOOR( ENT(pev) );
			
			// Origin in solid?  Set to org at the end of the sequence
			if ( drop < 0 )
				pev->origin = oldOrigin;
			else if ( drop == 0 ) // Hanging in air?
			{
				pev->origin.z = new_origin.z;
				pev->flags &= ~FL_ONGROUND;
			}
			// else entity hit floor, leave there

			// pEntity->pev->origin.z = new_origin.z + 5.0; // damn, got to fix this

			UTIL_SetOrigin( this, pev->origin );
			pev->effects |= EF_NOINTERP;
		}

		// We should have some animation to put these guys in, but for now it's idle.
		// Due to NOINTERP above, there won't be any blending between this anim & the sequence
		m_Activity = ACT_RESET;
	}
	// set them back into a normal state
	pev->enemy = NULL;
	if ( pev->health > 0 )
		m_IdealMonsterState = MONSTERSTATE_IDLE; // m_previousState;
	else
	{
		// Dropping out because he got killed
		// Can't call killed() no attacker and weirdness (late gibbing) may result
		m_IdealMonsterState = MONSTERSTATE_DEAD;
		SetConditions( bits_COND_LIGHT_DAMAGE );
		pev->deadflag = DEAD_DYING;
		FCheckAITrigger();
		pev->deadflag = DEAD_NO;
	}


	//	SetAnimation( m_MonsterState );
	//LRC- removed, was never implemented. ClearBits(pev->spawnflags, SF_MONSTER_WAIT_FOR_SCRIPT );

	return TRUE;
}




class CScriptedSentence : public CBaseToggle
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT FindThink( void );
	void EXPORT DelayThink( void );
	void EXPORT DurationThink( void );
	int	 ObjectCaps( void ) { return (CBaseToggle :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }

	STATE GetState() { return m_playing?STATE_ON:STATE_OFF; }

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	CBaseMonster *FindEntity( CBaseEntity *pActivator );
	BOOL AcceptableSpeaker( CBaseMonster *pMonster );
	BOOL StartSentence( CBaseMonster *pTarget );


private:
	int		m_iszSentence;		// string index for idle animation
	int		m_iszEntity;	// entity that is wanted for this sentence
	float	m_flRadius;		// range to search
	float	m_flDuration;	// How long the sentence lasts
	float	m_flRepeat;	// maximum repeat rate
	float	m_flAttenuation;
	float	m_flVolume;
	BOOL	m_active; // is the sentence enabled? (for m_flRepeat)
	BOOL	m_playing; //LRC- is the sentence playing? (for GetState)
	int		m_iszListener;	// name of entity to look at while talking
};

#define SF_SENTENCE_ONCE		0x0001
#define SF_SENTENCE_FOLLOWERS	0x0002	// only say if following player
#define SF_SENTENCE_INTERRUPT	0x0004	// force talking except when dead
#define SF_SENTENCE_CONCURRENT	0x0008	// allow other people to keep talking

TYPEDESCRIPTION	CScriptedSentence::m_SaveData[] = 
{
	DEFINE_FIELD( CScriptedSentence, m_iszSentence, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CScriptedSentence, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flRepeat, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flAttenuation, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CScriptedSentence, m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( CScriptedSentence, m_playing, FIELD_BOOLEAN ),
	DEFINE_FIELD( CScriptedSentence, m_iszListener, FIELD_STRING ),
};


IMPLEMENT_SAVERESTORE( CScriptedSentence, CBaseToggle );

LINK_ENTITY_TO_CLASS( scripted_sentence, CScriptedSentence );

void CScriptedSentence :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "sentence"))
	{
		m_iszSentence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "entity"))
	{
		m_iszEntity = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "duration"))
	{
		m_flDuration = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "radius"))
	{
		m_flRadius = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "refire"))
	{
		m_flRepeat = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "attenuation"))
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if(FStrEq(pkvd->szKeyName, "volume"))
	{
		m_flVolume = atof( pkvd->szValue ) * 0.1;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "listener"))
	{
		m_iszListener = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}


void CScriptedSentence :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !m_active )
		return;
//	ALERT( at_console, "Firing sentence: %s\n", STRING(m_iszSentence) );
	m_hActivator = pActivator;
	SetThink(&CScriptedSentence :: FindThink );
	SetNextThink( 0 );
}


void CScriptedSentence :: Spawn( void )
{
	pev->solid = SOLID_NOT;
	
	m_active = TRUE;
	m_playing = FALSE; //LRC
	// if no targetname, start now
	if ( !pev->targetname )
	{
		SetThink(&CScriptedSentence :: FindThink );
		SetNextThink( 1.0 );
	}

	switch( pev->impulse )
	{
	case 1: // Medium radius
		m_flAttenuation = ATTN_STATIC;
		break;
	
	case 2:	// Large radius
		m_flAttenuation = ATTN_NORM;
		break;

	case 3:	//EVERYWHERE
		m_flAttenuation = ATTN_NONE;
		break;
	
	default:
	case 0: // Small radius
		m_flAttenuation = ATTN_IDLE;
		break;
	}
	pev->impulse = 0;

	// No volume, use normal
	if ( m_flVolume <= 0 )
		m_flVolume = 1.0;
}


void CScriptedSentence :: FindThink( void )
{
	if (!m_iszEntity) //LRC- no target monster given: speak through HEV
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( NULL, "player" );
		if (pPlayer)
		{
			m_playing = TRUE;
			if ((STRING(m_iszSentence))[0] == '!')
				pPlayer->SetSuitUpdate((char*)STRING(m_iszSentence),FALSE,0);
			else
				pPlayer->SetSuitUpdate((char*)STRING(m_iszSentence),TRUE,0);
			if ( pev->spawnflags & SF_SENTENCE_ONCE )
				UTIL_Remove( this );
			SetThink(&CScriptedSentence :: DurationThink );
			SetNextThink( m_flDuration );
			m_active = FALSE;
		}
		else
			ALERT( at_debug, "ScriptedSentence: can't find \"player\" to play HEV sentence!?\n");
		return;
	}

	CBaseMonster *pMonster = FindEntity( m_hActivator );
	if ( pMonster )
	{
		m_playing = TRUE;
		StartSentence( pMonster );
		if ( pev->spawnflags & SF_SENTENCE_ONCE )
			UTIL_Remove( this );
		SetThink(&CScriptedSentence :: DurationThink );
		SetNextThink( m_flDuration );
		m_active = FALSE;
//		ALERT( at_console, "%s: found monster %s\n", STRING(m_iszSentence), STRING(m_iszEntity) );
	}
	else
	{
//		ALERT( at_console, "%s: can't find monster %s\n", STRING(m_iszSentence), STRING(m_iszEntity) );
		SetNextThink( m_flRepeat + 0.5 );
	}
}

//LRC
void CScriptedSentence :: DurationThink( void )
{
	m_playing = FALSE;
	SetNextThink( m_flRepeat );
	SetThink(&CScriptedSentence :: DelayThink );
}

void CScriptedSentence :: DelayThink( void )
{
	m_active = TRUE;
	if ( !pev->targetname )
		SetNextThink( 0.1 );
	SetThink(&CScriptedSentence :: FindThink );
}


BOOL CScriptedSentence :: AcceptableSpeaker( CBaseMonster *pMonster )
{
	if ( pMonster )
	{
		if ( pev->spawnflags & SF_SENTENCE_FOLLOWERS )
		{
			if ( pMonster->m_hTargetEnt == NULL || !FClassnameIs(pMonster->m_hTargetEnt->pev, "player") )
				return FALSE;
		}
		BOOL override;
		if ( pev->spawnflags & SF_SENTENCE_INTERRUPT )
			override = TRUE;
		else
			override = FALSE;
		if ( pMonster->CanPlaySentence( override ) )
			return TRUE;
	}
	return FALSE;
}


CBaseMonster *CScriptedSentence :: FindEntity( CBaseEntity *pActivator )
{
	CBaseEntity *pTarget;
	CBaseMonster *pMonster;

	pTarget = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEntity), pActivator);
	pMonster = NULL;

	while ( pTarget )
	{
		pMonster = pTarget->MyMonsterPointer( );
		if ( pMonster != NULL )
		{
			if ( AcceptableSpeaker( pMonster ) )
				return pMonster;
//			ALERT( at_console, "%s (%s), not acceptable\n", STRING(pMonster->pev->classname), STRING(pMonster->pev->targetname) );
		}
		pTarget = UTIL_FindEntityByTargetname(pTarget, STRING(m_iszEntity), pActivator);
	}
	
	pTarget = NULL;
	while ((pTarget = UTIL_FindEntityInSphere( pTarget, pev->origin, m_flRadius )) != NULL)
	{
		if (FClassnameIs( pTarget->pev, STRING(m_iszEntity)))
		{
			if ( FBitSet( pTarget->pev->flags, FL_MONSTER ))
			{
				pMonster = pTarget->MyMonsterPointer( );
				if ( AcceptableSpeaker( pMonster ) )
					return pMonster;
			}
		}
	}
	
	return NULL;
}


BOOL CScriptedSentence :: StartSentence( CBaseMonster *pTarget )
{
	if ( !pTarget )
	{
		ALERT( at_aiconsole, "Not Playing sentence %s\n", STRING(m_iszSentence) );
		return NULL;
	}

	BOOL bConcurrent = FALSE;
	//LRC: Er... if the "concurrent" flag is NOT set, we make bConcurrent true!?
	if ( !(pev->spawnflags & SF_SENTENCE_CONCURRENT) )
		bConcurrent = TRUE;

	CBaseEntity *pListener = NULL;
	if (!FStringNull(m_iszListener))
	{
		float radius = m_flRadius;

		if ( FStrEq( STRING(m_iszListener ), "player" ) )
			radius = 4096;	// Always find the player

		pListener = UTIL_FindEntityGeneric( STRING( m_iszListener ), pTarget->pev->origin, radius );
	}

	pTarget->PlayScriptedSentence( STRING(m_iszSentence), m_flDuration,  m_flVolume, m_flAttenuation, bConcurrent, pListener );
	ALERT( at_aiconsole, "Playing sentence %s (%.1f)\n", STRING(m_iszSentence), m_flDuration );
	SUB_UseTargets( NULL, USE_TOGGLE, 0 );
	return TRUE;
}





/*

*/


//=========================================================
// Furniture - this is the cool comment I cut-and-pasted
//=========================================================
class CFurniture : public CBaseMonster
{
public:
	void Spawn ( void );
	void Die( void );
	int	 Classify ( void );
	virtual int	ObjectCaps( void ) { return (CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION); }
};


LINK_ENTITY_TO_CLASS( monster_furniture, CFurniture );


//=========================================================
// Furniture is killed
//=========================================================
void CFurniture :: Die ( void )
{
	SetThink(&CFurniture :: SUB_Remove );
	SetNextThink( 0 );
}

//=========================================================
// This used to have something to do with bees flying, but 
// now it only initializes moving furniture in scripted sequences
//=========================================================
void CFurniture :: Spawn( )
{
	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev),	STRING(pev->model));

	pev->movetype	= MOVETYPE_NONE;
	pev->solid		= SOLID_BBOX;
	pev->health		= 80000;
	pev->takedamage = DAMAGE_AIM;
	pev->effects		= 0;
	pev->yaw_speed		= 0;
	pev->sequence		= 0;
	pev->frame			= 0;

//	pev->nextthink += 1.0;
//	SetThink (WalkMonsterDelay);

	ResetSequenceInfo( );
	pev->frame = 0;
	MonsterInit();
}

//=========================================================
// ID's Furniture as neutral (noone will attack it)
//=========================================================
int CFurniture::Classify ( void )
{
	return m_iClass?m_iClass:CLASS_NONE;
}


