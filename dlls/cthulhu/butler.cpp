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
//=========================================================
// human butler (passive servant)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"


enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// none yet...

//=======================================================
// Butler
//=======================================================

#include "butler.h"


LINK_ENTITY_TO_CLASS( monster_butler, CButler );

TYPEDESCRIPTION	CButler::m_SaveData[] = 
{
	DEFINE_FIELD( CButler, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CButler, m_fearTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CButler, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlButlerFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slButlerFollow[] =
{
	{
		tlButlerFollow,
		ARRAYSIZE ( tlButlerFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlButlerFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slButlerFollowScared[] =
{
	{
		tlButlerFollowScared,
		ARRAYSIZE ( tlButlerFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlButlerFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slButlerFaceTargetScared[] =
{
	{
		tlButlerFaceTargetScared,
		ARRAYSIZE ( tlButlerFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlButlerStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slButlerStopFollowing[] =
{
	{
		tlButlerStopFollowing,
		ARRAYSIZE ( tlButlerStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlButlerFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slButlerFaceTarget[] =
{
	{
		tlButlerFaceTarget,
		ARRAYSIZE ( tlButlerFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlButlerPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slButlerPanic[] =
{
	{
		tlButlerPanic,
		ARRAYSIZE ( tlButlerPanic ),
		0,
		0,
		"ButlerPanic"
	},
};


Task_t	tlIdleButlerStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleButlerStand[] =
{
	{ 
		tlIdleButlerStand,
		ARRAYSIZE ( tlIdleButlerStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_CLIENT_PUSH	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleButlerStand"

	},
};


Task_t	tlButlerCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slButlerCover[] =
{
	{ 
		tlButlerCover,
		ARRAYSIZE ( tlButlerCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"ButlerCover"
	},
};



Task_t	tlButlerHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slButlerHide[] =
{
	{ 
		tlButlerHide,
		ARRAYSIZE ( tlButlerHide ), 
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ButlerHide"
	},
};


Task_t	tlButlerStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slButlerStartle[] =
{
	{ 
		tlButlerStartle,
		ARRAYSIZE ( tlButlerStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ButlerStartle"
	},
};



Task_t	tlButlerFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slButlerFear[] =
{
	{ 
		tlButlerFear,
		ARRAYSIZE ( tlButlerFear ), 
		bits_COND_NEW_ENEMY,
		0,
		"ButlerFear"
	},
};


DEFINE_CUSTOM_SCHEDULES( CButler )
{
	slButlerFollow,
	slButlerFaceTarget,
	slIdleButlerStand,
	slButlerFear,
	slButlerCover,
	slButlerHide,
	slButlerStartle,
	slButlerStopFollowing,
	slButlerPanic,
	slButlerFollowScared,
	slButlerFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CButler, CTalkMonster );


void CButler::DeclineFollowing( void )
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
}


void CButler :: Scream( void )
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM );
	}
}


Activity CButler::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CButler :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
//		if ( FOkToSpeak() )
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "SC_HEAL", 2, VOL_NORM, ATTN_IDLE );

		TaskComplete();
		break;

	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		if ( RANDOM_FLOAT( 0, 1 ) < pTask->flData )
			Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if ( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			if ( m_hEnemy->IsPlayer() )
				PlaySentence( "SC_PLFEAR", 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( "SC_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;

	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
					TaskFail();
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CButler :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( MovementIsComplete() )
			TaskComplete();
		if ( RANDOM_LONG(0,31) < 8 )
			Scream();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( RANDOM_LONG(0,63)< 8 )
				Scream();

			if ( m_hEnemy == NULL )
			{
				TaskFail();
			}
			else
			{
				float distance;

				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5 )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if ( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
					m_movementActivity = ACT_WALK_SCARED;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
					m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;

	case TASK_HEAL:
		if ( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if ( TargetDistance() > 90 )
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CButler :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CButler :: SetYawSpeed ( void )
{
	int ys;

	ys = 90;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CButler :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	CTalkMonster::HandleAnimEvent( pEvent );
}

//=========================================================
// Spawn
//=========================================================
void CButler :: Spawn( void )
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/butler.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= gSkillData.butlerHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so butler will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;
	
	MonsterInit();
	SetUse( FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CButler :: Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/butler.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	// every new butler must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CButler :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// butler will try to talk to friends in this order:

	m_szFriends[0] = "monster_sirhenry";
	m_szFriends[1] = "monster_butler";
	m_szFriends[2] = "monster_sitting_butler";
	m_szFriends[3] = "";
	m_szFriends[4] = "";

	// butler speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]  =	"CIV_ANSWER";
		m_szGrp[TLK_QUESTION] =	"CIV_QUESTION";
		m_szGrp[TLK_IDLE] =		"BUT_IDLE";
		m_szGrp[TLK_STARE] =	NULL;

		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_USE] =	NULL;
		else
			m_szGrp[TLK_USE] =	NULL;

		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = NULL;
		else
			m_szGrp[TLK_UNUSE] = NULL;

		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	NULL;
		else
			m_szGrp[TLK_DECLINE] =	NULL;

		m_szGrp[TLK_STOP] =		"SC_STOP";
		m_szGrp[TLK_NOSHOOT] =	"SC_SCARED";
		m_szGrp[TLK_HELLO] =	"BUT_HELLO";

		m_szGrp[TLK_PLHURT1] =	NULL;
		m_szGrp[TLK_PLHURT2] =	NULL; 
		m_szGrp[TLK_PLHURT3] =	NULL;

		m_szGrp[TLK_PHELLO] =	"BUT_GREET";
		m_szGrp[TLK_PIDLE] =	"BUT_IDLE";
		m_szGrp[TLK_PQUESTION] = NULL;
		m_szGrp[TLK_SMELL] =	"SC_SMELL";
	
		m_szGrp[TLK_WOUND] =	"SC_WOUND";
		m_szGrp[TLK_MORTAL] =	"SC_MORTAL";
	}

	m_voicePitch = 95;
}

int CButler :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	// make sure friends talk about it if player hurts butler...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CButler :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CButler :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime )
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,4))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CButler :: DeathSound ( void )
{
	PainSound();
}


void CButler::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}


void CButler :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CButler :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that butler will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slButlerFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slButlerFollow;
	
	case SCHED_CANT_FOLLOW:
		return slButlerStopFollowing;

	case SCHED_PANIC:
		return slButlerPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slButlerFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slButlerFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that butler will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleButlerStand;
		else
			return psched;

	case SCHED_HIDE:
		return slButlerHide;

	case SCHED_STARTLE:
		return slButlerStartle;

	case SCHED_FEAR:
		return slButlerFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CButler :: GetSchedule ( void )
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( pEnemy )
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if ( pSound )
			{
				if ( pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) )
				{
					if ( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if ( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if ( relationship != R_DL && relationship != R_HT )
			{
				// If I'm already close enough to my target
				if ( TargetDistance() <= 128 )
				{
					if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, butler won't move out of your way.  Keep This?  If not, write move away scared
			{
				if ( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return slButlerFear;					// Point and scream!
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slButlerCover;		// Take Cover
		
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slButlerCover;			// Run & Cower
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CButler :: GetIdealState ( void )
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( IsFollowing() )
			{
				int relationship = IRelationship( m_hEnemy );
				if ( relationship != R_FR || relationship != R_HT && !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing( TRUE );
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			if ( IsFollowing() )
				StopFollowing( TRUE );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if ( pEnemy != NULL )
			{
				if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
				{
					// Strip enemy when going to alert
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = NULL;
					return m_IdealMonsterState;
				}
				// Follow if only scared a little
				if ( m_hTargetEnt != NULL )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}

				if ( HasConditions ( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}

			}
		}
		break;
	}

	return CTalkMonster::GetIdealState();
}


int CButler::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}


//=========================================================
// Dead Butler PROP
//=========================================================

char *CDeadButler::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadButler::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_butler_dead, CDeadButler );

//
// ********** CDeadButler SPAWN **********
//
void CDeadButler :: Spawn( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/butler.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/butler.mdl");
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.butlerHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead Butler with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Butler PROP
//=========================================================


LINK_ENTITY_TO_CLASS( monster_sitting_butler, CSittingButler );
TYPEDESCRIPTION	CSittingButler::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingButler, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingButler, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingButler, CButler );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


#define SF_SITTINGBUTLER_POSTDISASTER 1024

//
// ********** Butler SPAWN **********
//
void CSittingButler :: Spawn( )
{
	PRECACHE_MODEL("models/butler.mdl");
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/butler.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->health			= 50;
	
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView		= VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	if (!FBitSet(pev->spawnflags, SF_SITTINGBUTLER_POSTDISASTER)) //LRC- allow a sitter to be postdisaster.
		SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!
	
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (SittingThink);
	SetNextThink( 0.1 );

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingButler :: Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingButler :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
}


int CSittingButler::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingButler :: SittingThink( void )
{
	CBaseEntity *pent;	

	StudioFrameAdvance( );

	// try to greet player
	if (FIdleHello())
	{
		pent = FindNearestFriend(TRUE);
		if (pent)
		{
			float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
				
			if (yaw > 0)
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;
		
		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, 0 );
		}
	}
	else if (m_fSequenceFinished)
	{
		int i = RANDOM_LONG(0,99);
		m_headTurn = 0;
		
		if (m_flResponseDelay && gpGlobals->time > m_flResponseDelay)
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if (i < 30)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	

			// turn towards player or nearest friend and speak

			if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
				pent = FindNearestFriend(TRUE);
			else
				pent = FindNearestFriend(FALSE);

			if (!FIdleSpeak() || !pent)
			{	
				m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

				if (yaw > 180) yaw -= 360;
				if (yaw < -180) yaw += 360;
				
				if (yaw > 0)
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT(at_console, "sitting speak\n");
			}
		}
		else if (i < 60)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	
			m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
			if (RANDOM_LONG(0,99) < 5)
			{
				//ALERT(at_console, "sitting speak2\n");
				FIdleSpeak();
			}
		}
		else if (i < 80)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if (i < 100)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

// prepare sitting butler to answer a question
void CSittingButler :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingButler :: FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;
	
	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();
		
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting butlers nearby
	CBaseEntity *pentFriend = FindNearestFriend(FALSE);

	if (pentFriend && RANDOM_LONG(0,1))
	{
		CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pentFriend->pev);
		pTalkMonster->SetAnswerQuestion( this );
		
		IdleHeadTurn(pentFriend->pev->origin);
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// otherwise, play an idle statement
	if (RANDOM_LONG(0,1))
	{
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}
