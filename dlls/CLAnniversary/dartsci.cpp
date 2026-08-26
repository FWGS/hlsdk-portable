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

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

#define BOLT_AIR_VELOCITY	2000
#define BOLT_WATER_VELOCITY	1000

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CScDart : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir);
	void Touch(CBaseEntity *pOther);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS(scdart, CScDart);

TYPEDESCRIPTION	CScDart::m_SaveData[] =
{
	DEFINE_FIELD(CScDart, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CScDart, CBaseEntity);

void CScDart::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CScDart::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir)
{
	CScDart *pSpit = GetClassPtr((CScDart *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CScDart::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	if (!pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/xbow_hit1.wav", 1, ATTN_NORM, 0, iPitch);
	}
	else
	{
		pOther->TakeDamage(pev, pev, 5, DMG_POISON);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, iPitch);
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSQUID_AE_SPIT		( 1 )
#define		BSQUID_AE_BITE		( 2 )
#define		BSQUID_AE_BLINK		( 3 )
#define		BSQUID_AE_TAILWHIP	( 4 )
#define		BSQUID_AE_HOP		( 5 )
#define		BSQUID_AE_THROW		( 6 )

class CDartsci : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	void RunAI(void);
	BOOL FValidateHintType(short sHint);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IRelationship(CBaseEntity *pTarget);
	int IgnoreConditions(void);
	MONSTERSTATE GetIdealState(void);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
};
LINK_ENTITY_TO_CLASS(monster_dartsci, CDartsci);

TYPEDESCRIPTION	CDartsci::m_SaveData[] =
{
	DEFINE_FIELD(CDartsci, m_fCanThreatDisplay, FIELD_BOOLEAN),
	DEFINE_FIELD(CDartsci, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CDartsci, m_flNextSpitTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CDartsci, CBaseMonster);

//=========================================================
// IgnoreConditions 
//=========================================================
int CDartsci::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CDartsci::IRelationship(CBaseEntity *pTarget)
{
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CDartsci::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CDartsci::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CDartsci::CheckMeleeAttack1(float flDot, float flDist)
{
	if (m_hEnemy->pev->health <= gSkillData.bullsquidDmgWhip && flDist <= 85 && flDot >= 0.7)
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
BOOL CDartsci::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7 && !HasConditions(bits_COND_CAN_MELEE_ATTACK1))		// The player & bullsquid can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CDartsci::FValidateHintType(short sHint)
{
	int i;

	static short sSquidHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for (i = 0; i < ARRAYSIZE(sSquidHints); i++)
	{
		if (sSquidHints[i] == sHint)
		{
			return TRUE;
		}
	}

	ALERT(at_aiconsole, "Couldn't validate hint type");
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CDartsci::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDartsci::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CDartsci::IdleSound(void)
{

}

//=========================================================
// PainSound 
//=========================================================
void CDartsci::PainSound(void)
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
	int iPitch = RANDOM_LONG(80, 100);

	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CDartsci::AlertSound(void)
{
	int iPitch = RANDOM_LONG(80, 100);

	switch (RANDOM_LONG(0, 5))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dartsci/alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dartsci/alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dartsci/alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dartsci/alert4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dartsci/alert5.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDartsci::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 570;	break;
	case	ACT_RUN:			ys = 570;	break;
	case	ACT_IDLE:			ys = 570;	break;
	case	ACT_RANGE_ATTACK1:	ys = 570;	break;
	default:
		ys = 570;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDartsci::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case BSQUID_AE_SPIT:
	{
						   Vector	vecSpitOffset;
						   Vector	vecSpitDir;
						   Vector anglesAim = pev->v_angle + pev->punchangle;
						   UTIL_MakeVectors(anglesAim);
						   anglesAim.x = -anglesAim.x;
						   UTIL_MakeVectors(pev->angles);

						   // !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
						   // we should be able to read the position of bones at runtime for this info.
						   vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 55);
						   vecSpitOffset = (pev->origin + vecSpitOffset);
						   if (m_hEnemy)
								vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 15) - vecSpitOffset).Normalize();
						   else
							   vecSpitDir = ((pev->origin + gpGlobals->v_forward * 32) - vecSpitOffset).Normalize();

						   // do stuff for this event.
						   AttackSound();

						   CScDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, anglesAim);
	}
		break;

	case BSQUID_AE_BITE:
	{
						   // SOUND HERE!
						   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.bullsquidDmgBite, DMG_SLASH);

						   if (pHurt)
						   {
							   //pHurt->pev->punchangle.z = -15;
							   //pHurt->pev->punchangle.x = -45;
							   pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
							   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
							   switch (RANDOM_LONG(0, 2))
							   {
							   case 0:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, 100);
								   break;
							   case 1:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, 100);
								   break;
							   case 2:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, 100);
								   break;
							   }
						   }
	}
		break;

	case BSQUID_AE_TAILWHIP:
	{
							   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.bullsquidDmgWhip, DMG_CLUB | DMG_ALWAYSGIB);
							   if (pHurt)
							   {
								   //pHurt->pev->punchangle.z = -20;
								   //pHurt->pev->punchangle.x = 20;
								   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
								   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
								   switch (RANDOM_LONG(0, 2))
								   {
								   case 0:
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, 100);
									   break;
								   case 1:
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, 100);
									   break;
								   case 2:
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, 100);
									   break;
								   }
							   }
	}
		break;

	case BSQUID_AE_THROW:
	{
							int iPitch;

							// squid throws its prey IF the prey is a client. 
							CBaseEntity *pHurt = CheckTraceHullAttack(70, 0, 0);


							if (pHurt)
							{
								// croonchy bite sound
								iPitch = RANDOM_FLOAT(80, 100);
								switch (RANDOM_LONG(0, 2))
								{
								case 0:
									EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, iPitch);
									break;
								case 1:
									EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, iPitch);
									break;
								case 2:
									EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, iPitch);
									break;
								}


								//pHurt->pev->punchangle.x = RANDOM_LONG(0,34) - 5;
								//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
								//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;

								// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
								UTIL_ScreenShake(pHurt->pev->origin, 25.0, 1.5, 0.7, 2);

								if (pHurt->IsPlayer())
								{
									UTIL_MakeVectors(pev->angles);
									pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
								}
							}
	}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CDartsci::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/dartsci.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 50;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDartsci::Precache()
{
	PRECACHE_MODEL("models/dartsci.mdl");

	PRECACHE_MODEL("models/crossbow_bolt.mdl");// spit projectile.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event
	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");

	PRECACHE_SOUND("dartsci/blowdart.wav");

	PRECACHE_SOUND("scientist/sci_die1.wav");
	PRECACHE_SOUND("scientist/sci_die2.wav");
	PRECACHE_SOUND("scientist/sci_die3.wav");
	PRECACHE_SOUND("scientist/sci_die4.wav");

	PRECACHE_SOUND("dartsci/alert1.wav");
	PRECACHE_SOUND("dartsci/alert2.wav");
	PRECACHE_SOUND("dartsci/alert3.wav");
	PRECACHE_SOUND("dartsci/alert4.wav");
	PRECACHE_SOUND("dartsci/alert5.wav");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");

}

//=========================================================
// DeathSound
//=========================================================
void CDartsci::DeathSound(void)
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "scientist/sci_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "scientist/sci_die2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "scientist/sci_die3.wav", 1, ATTN_NORM);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "scientist/sci_die4.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CDartsci::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "dartsci/blowdart.wav", 1, ATTN_NORM);
}


//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CDartsci::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (pev->skin != 0)
	{
		// close eye if it was open.
		pev->skin = 0;
	}

	if (RANDOM_LONG(0, 39) == 0)
	{
		pev->skin = 1;
	}

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlDartsciRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slDartsciRangeAttack1[] =
{
	{
		tlDartsciRangeAttack1,
		ARRAYSIZE(tlDartsciRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlDartsciChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slDartsciChaseEnemy[] =
{
	{
		tlDartsciChaseEnemy1,
		ARRAYSIZE(tlDartsciChaseEnemy1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_SMELL_FOOD |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER |
		bits_SOUND_MEAT,
		"Squid Chase Enemy"
	},
};


DEFINE_CUSTOM_SCHEDULES(CDartsci)
{
	slDartsciRangeAttack1,
		slDartsciChaseEnemy
};

IMPLEMENT_CUSTOM_SCHEDULES(CDartsci, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CDartsci::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
								// dead enemy
								if (HasConditions(bits_COND_ENEMY_DEAD))
								{
									// call base class, all code to handle dead enemies is centralized there.
									return CBaseMonster::GetSchedule();
								}

								if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
								{
									return GetScheduleOfType(SCHED_RANGE_ATTACK1);
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
// GetScheduleOfType
//=========================================================
Schedule_t* CDartsci::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slDartsciRangeAttack1[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slDartsciChaseEnemy[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CDartsci::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
							   switch (RANDOM_LONG(0, 2))
							   {
							   case 0:
								   //EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl.wav", 1, ATTN_NORM );		
								   break;
							   case 1:
								   //EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl2.wav", 1, ATTN_NORM );	
								   break;
							   case 2:
								   //EMIT_SOUND( ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl3.wav", 1, ATTN_NORM );	
								   break;
							   }

							   CBaseMonster::StartTask(pTask);
							   break;
	}
	case TASK_GET_PATH_TO_ENEMY:
	{
								   if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
								   {
									   m_iTaskStatus = TASKSTATUS_COMPLETE;
								   }
								   else
								   {
									   ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
									   TaskFail();
								   }
								   break;
	}
	default:
	{
			   CBaseMonster::StartTask(pTask);
			   break;
	}
	}
}


//=========================================================
// RunTask
//=========================================================
void CDartsci::RunTask(Task_t *pTask)
{
	CBaseMonster::RunTask(pTask);
}


//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CDartsci::GetIdealState(void)
{
	int	iConditions;

	iConditions = IScheduleFlags();


	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}

