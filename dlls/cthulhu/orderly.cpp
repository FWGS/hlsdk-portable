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
// Orderly
//=========================================================

//=========================================================
// Hit groups!	
//=========================================================
/*

  1 - Head
  2 - Stomach
  3 - Gun

*/


#include	"extdll.h"
#include	"util.h"
#include	"plane.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"

int g_fOrderlyQuestion;				// true if an idle orderly asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define ORDERLY_VOL						0.70		// volume of orderly sounds
#define ORDERLY_ATTN					ATTN_NORM	// attenutation of orderlyist sentences
#define ORDERLY_LIMP_HEALTH				20
#define ORDERLY_DMG_HEADSHOT			( DMG_BULLET | DMG_CLUB )	// damage types that can kill a orderly with a single headshot.
#define ORDERLY_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	ORDERLY_SENTENCE_VOLUME			(float)0.70 // volume of orderly sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ORDERLY_AE_PUNCH		( 1 )
#define		ORDERLY_AE_CAUGHT_ENEMY	( 2 ) // orderly established sight with an enemy (player only) that had previously eluded the squad.

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_ORDERLY_SWEEP = LAST_COMMON_SCHEDULE + 1,
	SCHED_ORDERLY_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_ORDERLY_FOUND_ENEMY,
	SCHED_ORDERLY_WAIT_FACE_ENEMY,
	SCHED_ORDERLY_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_ORDERLY_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_ORDERLY_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_ORDERLY_SPEAK_SENTENCE,
	TASK_ORDERLY_CHECK_FIRE,
};

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_ORDERLY_NOFIRE	( bits_COND_SPECIAL1 )


#include "orderly.h"


LINK_ENTITY_TO_CLASS( monster_orderly, COrderly );

TYPEDESCRIPTION	COrderly::m_SaveData[] = 
{
	DEFINE_FIELD( COrderly, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( COrderly, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( COrderly, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( COrderly, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( COrderly, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COrderly, CSquadMonster );

const char *COrderly::pOrderlySentences[] = 
{
	"ORDERLY_GREN", // grenade scared orderly
	"ORDERLY_ALERT", // sees player
	"ORDERLY_COVER", // running to cover
	"ORDERLY_CHARGE",  // running out to get the enemy
	"ORDERLY_TAUNT", // say rude things
};

enum
{
	ORDERLY_SENT_NONE = -1,
	ORDERLY_SENT_GREN = 0,
	ORDERLY_SENT_ALERT,
	ORDERLY_SENT_COVER,
	ORDERLY_SENT_CHARGE,
	ORDERLY_SENT_TAUNT,
} ORDERLY_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some orderly sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a orderly says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the orderly has 
// started moving.
//=========================================================
void COrderly :: SpeakSentence( void )
{
	if ( m_iSentence == ORDERLY_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pOrderlySentences[ m_iSentence ], ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void COrderly :: GibMonster ( void )
{
	CBaseMonster :: GibMonster();
}

//=========================================================
// ISoundMask - Overidden for human orderlies because they 
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int COrderly :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL COrderly :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}
	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void COrderly :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = ORDERLY_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void COrderly :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden.
//=========================================================
BOOL COrderly :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL COrderly :: CheckMeleeAttack1 ( float flDot, float flDist )
{
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
// CheckRangeAttack1 - overridden for orderlies.
//=========================================================
BOOL COrderly :: CheckRangeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the orderlies (non-existant) second range
// attack. 
//=========================================================
BOOL COrderly :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void COrderly :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// TakeDamage - overridden for the orderly because the orderly
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
int COrderly :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );

	return CSquadMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COrderly :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
		ys = 150;		
		break;
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
		ys = 180;		
		break;
	case ACT_RANGE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_RANGE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK1:	
		ys = 120;	
		break;
	case ACT_MELEE_ATTACK2:	
		ys = 120;	
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:	
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

void COrderly :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fOrderlyQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fOrderlyQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_CHECK", ORDERLY_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fOrderlyQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_QUEST", ORDERLY_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fOrderlyQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_IDLE", ORDERLY_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fOrderlyQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_CLEAR", ORDERLY_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_ANSWER", ORDERLY_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fOrderlyQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COrderly :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_HUMAN_MILITARY;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void COrderly :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch( pEvent->event )
	{
		case ORDERLY_AE_PUNCH:
		{
			Vector oldorig = pev->origin;
			CBaseEntity *pHurt = NULL;
			// check down below in stages for snakes...
			for (int dz = 0; dz >=-3; dz--)
			{
				pev->origin = oldorig;
				pev->origin.z += dz * 12;
				pHurt = CheckTraceHullAttack( 70, gSkillData.priestDmgKnife, DMG_SLASH );
				if (pHurt) 
				{
					break;
				}
			}
			pev->origin = oldorig;
			if ( pHurt )
			{
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case ORDERLY_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "ORDERLY_ALERT", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void COrderly :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/orderly.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-16,-16,-36), Vector(16,16,36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.cultistHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= ORDERLY_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the orderly spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector ( 0, 0, 19 );

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COrderly :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/orderly.mdl");
	
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );

	PRECACHE_SOUND("zombie/claw_strike1.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND("zombie/claw_miss1.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;
}	

//=========================================================
// start task
//=========================================================
void COrderly :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_ORDERLY_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_ORDERLY_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_ORDERLY_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// orderly no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_ORDERLY_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CSquadMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void COrderly :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ORDERLY_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}
}

//=========================================================
// PainSound
//=========================================================
void COrderly :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );	
			break;
		case 1:
			EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );	
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void COrderly :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// OrderlyFail
//=========================================================
Task_t	tlOrderlyFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slOrderlyFail[] =
{
	{
		tlOrderlyFail,
		ARRAYSIZE ( tlOrderlyFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Orderly Fail"
	},
};

//=========================================================
// Orderly Combat Fail
//=========================================================
Task_t	tlOrderlyCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slOrderlyCombatFail[] =
{
	{
		tlOrderlyCombatFail,
		ARRAYSIZE ( tlOrderlyCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Orderly Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlOrderlyVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slOrderlyVictoryDance[] =
{
	{ 
		tlOrderlyVictoryDance,
		ARRAYSIZE ( tlOrderlyVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"OrderlyVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the orderly to attack.
//=========================================================
Task_t tlOrderlyEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_ORDERLY_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_ORDERLY_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slOrderlyEstablishLineOfFire[] =
{
	{ 
		tlOrderlyEstablishLineOfFire,
		ARRAYSIZE ( tlOrderlyEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"OrderlyEstablishLineOfFire"
	},
};

//=========================================================
// OrderlyFoundEnemy - orderly established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlOrderlyFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slOrderlyFoundEnemy[] =
{
	{ 
		tlOrderlyFoundEnemy,
		ARRAYSIZE ( tlOrderlyFoundEnemy ), 
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"OrderlyFoundEnemy"
	},
};

//=========================================================
// OrderlyCombatFace Schedule
//=========================================================
Task_t	tlOrderlyCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_WAIT,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ORDERLY_SWEEP	},
};

Schedule_t	slOrderlyCombatFace[] =
{
	{ 
		tlOrderlyCombatFace1,
		ARRAYSIZE ( tlOrderlyCombatFace1 ), 
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or Orderly gets hurt.
//=========================================================
Task_t	tlOrderlySignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ORDERLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ORDERLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ORDERLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ORDERLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_ORDERLY_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slOrderlySignalSuppress[] =
{
	{ 
		tlOrderlySignalSuppress,
		ARRAYSIZE ( tlOrderlySignalSuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_ORDERLY_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlOrderlySuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ORDERLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ORDERLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ORDERLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ORDERLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_ORDERLY_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slOrderlySuppress[] =
{
	{ 
		tlOrderlySuppress,
		ARRAYSIZE ( tlOrderlySuppress ), 
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_ORDERLY_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// Orderly wait in cover - we don't allow danger or the ability
// to attack to break a Orderly's run to cover schedule, but
// when a Orderly is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlOrderlyWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slOrderlyWaitInCover[] =
{
	{ 
		tlOrderlyWaitInCover,
		ARRAYSIZE ( tlOrderlyWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"OrderlyWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlOrderlyTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_ORDERLY_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_ORDERLY_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_ORDERLY_WAIT_FACE_ENEMY	},
};

Schedule_t	slOrderlyTakeCover[] =
{
	{ 
		tlOrderlyTakeCover1,
		ARRAYSIZE ( tlOrderlyTakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};


//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlOrderlyTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slOrderlyTakeCoverFromBestSound[] =
{
	{ 
		tlOrderlyTakeCoverFromBestSound,
		ARRAYSIZE ( tlOrderlyTakeCoverFromBestSound ), 
		0,
		0,
		"OrderlyTakeCoverFromBestSound"
	},
};

//=========================================================
// Orderly reload schedule
//=========================================================
Task_t	tlOrderlyHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slOrderlyHideReload[] = 
{
	{
		tlOrderlyHideReload,
		ARRAYSIZE ( tlOrderlyHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"OrderlyHideReload"
	}
};

//=========================================================
// Do a turning sweep of the area
//=========================================================
Task_t	tlOrderlySweep[] =
{
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
};

Schedule_t	slOrderlySweep[] =
{
	{ 
		tlOrderlySweep,
		ARRAYSIZE ( tlOrderlySweep ), 
		
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Orderly Sweep"
	},
};


DEFINE_CUSTOM_SCHEDULES( COrderly )
{
	slOrderlyFail,
	slOrderlyCombatFail,
	slOrderlyVictoryDance,
	slOrderlyEstablishLineOfFire,
	slOrderlyFoundEnemy,
	slOrderlyCombatFace,
	slOrderlySignalSuppress,
	slOrderlySuppress,
	slOrderlyWaitInCover,
	slOrderlyTakeCover,
	slOrderlyTakeCoverFromBestSound,
	slOrderlyHideReload,
	slOrderlySweep,
};

IMPLEMENT_CUSTOM_SCHEDULES( COrderly, CSquadMonster );

//=========================================================
// SetActivity 
//=========================================================
void COrderly :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_MELEE_ATTACK1:

		// randomly stand or crouch
		if (RANDOM_LONG(0,99) == 0)
			m_fStanding = 0;
		else
			m_fStanding = 1;
		 
		// a short enemy...probably a snake...
		if ((m_hEnemy != NULL) && (m_hEnemy->pev->maxs.z < 36))
		{
			m_fStanding = 0;
		}

		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "ref_shoot_kosh" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouch_shoot_kosh" );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= ORDERLY_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_RUN );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= ORDERLY_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_WALK );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
//			NewActivity = ACT_IDLE_ANGRY;
			NewActivity = ACT_IDLE;
		}
		iSequence = LookupActivity ( NewActivity );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			iSequence = LookupActivity ( ACT_IDLE );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *COrderly :: GetSchedule( void )
{

	// clear old sentence
	m_iSentence = ORDERLY_SENT_NONE;

	// orderlies place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the orderly's signal that a grenade has landed nearby,
				// and the orderly should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_GREN", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else 
					{
						//!!!KELLY - the leader of a squad of orderlies has just seen the player or a 
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight" 
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_ALERT", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
							/*
								else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) && 
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) && 
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_MONST", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
								*/

							JustSpoke();
						}
					}
				}
			}
			
// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this orderly was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_COVER", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
						m_iSentence = ORDERLY_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( OccupySlot( bits_SLOTS_ORDERLY_ENGAGE ) )
				{
					//!!!KELLY - orderly cannot see the enemy and has just decided to 
					// charge the enemy's position. 
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_CHARGE", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
						m_iSentence = ORDERLY_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_ORDERLY_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - orderly is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// orderly's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "ORDERLY_TAUNT", ORDERLY_SENTENCE_VOLUME, ORDERLY_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}
			
			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_ORDERLY_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}
	
	// no special cases here, call the base class
	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* COrderly :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				return &slOrderlyTakeCover[ 0 ];
			}
			else
			{
				return &slOrderlyTakeCover[ 0 ];
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slOrderlyTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_ORDERLY_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_ORDERLY_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_ORDERLY_ELOF_FAIL:
		{
			// human orderly is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_ORDERLY_ESTABLISH_LINE_OF_FIRE:
		{
			return &slOrderlyEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_COMBAT_FACE:
		{
			return &slOrderlyCombatFace[ 0 ];
		}
	case SCHED_ORDERLY_WAIT_FACE_ENEMY:
		{
			return &slOrderlyWaitInCover[ 0 ];
		}
	case SCHED_ORDERLY_SWEEP:
		{
			return &slOrderlySweep[ 0 ];
		}
	case SCHED_ORDERLY_FOUND_ENEMY:
		{
			return &slOrderlyFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slOrderlyFail[ 0 ];
				}
			}

			return &slOrderlyVictoryDance[ 0 ];
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// orderly has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slOrderlyCombatFail[ 0 ];
			}

			return &slOrderlyFail[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}


//=========================================================
// DEAD ORDERLY PROP
//=========================================================

char *CDeadOrderly::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadOrderly::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_orderly_dead, CDeadOrderly );

//=========================================================
// ********** DeadOrderly SPAWN **********
//=========================================================
void CDeadOrderly :: Spawn( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/orderly.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/orderly.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead orderly with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

	MonsterInitDead();
}
