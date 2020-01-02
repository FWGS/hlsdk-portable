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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

//=========================================================
// archer
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"weapons.h"
#include	"ichthyosaur.h"

#define SEARCH_RETRY	16

#define ARCHER_SPEED 40

extern CGraph WorldGraph;

#define EYE_MAD		0
#define EYE_BASE	1
#define EYE_CLOSED	2
#define EYE_BACK	3
#define EYE_LOOK	4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

// UNDONE: Save/restore here
class CArcher : public CIchthyosaur
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	Schedule_t *GetSchedule( void );

	void StartTask( Task_t *pTask );

	void MonsterThink( void );
	void Stop( void );
	void Swim( void );

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];

	void IdleSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void BiteSound( void );
	void DeathSound( void );
	void PainSound( void );
};

LINK_ENTITY_TO_CLASS( monster_archer, CArcher )

const char *CArcher::pIdleSounds[] =
{
	"archer/arch_idle1.wav",
	"archer/arch_idle2.wav",
	"archer/arch_idle3.wav",
};

const char *CArcher::pAlertSounds[] =
{
	"archer/arch_alert1.wav",
	"archer/arch_alert2.wav",
};

const char *CArcher::pAttackSounds[] =
{
	"archer/arch_attack1.wav",
	"archer/arch_attack2.wav",
};

const char *CArcher::pBiteSounds[] =
{
	"archer/arch_bite1.wav",
	"archer/arch_bite2.wav",
};

const char *CArcher::pPainSounds[] =
{
	"archer/arch_pain1.wav",
	"archer/arch_pain2.wav",
	"archer/arch_pain3.wav",
	"archer/arch_pain4.wav",
};

const char *CArcher::pDieSounds[] =
{
	"archer/arch_die1.wav",
	"archer/arch_die2.wav",
	"archer/arch_die3.wav",
};

#define EMIT_ICKY_SOUND( chan, array ) \
	EMIT_SOUND_DYN( ENT( pev ), chan , array[RANDOM_LONG( 0, ARRAYSIZE( array ) - 1 )], 1.0, 0.6, 0, RANDOM_LONG( 95, 105 ) ); 

void CArcher::IdleSound( void )
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pIdleSounds ); 
}

void CArcher::AlertSound( void ) 
{
	EMIT_ICKY_SOUND( CHAN_VOICE, pAlertSounds ); 
}

void CArcher::AttackSound( void )
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pAttackSounds );
}

void CArcher::BiteSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_WEAPON, pBiteSounds );
}

void CArcher::DeathSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pDieSounds ); 
}

void CArcher::PainSound( void )	
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pPainSounds ); 
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum
{
	TASK_ARCHER_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_ARCHER_SWIM,
	TASK_ARCHER_FLOAT
};

#define ARCHER_AE_SHAKE_RIGHT 1
#define ARCHER_AE_SHAKE_LEFT  2

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CArcher::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int bDidAttack = FALSE;
	switch( pEvent->event )
	{
	case ARCHER_AE_SHAKE_RIGHT:
	case ARCHER_AE_SHAKE_LEFT:
		{
			if( m_hEnemy != 0 && FVisible( m_hEnemy ) )
			{
				CBaseEntity *pHurt = m_hEnemy;

				if( ( m_flEnemyTouched < gpGlobals->time - 0.2f ) && ( m_hEnemy->BodyTarget( pev->origin ) - pev->origin).Length() > ( 32 + 16 + 32 ) )
					break;

				Vector vecShootDir = ShootAtEnemy( pev->origin );
				UTIL_MakeAimVectors( pev->angles );

				if( DotProduct( vecShootDir, gpGlobals->v_forward ) > 0.707f )
				{
					m_bOnAttack = TRUE;
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 300;
					if( pHurt->IsPlayer() )
					{
						pHurt->pev->angles.x += RANDOM_FLOAT( -35, 35 );
						pHurt->pev->angles.y += RANDOM_FLOAT( -90, 90 );
						pHurt->pev->angles.z = 0;
						pHurt->pev->fixangle = TRUE;
					}
					pHurt->TakeDamage( pev, pev, gSkillData.ichthyosaurDmgShake / 2, DMG_SLASH );
				}
			}
			BiteSound();

			bDidAttack = TRUE;
		}
		break;
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;
	}

	if( bDidAttack )
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 32;
		UTIL_Bubbles( vecSrc - Vector( 8, 8, 8 ), vecSrc + Vector( 8, 8, 8 ), 16 );
	}
}

//=========================================================
// Spawn
//=========================================================
void CArcher::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/archer.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ) );

	pev->solid		= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health		= gSkillData.ichthyosaurHealth * 0.5f;
	pev->view_ofs		= Vector( 0, 0, 16 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	SetBits(pev->flags, FL_SWIM);
	SetFlyingSpeed( ARCHER_SPEED );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	m_afCapability		= bits_CAP_RANGE_ATTACK1 | bits_CAP_SWIM;

	MonsterInit();

	SetTouch( &CArcher::BiteTouch );
	SetUse( &CArcher::CombatUse );

	m_idealDist = 384;
	m_flMinSpeed = 40;
	m_flMaxSpeed = 50;
	m_flMaxDist = 384;

	Vector Forward;
	UTIL_MakeVectorsPrivate( pev->angles, Forward, 0, 0 );
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CArcher::Precache()
{
	PRECACHE_MODEL( "models/archer.mdl" );

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CArcher::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 75;
		return GetScheduleOfType( SCHED_IDLE_WALK );
		break;
	case MONSTERSTATE_ALERT:
		m_flightSpeed = 90;
		return GetScheduleOfType( SCHED_IDLE_WALK );
		break;
	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 200;
		// eat them
		if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}

		// chase them down and eat them
		if( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_CHASE_ENEMY );
		}
		if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			m_bOnAttack = TRUE;
		}
		if( pev->health < pev->max_health - 20 )
		{
			m_bOnAttack = TRUE;
		}

		return GetScheduleOfType( SCHED_STANDOFF );
		break;
	default:
		break;
	}

	return CFlyingMonster::GetSchedule();
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CArcher::StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ARCHER_FLOAT:
		SetSequenceByName( RANDOM_LONG( 0, 1 ) ? "die2" : "die1" );
		break;
	default:
		CIchthyosaur::StartTask( pTask );
		break;
	}
}

void CArcher::MonsterThink( void )
{
	CFlyingMonster::MonsterThink();

	if( pev->deadflag == DEAD_NO )
	{
		if( m_MonsterState != MONSTERSTATE_SCRIPT )
		{
			Swim();
		}
	}
}

void CArcher::Stop( void ) 
{
	if( !m_bOnAttack )
		m_flightSpeed = 40.0f;
}

void CArcher::Swim()
{
	//int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if( FBitSet( pev->flags, FL_ONGROUND ) )
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate( Angles, Forward, Right, Up );

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	if( m_bOnAttack && m_flightSpeed < m_flMaxSpeed )
	{
		m_flightSpeed += 40;
	}
	if( m_flightSpeed < 90 )
	{
		if( m_IdealActivity == ACT_RUN )
			SetActivity( ACT_WALK );
		if( m_IdealActivity == ACT_WALK )
			pev->framerate = m_flightSpeed / 150.0f;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if( m_IdealActivity == ACT_WALK )
			SetActivity( ACT_RUN );
		if( m_IdealActivity == ACT_RUN)
			pev->framerate = m_flightSpeed / 75.0f;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}
/*
	if( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
		m_pBeam->PointEntInit( pev->origin + m_SaveVelocity, entindex() );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetColor( 255, 180, 96 );
		m_pBeam->SetBrightness( 192 );
	}
*/
#define PROBE_LENGTH 150
	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate( Angles, Forward, Right, Up );

	Vector f, u, l, r, d;
	f = DoProbe( start + PROBE_LENGTH * Forward );
	r = DoProbe( start + PROBE_LENGTH / 3 * Forward + Right );
	l = DoProbe( start + PROBE_LENGTH / 3 * Forward - Right );
	u = DoProbe( start + PROBE_LENGTH / 3 * Forward + Up );
	d = DoProbe( start + PROBE_LENGTH / 3 * Forward - Up );

	Vector SteeringVector = f + r + l + u + d;
	m_SaveVelocity = ( m_SaveVelocity + SteeringVector / 2 ).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate( Angles, Forward, Right, Up );
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if( flDot > 0.5f )
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if( flDot > 0.0f )
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * ( flDot + 0.5f );
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 40;

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );

	// ALERT( at_console, "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );
/*
	m_pBeam->SetStartPos( pev->origin + pev->velocity );
	m_pBeam->RelinkBeam();
*/
	// ALERT( at_console, "speed %f\n", m_flightSpeed );

	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if( Angles.x > 180 )
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach( Angles.x, pev->angles.x, 50 * 0.1f );
	if( pev->angles.x < -180 )
		pev->angles.x = -180;
	if( pev->angles.x > 180 )
		pev->angles.x = 180;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if( fabs( Angles.y - pev->angles.y ) < fabs( turn ) )
	{
		turn = Angles.y - pev->angles.y;
	}
	if( fabs( Angles.y - pev->angles.y + 360 ) < fabs( turn ) )
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if( fabs( Angles.y - pev->angles.y - 360 ) < fabs( turn ) )
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1f;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if( fabs( turn ) > speed )
	{
		if( turn < 0.0f )
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
	pev->angles.y = fmod( ( pev->angles.y + 360.0f ), 360.0f );

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8f + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj * 0.25f );

	// Roll Smoothing
	//
	turn = 360;
	if( fabs( Angles.z - pev->angles.z ) < fabs( turn ) )
	{
		turn = Angles.z - pev->angles.z;
	}
	if( fabs( Angles.z - pev->angles.z + 360 ) < fabs( turn ) )
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if( fabs( Angles.z - pev->angles.z - 360 ) < fabs( turn ) )
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed / 2 * 0.1f;

	if( fabs( turn ) < speed )
	{
		pev->angles.z += turn;
	}
	else
	{
		if( turn < 0.0f )
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}

	if( pev->angles.z < -20 )
		pev->angles.z = -20;
	if( pev->angles.z > 20 )
		pev->angles.z = 20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up );

	// UTIL_MoveToOrigin ( ENT( pev ), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}
#endif
