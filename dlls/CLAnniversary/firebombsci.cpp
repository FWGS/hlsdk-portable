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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		THROWBOMB	( 1 )
#define		PICKBOMB	( 2 )

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CFirebombsci : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);

	float m_flNextFlinch;

	Vector vecToss;

	void PainSound(void);
	void AlertSound(void);
	void AttackSound(void);

	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pThrowSounds[];

	Schedule_t	*GetSchedule(void);
	Schedule_t  *GetScheduleOfType(int Type);

	CUSTOM_SCHEDULES;
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);

	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_firebombsci, CFirebombsci);

const char *CFirebombsci::pThrowSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CFirebombsci::pAlertSounds[] =
{
	"firebombsci/fbs_alert1.wav",
	"firebombsci/fbs_alert2.wav",
	"firebombsci/fbs_alert3.wav",
	"firebombsci/fbs_alert4.wav",
};

const char *CFirebombsci::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFirebombsci::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

Task_t	tlSciTakeCover1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)0 },
	{ TASK_WAIT, (float)0.2 },
	{ TASK_FIND_COVER_FROM_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
};

Schedule_t	slSciTakeCover[] =
{
	{
		tlSciTakeCover1,
		ARRAYSIZE(tlSciTakeCover1),
		0,
		0,
		"TakeCover"
	},
};

DEFINE_CUSTOM_SCHEDULES(CFirebombsci)
{
	slSciTakeCover,
};

IMPLEMENT_CUSTOM_SCHEDULES(CFirebombsci, CBaseMonster);


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFirebombsci::SetYawSpeed(void)
{
	int ys;

	ys = 480;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CFirebombsci::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if (IsAlive())
		PainSound();
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CFirebombsci::PainSound(void)
{
	int pitch = 80 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CFirebombsci::AlertSound(void)
{
	int pitch = 80 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CFirebombsci::AttackSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pThrowSounds[RANDOM_LONG(0, ARRAYSIZE(pThrowSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFirebombsci::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case PICKBOMB:
	{
					 pev->body = 1;
	}
		break;

	case THROWBOMB:
	{
					  UTIL_MakeVectors(pev->angles);
					  vecToss = vecToss + gpGlobals->v_up * 72;
					  vecToss = vecToss - gpGlobals->v_forward * 140;
					  CGrenade::ShootFirebomb(pev, pev->origin + gpGlobals->v_forward * 64 + Vector(0, 0, 64), vecToss);
					  pev->body = 0;
	}

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CFirebombsci::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/firebombsci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 80;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFirebombsci::Precache()
{
	PRECACHE_MODEL("models/firebombsci.mdl");
	PRECACHE_SOUND("weapons/firebomb_break.wav");
	PRECACHE_SOUND("weapons/firebomb_set.wav");
	PRECACHE_SOUND("weapons/firebomb_flame.wav");
	PRECACHE_MODEL("models/firebomb.mdl");
	PRECACHE_MODEL("sprites/firebombflame.spr");

	PRECACHE_SOUND_ARRAY(pThrowSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CFirebombsci::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;

}

BOOL CFirebombsci::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 128 && flDot >= 0.6 && m_hEnemy != NULL)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CFirebombsci::CheckRangeAttack1(float flDot, float flDist)
{
	//if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND))
	//{
	//	return FALSE;
	//}
	if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 512)
	{
		// don't try to spit at someone up really high or down really low.
		return FALSE;
	}
	if (HasConditions(bits_COND_SEE_ENEMY) && FBitSet(pev->flags, FL_ONGROUND) && flDist > 128 && flDist <= 1800 && flDot >= 0.8)
	{
		vecToss = VecCheckThrow(pev, GetGunPosition() + gpGlobals->v_up * 4 + gpGlobals->v_forward * 34, m_hEnemy->pev->origin, flDist, 0.8); // use dist as speed to get there in 1 secondx
		return TRUE;
	}
	return FALSE;
}

Schedule_t *CFirebombsci::GetSchedule(void)
{
	if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	return CBaseMonster::GetSchedule();
}

Schedule_t* CFirebombsci::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
										return &slSciTakeCover[0];
	}
	default:
	{
			   return CBaseMonster::GetScheduleOfType(Type);
	}
	}
}