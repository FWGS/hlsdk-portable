/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// bullsquid - big, spotty tentacle-mouthed meanie.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"bullsquid.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ZBULL_AE_SPIT		( 1 )
#define		ZBULL_AE_BITE		( 2 )
#define		ZBULL_AE_BLINK		( 3 )
#define		ZBULL_AE_TAILWHIP	( 4 )
#define		ZBULL_AE_HOP		( 5 )
#define		ZBULL_AE_THROW		( 6 )

class CEinarZombieBull : public CBullsquid
{
public:
	void Spawn();
	void Precache();
	int Classify();
	void SetYawSpeed();
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void IdleSound();
	void PainSound();
	void DeathSound();
	void AlertSound();
	void AttackSound();
	void BiteSound();

	void StartTask( Task_t *pTask );

	int IgnoreConditions();
	void RunAI();
	int IRelationship( CBaseEntity *pTarget );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack2( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }

	Schedule_t *GetSchedule();
	Schedule_t *GetScheduleOfType( int Type );

	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS( monster_th_zombiebull, CEinarZombieBull )

const char *CEinarZombieBull::pAttackSounds[] =
{
	"bull/bu_whip1.wav",
	"bull/bu_whip2.wav",
	"bull/bu_whip3.wav",
};

const char *CEinarZombieBull::pBiteSounds[] =
{
	"bull/bu_gore1.wav",
	"bull/bu_gore2.wav",
	"bull/bu_gore3.wav",
};

const char *CEinarZombieBull::pIdleSounds[] =
{
	"bull/bu_idle1.wav",
	"bull/bu_idle2.wav",
	"bull/bu_idle3.wav",
};

const char *CEinarZombieBull::pAlertSounds[] =
{
	"bull/bu_alert1.wav",
	"bull/bu_alert2.wav",
	"bull/bu_alert3.wav",
	"bull/bu_alert4.wav",
};

const char *CEinarZombieBull::pPainSounds[] =
{
	"bull/bu_pain1.wav",
	"bull/bu_pain2.wav",
	"bull/bu_pain3.wav",
};

const char *CEinarZombieBull::pDeathSounds[] =
{
	"bull/bu_die1.wav",
	"bull/bu_die2.wav",
	"bull/bu_die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CEinarZombieBull::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CEinarZombieBull::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CEinarZombieBull::IRelationship( CBaseEntity *pTarget )
{
	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CEinarZombieBull::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if( m_hEnemy != 0 && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();

		if( flDist > SQUID_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[m_iRouteIndex].vecLocation ).Length2D();// reusing flDist.

			if( FTriangulate( pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5f, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	m_flLastHurtTime = gpGlobals->time;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CEinarZombieBull::CheckMeleeAttack1( float flDot, float flDist )
{
	if( m_hEnemy->pev->health <= gSkillData.bullsquidDmgWhip && flDist <= 110 && flDot >= 0.7f )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CEinarZombieBull::CheckMeleeAttack2( float flDot, float flDist )
{
	if( flDist <= 110 && flDot >= 0.7f && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{									
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// IdleSound 
//=========================================================
void CEinarZombieBull::IdleSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), 1, ATTN_NORM );
}

//=========================================================
// PainSound 
//=========================================================
void CEinarZombieBull::PainSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1, ATTN_NORM );
}

//=========================================================
// AlertSound
//=========================================================
void CEinarZombieBull::AlertSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1, ATTN_NORM );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEinarZombieBull::SetYawSpeed()
{
	int ys;

	ys = 95;

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CEinarZombieBull::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ZBULL_AE_BITE:
		{
			// SOUND HERE!
			CBaseEntity *pHurt = CheckTraceHullAttack( 110, gSkillData.bullsquidDmgBite, DMG_SLASH );
			if( pHurt )
			{
				//pHurt->pev->punchangle.z = -15;
				//pHurt->pev->punchangle.x = -45;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 50;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 30;
			}
		}
		break;
	case ZBULL_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 110, gSkillData.bullsquidDmgWhip, DMG_CLUB | DMG_ALWAYSGIB );
			if( pHurt )
			{
				// pHurt->pev->punchangle.z = -20;
				// pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 500;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 30;
			}
		}
		break;
	case ZBULL_AE_THROW:
		{
			int iPitch;

			// squid throws its prey IF the prey is a client. 
			CBaseEntity *pHurt = CheckTraceHullAttack( 110, 0, 0 );
			if( pHurt )
			{
				// croonchy bite sound
				iPitch = RANDOM_FLOAT( 90, 110 );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pBiteSounds ), 1, ATTN_NORM, 0, iPitch );
				//pHurt->pev->punchangle.x = RANDOM_LONG( 0, 34 ) - 5;
				//pHurt->pev->punchangle.z = RANDOM_LONG( 0, 49 ) - 25;
				//pHurt->pev->punchangle.y = RANDOM_LONG( 0, 89 ) - 45;

				// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
				UTIL_ScreenShake( pHurt->pev->origin, 25.0, 1.5, 0.7, 2 );

				if( pHurt->IsPlayer() )
				{
					UTIL_MakeVectors( pev->angles );
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 500 + gpGlobals->v_up * 30;
				}
			}
		}
		break;
	default:
		CBullsquid::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CEinarZombieBull::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/zombiebull.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.bullsquidHealth * 5;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarZombieBull::Precache()
{
	PRECACHE_MODEL( "models/zombiebull.mdl" );

	PRECACHE_SOUND( "zombie/claw_miss2.wav" );// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );

	PRECACHE_SOUND( "bull/bu_paw1.wav" );

	PRECACHE_SOUND( "bull/bu_runstep1.wav" );
	PRECACHE_SOUND( "bull/bu_runstep2.wav" );

	PRECACHE_SOUND( "bull/bu_walkstep1.wav" );
	PRECACHE_SOUND( "bull/bu_walkstep2.wav" );
}

//=========================================================
// DeathSound
//=========================================================
void CEinarZombieBull::DeathSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDeathSounds ), 1, ATTN_NORM );
}

//=========================================================
// AttackSound
//=========================================================
void CEinarZombieBull::AttackSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1, ATTN_NORM );
}

//=========================================================
// BiteSound
//=========================================================
void CEinarZombieBull::BiteSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pBiteSounds ), 1, ATTN_NORM );
}

//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CEinarZombieBull::RunAI()
{
	// first, do base class stuff
	CBaseMonster::RunAI();
	if( m_hEnemy != 0 && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if( ( pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CEinarZombieBull::GetSchedule()
{
	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		return CBaseMonster::GetSchedule();

	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			return GetScheduleOfType( SCHED_WAKE_ANGRY );
		}

		if( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}

		if( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
		{
			return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
		}

		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	}
	default:
		break;
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t *CEinarZombieBull::GetScheduleOfType( int Type ) 
{
	switch( Type )
	{
	case SCHED_RANGE_ATTACK1:
	case SCHED_SQUID_SNIFF_AND_EAT:
		return CBaseMonster::GetScheduleOfType( Type );
		break;
	}

	return CBullsquid::GetScheduleOfType( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CEinarZombieBull::StartTask( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			AttackSound();

			CBaseMonster::StartTask( pTask );
			break;
		}
	default:
		{
			CBullsquid::StartTask( pTask );
			break;
		}
	}
}
