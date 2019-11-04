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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include    "flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"weapons.h"
#include	"stukagrenade.h"

#include "Stukabat.h"


LINK_ENTITY_TO_CLASS( monster_stukabat, CStukabat );
LINK_ENTITY_TO_CLASS( info_node_stukabat, CStukabatNode );


//=========================================================
// stukabat nodes
//=========================================================


TYPEDESCRIPTION	CStukabatNode::m_SaveData[] = 
{
	DEFINE_FIELD( CStukabatNode, mbInUse, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CStukabatNode, CBaseEntity );

//=========================================================
// nodes start out as ents in the world. As they are spawned,
// the node info is recorded then the ents are discarded.
//=========================================================
void CStukabatNode :: KeyValue( KeyValueData *pkvd )
{
	CBaseEntity::KeyValue( pkvd );
}

//=========================================================
//=========================================================
void CStukabatNode :: Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;// always solid_not 
	mbInUse = false;
	pev->classname	= MAKE_STRING("info_node_stukabat");
}


//=========================================================
// stukabat
//=========================================================

#define SEARCH_RETRY	16

#define STUKABAT_SPEED 200

extern CGraph WorldGraph;


#define STUKABAT_AE_BOMB		1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================


TYPEDESCRIPTION	CStukabat::m_SaveData[] = 
{
	DEFINE_FIELD( CStukabat, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CStukabat, m_idealDist, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flNextMeleeAttack, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flNextRangedAttack, FIELD_FLOAT ),
	//DEFINE_FIELD( CStukabat, m_bOnAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CStukabat, m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flMinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CStukabat, m_flNextAlert, FIELD_TIME ),
	DEFINE_FIELD( CStukabat, m_flLandTime, FIELD_TIME ),
	DEFINE_FIELD( CStukabat, m_iMode, FIELD_INTEGER ),
	DEFINE_FIELD( CStukabat, meMedium, FIELD_INTEGER ),
	DEFINE_FIELD( CStukabat, mpCeilingNode, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CStukabat, CFlyingMonster );


const char *CStukabat::pIdleSounds[] = 
{
	"stukabat/stukabat_idle1.wav",
	"stukabat/stukabat_idle2.wav",
	"stukabat/stukabat_idle3.wav",
	"stukabat/stukabat_idle4.wav",
};

const char *CStukabat::pAlertSounds[] = 
{
	"stukabat/ng_alert1.wav",
};

const char *CStukabat::pAttackSounds[] = 
{
	"stukabat/ng_attack1.wav",
};

const char *CStukabat::pSlashSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CStukabat::pPainSounds[] = 
{
	"stukabat/ng_pain1.wav",
	"stukabat/ng_pain2.wav",
	"stukabat/ng_pain3.wav",
};

const char *CStukabat::pDieSounds[] = 
{
	"stukabat/ng_die1.wav",
	"stukabat/ng_die2.wav",
};

#define EMIT_STUKABAT_SOUND( chan, array ) \
	EMIT_SOUND_DYN ( ENT(pev), chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], 1.0, 0.6, 0, RANDOM_LONG(95,105) ); 


void CStukabat :: IdleSound( void )	
{ 
	EMIT_STUKABAT_SOUND( CHAN_VOICE, pIdleSounds ); 
}

void CStukabat :: AlertSound( void ) 
{ 
	EMIT_STUKABAT_SOUND( CHAN_VOICE, pAlertSounds ); 
}

void CStukabat :: AttackSound( void ) 
{ 
	EMIT_STUKABAT_SOUND( CHAN_VOICE, pAttackSounds );
}

void CStukabat :: SlashSound( void ) 
{ 
	EMIT_STUKABAT_SOUND( CHAN_WEAPON, pSlashSounds );
}

void CStukabat :: DeathSound( void ) 
{ 
	EMIT_STUKABAT_SOUND( CHAN_VOICE, pDieSounds ); 
}

void CStukabat :: PainSound( void )	
{ 
	EMIT_STUKABAT_SOUND( CHAN_VOICE, pPainSounds ); 
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum 
{
	TASK_STUKABAT_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_STUKABAT_FLY,
	TASK_STUKABAT_MOVETOGROUND,
	TASK_STUKABAT_LANDGROUND,
	TASK_STUKABAT_TAKEOFFGROUND,
	TASK_STUKABAT_FIND_NODE,
	TASK_STUKABAT_LANDCEILING,
	TASK_STUKABAT_TAKEOFFCEILING,
	TASK_STUKABAT_BOMB_OK,
	TASK_STUKABAT_FALL
};

// Note: tried hovering, didn't like it

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlFlyAround[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_FLY },
	{ TASK_STUKABAT_FLY,		0.0 },
};

static Schedule_t	slFlyAround[] =
{
	{ 
		tlFlyAround,
		ARRAYSIZE(tlFlyAround), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"FlyAround"
	},
};

static Task_t	tlFlyAgitated[] =
{
	{ TASK_STOP_MOVING,				(float) 0 },
	{ TASK_SET_ACTIVITY,			(float)ACT_RUN },
	{ TASK_WAIT,					(float)2.0 },
};

static Schedule_t	slFlyAgitated[] =
{
	{ 
		tlFlyAgitated,
		ARRAYSIZE(tlFlyAgitated), 
		0, 
		0, 
		"FlyAgitated"
	},
};


static Task_t	tlCircleEnemy[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_FLY },
	{ TASK_STUKABAT_CIRCLE_ENEMY, 0.0 },
};

static Schedule_t	slCircleEnemy[] =
{
	{ 
		tlCircleEnemy,
		ARRAYSIZE(tlCircleEnemy), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"CircleEnemy"
	},
};


Task_t tlStukabatTwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_STUKABAT_FALL,		(float)0 },
	{ TASK_DIE,					(float)0 },
};

Schedule_t slStukabatTwitchDie[] =
{
	{
		tlStukabatTwitchDie,
		ARRAYSIZE( tlStukabatTwitchDie ),
		0,
		0,
		"Die"
	},
};

static Task_t	tlLandGround[] =
{
	{ TASK_STUKABAT_MOVETOGROUND,	0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_LAND },
	{ TASK_STUKABAT_LANDGROUND,		0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE },
};

static Schedule_t	slLandGround[] =
{
	{ 
		tlLandGround,
		ARRAYSIZE(tlLandGround), 
		0,
		0,
		"LandGround"
	},
};

static Task_t	tlTakeOffGround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_LEAP },
	{ TASK_STUKABAT_TAKEOFFGROUND,	0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_FLY  },
};

static Schedule_t	slTakeOffGround[] =
{
	{ 
		tlTakeOffGround,
		ARRAYSIZE(tlTakeOffGround), 
		0,
		0,
		"TakeOffGround"
	},
};

static Task_t	tlLandCeiling[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_FAIL },
	{ TASK_STUKABAT_FIND_NODE,		0				},
	{ TASK_GET_PATH_TO_TARGET,		(float)0		},
	{ TASK_WALK_PATH,				(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0		},
	{ TASK_SET_ACTIVITY,			(float)ACT_STAND },
	{ TASK_STUKABAT_LANDCEILING,	0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE },
};

static Schedule_t	slLandCeiling[] =
{
	{ 
		tlLandCeiling,
		ARRAYSIZE(tlLandCeiling), 
		0,
		0,
		"LandCeiling"
	},
};

static Task_t	tlTakeOffCeiling[] =
{
	{ TASK_STUKABAT_TAKEOFFCEILING,	0				},
	{ TASK_SET_ACTIVITY,			(float)ACT_FLY },
};

static Schedule_t	slTakeOffCeiling[] =
{
	{ 
		tlTakeOffCeiling,
		ARRAYSIZE(tlTakeOffCeiling), 
		0,
		0,
		"TakeOffCeiling"
	},
};

static Task_t	tlBombAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_FAIL },
	{ TASK_GET_PATH_TO_SPOT,		(float)0		},		// todo: overwrite this...
	{ TASK_WALK_PATH,				(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0		},
	{ TASK_STUKABAT_BOMB_OK,		(float)0		},		// checks 2D length <= 128, z length <= 1024 && > 128, path is unobstructed
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

static Schedule_t	slBombAttack[] =
{
	{ 
		tlBombAttack,
		ARRAYSIZE(tlBombAttack), 
		0,
		0,
		"BombAttack"
	},
};


DEFINE_CUSTOM_SCHEDULES(CStukabat)
{
    slFlyAround,
	slFlyAgitated,
	slCircleEnemy,
	slStukabatTwitchDie,
	slLandGround,
	slTakeOffGround,
	slLandCeiling,
	slTakeOffCeiling,
	slBombAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES(CStukabat, CFlyingMonster);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CStukabat :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CStukabat :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 64 && flDot >= 0.7 && m_flNextMeleeAttack <= gpGlobals->time )
	{
		return TRUE;
	}
	return FALSE;
}

/*
void CStukabat::SlashTouch( CBaseEntity *pOther )
{
	// slash if we hit who we want to eat
	if ( pOther == m_hEnemy ) 
	{
		m_flNextMeleeAttack = gpGlobals->time;
		m_bOnAttack = TRUE;
	}
}
*/

void CStukabat::CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	//if ( !ShouldToggle( useType, m_bOnAttack ) )
	if ( !ShouldToggle( useType ) )
		return;

	/*
	if (m_bOnAttack)
	{
		m_bOnAttack = 0;
	}
	else
	{
		m_bOnAttack = 1;
	}
	*/
}

//=========================================================
// CheckRangeAttack1  - Fly in for a chomp
//
//=========================================================
BOOL CStukabat :: CheckRangeAttack1 ( float flDot, float flDist )
{
	//if ( flDot > -0.7 && (m_bOnAttack || ( flDist <= 384 && m_idealDist <= 384)))
	if ( flDot > -0.7 && flDist <= 2048 && m_flNextRangedAttack <= gpGlobals->time)
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2  - Fly in for a chomp
//
//=========================================================
BOOL CStukabat :: CheckRangeAttack2 ( float flDot, float flDist )
{
	//if ( flDot > -0.7 && (m_bOnAttack || ( flDist <= 384 && m_idealDist <= 384)))
	if ( flDot > -0.7 && ( flDist <= 384 && m_idealDist <= 384) && m_flNextMeleeAttack <= gpGlobals->time)
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CStukabat :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}



//=========================================================
// Killed - overrides CFlyingMonster.
//
void CStukabat :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->velocity = Vector(0,0,0);
	pev->gravity = 1.0;
	pev->angles.x = 0;
	pev->deadflag = DEAD_DYING;

	//CBaseMonster::Killed( pevAttacker, iGib );
	CFlyingMonster::Killed( pevAttacker, iGib );
	pev->movetype = MOVETYPE_TOSS;
}

void CStukabat::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health. 
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.
}

void CStukabat :: Slash( void )
{
	if (m_hEnemy != NULL && FVisible( m_hEnemy ))
	{
		CBaseEntity *pHurt = m_hEnemy;

		Vector vecShootDir = ShootAtEnemy( pev->origin );
		UTIL_MakeAimVectors ( pev->angles );

		if (DotProduct( vecShootDir, gpGlobals->v_forward ) > 0.707)
		{
			//m_bOnAttack = TRUE;
			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			//pHurt->TakeDamage( pev, pev, gSkillData.stukabatDmgSlash, DMG_SLASH );
			pHurt->TakeDamage( pev, pev, gSkillData.nightgauntDmgSlash, DMG_SLASH );
		}
	}
	SlashSound();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStukabat :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case STUKABAT_AE_BOMB:
		{
			Vector vecAngle;
			UTIL_MakeVectors(vecAngle);
			vecAngle.Normalize();
			vecAngle = 20.0 * vecAngle;

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			
			// todo: need to set hackedgunposition
			CStukaGrenade::ShootContact( pev, GetGunPosition(), vecAngle );

			m_flNextRangedAttack = gpGlobals->time + RANDOM_FLOAT(7.0, 10.0);
		}
		break;
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CStukabat :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/stukabat.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, -2 ), Vector( 16, 16, 16 ) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
//		pev->health			= gSkillData.stukabatHealth;
//		pev->health			= gSkillData.zombieHealth;
		pev->health			= 5;
	pev->view_ofs		= Vector ( 0, 0, 16 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	SetBits(pev->flags, FL_FLY);
	SetFlyingSpeed( STUKABAT_SPEED );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_RANGE_ATTACK2 | bits_CAP_FLY;

	meMedium	= SB_INAIR;
	m_iMode		= STUKABAT_IDLE;

	MonsterInit();

	// PhilG: hack to allow CheckMeleeAttack1 to be used
	m_afCapability |= bits_CAP_MELEE_ATTACK1;

	//SetTouch( SlashTouch );
	m_flNextRangedAttack = gpGlobals->time;
	m_flNextMeleeAttack = gpGlobals->time;
	SetTouch( NULL );
	SetUse( CombatUse );

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 300;
	m_flMaxDist = 384;

	m_flLandTime		= gpGlobals->time;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;

	mpCeilingNode = NULL;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStukabat :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/stukabat.mdl");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pSlashSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	PRECACHE_SOUND("weapons/glauncher.wav");
}

void CStukabat :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK2:
		// get aimable sequence
		iSequence = LookupSequence( "Attack_claw" );
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
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

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CStukabat::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch(m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = STUKABAT_SPEED / 4;
		switch (meMedium)
		{
		case SB_ONGROUND:
			return GetScheduleOfType( SCHED_IDLE_STAND );
			break;
		case SB_ONCEILING:
			return GetScheduleOfType( SCHED_IDLE_STAND );
			break;
		case SB_INAIR:
		default:
			return GetScheduleOfType( SCHED_IDLE_WALK );
			break;
		}

	case MONSTERSTATE_ALERT:
		m_flightSpeed = STUKABAT_SPEED - 50;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_COMBAT:
		switch (meMedium)
		{
		case SB_INAIR:
			{
				m_flMaxSpeed = STUKABAT_SPEED + 100;

				// chase them down and eat them
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				// can we claw? (which is defined as a range attack for some unknown reason)
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
				{
					if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
					{
						return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
					}
					else
					{
						return GetScheduleOfType( SCHED_CHASE_ENEMY );
					}
				}
				/*
				if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				{
					m_bOnAttack = TRUE;
				}
				if ( pev->health < pev->max_health - 20 )
				{
					m_bOnAttack = TRUE;
				}
				*/

				return GetScheduleOfType( SCHED_STANDOFF );
			}
			break;
		case SB_ONGROUND:
		case SB_ONCEILING:
		default:
			return GetScheduleOfType( SCHED_COMBAT_STAND );
			break;
		}
	}

	return CFlyingMonster :: GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CStukabat :: GetScheduleOfType ( int Type ) 
{
	int iRand;

	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch	( Type )
	{
	case SCHED_IDLE_STAND:
	case SCHED_IDLE_WALK:
		switch (meMedium)
		{
		case SB_ONGROUND:
			// randomly want to take off, if we have been on ground for at least 10 secs
			if (RANDOM_LONG(0,30) == 0 && gpGlobals->time > m_flLandTime + 10.0)
			{
				return slTakeOffGround;
			}
			return CBaseMonster :: GetScheduleOfType( Type );
			break;
		case SB_ONCEILING:
			// randomly want to take off, if we have been on ground for at least 10 secs
			if (RANDOM_LONG(0,10) == 0 && gpGlobals->time > m_flLandTime + 10.0)
			{
				return slTakeOffCeiling;
			}
			return CBaseMonster :: GetScheduleOfType( Type );
			break;
		case SB_INAIR:
		default:
			// randomly want to land if we have been in the air at least 10 secs
			if (gpGlobals->time > m_flLandTime + 10.0)
			{
				iRand = RANDOM_LONG(0,10);
				if (iRand == 1)
				{
					return slLandGround;
				}
				else if (iRand < 4)
				{
					return slLandCeiling;
				}
			}
			return slFlyAround;
			break;
		}
	case SCHED_STANDOFF:
		return slCircleEnemy;
	case SCHED_COMBAT_STAND:
		switch (meMedium)
		{
		case SB_ONGROUND:
			return slTakeOffGround;
			break;
		case SB_ONCEILING:
			return slTakeOffCeiling;
			break;
		default:
		case SB_INAIR:
			// this should never happen
			return slFlyAround;
			break;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slBombAttack;
	case SCHED_FAIL:
		return slFlyAgitated;
	case SCHED_DIE:
		return slStukabatTwitchDie;
	case SCHED_CHASE_ENEMY:
		AttackSound( );
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CStukabat::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_STUKABAT_CIRCLE_ENEMY:
		break;
	case TASK_STUKABAT_FLY:
		break;
	case TASK_STUKABAT_MOVETOGROUND:
		// angle downwards
		//if (pev->angles.x > -60) pev->angles.x = -60;
		break;
	case TASK_STUKABAT_LANDGROUND:
		SetActivity(ACT_LAND);
		LandGround();
		break;
	case TASK_STUKABAT_TAKEOFFGROUND:
		SetActivity(ACT_LEAP);
		TakeOffGround();
		break;
	case TASK_STUKABAT_FIND_NODE:
		FindNode();
		m_hTargetEnt = mpCeilingNode;
		m_movementActivity = ACT_FLY;
		break;
	case TASK_STUKABAT_LANDCEILING:
		// stop moving
		m_flightSpeed = 0.0;
		m_flGroundSpeed = 0.0;
		pev->velocity = Vector(0,0,0);

		SetActivity(ACT_STAND);
		LandCeiling();
		break;
	case TASK_STUKABAT_TAKEOFFCEILING:
		TakeOffGround();
		break;
	case TASK_STUKABAT_BOMB_OK:
		{
			bool bOK = false;
			// do we have an enemy
			if (m_hEnemy != NULL)
			{
				//CBaseEntity* pEnemy = CBaseEntity::Instance(m_hEnemy);
				CBaseEntity* pEnemy = (m_hEnemy);
				if (pEnemy && pEnemy->IsAlive())
				{
					// are we at least 128 (2D) away from enemy
					if ((pev->origin - pEnemy->pev->origin).Length2D() <= 128)
					{
						float dz = pev->origin.z - pEnemy->pev->origin.z;
						// are we a good height above
						if (dz > 128 && dz <= 1024)
						{
							TraceResult tr;

							UTIL_TraceLine( pev->origin, pEnemy->pev->origin, ignore_monsters, NULL, &tr );
							// are we obstructed (at least before 95% of the distance
							if (tr.flFraction > 0.95)
							{
								bOK = true;
							}
						}
					}
				}
			}
			if (bOK)
			{
				TaskComplete();
			}
			else
			{
				TaskFail();
			}
		}
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			//m_bOnAttack = TRUE;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_STUKABAT_FALL:
		m_movementActivity = ACT_FALL;
		//m_IdealActivity = ACT_FALL;
		SetActivity ( ACT_FALL );
		//SetSequenceByName( "fall_cycler" );
		break;

	// overridden task
	case TASK_WALK_PATH:
		{
			switch (meMedium)
			{
			case SB_ONGROUND:
				m_movementActivity = ACT_WALK;
				break;
			case SB_ONCEILING:
			case SB_INAIR:
			default:
				m_movementActivity = ACT_FLY;
				break;
			}
			TaskComplete();
			break;
		}

	case TASK_GET_PATH_TO_SPOT:
		{
			// get the target location
			if (m_hEnemy == NULL)
			{
				TaskFail();
				break;
			}

			//CBaseEntity* pEnemy = CBaseEntity::Instance(m_hEnemy);
			CBaseEntity* pEnemy = (m_hEnemy);

			TraceResult tr;

			// trace directly up for 1024
			Vector up = pEnemy->pev->origin;
			up.z += 1024;

			UTIL_TraceLine( pEnemy->pev->origin, up, ignore_monsters, NULL, &tr );

			// choose highest point (minus a bit)
			m_vecMoveGoal = tr.vecEndPos;
			m_vecMoveGoal.z -= 64;

			// will we be too close to his head?
			if ((pEnemy->pev->origin - m_vecMoveGoal).Length() < 128)
			{
				TaskFail();
				break;
			}

			if ( BuildRoute ( m_vecMoveGoal, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToSpot failed!!\n" );
				TaskFail();
			}
		}
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CStukabat :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_STUKABAT_CIRCLE_ENEMY:
		if (m_hEnemy == NULL)
		{
			TaskComplete( );
		}
		else if (FVisible( m_hEnemy ))
		{
			Vector vecFrom = m_hEnemy->EyePosition( );

			Vector vecDelta = (pev->origin - vecFrom).Normalize( );
			Vector vecFly = CrossProduct( vecDelta, Vector( 0, 0, 1 ) ).Normalize( );
			
			if (DotProduct( vecFly, m_SaveVelocity ) < 0)
				vecFly = vecFly * -1.0;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecFly * 32;

			// ALERT( at_console, "vecPos %.0f %.0f %.0f\n", vecPos.x, vecPos.y, vecPos.z );

			TraceResult tr;
		
			UTIL_TraceHull( vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;

			// ALERT( at_console, "m_SaveVelocity %.2f %.2f %.2f\n", m_SaveVelocity.x, m_SaveVelocity.y, m_SaveVelocity.z );

			if (HasConditions( bits_COND_ENEMY_FACING_ME ) && m_hEnemy->FVisible( this ))
			{
				m_flNextAlert -= 0.1;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flightSpeed > m_flMinSpeed)
				{
					m_flightSpeed -= 2;
				}
				else if (m_flightSpeed < m_flMinSpeed)
				{
					m_flightSpeed += 2;
				}
				if (m_flMinSpeed < m_flMaxSpeed)
				{
					m_flMinSpeed += 0.5;
				}
			}
			else 
			{
				m_flNextAlert += 0.1;

				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flightSpeed < m_flMaxSpeed)
				{
					m_flightSpeed += 4;
				}
			}
			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound( );
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
		}

		// PhilG: they will circle me forever unless I break it...
		if (RANDOM_LONG(0,50) == 1)
		{
			TaskComplete();
		}

		break;
	case TASK_STUKABAT_FLY:
		//if (m_fSequenceFinished)
		// the sequence loops, so it is very rare that the 'finished' flag is set
		// set we randomly finish the task, which will recall the result in the
		// getting of a new schedule (possibly the same one again)
		if (RANDOM_LONG(0,20) == 1)
		{
			TaskComplete( );
		}
		break;
	case TASK_STUKABAT_MOVETOGROUND:
		{
			// how far above the floor are we?
			float fz = FloorZ(pev->origin);
			if (pev->origin.z - fz < 6)
			{
				// we are very close to the floor, so drop the last little bit
				DROP_TO_FLOOR(ENT(pev));
				TaskComplete( );
			}
			else
			{
				int iContents = UTIL_PointContents(pev->origin + Vector(0,0,-6));
				if ((iContents == CONTENTS_WATER) ||
					(iContents == CONTENTS_LAVA) ||
					(iContents == CONTENTS_SLIME))
				{
					TaskFail();
					break;
				}
				// note: the stukabat origin is 2 units above the bottom of its hull
				// hence if we are checking for 6 units (above), we can only safely push down 4 units
				if ( CheckLocalMove ( pev->origin, pev->origin + Vector(0,0,-4), NULL, NULL ) == LOCALMOVE_VALID)
				{
					pev->origin.z -= 4;
				}
			}
		}
		break;
	case TASK_STUKABAT_LANDGROUND:
		if (m_fSequenceFinished)
		{
			TaskComplete( );
		}
		break;
	case TASK_STUKABAT_TAKEOFFGROUND:
		if (m_fSequenceFinished)
		{
			TaskComplete( );
		}
		break;
	case TASK_STUKABAT_FIND_NODE:
		if (mpCeilingNode)
		{
			TaskComplete( );
		}
		else
		{
			TaskFail();
		}
		break;
	case TASK_STUKABAT_LANDCEILING:
		if (m_fSequenceFinished)
		{
			mpCeilingNode->mbInUse = true;
			TaskComplete( );
		}
		break;
	case TASK_STUKABAT_TAKEOFFCEILING:
		{
			mpCeilingNode->mbInUse = false;
			mpCeilingNode = NULL;
			TaskComplete( );
		}
		break;
	case TASK_RANGE_ATTACK2:
		{
			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			if (gpGlobals->time > m_flNextMeleeAttack)
			{
				// do the attack
				Slash();
				// set the next melee attack time
				m_flNextMeleeAttack = gpGlobals->time + RANDOM_FLOAT(3.0,6.0);
			}
		}
		break;
	case TASK_STUKABAT_FALL:
		if (FBitSet(pev->flags, FL_ONGROUND))
		{
			if ( RANDOM_LONG(0,4) == 1 )
			{
				m_IdealActivity = ACT_DIEVIOLENT;
			}
			else
			{
				m_IdealActivity = ACT_DIESIMPLE;
			}
			TaskComplete( );
		}
		else
		{
			if (m_fSequenceFinished)
			{
				SetActivity ( ACT_FALL );
			}
			MoveExecute(NULL, Vector(0,0,-1), 0.1);
		}
		break;
	case TASK_DIE:
		if ( m_fSequenceFinished )
		{
			CFlyingMonster :: RunTask ( pTask );

			pev->deadflag = DEAD_DEAD;

			TaskComplete( );
		}
		break;

	default: 
		CFlyingMonster :: RunTask ( pTask );
		break;
	}
}



float CStukabat::VectorToPitch( const Vector &vec )
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int) (atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CStukabat::FindNode()
{
	mpCeilingNode = NULL;

	Vector vDistance;
	const int MAX_NODE = 1000;
	int iNode = 0;
	CStukabatNode* pNodesInSphere[MAX_NODE];
	CBaseEntity* pNode = UTIL_FindEntityByClassname(NULL, "info_node_stukabat");
	while (pNode)
	{
		vDistance = pev->origin - pNode->pev->origin;
		if (vDistance.Length() <= 1024.0 && !((CStukabatNode*)pNode)->mbInUse)
		{
			pNodesInSphere[iNode] = (CStukabatNode*)pNode;
			iNode++;
		}

		pNode = UTIL_FindEntityByClassname(pNode, "info_node_stukabat");
	}

	// no viable nodes found
	if (iNode == 0) return;

	// choose a viable node at random
	int iRand = RANDOM_LONG(0,iNode-1);

	mpCeilingNode = pNodesInSphere[iRand];
}

int CStukabat :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	int iContents = UTIL_PointContents(vecEnd);
	if ((iContents == CONTENTS_WATER) ||
		(iContents == CONTENTS_LAVA) ||
		(iContents == CONTENTS_SLIME))
	{
		return FALSE;
	}

	TraceResult tr;

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( tr.vecEndPos - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		// PhilG
		if (gpGlobals->trace_ent)
		{
			CBaseEntity* pBlocker = CBaseEntity::Instance(gpGlobals->trace_ent);
		}

		if ( tr.flFraction < 1.0 && pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}

void CStukabat::Move(float flInterval)
{
	if (meMedium == SB_INAIR)
	{
		CFlyingMonster::Move( flInterval );
	}
	else
	{
		float		flWaypointDist;
		Vector		vecApex;

		// local move to waypoint.
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
		MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );

		ChangeYaw ( pev->yaw_speed );
		UTIL_MakeVectors( pev->angles );

		if ( RANDOM_LONG(0,7) == 1 )
		{
			// randomly check for blocked path.(more random load balancing)
			if ( !WALK_MOVE( ENT(pev), pev->ideal_yaw, 4, WALKMOVE_NORMAL ) )
			{
				// stuck, so just pick a new spot to run off to
				PickNewDest( m_iMode );
			}
		}
		
		WALK_MOVE( ENT(pev), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );

		// if the waypoint is closer than step size, then stop after next step (ok for rat to overshoot)
		if ( flWaypointDist <= m_flGroundSpeed * flInterval )
		{
			// take truncated step and stop

			SetActivity ( ACT_CROUCHIDLE );

			if ( m_iMode == STUKABAT_SMELL_FOOD )
			{
				m_iMode = STUKABAT_EAT;
			}
			else
			{
				m_iMode = STUKABAT_IDLE;
			}
		}
	}
}

float CStukabat::FlPitchDiff( void )
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod( pev->angles.z );

	if ( flCurrentPitch == pev->idealpitch )
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else 
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CStukabat :: ChangePitch( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlPitchDiff();
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}
		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	}
	return 0;
}

float CStukabat::ChangeYaw( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0;

		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( diff < -20 )
				target = 20;
			else if ( diff > 20 )
				target = -20;
		}
		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * 0.1 );
	}
	return CFlyingMonster::ChangeYaw( speed );
}


Activity CStukabat:: GetStoppedActivity( void )
{ 
	//if ( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
	//	return ACT_IDLE;
	//return ACT_WALK;
	switch (meMedium)
	{
	case SB_ONGROUND:
		return ACT_CROUCHIDLE;
		break;
	case SB_ONCEILING:
		return ACT_IDLE;
		break;
	case SB_INAIR:
	default:
		return ACT_FLY;
		break;
	}
}

void CStukabat::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
	CFlyingMonster::MoveExecute(pTargetEnt, vecDir, flInterval);
}


void CStukabat::MonsterThink ( void )
{
	float flInterval = 0.1;

	if (!IsAlive() || pev->deadflag == DEAD_DYING) m_iMode = STUKABAT_DEAD;

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			switch (meMedium)
			{
			case SB_ONCEILING:
				MonsterThinkOnCeiling();
				break;
			case SB_ONGROUND:
				MonsterThinkOnGround();
				break;
			case SB_INAIR:
			default:
				MonsterThinkInAir();
				break;
			}
		}
	}
	else if (pev->deadflag == DEAD_DYING)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			SetNextThink( 0.1 );// keep monster thinking.
			RunAI();
			DispatchAnimEvents( );
			StudioFrameAdvance( );
		}
	}

}

void CStukabat::MonsterThinkInAir ( void )
{
	// we just want to run the activity if we are doing any of the following
	// taking off from the ground
	// landing (ground or ceiling)
	// attacking
	if ((m_IdealActivity == ACT_LEAP) ||
		(m_IdealActivity == ACT_LAND) ||
		(m_IdealActivity == ACT_STAND))
	{
		SetNextThink( 0.1 );// keep monster thinking
		RunAI();
		StudioFrameAdvance( ); // animate
		return;
	}

	CFlyingMonster::MonsterThink( );

	// if we are not dead, in the air and not attacking
	if (m_MonsterState != MONSTERSTATE_DEAD && 
		meMedium == SB_INAIR &&
		m_IdealActivity != ACT_RANGE_ATTACK1 &&
		m_IdealActivity != ACT_RANGE_ATTACK2)
	{
		Fly();
	}
}

void CStukabat::MonsterThinkOnCeiling ( void )
{
	CBaseMonster::MonsterThink( );
}

void CStukabat::MonsterThinkOnGround ( void )
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		SetNextThink( RANDOM_FLOAT(1,1.5) );
	else
		SetNextThink( 0.1 );// keep monster thinking

	RunAI();

	if (m_IdealActivity == ACT_IDLE)
	{
		pev->sequence = LookupActivity ( ACT_CROUCHIDLE );
		m_Activity = ACT_CROUCHIDLE;
		m_IdealActivity = ACT_CROUCHIDLE;
	}
	if (m_IdealActivity == ACT_CROUCHIDLE && m_fSequenceFinished)
	{
		SetActivity(ACT_CROUCHIDLE);
	}

	float flInterval = StudioFrameAdvance( ); // animate

	// if we are landing, do nothing
	if (m_IdealActivity == ACT_LAND) return;

	switch (m_iMode)
	{
	case	STUKABAT_IDLE:
	case	STUKABAT_EAT:
		{
			// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
			if ( RANDOM_LONG(0,1) == 1 )
			{
				Look( 150 );
				if (HasConditions(bits_COND_SEE_FEAR))
				{
					// if see something scary
					//ALERT ( at_aiconsole, "Scared\n" );
					Eat( 30 +  ( RANDOM_LONG(0,14) ) );// rat will ignore food for 30 to 45 seconds
					PickNewDest( STUKABAT_SCARED_BY_ENT );
					SetActivity ( ACT_RUN );
				}
				else if ( RANDOM_LONG(0,1) == 1 )
				{
					// if rat doesn't see anything, there's still a chance that it will move. (boredom)
					//ALERT ( at_aiconsole, "Bored\n" );
					PickNewDest( STUKABAT_BORED );
					SetActivity ( ACT_WALK );

					if ( m_iMode == STUKABAT_EAT )
					{
						// rat will ignore food for 30 to 45 seconds if it got bored while eating. 
						Eat( 30 +  ( RANDOM_LONG(0,14) ) );
					}
				}
			}
	
			// don't do this stuff if eating!
			if ( m_iMode == STUKABAT_IDLE )
			{
				if ( FShouldEat() )
				{
					Listen();
				}

				if ( HasConditions(bits_COND_SMELL_FOOD) )
				{
					CSound *pSound;

					pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );

					// rat smells food and is just standing around. Go to food unless food isn't on same z-plane.
					if ( pSound && abs( pSound->m_vecOrigin.z - pev->origin.z ) <= 3 )
					{
						PickNewDest( STUKABAT_SMELL_FOOD );
						SetActivity ( ACT_WALK );
					}
				}
				else
				{
					//SetActivity ( ACT_CROUCHIDLE );
				}
			}

			break;
		}
	case	STUKABAT_DEAD:
		return;
		break;
	}
	if ( m_flGroundSpeed != 0 )
	{
		Move( 0.1 );
	}
}

void CStukabat :: Stop( void ) 
{
	//if (!m_bOnAttack)
		m_flightSpeed = 80.0;
}

void CStukabat::Fly( )
{
	int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

/*
	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}
*/
	
	//if (meMedium == SB_ONGROUND)
	//{
	//	pev->angles.x = 0;
	//	SetActivity( ACT_WALK );
	//	return;
	//}

	//if (m_bOnAttack && m_flightSpeed < m_flMaxSpeed)
	if (m_flightSpeed < m_flMaxSpeed)
	{
		m_flightSpeed += 40;
	}

	SetActivity( ACT_FLY );
	
	/*
	if (m_flightSpeed < 180)
	{
		SetActivity( ACT_FLY );
		//if (m_IdealActivity == ACT_RUN)
		//	SetActivity( ACT_WALK );
		//if (m_IdealActivity == ACT_WALK)
		//	pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		switch (meMedium)
		{
		case SB_ONGROUND:
			//SetActivity( ACT_WALK );
			break;
		case SB_ONCEILING:
			SetActivity( ACT_WALK );
			break;
		default:
			SetActivity( ACT_FLY );
			break;
		}
		//if (m_IdealActivity == ACT_WALK)
		//	SetActivity( ACT_RUN );
		//if (m_IdealActivity == ACT_RUN)
		//	pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}
	*/

#define PROBE_LENGTH 150

	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * (Forward+Right));
	l = DoProbe(start + PROBE_LENGTH/3 * (Forward-Right));
	u = DoProbe(start + PROBE_LENGTH/3 * (Forward+Up));
	d = DoProbe(start + PROBE_LENGTH/3 * (Forward-Up));

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80;

	
	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -80) pev->angles.x = -80;
	if (pev->angles.x >  80) pev->angles.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20) pev->angles.z = -20;
	if (pev->angles.z >  20) pev->angles.z =  20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);

}


void CStukabat::LandGround(void)
{
	ClearBits( pev->flags, FL_FLY );
	meMedium			= SB_ONGROUND;
	m_afCapability		= bits_CAP_RANGE_ATTACK2;
	pev->movetype		= MOVETYPE_STEP;
	pev->angles.x		= 0;
	//pev->angles.z		= 0;
	m_flLandTime		= gpGlobals->time;
}

void CStukabat::LandCeiling(void)
{
	meMedium			= SB_ONCEILING;
	m_afCapability		= bits_CAP_FLY;
	pev->movetype		= MOVETYPE_FLY;
	pev->angles.x		= 0;
	//pev->angles.z		= 0;
	m_flLandTime		= gpGlobals->time;
}

void CStukabat::TakeOffGround(void)
{
	SetBits( pev->flags, FL_FLY );
	meMedium			= SB_INAIR;
	m_afCapability		= bits_CAP_RANGE_ATTACK2 | bits_CAP_FLY;
	pev->movetype		= MOVETYPE_FLY;
	m_flLandTime		= gpGlobals->time;
}

Vector CStukabat::DoProbe(const Vector &Probe)
{
	Vector WallNormal = Vector(0,0,1); // AIR normal is Straight Up for flying thing.
	float frac;
	BOOL bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0) tr.flFraction = 0.0;
		if (tr.flFraction > 1.0) tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();
		
		return SteeringVector;
	}
	return Vector(0, 0, 0);
}

//=========================================================
// Picks a new spot for rat to run to.(
//=========================================================
void CStukabat :: PickNewDest ( int iCondition )
{
	Vector	vecNewDir;
	Vector	vecDest;
	float	flDist;

	m_iMode = iCondition;

	if ( m_iMode == STUKABAT_SMELL_FOOD )
	{
		// find the food and go there.
		CSound *pSound;

		pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );

		if ( pSound )
		{
			m_Route[ 0 ].vecLocation.x = pSound->m_vecOrigin.x + ( 3 - RANDOM_LONG(0,5) );
			m_Route[ 0 ].vecLocation.y = pSound->m_vecOrigin.y + ( 3 - RANDOM_LONG(0,5) );
			m_Route[ 0 ].vecLocation.z = pSound->m_vecOrigin.z;
			m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
			m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
			return;
		}
	}

	do 
	{
		// picks a random spot, requiring that it be at least 128 units away
		// else, the rat will pick a spot too close to itself and run in 
		// circles. this is a hack but buys me time to work on the real monsters.
		vecNewDir.x = RANDOM_FLOAT( -1, 1 );
		vecNewDir.y = RANDOM_FLOAT( -1, 1 );
		flDist		= 256 + ( RANDOM_LONG(0,255) );
		vecDest = pev->origin + vecNewDir * flDist;

	} while ( ( vecDest - pev->origin ).Length2D() < 128 );

	m_Route[ 0 ].vecLocation.x = vecDest.x;
	m_Route[ 0 ].vecLocation.y = vecDest.y;
	m_Route[ 0 ].vecLocation.z = pev->origin.z;
	m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[ 0 ].iType );

	if ( RANDOM_LONG(0,9) == 1 )
	{
		// every once in a while, a rat will play a skitter sound when they decide to run
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "roach/rch_walk.wav", 1, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
	}
}



#endif