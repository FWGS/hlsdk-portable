/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// human scientist (passive lab worker)
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
#include	"player.h"
#include	"weapons.h"

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
#define		TAKEBOY_AE_HEAL		( 1 )
#define		TAKEBOY_AE_NEEDLEON	( 2 )
#define		TAKEBOY_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CTakeBoy : public CTalkMonster
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

	void SetActivity ( Activity newActivity );
	Activity GetStoppedActivity( void );
	int ISoundMask( void );
	void DeclineFollowing( void );

	float	CoverRadius( void ) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 10; }

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );
	void RunAI( void );

	void			Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_fearTime;
	float m_attackTime;
};

LINK_ENTITY_TO_CLASS( monster_takeboy, CTakeBoy );

TYPEDESCRIPTION	CTakeBoy::m_SaveData[] = 
{
	DEFINE_FIELD( CTakeBoy, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CTakeBoy, m_fearTime, FIELD_TIME ),
	DEFINE_FIELD( CTakeBoy, m_attackTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CTakeBoy, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBoyFollow[] =
{
//	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBoyFollow[] =
{
	{
		tlBoyFollow,
		ARRAYSIZE ( tlBoyFollow ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlBoyFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)100		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slBoyFollowScared[] =
{
	{
		tlBoyFollowScared,
		ARRAYSIZE ( tlBoyFollowScared ),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlBoyFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slBoyFaceTargetScared[] =
{
	{
		tlBoyFaceTargetScared,
		ARRAYSIZE ( tlBoyFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlBoyStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slBoyStopFollowing[] =
{
	{
		tlBoyStopFollowing,
		ARRAYSIZE ( tlBoyStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlBoyFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBoyFaceTarget[] =
{
	{
		tlBoyFaceTarget,
		ARRAYSIZE ( tlBoyFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlBoyPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slBoyPanic[] =
{
	{
		tlBoyPanic,
		ARRAYSIZE ( tlBoyPanic ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_COND_ENEMY_DEAD,
		"BoyPanic"
	},
};


Task_t	tlIdleBoyStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBoyStand[] =
{
	{ 
		tlIdleBoyStand,
		ARRAYSIZE ( tlIdleBoyStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_CAN_MELEE_ATTACK1 |
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
		"IdleBoyStand"

	},
};


Task_t	tlTakeboyCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slTakeboyCover[] =
{
	{ 
		tlTakeboyCover,
		ARRAYSIZE ( tlTakeboyCover ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		0,
		"TakeCover"
	},
};



Task_t	tlTakeboyHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slTakeboyHide[] =
{
	{ 
		tlTakeboyHide,
		ARRAYSIZE ( tlTakeboyHide ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"TakeboyHide"
	},
};


Task_t	tlTakeboyStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slTakeboyStartle[] =
{
	{ 
		tlTakeboyStartle,
		ARRAYSIZE ( tlTakeboyStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"TakeboyStartle"
	},
};



Task_t	tlBoyFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slBoyFear[] =
{
	{ 
		tlBoyFear,
		ARRAYSIZE ( tlBoyFear ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};


DEFINE_CUSTOM_SCHEDULES( CTakeBoy )
{
	slBoyFollow,
	slBoyFaceTarget,
	slIdleBoyStand,
	slBoyFear,
	slTakeboyCover,
	slTakeboyHide,
	slTakeboyStartle,
	slBoyStopFollowing,
	slBoyPanic,
	slBoyFollowScared,
	slBoyFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CTakeBoy, CTalkMonster );


void CTakeBoy::DeclineFollowing( void )
{

}

Activity CTakeBoy::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CTakeBoy :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SCREAM:
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		TaskComplete();
		break;

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_WALK;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK, 0.5 ) )
					TaskFail();
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CTakeBoy :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( MovementIsComplete() )
			TaskComplete();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
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
int	CTakeBoy :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTakeBoy :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTakeBoy :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	CTalkMonster::HandleAnimEvent( pEvent );
}


//=========================================================
// RunAI
//=========================================================
void CTakeBoy :: RunAI( void )
{
	CBaseMonster :: RunAI();
}

//=========================================================
// Spawn
//=========================================================
void CTakeBoy :: Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/mrvalve_takeboy.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if(pev->health == 0){
	pev->health			= 120;
	}

	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	m_canbarnacle_mode  = 1;

	m_candrownwater = 1;

	m_lovehate          = 20;
	m_canheadcrab_mode  = 0;

	MonsterInit();
	SetUse( &CTakeBoy::FollowerUse );

//	m_forcefuckdoor  = TRUE;
	m_user_aimflag   = FALSE;

	m_cover_dist = 256;

	m_killed_exp = 10;
	m_rpgms_level = 10;

	pev->netname = MAKE_STRING( "TakeBoy" );
	m_rpgms_type = 0;//���ɼ������
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTakeBoy :: Precache( void )
{
	PRECACHE_MODEL("models/mrvalve_takeboy.mdl");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CTakeBoy :: TalkInit()
{
	
	CTalkMonster::TalkInit();

		m_szGrp[TLK_ANSWER]  =	NULL;
		m_szGrp[TLK_QUESTION] =	NULL;
		m_szGrp[TLK_IDLE] =		NULL;
		m_szGrp[TLK_STARE] =	NULL;
		m_szGrp[TLK_USE] =		NULL;
		m_szGrp[TLK_UNUSE] =	NULL;
		m_szGrp[TLK_STOP] =		NULL;
		m_szGrp[TLK_NOSHOOT] =	NULL;
		m_szGrp[TLK_HELLO] =	NULL;

		m_szGrp[TLK_PLHURT1] =	NULL;
		m_szGrp[TLK_PLHURT2] =	NULL;
		m_szGrp[TLK_PLHURT3] =	NULL;

		m_szGrp[TLK_PHELLO] =	NULL;
		m_szGrp[TLK_PIDLE] =	NULL;
		m_szGrp[TLK_PQUESTION] = NULL;
		m_szGrp[TLK_SMELL] =	NULL;
		
		m_szGrp[TLK_WOUND] =	NULL;
		m_szGrp[TLK_MORTAL] =	NULL;


		m_voicePitch = 100;
}

int CTakeBoy :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if(m_alert == 0 && m_hEnemy != NULL){
		m_alert = 100;
	}

	//if ( IsFollowing() && pev->weapons == 0 ){
	//StopFollowing( TRUE );
	//}
	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CTakeBoy :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CTakeBoy :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime )
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);
/*
	switch (RANDOM_LONG(0,2))
	{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
*/
}

//=========================================================
// DeathSound 
//=========================================================
void CTakeBoy :: DeathSound ( void )
{
/*
	switch (RANDOM_LONG(0,2))
	{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
*/
}

void CTakeBoy::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}


void CTakeBoy :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CTakeBoy :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBoyFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBoyFollow;
	
	case SCHED_CANT_FOLLOW:
		return slBoyStopFollowing;

	case SCHED_PANIC:
		return slBoyPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slBoyFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slBoyFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleBoyStand;
		else
			return psched;

	case SCHED_HIDE:
		return slTakeboyHide;

	case SCHED_STARTLE:
		return slTakeboyStartle;

	case SCHED_FEAR:
		return slBoyFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CTakeBoy :: GetSchedule ( void )
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
		if ( HasConditions( bits_COND_HEAR_SOUND ) && pev->weapons == 0 )
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
				//StopFollowing( FALSE );
				break;
			}

			if ( m_lovehate <= 0 )
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
			else	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
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
		
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// clear the current (dead) enemy and try to find another.
				m_hEnemy = NULL;

				if ( GetEnemy() )
				{
					ClearConditions( bits_COND_ENEMY_DEAD );
					return GetSchedule();
				}
				else
				{
					Forget( bits_MEMORY_FLINCHED );
					SetState( MONSTERSTATE_ALERT );
					return GetSchedule();
				}
			}


				if ( HasConditions( bits_COND_NEW_ENEMY ) )
					return slBoyFear;					// Point and scream!

				if ( HasConditions( bits_COND_SEE_ENEMY ) )
					return slTakeboyCover;		// Take Cover
				
				if ( HasConditions( bits_COND_HEAR_SOUND ) )
					return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

				return slTakeboyCover;			// Run & Cower


			break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CTakeBoy :: GetIdealState ( void )
{
	if(IsFollowing() && pev->weapons != 0){
	return CTalkMonster::GetIdealState();
	}

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
				//StopFollowing( TRUE );
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			//if ( IsFollowing() )
			//	StopFollowing( TRUE );
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
				if ( m_hTargetEnt != NULL && m_HenemyEnemyMe == 0)
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
