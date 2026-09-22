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

#define		NUM_SCIENTIST_HEADS		5 // four heads available for scientist model
enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

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
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
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

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	float	CoverRadius( void ) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 10; }

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
	void RunAI( void );

	void			Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
	float m_attackTime;
};

LINK_ENTITY_TO_CLASS( monster_scientist, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_tr, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_mad, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_garry, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_civ, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_hos, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_cookie, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_yell, CScientist );

TYPEDESCRIPTION	CScientist::m_SaveData[] = 
{
	DEFINE_FIELD( CScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_fearTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_attackTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CScientist, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFollow[] =
{
//	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE ( tlFollow ),
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)100		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slFollowScared[] =
{
	{
		tlFollowScared,
		ARRAYSIZE ( tlFollowScared ),
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slFaceTargetScared[] =
{
	{
		tlFaceTargetScared,
		ARRAYSIZE ( tlFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE ( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)50		},	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_SAY_HEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_ARM	},			// Whip out the needle
	{ TASK_HEAL,				(float)0	},	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slHeal[] =
{
	{
		tlHeal,
		ARRAYSIZE ( tlHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};


Task_t	tlFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE ( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlSciPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSciPanic[] =
{
	{
		tlSciPanic,
		ARRAYSIZE ( tlSciPanic ),
		bits_COND_CAN_MELEE_ATTACK1,
		bits_COND_ENEMY_DEAD,
		"SciPanic"
	},
};


Task_t	tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleSciStand[] =
{
	{ 
		tlIdleSciStand,
		ARRAYSIZE ( tlIdleSciStand ), 
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
		"IdleSciStand"

	},
};


Task_t	tlScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slScientistCover[] =
{
	{ 
		tlScientistCover,
		ARRAYSIZE ( tlScientistCover ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		0,
		"TakeCover"
	},
};



Task_t	tlScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slScientistHide[] =
{
	{ 
		tlScientistHide,
		ARRAYSIZE ( tlScientistHide ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};


Task_t	tlScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slScientistStartle[] =
{
	{ 
		tlScientistStartle,
		ARRAYSIZE ( tlScientistStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ScientistStartle"
	},
};



Task_t	tlFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slFear[] =
{
	{ 
		tlFear,
		ARRAYSIZE ( tlFear ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};


DEFINE_CUSTOM_SCHEDULES( CScientist )
{
	slFollow,
	slFaceTarget,
	slIdleSciStand,
	slFear,
	slScientistCover,
	slScientistHide,
	slScientistStartle,
	slHeal,
	slStopFollowing,
	slSciPanic,
	slFollowScared,
	slFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CScientist, CTalkMonster );

void CScientist::DeclineFollowing( void )
{
	if(m_hEnemy != NULL){
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( "SC_POK", 2, VOL_NORM, ATTN_NORM );
	}
}


BOOL CScientist :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 65;

	if(pev->weapons == 0){
	return FALSE;
	}

	if(m_attackTime > gpGlobals->time)
		return FALSE;

	if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 128 )
		return FALSE;

	
	if(m_no_cover_mode == 1 || m_HenemyEnemyMe >= 1){
		dist = 75;
	}
	else if ( m_HenemyEnemyMe == 0 ){
		dist = 60;
	}

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= dist && flDot >= 0.7 && m_hEnemy != NULL )
	{
		return TRUE;
	}

	return FALSE;
}

void CScientist :: Scream( void )
{
	if ( FOkToSpeak() )
	{
		if(m_hEnemy != NULL){
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM );
		}
	}
}


Activity CScientist::GetStoppedActivity( void )
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CScientist :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		if ( FOkToSpeak() && !m_selfmode){
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "SC_HEAL", 2, VOL_NORM, ATTN_IDLE );
		}
		TaskComplete();
		break;

	case TASK_SCREAM:
		if(!m_longming && !m_selfmode){
		Scream();
		}
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		if ( RANDOM_FLOAT( 0, 1 ) < pTask->flData && !m_longming && !m_selfmode )
			Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if ( FOkToSpeak() && !m_longming && !m_selfmode )
		{
			if(m_hEnemy != NULL){
				Talk( 2 );
				m_hTalkTarget = m_hEnemy;
				if ( m_hEnemy->IsPlayer() ){
				PlaySentence( "SC_PLFEAR", 5, VOL_NORM, ATTN_NORM );
				}
				else{
				PlaySentence( "SC_FEAR", 5, VOL_NORM, ATTN_NORM );
				}

				Alert_Ally(m_hEnemy);//��������
			}
		}
		TaskComplete();
		break;

	case TASK_HEAL:
		m_IdealActivity = ACT_USE;
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

void CScientist :: RunTask( Task_t *pTask )
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
int	CScientist :: Classify ( void )
{
	if ( pev->team == 1 ){
		return	CLASS_HUMAN_MILITARY;
	}
	else{
		return	CLASS_PLAYER_ALLY;
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CScientist :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CScientist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1;
	dmg1 = 10;

	if(pev->max_health >= 100)
	dmg1 += 15;//���¿�ѧ�ң��˺�+15

	switch( pEvent->event )
	{		
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		{
		SetBodygroup( 2, 1 );
		}
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		{
		SetBodygroup( 2, 0 );
		}
		break;
	case 4:
		{
			if(pev->max_health < 200 && !m_guard_mode && m_HenemyEnemyMe >= 1)
			m_attackTime = gpGlobals->time + 2.0;

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
			{
				pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 150;
			}

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg1, DMG_CLUB );
				if ( pHurt )
				{
					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 150;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else if(m_hEnemy != NULL){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 75){
					m_hEnemy->TakeDamage( pev, pev, dmg1, DMG_CLUB );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
		}
		break;

	case 5:
		{
			SetBodygroup( 2, 4 );
		}
		break;

	case 6:
		{
			StopAnimation();
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}


//=========================================================
// RunAI
//=========================================================
void CScientist :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( FClassnameIs( pev, "monster_scientist_garry") && pev->frags == 1 ){
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->deadflag = DEAD_DEAD;
	SetThink( NULL );
	pev->sequence = 60;
	return;
	}

	if ( FClassnameIs( pev, "monster_scientist") ){
			if(pev->sequence == LookupSequence( "sitidle" ) || pev->sequence == LookupSequence( "kneel" )){
			m_duckseq = 4;
			}
			else if(m_duckseq != 0){
			m_duckseq = 0;
			}
			
			if(pev->weapons != 0){//��սAI?
				if(pev->max_health < 200 && !m_guard_mode && m_hEnemy != NULL){
					float distance;
					distance = ( m_hEnemy->pev->origin - pev->origin ).Length2D();
					if(m_HenemyEnemyMe >= 2){
						if ( m_pSchedule )
						{
								const char *pName = NULL;
								pName = m_pSchedule->pName;
								if ( !strcmp( pName, "Chase Enemy") ){
										TaskFail();
										if(m_attackTime <= gpGlobals->time ){
										m_attackTime = gpGlobals->time + 1.0;
										}
								}
						}
					}
				}
			}

			if(pev->weapons < 0){
				pev->weapons = RANDOM_LONG(1,2);
				SetBodygroup( 1, RANDOM_LONG(0,9));
				if( GetBodygroup( 1 ) == 2 ){
				pev->skin = 1;
				}
				else if(GetBodygroup( 1 ) == 7){
				pev->skin = 0;
				SetBodygroup( 0, 1);
				}
				else{
				pev->skin = 0;
				}
			}
			else if(pev->weapons == 1 && GetBodygroup( 2 ) != 2){
			SetBodygroup( 2, 2);
			}
			else if(pev->weapons == 2 && GetBodygroup( 2 ) != 3){
			SetBodygroup( 2, 3);
			}
	}

	if(m_makerspawn_call > 0){
		m_makerspawn_call = 0;
		int oldbody = pev->body;
		pev->body = 0;
		if ( oldbody < 0 )
		{
			SetBodygroup( 1, RANDOM_LONG(0,6));
		}
		else{
			SetBodygroup( 1, oldbody);
		}
	}

		if(m_killbyheadcrab >= 1){
				if ( GetBodygroup( 3 ) == 0 ){
				SetBodygroup( 3, 1 );
				pev->health = pev->max_health;
				}
				m_crabzombie_begain += 1;
				if(m_crabzombie_begain >= 80){
					if (IsMoving() || m_fightmode == 1){
					m_crabzombie_begain -= 10;
					return;
					}
					Vector trace_origin;
					trace_origin = pev->origin + Vector(0,0,38);
					TraceResult trace;
					UTIL_TraceHull(trace_origin, trace_origin, dont_ignore_monsters, human_hull, ENT(pev),&trace);
					if ( trace.fStartSolid )
					{
					m_crabzombie_begain -= 10;
					return;
					}
					SpawnBlood(Center(), BloodColor(), 250);
					CBaseEntity *pZombie = Create( "monster_zombie", pev->origin, pev->angles, edict() );
					pZombie->pev->angles.x = 0;
					pZombie->pev->angles.z = 0;
					CBaseMonster *pMonster = pZombie->MyMonsterPointer( );
					pMonster->SetActivity( ACT_FALL );
					UTIL_Remove( this );
					m_killbyheadcrab = 0;
					return;
				}
	}
}

//=========================================================
// Spawn
//=========================================================
void CScientist :: Spawn( void )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if(pev->health == 0){
	pev->health			= 30;
	m_headdef			= 1;//������ͷ��
	}

	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	m_canbarnacle_mode  = 1;

	m_candrownwater = 1;
	
	if ( FClassnameIs( pev, "monster_scientist") || FClassnameIs( pev, "monster_scientist_tr") ){
		int oldbody = pev->body;
		pev->body = 0;
		if ( oldbody < 0 )
		{
			SetBodygroup( 1, RANDOM_LONG(0,9));
		}
		else{
			SetBodygroup( 1, oldbody);
		}

		if(oldbody == 2){
		pev->skin = 1;
		}
		else if(oldbody == 7){
		pev->skin = 0;
		SetBodygroup( 0, 1);
		}
		else{
		pev->skin = 0;
		}
		
		m_lovehate          = 20;
		m_canheadcrab_mode  = 1;
	}

	if ( FClassnameIs( pev, "monster_scientist_mad")){//ʳѩ��
	SET_MODEL(ENT(pev), "models/scientist_mad.mdl");
	}

	if ( FClassnameIs( pev, "monster_scientist_civ") ){
	SET_MODEL(ENT(pev), "models/scientist_civ.mdl");
	pev->health			= 40;
	pev->body = 0;
	m_canheadcrab_mode  = 0;
	m_lovehate          = 20;
	}
	else if ( FClassnameIs( pev, "monster_scientist_garry") ){
	SET_MODEL(ENT(pev), "models/scientist_garry.mdl");
	pev->health			= 90;
	pev->body = 0;
	m_canheadcrab_mode  = 0;
	m_lovehate          = 30;
	}
	else if ( FClassnameIs( pev, "monster_scientist_hos") ){
	SET_MODEL(ENT(pev), "models/scientist_hos.mdl");
	pev->health			= 40;
	pev->body = 0;
	m_canheadcrab_mode  = 0;
	m_lovehate          = 20;
	}
	else if ( FClassnameIs( pev, "monster_scientist_cookie") ){
	SET_MODEL(ENT(pev), "models/scientist_cookie.mdl");
	pev->health			= 40;
	pev->body = 0;
	m_canheadcrab_mode  = 0;
	m_lovehate          = 20;
	}
	else if ( FClassnameIs( pev, "monster_scientist_yell") ){
	SET_MODEL(ENT(pev), "models/scientist_yell.mdl");
	pev->health			= 100;
	pev->body = 0;
	m_canheadcrab_mode  = 0;
	m_lovehate          = 30;
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	MonsterInit();
	SetUse( &CScientist::FollowerUse );

	m_forcefuckdoor  = TRUE;
	m_user_aimflag   = FALSE;

	if (FClassnameIs( pev, "monster_scientist_garry") || 
	FClassnameIs( pev, "monster_scientist_yell") || 
	FClassnameIs( pev, "monster_scientist_hos") ){
	m_MoveFail_FuckRoad = TRUE;
	}

	if ( FClassnameIs( pev, "monster_scientist_tr") ){
	pev->takedamage = DAMAGE_NO;
	m_godmode = TRUE;
	}

	m_cover_dist = 256;

	if(pev->weapons == 0){
		m_killed_exp = 20;
		m_rpgms_actor = 7;
		m_rpgms_level = 1;
		m_rpgms_exp = 0;
		m_rpgms_type = 1;
		pev->netname = MAKE_STRING( "Scientist" );
		m_rpgms_skill1_learn = 20;//ֹʹҩ
	}
	else{
		if(pev->health <= 30){//Ѫ������!
		pev->health	= 60;
		pev->max_health	= pev->health;
		}

		m_killed_exp = 30;
		m_rpgms_actor = 7;
		m_rpgms_level = 3;
		m_rpgms_exp = 0;
		m_rpgms_type = 1;
		m_headdef = 0;
		m_killed_exp = 10;

		pev->netname = MAKE_STRING( "Scientist" );
	}

	if(FClassnameIs( pev, "monster_scientist_yell")){
		m_rpgms_actor = 5;
		m_rpgms_exp = 0;
		m_rpgms_type = 1;
		m_rpgms_level = 4;
		pev->netname = MAKE_STRING( "Yell" );
		m_rpgms_skill1_learn = 15;
		m_killed_exp = 150;//Bug Fix 3.0 ɱ�������и��ྭ��ֵ�ζ�ħ������
		m_headdef = 0;
	}

	if(FClassnameIs( pev, "monster_scientist_hos") 
	|| FClassnameIs( pev, "monster_scientist_cookie")
	|| FClassnameIs( pev, "monster_scientist_civ")
	|| FClassnameIs( pev, "monster_scientist_garry")){
		pev->netname = MAKE_STRING( "Person" );
		m_rpgms_type = 0;//���ɼ������
	}

}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CScientist :: Precache( void )
{
	UTIL_PrecacheOther( "monster_zombie" );

	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_MODEL("models/scientist_mad.mdl");
	PRECACHE_MODEL("models/scientist_yell.mdl");
	PRECACHE_MODEL("models/scientist_garry.mdl");
	PRECACHE_MODEL("models/scientist_civ.mdl");
	PRECACHE_MODEL("models/scientist_hos.mdl");
	PRECACHE_MODEL("models/scientist_cookie.mdl");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	PRECACHE_SOUND("scientist/scream01.wav");
	PRECACHE_SOUND("scientist/scream02.wav");
	PRECACHE_SOUND("scientist/scream7.wav");
	
	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CScientist :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:
	if ( FClassnameIs( pev, "monster_scientist_yell") ){
		// scientists speach group names (group names are in sentences.txt)

		m_szGrp[TLK_ANSWER]  =	NULL;
		m_szGrp[TLK_QUESTION] =	NULL;
		m_szGrp[TLK_IDLE] =		NULL;
		m_szGrp[TLK_STARE] =	NULL;
		m_szGrp[TLK_USE] =		"YELL_OK";
		m_szGrp[TLK_UNUSE] =	"YELL_WAIT";
		m_szGrp[TLK_STOP] =		"YELL_STOP";
		m_szGrp[TLK_NOSHOOT] =	NULL;
		m_szGrp[TLK_HELLO] =	NULL;

		m_szGrp[TLK_PLHURT1] =	NULL;
		m_szGrp[TLK_PLHURT2] =	NULL;
		m_szGrp[TLK_PLHURT3] =	NULL;

		m_szGrp[TLK_PHELLO] =	NULL;
		m_szGrp[TLK_PIDLE] =	NULL;
		m_szGrp[TLK_PQUESTION] = NULL;
		m_szGrp[TLK_SMELL] =	"YELL_SMELL";
		
		m_szGrp[TLK_WOUND] =	"YELL_WOUND";
		m_szGrp[TLK_MORTAL] =	"YELL_MORTAL";


		m_voicePitch = 100;
	}
	else if ( FClassnameIs( pev, "monster_scientist") ){
		if ( FClassnameIs( pev, "monster_scientist_tr") ){
		m_szFriends[0] = NULL;
		m_szFriends[1] = NULL;
		}
		else{
		m_szFriends[0] = "monster_scientist";
		m_szFriends[1] = "monster_barney";
		}

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"SC_ANSWER";
	m_szGrp[TLK_QUESTION] =	"SC_QUESTION";
	m_szGrp[TLK_IDLE] =		"SC_IDLE";
	m_szGrp[TLK_STARE] =	"SC_STARE";
	m_szGrp[TLK_USE] =		"SC_OK";
	m_szGrp[TLK_UNUSE] =	"SC_WAIT";
	m_szGrp[TLK_STOP] =		"SC_STOP";
	m_szGrp[TLK_NOSHOOT] =	"SC_SCARED";
	m_szGrp[TLK_HELLO] =	"NULL";

	m_szGrp[TLK_PLHURT1] =	"!SC_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!SC_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!SC_CUREC";

	m_szGrp[TLK_PHELLO] =	"NULL";
	m_szGrp[TLK_PIDLE] =	"SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
	m_szGrp[TLK_SMELL] =	"NULL";
	
	m_szGrp[TLK_WOUND] =	"SC_WOUND";
	m_szGrp[TLK_MORTAL] =	"SC_MORTAL";


	m_voicePitch = 100;

	}
	else{
		// scientists speach group names (group names are in sentences.txt)

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
}

int CScientist :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if(pev->max_health < 200 && !m_guard_mode && m_attackTime <= gpGlobals->time && pev->weapons != 0 && m_HenemyEnemyMe >= 1){
		if(!IsFollowing() && IsMoving()){
		RouteClear();
		m_attackTime = gpGlobals->time + 4.0;
		}
	}

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
int CScientist :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}
	
//=========================================================
// PainSound
//=========================================================
void CScientist :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime )
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	if( FClassnameIs( pev, "monster_scientist_yell") ){
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
	else{
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CScientist :: DeathSound ( void )
{
	if( FClassnameIs( pev, "monster_scientist_yell") ){
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
	else{
		switch (RANDOM_LONG(0,2))
		{
		case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/scream01.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/scream02.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/scream7.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
}

void CScientist::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}


void CScientist :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CScientist :: GetScheduleOfType ( int Type )
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
			return slFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slFollow;
	
	case SCHED_CANT_FOLLOW:
		return slStopFollowing;

	case SCHED_PANIC:
		return slSciPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleSciStand;
		else
			return psched;

	case SCHED_HIDE:
		return slScientistHide;

	case SCHED_STARTLE:
		return slScientistStartle;

	case SCHED_FEAR:
		return slFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CScientist :: GetSchedule ( void )
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

	if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) ){
	return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
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
				StopFollowing( FALSE );
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
					if ( CanHeal() )	// Heal opportunistically
						return slHeal;
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

			if ( !HasConditions(bits_COND_CAN_MELEE_ATTACK1) && pev->weapons != 0){
				if ( m_hEnemy != NULL && m_attackTime <= gpGlobals->time){

					if(m_guard_mode)//����ģʽ
					return GetScheduleOfType( SCHED_COMBAT_FACE );

					if ( HasConditions(bits_COND_SEE_ENEMY) ){
						float distance;
						distance = ( m_hEnemy->pev->origin - pev->origin ).Length2D();
						if(m_HenemyEnemyMe == 0){//�޷��գ��ɾ�����
							if(distance <= 512 
							|| FClassnameIs( m_hEnemy->pev, "monster_zombie" ) 
							|| FClassnameIs( m_hEnemy->pev, "monster_houndeye" ) 
							|| FClassnameIs( m_hEnemy->pev, "monster_alien_slave" ) 
							|| FClassnameIs( m_hEnemy->pev, "monster_headcrab" ) 
							|| FClassnameIs( m_hEnemy->pev, "monster_zombie_barney" )){
							return GetScheduleOfType( SCHED_CHASE_ENEMY );
							}
							else{
							return GetScheduleOfType( SCHED_COMBAT_FACE );//������Ҳ������
							}
						}
						else if(m_HenemyEnemyMe == 1){//�ͷ��գ�С�Ľ���Щ����������
							if(distance <= 384){
							return GetScheduleOfType( SCHED_CHASE_ENEMY );
							}
						}
						else if(m_HenemyEnemyMe >= 2){//�ܣ��ܲ��˲���
							if(distance <= 128){
							return GetScheduleOfType( SCHED_CHASE_ENEMY );
							}
						}
					}
				}
			}

			if ( m_HenemyEnemyMe >= 1 || pev->weapons == 0 || !IsFollowing() ){
				if ( HasConditions( bits_COND_NEW_ENEMY ) )
					return slFear;					// Point and scream!

				if ( HasConditions( bits_COND_SEE_ENEMY ) )
					return slScientistCover;		// Take Cover
				
				if ( HasConditions( bits_COND_HEAR_SOUND ) )
					return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

				return slScientistCover;			// Run & Cower
			}
			else{
				return GetScheduleOfType( SCHED_COMBAT_FACE );
			}

			break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CScientist :: GetIdealState ( void )
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



BOOL CScientist::CanHeal( void )
{ 
	if ( pev->weapons != 0 || (m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.6))
	||	FClassnameIs( pev, "monster_scientist_civ") 
    ||	FClassnameIs( pev, "monster_scientist_cookie") 
	|| FClassnameIs( pev, "monster_scientist_hos") 
	|| FClassnameIs( pev, "monster_scientist_yell")
	|| (pev->spawnflags & SF_MONSTER_GAG) )
		return FALSE;

	return TRUE;
}

void CScientist::Heal( void )
{
	if ( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if ( target.Length() > 100 )
		return;

	CBasePlayer *player = GetClassPtr((CBasePlayer *)m_hTargetEnt->pev);
	if(player){
		if(player->m_needleheal2 == 0){
		player->m_needleheal2 += (int)player->pev->max_health * 0.2;
		// Don't heal again for 2 minute
		m_healTime = gpGlobals->time + 120;
		}
	}
}

int CScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}


//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};
char *CDeadScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadScientist::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_scientist_dead, CDeadScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_civ_dead, CDeadScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_yell_dead, CDeadScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_hos_dead, CDeadScientist );
//
// ********** DeadScientist SPAWN **********
//
void CDeadScientist :: Spawn( )
{
	PRECACHE_MODEL("models/scientist.mdl");
	PRECACHE_MODEL("models/scientist_civ.mdl");
	PRECACHE_MODEL("models/scientist_yell.mdl");
	PRECACHE_MODEL("models/scientist_hos.mdl");

	if ( FClassnameIs( pev, "monster_scientist_civ_dead") ){
	SET_MODEL(ENT(pev), "models/scientist_civ.mdl");
	}
	else if ( FClassnameIs( pev, "monster_scientist_yell_dead") ){
	SET_MODEL(ENT(pev), "models/scientist_yell.mdl");
	}
	else if ( FClassnameIs( pev, "monster_scientist_hos_dead") ){
	SET_MODEL(ENT(pev), "models/scientist_hos.mdl");
	}
	else{
	SET_MODEL(ENT(pev), "models/scientist.mdl");

		int oldbody = pev->body;
		pev->body = 0;
		if ( oldbody < 0 )
		{
			SetBodygroup( 1, RANDOM_LONG(0,6));
		}
		else{
			SetBodygroup( 1, oldbody);
		}

		if(oldbody == 2){
		pev->skin = 1;
		}
		else{
		pev->skin = 0;
		}

	}

	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 10;//gSkillData.scientistHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead scientist with bad pose\n" );
	}

	m_die_dont_move = 1;

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Scientist PROP
//=========================================================

class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
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

LINK_ENTITY_TO_CLASS( monster_sitting_scientist, CSittingScientist );
TYPEDESCRIPTION	CSittingScientist::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingScientist, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingScientist, CScientist );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


//
// ********** Scientist SPAWN **********
//
void CSittingScientist :: Spawn( )
{
	PRECACHE_MODEL("models/scientist.mdl");
	SET_MODEL(ENT(pev), "models/scientist.mdl");
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

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS-1);// pick a head, any head
	}
	// Luther is black, make his hands black
	if ( pev->body == HEAD_LUTHER )
		pev->skin = 1;
	
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (&CSittingScientist::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingScientist :: Precache( void )
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingScientist :: Classify ( void )
{
	if ( pev->team == 1 ){
		return	CLASS_HUMAN_MILITARY;
	}
	else{
		return	CLASS_PLAYER_ALLY;
	}
}


int CSittingScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingScientist :: SittingThink( void )
{
	CBaseEntity *pent;	

	StudioFrameAdvance( );

	// try to greet player
	if (FIdleHello() && m_lovehate != 810)
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

// prepare sitting scientist to answer a question
void CSittingScientist :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingScientist :: FIdleSpeak ( void )
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
	//	SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// otherwise, play an idle statement
	if (RANDOM_LONG(0,1))
	{
	//	SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}


class CMeatHostage : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_HUMAN_MILITARY; }
	void Killed( entvars_t *pevAttacker, int iGib );
};

LINK_ENTITY_TO_CLASS( monster_meat_hos, CMeatHostage );
LINK_ENTITY_TO_CLASS( monster_meat_hos_dead, CMeatHostage );
//
// ********** DeadScientist SPAWN **********
//
void CMeatHostage :: Spawn( )
{
	PRECACHE_MODEL("models/scientist_hos.mdl");
	SET_MODEL(ENT(pev), "models/scientist_hos.mdl");
	pev->body = 0;
	pev->skin = 0;
	pev->effects		= 0;
	pev->health			= RANDOM_LONG(50,125);
	pev->takedamage		= DAMAGE_YES;
	pev->friction		= 1.0;

	InitBoneControllers();

	pev->solid			= SOLID_BBOX;

	m_bloodColor = BLOOD_COLOR_RED;

	if ( FClassnameIs( pev, "monster_meat_hos_dead")){
	pev->sequence = LookupSequence( "lying_on_back" );
	m_die = 1;
	pev->health = 10;
	pev->deadflag = DEAD_DEAD;
	}
	else{
	pev->sequence = LookupSequence( "rocketcrawl" );
	m_die = 0;
	pev->deadflag = DEAD_NO;
	pev->gravity = 1.6;//Bug Fix 3.0 ���ⱻ�ƶ���ס���ʹ�
	}

	ResetSequenceInfo( );
	pev->framerate = 0;

	SetBits (pev->flags, FL_MONSTER);
	
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
}

void CMeatHostage::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
	m_die = 1;
	pev->sequence = LookupSequence( "lying_on_back" );
	}
	if ( ShouldGibMonster( iGib ) && m_gibed == 0 ){
	CallGibMonster();
	}
}