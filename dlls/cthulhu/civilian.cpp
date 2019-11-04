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
// human civilian (passive lab worker)
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


#define		NUM_CIVILIAN_BODIES		6 // six bodies available for civilian model
enum 
{ 
	BODY_DOCTOR = 0, 
	BODY_GANGSTER = 1, 
	BODY_JOURNALIST = 2, 
	BODY_MI5 = 3, 
	BODY_PATIENT = 4, 
	BODY_WORKER = 5 
};

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
#define		DOCTOR_AE_HEAL		( 1 )
#define		DOCTOR_AE_NEEDLEON	( 2 )
#define		DOCTOR_AE_NEEDLEOFF	( 3 )

//=======================================================
// civilian
//=======================================================

class CCivilian : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int FriendNumber( int arrayNumber );
	void SetActivity ( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void DeclineFollowing( void );

	float	CoverRadius( void ) { return 1200; }		// Need more room for cover because civilians want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal( void );
	void	Heal( void );
	void	Scream( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	
	void TalkInit( void );

	void			Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS( monster_civilian, CCivilian );

TYPEDESCRIPTION	CCivilian::m_SaveData[] = 
{
	DEFINE_FIELD( CCivilian, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CCivilian, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CCivilian, m_fearTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CCivilian, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlCivFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slCivFollow[] =
{
	{
		tlCivFollow,
		ARRAYSIZE ( tlCivFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"CivFollow"
	},
};

Task_t	tlCivFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slCivFollowScared[] =
{
	{
		tlCivFollowScared,
		ARRAYSIZE ( tlCivFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"CivFollowScared"
	},
};

Task_t	tlCivFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slCivFaceTargetScared[] =
{
	{
		tlCivFaceTargetScared,
		ARRAYSIZE ( tlCivFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"CivFaceTargetScared"
	},
};

Task_t	tlCivStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slCivStopFollowing[] =
{
	{
		tlCivStopFollowing,
		ARRAYSIZE ( tlCivStopFollowing ),
		0,
		0,
		"CivStopFollowing"
	},
};


Task_t	tlDocHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)50		},	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_SAY_HEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_ARM	},			// Whip out the needle
	{ TASK_HEAL,				(float)0	},	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slDocHeal[] =
{
	{
		tlDocHeal,
		ARRAYSIZE ( tlDocHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"DocHeal"
	},
};


Task_t	tlCivFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slCivFaceTarget[] =
{
	{
		tlCivFaceTarget,
		ARRAYSIZE ( tlCivFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"CivFaceTarget"
	},
};


Task_t	tlCivPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slCivPanic[] =
{
	{
		tlCivPanic,
		ARRAYSIZE ( tlCivPanic ),
		0,
		0,
		"CivPanic"
	},
};


Task_t	tlIdleCivStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleCivStand[] =
{
	{ 
		tlIdleCivStand,
		ARRAYSIZE ( tlIdleCivStand ), 
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
		"IdleCivStand"

	},
};


Task_t	tlCivilianCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slCivilianCover[] =
{
	{ 
		tlCivilianCover,
		ARRAYSIZE ( tlCivilianCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"CivilianCover"
	},
};



Task_t	tlCivilianHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slCivilianHide[] =
{
	{ 
		tlCivilianHide,
		ARRAYSIZE ( tlCivilianHide ), 
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"CivilianHide"
	},
};


Task_t	tlCivilianStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slCivilianStartle[] =
{
	{ 
		tlCivilianStartle,
		ARRAYSIZE ( tlCivilianStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"CivilianStartle"
	},
};



Task_t	tlCivFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slCivFear[] =
{
	{ 
		tlCivFear,
		ARRAYSIZE ( tlCivFear ), 
		bits_COND_NEW_ENEMY,
		0,
		"CivFear"
	},
};


DEFINE_CUSTOM_SCHEDULES( CCivilian )
{
	slCivFollow,
	slCivFaceTarget,
	slIdleCivStand,
	slCivFear,
	slCivilianCover,
	slCivilianHide,
	slCivilianStartle,
	slDocHeal,
	slCivStopFollowing,
	slCivPanic,
	slCivFollowScared,
	slCivFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CCivilian, CTalkMonster );


void CCivilian::DeclineFollowing( void )
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM ); //LRC
}


void CCivilian :: Scream( void )
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM );
	}
}


Activity CCivilian::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CCivilian :: StartTask( Task_t *pTask )
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

void CCivilian :: RunTask( Task_t *pTask )
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
int	CCivilian :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCivilian :: SetYawSpeed ( void )
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
void CCivilian :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case DOCTOR_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case DOCTOR_AE_NEEDLEON:
		{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_CIVILIAN_BODIES) + NUM_CIVILIAN_BODIES * 1;
		}
		break;
	case DOCTOR_AE_NEEDLEOFF:
		{
		int oldBody = pev->body;
		pev->body = (oldBody % NUM_CIVILIAN_BODIES) + NUM_CIVILIAN_BODIES * 0;
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CCivilian :: Spawn( void )
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/civilian.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= gSkillData.scientistHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so civilians will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_BODIES-1);// pick a head, any head
	}
	
	MonsterInit();
	SetUse( FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCivilian :: Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/civilian.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	// every new civilian must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CCivilian :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// civilian will try to talk to friends in this order:

	m_szFriends[0] = "monster_civilian";
	m_szFriends[1] = "monster_sitting_civilian";
	m_szFriends[2] = "monster_policeman";
	m_szFriends[3] = "monster_scientist";
	m_szFriends[4] = "monster_sitting_scientist";

	// civilians speach group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_ANSWER]  =	"CIV_ANSWER";
		m_szGrp[TLK_QUESTION] =	"CIV_QUESTION";
		m_szGrp[TLK_IDLE] =		"CIV_IDLE";
		m_szGrp[TLK_STARE] =	"SC_STARE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_USE] =	"SC_PFOLLOW";
		else
			m_szGrp[TLK_USE] =	"CIV_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "SC_PWAIT";
		else
			m_szGrp[TLK_UNUSE] = "SC_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] =	"CIV_POK";
		else
			m_szGrp[TLK_DECLINE] =	"SC_NOTOK";
		m_szGrp[TLK_STOP] =		"SC_STOP";
		m_szGrp[TLK_NOSHOOT] =	"SC_SCARED";
		m_szGrp[TLK_HELLO] =	"CIV_HELLO";

		m_szGrp[TLK_PLHURT1] =	"!SC_CUREA";
		m_szGrp[TLK_PLHURT2] =	"!SC_CUREB"; 
		m_szGrp[TLK_PLHURT3] =	"!SC_CUREC";

		m_szGrp[TLK_PHELLO] =	"CIV_PHELLO";
		m_szGrp[TLK_PIDLE] =	"CIV_PIDLE";
		m_szGrp[TLK_PQUESTION] = "CIV_PQUEST";
		m_szGrp[TLK_SMELL] =	"CIV_SMELL";
	
		m_szGrp[TLK_WOUND] =	"SC_WOUND";
		m_szGrp[TLK_MORTAL] =	"SC_MORTAL";
	}

	// get voice for head
	switch (pev->body % 3)
	{
	default:
	case BODY_DOCTOR:		m_voicePitch = 105; break;	
	case BODY_GANGSTER:		m_voicePitch = 95; break;	
	case BODY_JOURNALIST:	m_voicePitch = 100;  break;	
	case BODY_MI5:			m_voicePitch = 105;  break;
	case BODY_PATIENT:		m_voicePitch = 100;  break;	
	case BODY_WORKER:		m_voicePitch = 95;  break;
	}
}

int CCivilian :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}

	// make sure friends talk about it if player hurts civilian...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CCivilian :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CCivilian :: PainSound ( void )
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
void CCivilian :: DeathSound ( void )
{
	PainSound();
}


void CCivilian::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}


void CCivilian :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CCivilian :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that civilian will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slCivFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slCivFollow;
	
	case SCHED_CANT_FOLLOW:
		return slCivStopFollowing;

	case SCHED_PANIC:
		return slCivPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slCivFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slCivFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that civilian will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleCivStand;
		else
			return psched;

	case SCHED_HIDE:
		return slCivilianHide;

	case SCHED_STARTLE:
		return slCivilianStartle;

	case SCHED_FEAR:
		return slCivFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CCivilian :: GetSchedule ( void )
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
					if ( CanHeal() )	// Heal opportunistically
						return slDocHeal;
					if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
						return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else	// UNDONE: When afraid, civilian won't move out of your way.  Keep This?  If not, write move away scared
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
			return slCivFear;					// Point and scream!
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slCivilianCover;		// Take Cover
		
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slCivilianCover;			// Run & Cower
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CCivilian :: GetIdealState ( void )
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


BOOL CCivilian::CanHeal( void )
{
	if (!(pev->spawnflags & SF_DOCTOR_CAN_HEAL))
		return FALSE;

	// are we a doctor or a doctor holding a needle
	if ( (pev->body != BODY_DOCTOR) && (pev->body != BODY_DOCTOR + NUM_CIVILIAN_BODIES) )
		return FALSE;

	if ( (m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.5)) )
		return FALSE;

	return TRUE;
}

void CCivilian::Heal( void )
{
	if ( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if ( target.Length() > 100 )
		return;

	m_hTargetEnt->TakeHealth( gSkillData.scientistHeal, DMG_GENERIC );
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
}

int CCivilian::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}


//=========================================================
// Dead Civilian PROP
//=========================================================
class CDeadCivilian : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadCivilian::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadCivilian::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_civilian_dead, CDeadCivilian );

//
// ********** CDeadCivilian SPAWN **********
//
void CDeadCivilian :: Spawn( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/civilian.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/civilian.mdl");
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.civilianHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_BODIES-1);// pick a head, any head
	}

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_debug, "Dead civilian with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Civilian PROP
//=========================================================

class CSittingCivilian : public CCivilian // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn( void );
	void  Precache( void );

	void EXPORT SittingThink( void );
	int	Classify ( void );
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion( CTalkMonster *pSpeaker );
	int FriendNumber( int arrayNumber );

	int FIdleSpeak ( void );
	int		m_baseSequence;	
	int		m_headTurn;
	float	m_flResponseDelay;
};

LINK_ENTITY_TO_CLASS( monster_sitting_civilian, CSittingCivilian );
TYPEDESCRIPTION	CSittingCivilian::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingCivilian, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingCivilian, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingCivilian, CCivilian );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


#define SF_SITTINGCIV_POSTDISASTER 1024

//
// ********** Civilian SPAWN **********
//
void CSittingCivilian :: Spawn( )
{
	PRECACHE_MODEL("models/civilian.mdl");
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/civilian.mdl");
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

	if (!FBitSet(pev->spawnflags, SF_SITTINGCIV_POSTDISASTER)) //LRC- allow a sitter to be postdisaster.
		SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_BODIES-1);// pick a head, any head
	}
	
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (SittingThink);
	SetNextThink( 0.1 );

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingCivilian :: Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingCivilian :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_HUMAN_PASSIVE;
}


int CSittingCivilian::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingCivilian :: SittingThink( void )
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

// prepare sitting civilian to answer a question
void CSittingCivilian :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingCivilian :: FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;
	
	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();
		
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting civilians nearby
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
