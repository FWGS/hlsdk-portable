/***
*
*	Copyright (c) 1996-PITCH_NORM1, Valve LLC. All rights reserved.
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
// Baby Gargantua
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "customentity.h"
#include "weapons.h"
#include "effects.h"
#include "soundent.h"
#include "decals.h"
#include "explode.h"
#include "func_break.h"

//=========================================================
// Baby Gargantua Monster
//=========================================================
const float BGARG_ATTACKDIST = 80.0;

// Garg animation events
#define BGARG_AE_SLASH_LEFT 1
//#define BGARG_AE_BEAM_ATTACK_RIGHT	2		// No longer used
#define BGARG_AE_LEFT_FOOT 3
#define BGARG_AE_RIGHT_FOOT 4

#define BGARG_FLAME_LENGTH 300
#define BGARG_BEAM_SPRITE_NAME "sprites/xbeam3.spr"
#define BGARG_BEAM_SPRITE2 "sprites/xbeam3.spr"


#define ATTN_BGARG (ATTN_NORM)

class CBabyGargantua : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void DeathSound();
	void AlertSound();
	void PainSound();
	void IdleSound();
	BOOL CheckMeleeAttack1(float flDot, float flDist); // Swipe
	BOOL CheckMeleeAttack2(float flDot, float flDist); // Flames
	BOOL CheckRangeAttack1(float flDot, float flDist); // Stomp attack
	void SetObjectCollisionBox()
	{
		pev->absmin = pev->origin + Vector(-80, -80, 0);
		pev->absmax = pev->origin + Vector(80, 80, 214);
	}

	Schedule_t* GetScheduleOfType(int Type);
	void StartTask(Task_t* pTask);
	void RunTask(Task_t* pTask);
	void GibMonster();
	void FlameCreate();
	void FlameUpdate();
	void FlameControls(float angleX, float angleY);
	void FlameDestroy();
	inline bool FlameIsOn() { return m_pFlame[0] != NULL; }

	void FlameDamage(Vector vecStart, Vector vecEnd, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType);

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	float m_flNextPainTime;

	CUSTOM_SCHEDULES;

private:
	static const char* pAttackHitSounds[];
	static const char* pBeamAttackSounds[];
	static const char* pAttackMissSounds[];
	static const char* pFootSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackSounds[];

	CBaseEntity* BabyGargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CBeam* m_pFlame[4];	 // Flame beams

	float m_flameTime;	   // Time of next flame attack
	float m_painSoundTime; // Time of next pain sound
	float m_streakTime;	   // streak timer (don't send too many)
	float m_flameX;		   // Flame thrower aim
	float m_flameY;
};

LINK_ENTITY_TO_CLASS(monster_baby_gargantua, CBabyGargantua);

TYPEDESCRIPTION CBabyGargantua::m_SaveData[] =
{
	DEFINE_FIELD(CBabyGargantua, m_flameTime, FIELD_TIME),
	DEFINE_FIELD(CBabyGargantua, m_streakTime, FIELD_TIME),
	DEFINE_FIELD(CBabyGargantua, m_painSoundTime, FIELD_TIME),
	DEFINE_ARRAY(CBabyGargantua, m_pFlame, FIELD_CLASSPTR, 4),
	DEFINE_FIELD(CBabyGargantua, m_flameX, FIELD_FLOAT),
	DEFINE_FIELD(CBabyGargantua, m_flameY, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CBabyGargantua, CBaseMonster);

const char* CBabyGargantua::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CBabyGargantua::pBeamAttackSounds[] =
{
	"garg/gar_flameoff1.wav",
	"garg/gar_flameon1.wav",
	"garg/gar_flamerun1.wav",
};


const char* CBabyGargantua::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CBabyGargantua::pFootSounds[] =
{
	"bgarg/bgar_step1.wav",
	"bgarg/bgar_step2.wav",
};


const char* CBabyGargantua::pIdleSounds[] =
{
	"bgarg/bgar_idle1.wav",
	"bgarg/bgar_idle2.wav",
	"bgarg/bgar_idle3.wav",
	"bgarg/bgar_idle4.wav",
	"bgarg/bgar_idle5.wav",
};


const char* CBabyGargantua::pAttackSounds[] =
{
	"bgarg/bgar_attack1.wav",
	"bgarg/bgar_attack2.wav",
	"bgarg/bgar_attack3.wav",
};

const char* CBabyGargantua::pAlertSounds[] =
{
	"bgarg/bgar_alert1.wav",
	"bgarg/bgar_alert2.wav",
	"bgarg/bgar_alert3.wav",
};

const char* CBabyGargantua::pPainSounds[] =
{
	"bgarg/bgar_pain1.wav",
	"bgarg/bgar_pain2.wav",
	"bgarg/bgar_pain3.wav",
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
#if 0
enum
{
	SCHED_ = LAST_COMMON_SCHEDULE + 1,
};
#endif

enum
{
	TASK_SOUND_ATTACK = LAST_COMMON_TASK + 1,
	TASK_FLAME_SWEEP,
};

Task_t tlBGargFlame[] =
{
	{TASK_STOP_MOVING, (float)0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_SOUND_ATTACK, (float)0},
	// { TASK_PLAY_SEQUENCE,		(float)ACT_SIGNAL1	},
	{TASK_SET_ACTIVITY, (float)ACT_MELEE_ATTACK2},
	{TASK_FLAME_SWEEP, (float)4.5},
	{TASK_SET_ACTIVITY, (float)ACT_IDLE},
};

Schedule_t slBGargFlame[] =
{
	{tlBGargFlame,
		ARRAYSIZE(tlBGargFlame),
		0,
		0,
		"GargFlame"},
};


// primary melee attack
Task_t tlBGargSwipe[] =
{
	{TASK_STOP_MOVING, 0},
	{TASK_FACE_ENEMY, (float)0},
	{TASK_MELEE_ATTACK1, (float)0},
};

Schedule_t slBGargSwipe[] =
{
	{tlBGargSwipe,
		ARRAYSIZE(tlBGargSwipe),
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"GargSwipe"},
};


DEFINE_CUSTOM_SCHEDULES(CBabyGargantua) {
	slBGargFlame,
		slBGargSwipe,
};

IMPLEMENT_CUSTOM_SCHEDULES(CBabyGargantua, CBaseMonster);


void CBabyGargantua::FlameCreate()
{
	int i;
	Vector posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors(pev->angles);

	for (i = 0; i < 4; i++)
	{
		if (i < 2)
			m_pFlame[i] = CBeam::BeamCreate(BGARG_BEAM_SPRITE_NAME, 80);
		else
			m_pFlame[i] = CBeam::BeamCreate(BGARG_BEAM_SPRITE2, 40);
		if (m_pFlame[i])
		{
			int attach = i % 2;
			// attachment is 0 based in GetAttachment
			GetAttachment(attach + 1, posGun, angleGun);

			Vector vecEnd = (gpGlobals->v_forward * BGARG_FLAME_LENGTH) + posGun;
			UTIL_TraceLine(posGun, vecEnd, dont_ignore_monsters, edict(), &trace);

			m_pFlame[i]->PointEntInit(trace.vecEndPos, entindex());
			if (i < 2)
				m_pFlame[i]->SetColor(255, 130, 90);
			else
				m_pFlame[i]->SetColor(0, 120, 255);
			m_pFlame[i]->SetBrightness(190);
			m_pFlame[i]->SetFlags(BEAM_FSHADEIN);
			m_pFlame[i]->SetScrollRate(20);
			// attachment is 1 based in SetEndAttachment
			m_pFlame[i]->SetEndAttachment(attach + 2);
			CSoundEnt::InsertSound(bits_SOUND_COMBAT, posGun, 384, 0.3);
		}
	}
	EMIT_SOUND_DYN(edict(), CHAN_BODY, pBeamAttackSounds[1], 1.0, ATTN_NORM, 0, PITCH_NORM);
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pBeamAttackSounds[2], 1.0, ATTN_NORM, 0, PITCH_NORM);
}


void CBabyGargantua::FlameControls(float angleX, float angleY)
{
	if (angleY < -180)
		angleY += 360;
	else if (angleY > 180)
		angleY -= 360;

	if (angleY < -45)
		angleY = -45;
	else if (angleY > 45)
		angleY = 45;

	m_flameX = UTIL_ApproachAngle(angleX, m_flameX, 4);
	m_flameY = UTIL_ApproachAngle(angleY, m_flameY, 8);
	SetBoneController(0, m_flameY);
	SetBoneController(1, m_flameX);
}


void CBabyGargantua::FlameUpdate()
{
	int i;
	static float offset[2] = { 60, -60 };
	TraceResult trace;
	Vector vecStart, angleGun;
	bool streaks = false;

	for (i = 0; i < 2; i++)
	{
		if (m_pFlame[i])
		{
			Vector vecAim = pev->angles;
			vecAim.x += m_flameX;
			vecAim.y += m_flameY;

			UTIL_MakeVectors(vecAim);

			GetAttachment(i + 1, vecStart, angleGun);
			Vector vecEnd = vecStart + (gpGlobals->v_forward * BGARG_FLAME_LENGTH); //  - offset[i] * gpGlobals->v_right;

			UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, edict(), &trace);

			m_pFlame[i]->SetStartPos(trace.vecEndPos);
			m_pFlame[i + 2]->SetStartPos((vecStart * 0.6) + (trace.vecEndPos * 0.4));

			FlameDamage(vecStart, trace.vecEndPos, pev, pev, gSkillData.bgargDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN);

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_ELIGHT);
			WRITE_SHORT(entindex() + 0x1000 * (i + 2)); // entity, attachment
			WRITE_COORD(vecStart.x);					// origin
			WRITE_COORD(vecStart.y);
			WRITE_COORD(vecStart.z);
			WRITE_COORD(RANDOM_FLOAT(32, 48)); // radius
			WRITE_BYTE(236);				   // R
			WRITE_BYTE(140);				   // G
			WRITE_BYTE(52);				   // B
			WRITE_BYTE(2);					   // life * 10
			WRITE_COORD(0);					   // decay
			MESSAGE_END();
		}
	}
	if (streaks)
		m_streakTime = gpGlobals->time;
}



void CBabyGargantua::FlameDamage(Vector vecStart, Vector vecEnd, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType)
{
	CBaseEntity* pEntity = NULL;
	TraceResult tr;
	float flAdjustedDamage;
	Vector vecSpot;

	Vector vecMid = (vecStart + vecEnd) * 0.5;

	float searchRadius = (vecStart - vecMid).Length();

	Vector vecAim = (vecEnd - vecStart).Normalize();

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, vecMid, searchRadius)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			// UNDONE: this should check a damage mask, not an ignore
			if (iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore)
			{ // houndeyes don't hurt other houndeyes with their attack
				continue;
			}

			vecSpot = pEntity->BodyTarget(vecMid);

			float dist = DotProduct(vecAim, vecSpot - vecMid);
			if (dist > searchRadius)
				dist = searchRadius;
			else if (dist < -searchRadius)
				dist = searchRadius;

			Vector vecSrc = vecMid + dist * vecAim;

			UTIL_TraceLine(vecSrc, vecSpot, dont_ignore_monsters, ENT(pev), &tr);

			if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
			{ // the explosion can 'see' this entity, so hurt them!
				// decrease damage for an ent that's farther from the flame.
				dist = (vecSrc - tr.vecEndPos).Length();

				if (dist > 64)
				{
					flAdjustedDamage = flDamage - (dist - 64) * 0.4;
					if (flAdjustedDamage <= 0)
						continue;
				}
				else
				{
					flAdjustedDamage = flDamage;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage();
					pEntity->TraceAttack(pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize(), &tr, bitsDamageType);
					ApplyMultiDamage(pevInflictor, pevAttacker);
				}
				else
				{
					pEntity->TakeDamage(pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType);
				}
			}
		}
	}
}


void CBabyGargantua::FlameDestroy()
{
	int i;

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pBeamAttackSounds[0], 1.0, ATTN_NORM, 0, PITCH_NORM);
	for (i = 0; i < 4; i++)
	{
		if (m_pFlame[i])
		{
			UTIL_Remove(m_pFlame[i]);
			m_pFlame[i] = NULL;
		}
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CBabyGargantua::Classify()
{
	return CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBabyGargantua::SetYawSpeed()
{
	int ys;

	ys = 140;

	pev->yaw_speed = ys;
}


//=========================================================
// Spawn
//=========================================================
void CBabyGargantua::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/bgarg.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.bgargHealth;
	//pev->view_ofs		= Vector ( 0, 0, 96 );// taken from mdl file
	m_flFieldOfView = -0.2; // width of forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	m_flameTime = gpGlobals->time + 2;
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBabyGargantua::Precache()
{
	PRECACHE_MODEL("models/bgarg.mdl");
	PRECACHE_MODEL(BGARG_BEAM_SPRITE_NAME);
	PRECACHE_MODEL(BGARG_BEAM_SPRITE2);
	PRECACHE_SOUND("bgarg/bgar_die1.wav");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pBeamAttackSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pFootSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
}

void CBabyGargantua::DeathSound()
{
	SetBoneController(0, 0);
	SetBoneController(1, 0);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bgarg/bgar_die1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
}

//=========================================================
// AlertSound
//=========================================================
void CBabyGargantua::AlertSound()
{
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
}

//=========================================================
// AlertSound
//=========================================================
void CBabyGargantua::PainSound()
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 1.0;

	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pIdleSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
}

//=========================================================
// IdleSound
//=========================================================
void CBabyGargantua::IdleSound()
{
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, 150);
}

void CBabyGargantua::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// Don't inflict damage each other.
	if (bitsDamageType == DMG_BURN)
	{
		flDamage = 0;
	}

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}



BOOL CBabyGargantua::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckMeleeAttack1
// Garg swipe attack
//
//=========================================================
BOOL CBabyGargantua::CheckMeleeAttack1(float flDot, float flDist)
{
	//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if (flDot >= 0.7)
	{
		if (flDist <= BGARG_ATTACKDIST)
			return TRUE;
	}
	return FALSE;
}


// Flame thrower madness!
BOOL CBabyGargantua::CheckMeleeAttack2(float flDot, float flDist)
{
	//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if (gpGlobals->time > m_flameTime)
	{
		if (flDot >= 0.8 && flDist > BGARG_ATTACKDIST)
		{
			if (flDist <= BGARG_FLAME_LENGTH)
				return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
//
// Stomp attack
//
//=========================================================
BOOL CBabyGargantua::CheckRangeAttack1(float flDot, float flDist)
{
	return FALSE;
}




//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBabyGargantua::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BGARG_AE_SLASH_LEFT:
	{
		// HACKHACK!!!
		CBaseEntity* pHurt = BabyGargantuaCheckTraceHullAttack(BGARG_ATTACKDIST + 10.0, gSkillData.bgargDmgSlash, DMG_SLASH);
		if (pHurt)
		{
			if ((pHurt->pev->flags & (FL_MONSTER | FL_CLIENT)) != 0)
			{
				pHurt->pev->punchangle.x = -15; // pitch
				pHurt->pev->punchangle.y = -5; // yaw
				pHurt->pev->punchangle.z = 5;	// roll
				//UTIL_MakeVectors(pev->angles);	// called by CheckTraceHullAttack
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackMissSounds), 1.0, ATTN_NORM, 0, PITCH_NORM);

		Vector forward;
		UTIL_MakeVectorsPrivate(pev->angles, forward, NULL, NULL);
	}
	break;

	case BGARG_AE_RIGHT_FOOT:
	case BGARG_AE_LEFT_FOOT:
		EMIT_SOUND_DYN(edict(), CHAN_BODY, RANDOM_SOUND_ARRAY(pFootSounds), 0.5, ATTN_NORM, 0, PITCH_NORM);
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for BabyGargantua because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CBabyGargantua::BabyGargantuaCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += 64;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);

		if (iDamage > 0)
		{
			pEntity->TakeDamage(pev, pev, iDamage, iDmgType);
		}

		return pEntity;
	}

	return NULL;
}


Schedule_t* CBabyGargantua::GetScheduleOfType(int Type)
{
	// HACKHACK - turn off the flames if they are on and garg goes scripted / dead
	if (FlameIsOn())
		FlameDestroy();

	switch (Type)
	{
	case SCHED_MELEE_ATTACK2:
		return slBGargFlame;
	case SCHED_MELEE_ATTACK1:
		return slBGargSwipe;
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}


void CBabyGargantua::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FLAME_SWEEP:
		FlameCreate();
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		m_flameTime = gpGlobals->time + 6;
		m_flameX = 0;
		m_flameY = 0;
		break;

	case TASK_SOUND_ATTACK:
		if (RANDOM_LONG(0, 100) < 30)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_BGARG, 0, PITCH_NORM);
		TaskComplete();
		break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CBabyGargantua::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FLAME_SWEEP:
		if (gpGlobals->time > m_flWaitFinished)
		{
			FlameDestroy();
			TaskComplete();
			FlameControls(0, 0);
			SetBoneController(0, 0);
			SetBoneController(1, 0);
		}
		else
		{
			bool cancel = false;

			Vector angles = g_vecZero;

			FlameUpdate();
			CBaseEntity* pEnemy = m_hEnemy;
			if (pEnemy)
			{
				Vector org = pev->origin;
				org.z += 64;
				Vector dir = pEnemy->BodyTarget(org) - org;
				angles = UTIL_VecToAngles(dir);
				angles.x = -angles.x;
				angles.y -= pev->angles.y;
				if (dir.Length() > 400)
					cancel = true;
			}
			if (fabs(angles.y) > 60)
				cancel = true;

			if (cancel)
			{
				m_flWaitFinished -= 0.5;
				m_flameTime -= 0.5;
			}
			// FlameControls( angles.x + 2 * sin(gpGlobals->time*8), angles.y + 28 * sin(gpGlobals->time*8.5) );
			FlameControls(angles.x, angles.y);
		}
		break;

	default:
		CBaseMonster::RunTask(pTask);
		break;
	}
}

void CBabyGargantua::GibMonster()
{
	FlameDestroy();
	CBaseMonster::GibMonster();
}