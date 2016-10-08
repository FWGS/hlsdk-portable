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

class CZombieBull : public CBullsquid
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	void BiteSound(void);

	void StartTask(Task_t *pTask);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }

	Schedule_t *GetSchedule(void);

	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS(monster_th_zombiebull, CZombieBull);

const char *CZombieBull::pAttackSounds[] =
{
	"bull/bu_whip1.wav",
	"bull/bu_whip2.wav",
	"bull/bu_whip3.wav",
};

const char *CZombieBull::pBiteSounds[] =
{
	"bull/bu_gore1.wav",
	"bull/bu_gore2.wav",
	"bull/bu_gore3.wav",
};

const char *CZombieBull::pIdleSounds[] =
{
	"bull/bu_idle1.wav",
	"bull/bu_idle2.wav",
	"bull/bu_idle3.wav",
};

const char *CZombieBull::pAlertSounds[] =
{
	"bull/bu_alert1.wav",
	"bull/bu_alert2.wav",
	"bull/bu_alert3.wav",
	"bull/bu_alert4.wav",
};

const char *CZombieBull::pPainSounds[] =
{
	"bull/bu_pain1.wav",
	"bull/bu_pain2.wav",
	"bull/bu_pain3.wav",
};

const char *CZombieBull::pDeathSounds[] =
{
	"bull/bu_die1.wav",
	"bull/bu_die2.wav",
	"bull/bu_die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombieBull::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CZombieBull::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_hEnemy->pev->health <= gSkillData.bullsquidDmgWhip && flDist <= 95 && flDot >= 0.7)
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
BOOL CZombieBull::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 95 && flDot >= 0.7 && !HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{									
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// IdleSound 
//=========================================================
void CZombieBull::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM);
}

//=========================================================
// PainSound 
//=========================================================
void CZombieBull::PainSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM);
}

//=========================================================
// AlertSound
//=========================================================
void CZombieBull::AlertSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombieBull::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZBULL_AE_BITE:
	{
		// SOUND HERE!

		CBaseEntity *pHurt = CheckTraceHullAttack(90, gSkillData.zombiebullDmgBite, DMG_SLASH);
		if (pHurt)
		{
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 400;
		}
		else
		{
			//
			// Search for human repels.
			//
			CBaseEntity *pEntity = NULL;
			// iterate on all entities in the vicinity.
			while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 128)) != NULL)
			{
				if (pEntity->pev->takedamage != DAMAGE_NO)
				{
					if (FClassnameIs(pEntity->pev, "monster_hgrunt_repel"))
					{
						pEntity->TakeDamage(pev, pev, gSkillData.zombiebullDmgBite, DMG_SLASH | DMG_ALWAYSGIB);
					}
				}
			}
		}
	}
	break;

	case ZBULL_AE_TAILWHIP:
	{
		// croonchy bite sound
		BiteSound();

		CBaseEntity *pHurt = CheckTraceHullAttack(90, gSkillData.zombiebullDmgWhip, DMG_SLASH | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = -20;
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 16;
		}
	}
	break;

	case ZBULL_AE_THROW:
	{
		// squid throws its prey IF the prey is a client. 
		CBaseEntity *pHurt = CheckTraceHullAttack(90, 0, 0);

		if (pHurt)
		{
			// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
			UTIL_ScreenShake(pHurt->pev->origin, 25.0, 1.5, 0.7, 2);
		}
	}
	break;

	case ZBULL_AE_BLINK:
		break;

	default:
		CBullsquid::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CZombieBull::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zombiebull.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.zombiebullHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombieBull::Precache()
{
	PRECACHE_MODEL("models/zombiebull.mdl");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_SOUND("bull/bu_paw1.wav");

	PRECACHE_SOUND("bull/bu_runstep1.wav");
	PRECACHE_SOUND("bull/bu_runstep2.wav");

	PRECACHE_SOUND("bull/bu_walkstep1.wav");
	PRECACHE_SOUND("bull/bu_walkstep2.wav");
}


//=========================================================
// DeathSound
//=========================================================
void CZombieBull::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM);
}

//=========================================================
// AttackSound
//=========================================================
void CZombieBull::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1, ATTN_NORM);
}

//=========================================================
// BiteSound
//=========================================================
void CZombieBull::BiteSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pBiteSounds), 1, ATTN_NORM);
}

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CZombieBull::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
		return CBaseMonster::GetSchedule();

	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK2);
		}

		return GetScheduleOfType(SCHED_CHASE_ENEMY);

		break;
	}
	}

	return CBaseMonster::GetSchedule();
}


//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CZombieBull::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
		AttackSound();

		CBaseMonster::StartTask(pTask);
		break;
	}
	default:
	{
		CBullsquid::StartTask(pTask);
		break;
	}
	}
}