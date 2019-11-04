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

//=========================================================
// night_gaunt
//=========================================================

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

#define SEARCH_RETRY	16

#define NIGHT_GAUNT_SPEED 200

extern CGraph WorldGraph;


#define NGAUNT_AE_SLASH		1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================


#include "NightGaunt.h"


LINK_ENTITY_TO_CLASS( monster_nightgaunt, CNightGaunt );

TYPEDESCRIPTION	CNightGaunt::m_SaveData[] = 
{
	DEFINE_FIELD( CNightGaunt, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CNightGaunt, m_idealDist, FIELD_FLOAT ),
	DEFINE_FIELD( CNightGaunt, m_flEnemyTouched, FIELD_FLOAT ),
	DEFINE_FIELD( CNightGaunt, m_bOnAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CNightGaunt, m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CNightGaunt, m_flMinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CNightGaunt, m_flMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CNightGaunt, m_flNextAlert, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CNightGaunt, CFlyingMonster );


/* nightgaunt does not make idle sounds
const char *CNightGaunt::pIdleSounds[] = 
{
	"nightgaunt/nightgaunt_idle1.wav",
	"nightgaunt/nightgaunt_idle2.wav",
	"nightgaunt/nightgaunt_idle3.wav",
	"nightgaunt/nightgaunt_idle4.wav",
};
*/

const char *CNightGaunt::pAlertSounds[] = 
{
	"nightgaunt/ng_alert1.wav",
};

const char *CNightGaunt::pAttackSounds[] = 
{
	"nightgaunt/ng_attack1.wav",
};

const char *CNightGaunt::pSlashSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CNightGaunt::pPainSounds[] = 
{
	"nightgaunt/ng_pain1.wav",
	"nightgaunt/ng_pain2.wav",
	"nightgaunt/ng_pain3.wav",
};

const char *CNightGaunt::pDieSounds[] = 
{
	"nightgaunt/ng_die1.wav",
	"nightgaunt/ng_die2.wav",
};

#define EMIT_NIGHT_GAUNT_SOUND( chan, array ) \
	EMIT_SOUND_DYN ( ENT(pev), chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], 1.0, 0.6, 0, RANDOM_LONG(95,105) ); 


void CNightGaunt :: IdleSound( void )	
{ 
//	EMIT_NIGHT_GAUNT_SOUND( CHAN_VOICE, pIdleSounds ); 
}

void CNightGaunt :: AlertSound( void ) 
{ 
	EMIT_NIGHT_GAUNT_SOUND( CHAN_VOICE, pAlertSounds ); 
}

void CNightGaunt :: AttackSound( void ) 
{ 
	EMIT_NIGHT_GAUNT_SOUND( CHAN_VOICE, pAttackSounds );
}

void CNightGaunt :: SlashSound( void ) 
{ 
	EMIT_NIGHT_GAUNT_SOUND( CHAN_WEAPON, pSlashSounds );
}

void CNightGaunt :: DeathSound( void ) 
{ 
	EMIT_NIGHT_GAUNT_SOUND( CHAN_VOICE, pDieSounds ); 
}

void CNightGaunt :: PainSound( void )	
{ 
	EMIT_NIGHT_GAUNT_SOUND( CHAN_VOICE, pPainSounds ); 
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum 
{
	TASK_NIGHT_GAUNT_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_NIGHT_GAUNT_FLY,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlFlyAround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_NIGHT_GAUNT_FLY,		0.0 },
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
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_NIGHT_GAUNT_CIRCLE_ENEMY, 0.0 },
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


Task_t tlNightGauntTwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_DIE,					(float)0 },
};

Schedule_t slNightGauntTwitchDie[] =
{
	{
		tlNightGauntTwitchDie,
		ARRAYSIZE( tlNightGauntTwitchDie ),
		0,
		0,
		"Die"
	},
};


DEFINE_CUSTOM_SCHEDULES(CNightGaunt)
{
    slFlyAround,
	slFlyAgitated,
	slCircleEnemy,
	slNightGauntTwitchDie,
};
IMPLEMENT_CUSTOM_SCHEDULES(CNightGaunt, CFlyingMonster);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CNightGaunt :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}


Vector CNightGaunt :: Center ( void )
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 64 );
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CNightGaunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( flDot >= 0.7 && m_flEnemyTouched > gpGlobals->time - 0.2 )
	{
		return TRUE;
	}
	return FALSE;
}

void CNightGaunt::SlashTouch( CBaseEntity *pOther )
{
	// slash if we hit who we want to eat
	if ( pOther == m_hEnemy ) 
	{
		m_flEnemyTouched = gpGlobals->time;
		m_bOnAttack = TRUE;
	}
}

void CNightGaunt::CombatUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !ShouldToggle( useType, m_bOnAttack ) )
		return;

	if (m_bOnAttack)
	{
		m_bOnAttack = 0;
	}
	else
	{
		m_bOnAttack = 1;
	}
}

//=========================================================
// CheckRangeAttack1  - Fly in for a chomp
//
//=========================================================
BOOL CNightGaunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > -0.7 && (m_bOnAttack || ( flDist <= 384 && m_idealDist <= 384)))
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNightGaunt :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}



//=========================================================
// Killed - overrides CFlyingMonster.
//
void CNightGaunt :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->velocity = Vector(0,0,-100);
	pev->gravity = 1.0;
	pev->angles.x = 0;

	//CBaseMonster::Killed( pevAttacker, iGib );
	CFlyingMonster::Killed( pevAttacker, iGib );
}

void CNightGaunt::BecomeDead( void )
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health. 
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNightGaunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case NGAUNT_AE_SLASH:
		{
			if (m_hEnemy != NULL && FVisible( m_hEnemy ))
			{
				CBaseEntity *pHurt = m_hEnemy;

				if (m_flEnemyTouched < gpGlobals->time - 0.2 && (m_hEnemy->BodyTarget( pev->origin ) - pev->origin).Length() > (32+16+32))
					break;

				Vector vecShootDir = ShootAtEnemy( pev->origin );
				UTIL_MakeAimVectors ( pev->angles );

				if (DotProduct( vecShootDir, gpGlobals->v_forward ) > 0.707)
				{
					m_bOnAttack = TRUE;
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
//					if (pHurt->IsPlayer())
//					{
//						pHurt->pev->angles.x += RANDOM_FLOAT( -35, 35 );
//						pHurt->pev->angles.y += RANDOM_FLOAT( -90, 90 );
//						pHurt->pev->angles.z = 0;
//						pHurt->pev->fixangle = TRUE;
//					}
					pHurt->TakeDamage( pev, pev, gSkillData.nightgauntDmgSlash, DMG_SLASH );
				}
			}
			SlashSound();
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
void CNightGaunt :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/ngaunt.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.nightgauntHealth;
	pev->view_ofs		= Vector ( 0, 0, 64 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	SetBits(pev->flags, FL_FLY);
	SetFlyingSpeed( NIGHT_GAUNT_SPEED );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_FLY;

	MonsterInit();

	SetTouch( SlashTouch );
	SetUse( CombatUse );

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 300;
	m_flMaxDist = 384;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNightGaunt :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/ngaunt.mdl");

//	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pSlashSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CNightGaunt::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch(m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = NIGHT_GAUNT_SPEED / 2;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_ALERT:
		m_flightSpeed = NIGHT_GAUNT_SPEED - 50;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = NIGHT_GAUNT_SPEED + 50;
		// eat them
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}
		// chase them down and eat them
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_CHASE_ENEMY );
		}
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			m_bOnAttack = TRUE;
		}
		if ( pev->health < pev->max_health - 20 )
		{
			m_bOnAttack = TRUE;
		}

		return GetScheduleOfType( SCHED_STANDOFF );
	}

	return CFlyingMonster :: GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CNightGaunt :: GetScheduleOfType ( int Type ) 
{
	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch	( Type )
	{
	case SCHED_IDLE_WALK:
		return slFlyAround;
	case SCHED_STANDOFF:
		return slCircleEnemy;
	case SCHED_FAIL:
		return slFlyAgitated;
	case SCHED_DIE:
		return slNightGauntTwitchDie;
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
void CNightGaunt::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_NIGHT_GAUNT_CIRCLE_ENEMY:
		break;
	case TASK_NIGHT_GAUNT_FLY:
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			m_bOnAttack = TRUE;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CNightGaunt :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_NIGHT_GAUNT_CIRCLE_ENEMY:
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

		break;
	case TASK_NIGHT_GAUNT_FLY:
		if (m_fSequenceFinished)
		{
			TaskComplete( );
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



float CNightGaunt::VectorToPitch( const Vector &vec )
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
void CNightGaunt::Move(float flInterval)
{
	CFlyingMonster::Move( flInterval );
}

float CNightGaunt::FlPitchDiff( void )
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

float CNightGaunt :: ChangePitch( int speed )
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

float CNightGaunt::ChangeYaw( int speed )
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


Activity CNightGaunt:: GetStoppedActivity( void )
{ 
	if ( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CNightGaunt::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}


void CNightGaunt::MonsterThink ( void )
{
	CFlyingMonster::MonsterThink( );

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			Fly( );
		}
	}
}

void CNightGaunt :: Stop( void ) 
{
	if (!m_bOnAttack)
		m_flightSpeed = 80.0;
}

void CNightGaunt::Fly( )
{
	int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

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

	if (m_bOnAttack && m_flightSpeed < m_flMaxSpeed)
	{
		m_flightSpeed += 40;
	}
	if (m_flightSpeed < 180)
	{
		if (m_IdealActivity == ACT_RUN)
			SetActivity( ACT_WALK );
		if (m_IdealActivity == ACT_WALK)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if (m_IdealActivity == ACT_WALK)
			SetActivity( ACT_RUN );
		if (m_IdealActivity == ACT_RUN)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}

#define PROBE_LENGTH 150

	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

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


Vector CNightGaunt::DoProbe(const Vector &Probe)
{
	Vector WallNormal = Vector(0,0,-1); // AIR normal is Straight Down for flying thing.
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

#endif