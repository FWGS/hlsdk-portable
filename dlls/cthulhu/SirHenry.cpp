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
// human SirHenry
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
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"

#define BODY_GROUP					1
#define BODY_CIVILIAN				0
#define BODY_CULTIST				1
#define NUM_BODY_GROUPS				2

#define GUN_GROUP					2
#define GUN_NONE					0
#define GUN_BOOK1					1
#define GUN_BOOK2					2
#define GUN_KNIFE					3


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
	TASK_SAY_FEAR = LAST_TALKMONSTER_TASK + 1,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define		SIRHENRY_AE_KNIFE				( 1 )
#define		SIRHENRY_AE_SUMMON_POWERUP	( 2 )
#define		SIRHENRY_AE_SUMMON_DO			( 3 )
#define		SIRHENRY_AE_SUMMON_DONE		( 4 )
#define		SIRHENRY_AE_DROP_KNIFE		( 5 )

#define		SIRHENRY_MAX_BEAMS		8

//=======================================================
// Sir Henry
//=======================================================

#include "SirHenry.h"


LINK_ENTITY_TO_CLASS( monster_sirhenry, CSirHenry );

TYPEDESCRIPTION	CSirHenry::m_SaveData[] = 
{
	DEFINE_FIELD( CSirHenry, m_iHolding, FIELD_INTEGER ),
	DEFINE_ARRAY( CSirHenry, m_pBeam, FIELD_CLASSPTR, SIRHENRY_MAX_BEAMS ),
	DEFINE_FIELD( CSirHenry, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CSirHenry, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CSirHenry, m_flNextAttack, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSirHenry, CTalkMonster );

////////////////////////////////////////////////////////////////////////////////////////////
// Sounds
////////////////////////////////////////////////////////////////////////////////////////////

const char *CSirHenry::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CSirHenry::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlSirHenryFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slSirHenryFollow[] =
{
	{
		tlSirHenryFollow,
		ARRAYSIZE ( tlSirHenryFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlSirHenryFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slSirHenryFollowScared[] =
{
	{
		tlSirHenryFollowScared,
		ARRAYSIZE ( tlSirHenryFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlSirHenryFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slSirHenryFaceTargetScared[] =
{
	{
		tlSirHenryFaceTargetScared,
		ARRAYSIZE ( tlSirHenryFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlSirHenryStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slSirHenryStopFollowing[] =
{
	{
		tlSirHenryStopFollowing,
		ARRAYSIZE ( tlSirHenryStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlSirHenryFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slSirHenryFaceTarget[] =
{
	{
		tlSirHenryFaceTarget,
		ARRAYSIZE ( tlSirHenryFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlSirHenryPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSirHenryPanic[] =
{
	{
		tlSirHenryPanic,
		ARRAYSIZE ( tlSirHenryPanic ),
		0,
		0,
		"SciPanic"
	},
};


Task_t	tlIdleSirHenryStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleSirHenryStand[] =
{
	{ 
		tlIdleSirHenryStand,
		ARRAYSIZE ( tlIdleSirHenryStand ), 
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
		"IdleSciStand"

	},
};


Task_t	tlSirHenryCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slSirHenryCover[] =
{
	{ 
		tlSirHenryCover,
		ARRAYSIZE ( tlSirHenryCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"SirHenryCover"
	},
};



Task_t	tlSirHenryHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slSirHenryHide[] =
{
	{ 
		tlSirHenryHide,
		ARRAYSIZE ( tlSirHenryHide ), 
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"SirHenryHide"
	},
};


Task_t	tlSirHenryStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
//	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
//	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slSirHenryStartle[] =
{
	{ 
		tlSirHenryStartle,
		ARRAYSIZE ( tlSirHenryStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"SirHenryStartle"
	},
};



Task_t	tlSirHenryFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slSirHenryFear[] =
{
	{ 
		tlSirHenryFear,
		ARRAYSIZE ( tlSirHenryFear ), 
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};

// primary range attack
Task_t	tlSirHenryAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSirHenryAttack1[] =
{
	{ 
		tlSirHenryAttack1,
		ARRAYSIZE ( tlSirHenryAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"SirHenry Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CSirHenry )
{
	slSirHenryFollow,
	slSirHenryFaceTarget,
	slIdleSirHenryStand,
	slSirHenryFear,
	slSirHenryAttack1,
	slSirHenryCover,
	slSirHenryHide,
	slSirHenryStartle,
	slSirHenryStopFollowing,
	slSirHenryPanic,
	slSirHenryFollowScared,
	slSirHenryFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CSirHenry, CTalkMonster );


void CSirHenry::DeclineFollowing( void )
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
}


void CSirHenry :: Scream( void )
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM );
	}
}


Activity CSirHenry::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CSirHenry :: StartTask( Task_t *pTask )
{
	ClearBeams( );

	switch( pTask->iTask )
	{
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

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( m_hTargetEnt== NULL)
			{
				TaskFail();
			}
			else
			{
				if ((m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
					TaskComplete();
				else
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					if ( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
						TaskFail();
				}
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CSirHenry :: RunTask( Task_t *pTask )
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
			//if ( RANDOM_LONG(0,63)< 8 )
			//	Scream();

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

	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSirHenry :: Classify ( void )
{
	switch( GetBodygroup( BODY_GROUP ) )
	{
	case BODY_CULTIST:
		return m_iClass?m_iClass:CLASS_HUMAN_CULTIST;
		break;
	default:
		return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
		break;
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSirHenry :: SetYawSpeed ( void )
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
void CSirHenry :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SIRHENRY_AE_DROP_KNIFE:
			{
				if (m_iHolding == GUN_KNIFE)
				{
					Vector	vecGunPos;
					Vector	vecGunAngles;

					GetAttachment( 0, vecGunPos, vecGunAngles );

					// switch to body group with no gun.
					int bg = GetBodygroup(BODY_GROUP);
					m_iHolding = GUN_NONE;
					// set the correct body group for what Sir Henry is holding
					pev->body = NUM_BODY_GROUPS * m_iHolding + bg;

					DropItem( "weapon_knife", vecGunPos, vecGunAngles );
				}
			}
			break;

		case SIRHENRY_AE_KNIFE:
		{
			Vector oldorig = pev->origin;
			CBaseEntity *pHurt = NULL;
			// check down below in stages for snakes...
			for (int dz = 0; dz >= -3; dz--)
			{
				pev->origin = oldorig;
				pev->origin.z += dz * 12;
				pHurt = CheckTraceHullAttack( 70, gSkillData.sirhenryDmgKnife, DMG_SLASH );
				if (pHurt) 
				{
					break;
				}
			}
			pev->origin = oldorig;
			if ( pHurt )
			{
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case SIRHENRY_AE_SUMMON_POWERUP:
		{
			// speed up attack when on hard
			if (g_iSkillLevel == SKILL_HARD)
				pev->framerate = 1.5;

			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 255 );		// r
					WRITE_BYTE( 180 );		// g
					WRITE_BYTE( 96 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );

			}

			ArmBeam( -1 );
			ArmBeam( 1 );
			BeamGlow( );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
		break;

		case SIRHENRY_AE_SUMMON_DO:
		{
			ClearBeams( );

			// I WAS GOING TO GET THE SIRHENRY TO SUMMON MORE MONSTERS, BUT DECIDED AGAINST IT...
			// find a clear spot nearby
			// create a monster
			//CBaseEntity *pNew = Create( "monster_ghoul", pev->origin + (pev->angles*64), pev->angles );
			//CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
			//pNew->pev->spawnflags |= 1;
			//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );

			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );

			ApplyMultiDamage(pev, pev);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
		break;

		case SIRHENRY_AE_SUMMON_DONE:
		{
			ClearBeams( );
		}
		break;

		default:
			CTalkMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSirHenry :: Spawn( void )
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/SirHenry.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
	pev->health			= gSkillData.sirhenryHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so Sir Henry will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;
	
	MonsterInit();

	int bg = GetBodygroup(BODY_GROUP);
	if (bg == BODY_CIVILIAN)
	{
		SetUse( FollowerUse );
	}

	// set the correct body group for what Sir Henry is holding
	pev->body = NUM_BODY_GROUPS * m_iHolding + bg;
}

void CSirHenry::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "holding"))
	{
		m_iHolding = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
	{
		CTalkMonster::KeyValue( pkvd );
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSirHenry :: Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/SirHenry.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	int i;
	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	// every new SirHenry must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CSirHenry :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// Sir Henry will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_policeman";
	m_szFriends[3] = "";
	m_szFriends[4] = "";

	// Sir Henry speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]  =	"CIV_ANSWER";
		m_szGrp[TLK_QUESTION] =	"CIV_QUESTION";
		m_szGrp[TLK_IDLE] =		"SH_IDLE";
		m_szGrp[TLK_STARE] =	"SC_STARE";
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
		m_szGrp[TLK_NOSHOOT] =	NULL;
		m_szGrp[TLK_HELLO] =	"SH_HELLO";

		m_szGrp[TLK_PLHURT1] =	"!SC_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!SC_CUREB"; 
		m_szGrp[TLK_PLHURT3] =	"!SC_CUREC";

		m_szGrp[TLK_PHELLO] =	"SH_PHELLO";
		m_szGrp[TLK_PIDLE] =	"SH_IDLE";
		m_szGrp[TLK_PQUESTION] = "CIV_PQUEST";
		m_szGrp[TLK_SMELL] =	"SC_SMELL";
	
		m_szGrp[TLK_WOUND] =	"SC_WOUND";
		m_szGrp[TLK_MORTAL] =	"SC_MORTAL";
	}

	// always this pitch...
	m_voicePitch = 95;
}

int CSirHenry :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	// make sure friends talk about it if player hurts Sir Henry...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CSirHenry::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CSirHenry :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CSirHenry :: PainSound ( void )
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
void CSirHenry :: DeathSound ( void )
{
	PainSound();
}


void CSirHenry::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams( );
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSirHenry :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( m_iHolding == GUN_KNIFE )
	{// throw a gun if the cultist has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity* pGun = DropItem( "weapon_knife", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		
		int bg = GetBodygroup(BODY_GROUP);
		m_iHolding = GUN_NONE;
		// set the correct body group for what Sir Henry is holding
		pev->body = NUM_BODY_GROUPS * m_iHolding + bg;
	}

	CTalkMonster :: GibMonster();
}

void CSirHenry :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CSirHenry :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that Sir Henry will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slSirHenryFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slSirHenryFollow;
	
	case SCHED_CANT_FOLLOW:
		return slSirHenryStopFollowing;

	case SCHED_PANIC:
		return slSirHenryPanic;

	case SCHED_TARGET_CHASE_SCARED:
		//return slSirHenryFollowScared;
		return slSirHenryFollow;

	case SCHED_TARGET_FACE_SCARED:
		return slSirHenryFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that Sir Henry will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleSirHenryStand;
		else
			return psched;

	case SCHED_HIDE:
		return slSirHenryHide;

	case SCHED_STARTLE:
		return slSirHenryStartle;

	case SCHED_FEAR:
		return slSirHenryFear;

	case SCHED_FAIL:
		if (GetBodygroup(BODY_GROUP) == BODY_CULTIST)
		{
			if (HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
			{
				return CTalkMonster :: GetScheduleOfType( SCHED_MELEE_ATTACK1 ); ;
			}
		}
		else
		{
			return CTalkMonster::GetScheduleOfType( Type );
		}
		break;
	case SCHED_RANGE_ATTACK1:
		if (GetBodygroup(BODY_GROUP) == BODY_CULTIST)
		{
			return slSirHenryAttack1;
		}
		else
		{
			return CTalkMonster::GetScheduleOfType( Type );
		}
	case SCHED_RANGE_ATTACK2:
		if (GetBodygroup(BODY_GROUP) == BODY_CULTIST)
		{
			return slSirHenryAttack1;
		}
		else
		{
			return CTalkMonster::GetScheduleOfType( Type );
		}
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CSirHenry :: GetSchedule ( void )
{
	ClearBeams( );

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
			if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
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
		/*
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if ( pSound )
			{
				if ( pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) )
				{
					return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
				}
			}
		}
		*/

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
			else	// UNDONE: When afraid, Sir Henry won't move out of your way.  Keep This?  If not, write move away scared
			{
				if ( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
				{
				//	return GetScheduleOfType( SCHED_FEAR );					// React to something scary
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );					// React to something scary
				}
				
				//return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
				return GetScheduleOfType( SCHED_TARGET_FACE );	// face and follow, but I'm scared!
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if (GetBodygroup(BODY_GROUP) == BODY_CULTIST)
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}
		}
		else
		{
			if ( HasConditions( bits_COND_NEW_ENEMY ) )
				//return slSirHenryFear;					// Point and scream!
				return slSirHenryCover;					// Point and scream!
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				return slSirHenryCover;		// Take Cover
			
			if ( HasConditions( bits_COND_HEAR_SOUND ) )
				return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

			return slSirHenryCover;			// Run & Cower
		}
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CSirHenry :: GetIdealState ( void )
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
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}

			}
		}
		break;
	}

	return CTalkMonster::GetIdealState();
}


int CSirHenry::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CSirHenry :: ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;
	
	if (m_iBeams >= SIRHENRY_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CSirHenry :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CSirHenry :: WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;
	
	if (m_iBeams >= SIRHENRY_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

void CSirHenry :: ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= SIRHENRY_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSirHenry :: GetGunPosition( )
{
	return pev->origin + Vector( 0, 0, 24 );
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CSirHenry :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (m_iHolding != GUN_KNIFE) 
	{
		return FALSE;
	}

	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CSirHenry :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	return CTalkMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CSirHenry :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CSirHenry :: ClearBeams( )
{
	for (int i = 0; i < SIRHENRY_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}

//=========================================================
// Dead SirHenry PROP
//=========================================================

char *CDeadSirHenry::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadSirHenry::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_sirhenry_dead, CDeadSirHenry );

//
// ********** DeadSirHenry SPAWN **********
//
void CDeadSirHenry :: Spawn( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/SirHenry.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/SirHenry.mdl");
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.SirHenryHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead Sir Henry with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting SirHenry PROP
//=========================================================


LINK_ENTITY_TO_CLASS( monster_sitting_sirhenry, CSittingSirHenry );
TYPEDESCRIPTION	CSittingSirHenry::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingSirHenry, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingSirHenry, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingSirHenry, CSirHenry );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


#define SF_SITTINGSCI_POSTDISASTER 1024

//
// ********** SirHenry SPAWN **********
//
void CSittingSirHenry :: Spawn( )
{
	PRECACHE_MODEL("models/SirHenry.mdl");
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/SirHenry.mdl");
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

	if (!FBitSet(pev->spawnflags, SF_SITTINGSCI_POSTDISASTER)) //LRC- allow a sitter to be postdisaster.
		SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!
	
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (SittingThink);
	SetNextThink( 0.1 );

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingSirHenry :: Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingSirHenry :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
}


int CSittingSirHenry::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingSirHenry :: SittingThink( void )
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
	SetNextThink( 0.1 );
}

// prepare sitting Sir Henry to answer a question
void CSittingSirHenry :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingSirHenry :: FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;
	
	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();
		
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
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
