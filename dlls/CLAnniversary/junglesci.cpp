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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HC_AE_JUMPATTACK	( 2 )
#define		HC_AE_MELEEATTACK	( 1 )

Task_t	tlJSRangeAttack1[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_WAIT_RANDOM, (float)0.5 },
};

Schedule_t	slJSRangeAttack1[] =
{
	{
		tlJSRangeAttack1,
		ARRAYSIZE(tlJSRangeAttack1),
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t	tlJSRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slJSRangeAttack1Fast[] =
{
	{
		tlJSRangeAttack1Fast,
		ARRAYSIZE(tlJSRangeAttack1Fast),
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

class CJunglesci : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	void SetYawSpeed(void);
	void EXPORT LeapTouch(CBaseEntity *pOther);
	void PainSound(void);
	void DeathSound(void);
	void IdleSound(void);
	void AlertSound(void);
	void PrescheduleThink(void);
	int  Classify(void);
	float JumpCooldown;
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);

	virtual float GetDamageAmount(void) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch(void) { return 80; }
	virtual float GetSoundVolue(void) { return 1.0; }
	Schedule_t* GetScheduleOfType(int Type);

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};
LINK_ENTITY_TO_CLASS(monster_junglesci, CJunglesci);

DEFINE_CUSTOM_SCHEDULES(CJunglesci)
{
	slJSRangeAttack1,
		slJSRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES(CJunglesci, CBaseMonster);

const char *CJunglesci::pAlertSounds[] =
{
	"junglesci/alert1.wav",
	"junglesci/alert2.wav",
	"junglesci/alert3.wav",
	"junglesci/alert4.wav",
	"junglesci/alert5.wav",
};
const char *CJunglesci::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav",
};
const char *CJunglesci::pAttackSounds[] =
{
	"junglesci/chargescream.wav",
	"junglesci/chargescream2.wav",
};

const char *CJunglesci::pDeathSounds[] =
{
	"scientist/scream14.wav",
	"scientist/scream18.wav",
	"scientist/scream15.wav",
	"scientist/sci_die1.wav",
	"scientist/sci_die2.wav",
	"scientist/sci_die3.wav",
	"scientist/sci_die4.wav",
};

const char *CJunglesci::pBiteSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CJunglesci::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CJunglesci::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 760;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 760;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 760;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 760;
		break;
	default:
		ys = 760;
		break;
	}

	pev->yaw_speed = ys;
}

const char *CJunglesci::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CJunglesci::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

BOOL CJunglesci::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 100 && flDot >= 0.6 && m_hEnemy != NULL)
	{
		//pev->velocity = pev->velocity + gpGlobals->v_forward * 200;
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CJunglesci::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case HC_AE_MELEEATTACK:
	{
							  // do stuff for this event.
							  CBaseEntity *pHurt = CheckTraceHullAttack(100, gSkillData.zombieDmgBothSlash, DMG_SLASH);
							  if (pHurt)
							  {
								  int iSound = RANDOM_LONG(0, 2);
								  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
								  {
									  pHurt->pev->punchangle.x = 5;
									  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
									  pHurt->pev->speed = pHurt->pev->speed / 2;
									  EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), 1.0, ATTN_IDLE, 0, GetVoicePitch());
								  }
								  if (iSound != 0)
									  EMIT_SOUND_DYN(edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
							  }

	}
		break;
	case HC_AE_JUMPATTACK:
	{
							 ClearBits(pev->flags, FL_ONGROUND);

							 UTIL_SetOrigin(pev, pev->origin + Vector(0, 0, 1));// take him off ground so engine doesn't instantly reset onground 
							 UTIL_MakeVectors(pev->angles);

							 Vector vecJumpDir;
							 if (m_hEnemy != NULL)
							 {
								 float gravity = g_psv_gravity->value;
								 if (gravity <= 1)
									 gravity = 1;

								 // How fast does the headcrab need to travel to reach that height given gravity?
								 float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
								 //float height = (m_hEnemy->pev->origin.z);
								 if (height < 16)
									 height = 16;
								 float speed = sqrt(2 * gravity * height);
								 float time = speed / gravity;

								 // Scale the sideways velocity to get there at the right time
								 vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
								 vecJumpDir = vecJumpDir * (1.0 / time);

								 // Speed to offset gravity at the desired height
								 vecJumpDir.z = speed;

								 // Don't jump too far/fast
								 float distance = vecJumpDir.Length();

								 if (distance > 650)
								 {
									 vecJumpDir = vecJumpDir * (650.0 / distance);
								 }
							 }
							 else
							 {
								 // jump hop, don't care where
								 vecJumpDir = Vector(gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z) * 350;
							 }

							 int iSound = RANDOM_LONG(0, 2);
							 if (iSound != 0)
								 EMIT_SOUND_DYN(edict(), CHAN_VOICE, pAttackSounds[iSound], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());

							 pev->velocity = vecJumpDir;
							 m_flNextAttack = gpGlobals->time + 2;
	}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CJunglesci::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/junglesci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 80;
	pev->view_ofs = VEC_HUMAN_HULL_MAX;// position of the eyes relative to monster's origin.
	//pev->yaw_speed = 1;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CJunglesci::Precache()
{
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);
	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_MODEL("models/junglesci.mdl");
}


//=========================================================
// RunTask 
//=========================================================
void CJunglesci::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
	{
							   if (m_fSequenceFinished)
							   {
								   TaskComplete();
								   SetTouch(NULL);
								   m_IdealActivity = ACT_IDLE;
							   }
							   break;
	}
	default:
	{
			   CBaseMonster::RunTask(pTask);
	}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CJunglesci::LeapTouch(CBaseEntity *pOther)
{
	if (!pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), 1.0, ATTN_IDLE, 0, GetVoicePitch());

		pOther->TakeDamage(pev, pev, GetDamageAmount(), DMG_SLASH);
	}

	SetTouch(NULL);
}

//=========================================================
// PrescheduleThink
//=========================================================
void CJunglesci::PrescheduleThink(void)
{
	// make the crab coo a little bit in combat state
	if (m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT(0, 5) < 0.1)
	{
		IdleSound();
	}
}

void CJunglesci::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
							   EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
							   m_IdealActivity = ACT_RANGE_ATTACK1;
							   SetTouch(&CJunglesci::LeapTouch);
							   break;
	}
	default:
	{
			   CBaseMonster::StartTask(pTask);
	}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CJunglesci::CheckRangeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && FBitSet(pev->flags, FL_ONGROUND) && flDist >= 70 && flDist <= 600 && flDot >= 0.5 && JumpCooldown < gpGlobals->time)
	{
		JumpCooldown = gpGlobals->time + 10;
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CJunglesci::CheckRangeAttack2(float flDot, float flDist)
{
	return FALSE;
	// BUGBUG: Why is this code here?  There is no ACT_RANGE_ATTACK2 animation.  I've disabled it for now.
#if 0
	if (FBitSet(pev->flags, FL_ONGROUND) && flDist > 64 && flDist <= 256 && flDot >= 0.5)
	{
		return TRUE;
	}
	return FALSE;
#endif
}

int CJunglesci::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if (bitsDamageType & DMG_ACID)
		flDamage = 0;

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// IdleSound
//=========================================================
#define CRAB_ATTN_IDLE (float)1.5
void CJunglesci::IdleSound(void)
{
	//Nothing
}

//=========================================================
// AlertSound 
//=========================================================
void CJunglesci::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound 
//=========================================================
void CJunglesci::PainSound(void)
{
	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		switch (RANDOM_LONG(0, 10))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain3.wav", 1, ATTN_NORM, 0, 100); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain4.wav", 1, ATTN_NORM, 0, 100); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain5.wav", 1, ATTN_NORM, 0, 100); break;
		case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain6.wav", 1, ATTN_NORM, 0, 100); break;
		case 6: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain7.wav", 1, ATTN_NORM, 0, 100); break;
		case 7: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain8.wav", 1, ATTN_NORM, 0, 100); break;
		case 8: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain9.wav", 1, ATTN_NORM, 0, 100); break;
		case 9: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain10.wav", 1, ATTN_NORM, 0, 100); break;
		case 10: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain11.wav", 1, ATTN_NORM, 0, 100); break;
		}
		return;
	}
	//SPECIAL HAPPENING
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// DeathSound 
//=========================================================
void CJunglesci::DeathSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

Schedule_t* CJunglesci::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
	{
								return &slJSRangeAttack1[0];
	}
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}
