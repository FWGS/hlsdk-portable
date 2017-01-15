#if 0

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
// pitdrone
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

extern int iSquidSpitSprite;

#define		PITDRONE_SPRINT_DIST				256 // how close the squid has to get before starting to sprint and refusing to swerve

#define		PITDRONE_TOLERANCE_MELEE1_RANGE		85
#define		PITDRONE_TOLERANCE_MELEE2_RANGE		85

#define		PITDRONE_TOLERANCE_MELEE1_DOT		0.7
#define		PITDRONE_TOLERANCE_MELEE2_DOT		0.7

#define		PITDRONE_MELEE_ATTACK_RADIUS		70

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define PITDRONE_AE_THROW			( 1 )
#define PITDRONE_AE_SLASH_BOTH		( 2 )
#define PITDRONE_AE_SLASH_SINGLE	( 4 )

//=========================================================
// CPitdrone
//=========================================================
class CPitdrone : public CBullsquid
{
public:

	void Spawn(void);
	void Precache(void);

	void KeyValue(KeyValueData *pkvd);

	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	void StartTask(Task_t *pTask);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	void RunAI(void);

	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IRelationship(CBaseEntity *pTarget);
	int IgnoreConditions(void);

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

protected:

	int m_nAmmoCount;
	int m_fRightSlash;
};

LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitdrone);

TYPEDESCRIPTION	CPitdrone::m_SaveData[] =
{
	DEFINE_FIELD(CPitdrone, m_nAmmoCount, FIELD_INTEGER),
	DEFINE_FIELD(CPitdrone, m_fRightSlash, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CPitdrone, CBullsquid);

void CPitdrone::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "initammo"))
	{
		m_nAmmoCount = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBullsquid::KeyValue(pkvd);
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPitdrone::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CPitdrone::IgnoreConditions(void)
{
	return CBaseMonster::IgnoreConditions();
}

//=========================================================
// IRelationship - overridden for gonome
//=========================================================
int CPitdrone::IRelationship(CBaseEntity *pTarget)
{
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CPitdrone::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > PITDRONE_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CPitdrone::CheckRangeAttack1(float flDot, float flDist)
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
// CheckMeleeAttack1 - pitdrone is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CPitdrone::CheckMeleeAttack1(float flDot, float flDist)
{

	if (m_hEnemy->pev->health <= gSkillData.pitdroneDmgWhip && 
		flDist <= PITDRONE_TOLERANCE_MELEE1_RANGE &&
		flDot >= PITDRONE_TOLERANCE_MELEE1_DOT)
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - pitdrone is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CPitdrone::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= PITDRONE_TOLERANCE_MELEE2_RANGE &&
		flDot >= PITDRONE_TOLERANCE_MELEE2_DOT &&
		!HasConditions(bits_COND_CAN_MELEE_ATTACK1))
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// IdleSound 
//=========================================================
#define PITDRONE_ATTN_IDLE	(float)1.5
void CPitdrone::IdleSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_idle1.wav", 1, PITDRONE_ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_idle2.wav", 1, PITDRONE_ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_idle3.wav", 1, PITDRONE_ATTN_IDLE);
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CPitdrone::PainSound(void)
{
	int iPitch = RANDOM_LONG(85, 120);

	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_pain4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CPitdrone::AlertSound(void)
{
	int iPitch = RANDOM_LONG(140, 160);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pitdrone_alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPitdrone::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{

	case PITDRONE_AE_THROW:
	{
		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		UTIL_MakeVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.


		Vector vecArmPos, vecArmAng;
		GetAttachment(0, vecArmPos, vecArmAng);

		//vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23);
		//vecSpitOffset = (pev->origin + vecSpitOffset);
		vecSpitOffset = vecArmPos;
		vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

		vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);

		// do stuff for this event.
		AttackSound();

		// spew the spittle temporary ents.
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpitOffset);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(vecSpitOffset.x);	// pos
		WRITE_COORD(vecSpitOffset.y);
		WRITE_COORD(vecSpitOffset.z);
		WRITE_COORD(vecSpitDir.x);	// dir
		WRITE_COORD(vecSpitDir.y);
		WRITE_COORD(vecSpitDir.z);
		WRITE_SHORT(iSquidSpitSprite);	// model
		WRITE_BYTE(15);			// count
		WRITE_BYTE(210);			// speed
		WRITE_BYTE(25);			// noise ( client will divide by 100 )
		MESSAGE_END();

		CSquidSpit::Shoot(pev, vecSpitOffset, vecSpitDir * 1200); // Default: 900
	}
	break;

	case PITDRONE_AE_SLASH_SINGLE:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(PITDRONE_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_CLUB);
		if (pHurt)
		{
			// Left slash
			if (!m_fRightSlash)
			{
				pHurt->pev->punchangle.z = -20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 20;
			}
			else // Right slash
			{
				pHurt->pev->punchangle.z = 20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 20;
			}
		}

		m_fRightSlash = !m_fRightSlash;
	}
	break;

	case PITDRONE_AE_SLASH_BOTH:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(PITDRONE_MELEE_ATTACK_RADIUS, gSkillData.gonomeDmgOneSlash, DMG_CLUB);
		if (pHurt)
		{
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -150;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 20;
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
void CPitdrone::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/pit_drone.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.pitdroneHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	m_fRightSlash = FALSE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitdrone::Precache()
{
	PRECACHE_MODEL("models/pit_drone.mdl");

	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.

	iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike2.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_communicate1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate4.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_die1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_die2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_die3.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_eat.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_hunt1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt3.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_idle1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_idle2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_idle3.wav");

	PRECACHE_SOUND("bullchicken/pit_drone_melee_attack1.wav");
	PRECACHE_SOUND("bullchicken/pit_drone_melee_attack2.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_pain1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_pain4.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");
}

//=========================================================
// DeathSound
//=========================================================
void CPitdrone::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die3.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CPitdrone::AttackSound(void)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "pitdrone/pit_drone_attack_spike1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "pitdrone/pit_drone_attack_spike2.wav", 1, ATTN_NORM);
		break;
	}
}

//========================================================
// RunAI - overridden for pitdrone because there are things
// that need to be checked every think.
//========================================================
void CPitdrone::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < PITDRONE_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}

}


//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for pitdrone because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CPitdrone::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
		CBaseMonster::StartTask(pTask);
		break;

	default:
		CBullsquid::StartTask(pTask);
		break;

	}
}
#else

//-------------------------------------------------
// Opposing Forces monster Pit Drone
// Remaking by Demiurge aka Z-Zero SYSTEMS 2004
// All rights by GEARBOX
//
// FGD monster_pitdrone
//	[
//	initammo(string) : "Initial Ammo" : "6"
//	]
//
//-------------------------------------------------

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
#include	"weapons.h"

int		iPitDroneSpitSprite;
//=========================================================
// CPitDrone's spit projectile
//=========================================================
class CPitDroneSpit : public CBaseMonster
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT Touch(CBaseEntity *pOther);

	Vector m_vecForward;

public:
	static CPitDroneSpit *SpitCreate(void);
};

LINK_ENTITY_TO_CLASS(pitdronespit, CPitDroneSpit);

CPitDroneSpit *CPitDroneSpit::SpitCreate(void)
{
	// Create a new entity with CShock private data
	CPitDroneSpit *pSpit = GetClassPtr((CPitDroneSpit *)NULL);
	pSpit->pev->classname = MAKE_STRING("pitdronespit");
	pSpit->Spawn();

	return pSpit;
}

int	CPitDroneSpit::Classify(void)
{
	return	CLASS_ALIEN_BIOWEAPON;
}

void CPitDroneSpit::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("pitdronespit");

	pev->solid = SOLID_TRIGGER;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "models/pit_drone_spike.mdl");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_MakeAimVectors(pev->angles);
	m_vecForward = gpGlobals->v_forward;
	SetTouch(&CPitDroneSpit::Touch);
}

void CPitDroneSpit::Precache(void)
{
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hit2.wav");
}

void CPitDroneSpit::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/xbow_hit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (!pOther->pev->takedamage)
	{

		// make a horn in the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}
	}
	else
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->TraceAttack(pevOwner, gSkillData.pitdroneDmgSpit, pev->velocity.Normalize(), &tr, DMG_GENERIC | DMG_NEVERGIB);

		ApplyMultiDamage(pev, pevOwner);
	}

}

//
// PitDrone, main part.
//
#define HORNGROUP			1
#define PITDRONE_HORNS0		0
#define PITDRONE_HORNS1		1
#define PITDRONE_HORNS2		2
#define PITDRONE_HORNS3		3
#define PITDRONE_HORNS4		4
#define PITDRONE_HORNS5		5
#define PITDRONE_HORNS6		6
#define	PITDRONE_SPRINT_DIST			255
#define PITDRONE_FLINCH_DELAY			2		// at most one flinch every n secs

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_PDRONE_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
	SCHED_PDRONE_EAT,
	SCHED_PDRONE_SNIFF_AND_EAT,
	SCHED_PDRONE_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_PDRONE_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define PIT_DRONE_AE_SPIT			( 1 ) //wrong
#define PIT_DRONE_AE_SLASH1			( 2 ) //ok
#define PIT_DRONE_AE_ATTACK			( 4 ) //ok
#define PIT_DRONE_AE_HOPE			( 5 ) //no
#define PIT_DRONE_AE_SLASH2			( 6 ) //ok
#define PIT_DRONE_AE_RELOAD			( 7 ) //ok

class CPitDrone : public CBaseMonster
{

	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void SetYawSpeed(void);
	void KeyValue(KeyValueData *pkvd);

	int Classify(void);

	BOOL CheckRangeAttack1(float flDot, float flDist);
	void IdleSound(void);
	void PainSound(void);
	void AlertSound(void);
	void DeathSound(void);
	void AttackSound(void);
	void BodyChange(float horns);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IRelationship(CBaseEntity *pTarget);
	int IgnoreConditions(void);
	void StopTalking(void);
	BOOL ShouldSpeak(void);
	Schedule_t* GetSchedule(void);
	Schedule_t* GetScheduleOfType(int Type);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	void RunAI(void);
	CUSTOM_SCHEDULES;
	MONSTERSTATE GetIdealState(void);

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time.
	float	m_flNextSpitTime;// last time the PitDrone used the spit attack.
	float	m_flLastHurtTime;
	float	m_flNextSpeakTime;
	float	m_flNextWordTime;
	float	m_flNextFlinch;
	int		m_iLastWord;

	float m_ammo;
	float horns;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDieSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS(monster_pitdrone, CPitDrone);

TYPEDESCRIPTION	CPitDrone::m_SaveData[] =
{
	DEFINE_FIELD(CPitDrone, m_ammo, FIELD_FLOAT),
	DEFINE_FIELD(CPitDrone, m_fCanThreatDisplay, FIELD_BOOLEAN),
	DEFINE_FIELD(CPitDrone, m_flNextSpitTime, FIELD_TIME),
	DEFINE_FIELD(CPitDrone, m_flNextSpeakTime, FIELD_TIME),
	DEFINE_FIELD(CPitDrone, m_flNextWordTime, FIELD_TIME),
	DEFINE_FIELD(CPitDrone, m_iLastWord, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CPitDrone, CBaseMonster);

void CPitDrone::KeyValue(KeyValueData *pkvd)
{

	if (FStrEq(pkvd->szKeyName, "initammo"))
	{
		m_ammo = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

//=========================================================
// IgnoreConditions 
//=========================================================
int CPitDrone::IgnoreConditions(void)
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
			m_flNextFlinch = gpGlobals->time + PITDRONE_FLINCH_DELAY;
	}

	return iIgnore;

}

const char *CPitDrone::pAttackHitSounds[] =
{
	"pitdrone/pit_drone_melee_attack1.wav",
	"pitdrone/pit_drone_melee_attack2.wav",
};

const char *CPitDrone::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CPitDrone::pAttackSounds[] =
{
	"pitdrone/pit_drone_attack_spike1.wav",
	"pitdrone/pit_drone_attack_spike2.wav",
};

const char *CPitDrone::pIdleSounds[] =
{
	"pitdrone/pit_drone_idle1.wav",
	"pitdrone/pit_drone_idle2.wav",
	"pitdrone/pit_drone_idle3.wav",

};

const char *CPitDrone::pAlertSounds[] =
{
	"pitdrone/pit_drone_alert1.wav",
	"pitdrone/pit_drone_alert2.wav",
	"pitdrone/pit_drone_alert3.wav",
};

const char *CPitDrone::pPainSounds[] =
{
	"pitdrone/pit_drone_pain1.wav",
	"pitdrone/pit_drone_pain2.wav",
	"pitdrone/pit_drone_pain3.wav",
	"pitdrone/pit_drone_pain4.wav",
};

const char *CPitDrone::pDieSounds[] =
{
	"pitdrone/pit_drone_die1.wav",
	"pitdrone/pit_drone_die2.wav",
	"pitdrone/pit_drone_die3.wav",
};

//=========================================================
// IRelationship - overridden for gonome so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CPitDrone::IRelationship(CBaseEntity *pTarget)
{
	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for gonome so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CPitDrone::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the gonome is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > PITDRONE_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CPitDrone::CheckRangeAttack1(float flDot, float flDist)
{
	if (!horns)
	{
		//RunAI();
		return FALSE;
	}
	if (IsMoving() && flDist >= 512)
	{
		// gonome will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{

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
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPitDrone::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 120;	break;
	case	ACT_RUN:			ys = 120;	break;
	case	ACT_IDLE:			ys = 120;	break;
	case	ACT_RANGE_ATTACK1:	ys = 120;	break;
	default:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

void CPitDrone::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{

	case PIT_DRONE_AE_ATTACK:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.pitdroneDmgWhip, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case PIT_DRONE_AE_SLASH1:
	case PIT_DRONE_AE_SLASH2:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.pitdroneDmgBite, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case PIT_DRONE_AE_RELOAD:
	{
		if (bits_COND_NO_AMMO_LOADED)
		{
			horns = m_ammo;
			BodyChange(horns);
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
	}
	break;

	case PIT_DRONE_AE_HOPE:
	{

	}
	break;

	case PIT_DRONE_AE_SPIT:
	{
		if (!horns)
		{
			SetConditions(bits_COND_NO_AMMO_LOADED);
		}
		else
		{
			horns = -horns;
			BodyChange(horns);

			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			Vector	vecDirToEnemy;

			UTIL_MakeAimVectors(pev->angles);

			m_vecEnemyLKP = m_hEnemy->BodyTarget(pev->origin);
			vecDirToEnemy = (((m_vecEnemyLKP)-pev->origin) + gpGlobals->v_up * -25 + gpGlobals->v_right * -20).Normalize();

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = (gpGlobals->v_right * 4 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 40);
			vecSpitOffset = (pev->origin + vecSpitOffset);
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

			vecSpitDir.x += RANDOM_FLOAT(-0.01, 0.01);
			vecSpitDir.y += RANDOM_FLOAT(-0.01, 0.01);
			vecSpitDir.z += RANDOM_FLOAT(-0.01, 0);
			vecSpitDir.z *= -1;

			// do stuff for this event.
			AttackSound();


			CBaseEntity *pSpit = CBaseEntity::Create("pitdronespit", vecSpitOffset, UTIL_VecToAngles(vecSpitDir), edict());
			UTIL_MakeVectors(pSpit->pev->angles);
			pSpit->pev->velocity = gpGlobals->v_forward * 1300;
		}
	}
	break;


	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

int	CPitDrone::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

void CPitDrone::BodyChange(float horns)
{
	if (!horns)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS0);
	//		pev->body = PITDRONE_HORNS0;

	if (horns = 1)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS6);
	//		pev->body = PITDRONE_HORNS6;

	if (horns = 2)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS5);
	//		pev->body = PITDRONE_HORNS5;

	if (horns = 3)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS4);
	//		pev->body = PITDRONE_HORNS4;

	if (horns = 4)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS3);
	//		pev->body = PITDRONE_HORNS3;

	if (horns = 5)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS2);
	//		pev->body = PITDRONE_HORNS2;

	if (horns = 6)
		SetBodygroup(HORNGROUP, PITDRONE_HORNS1);
	//		pev->body = PITDRONE_HORNS1;

	return;
}
//=========================================================
// Spawn
//=========================================================
void CPitDrone::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/pit_drone.mdl");
	UTIL_SetSize(pev, Vector(-24, -24, 0), Vector(24, 24, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.pitdroneHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	horns = m_ammo;
	BodyChange(horns);
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPitDrone::Precache()
{
	int i;
	PRECACHE_MODEL("models/pit_drone.mdl");

	PRECACHE_MODEL("models/pit_drone_spike.mdl");// spit projectile.
	iPitDroneSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for (i = 0; i < ARRAYSIZE(pDieSounds); i++)
		PRECACHE_SOUND((char *)pDieSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackSounds); i++)
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_melee_attack2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_attack_spike1.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_communicate1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate3.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_communicate4.wav");

	PRECACHE_SOUND("pitdrone/pit_drone_eat.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt1.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt2.wav");
	PRECACHE_SOUND("pitdrone/pit_drone_hunt3.wav");

	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");

	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hit2.wav");

	UTIL_PrecacheOther("pitdronespit");

}


//=========================================================
// IdleSound
//=========================================================
#define PITDRONE_ATTN_IDLE	(float)1.5
void CPitDrone::IdleSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_idle1.wav", 1, PITDRONE_ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_idle2.wav", 1, PITDRONE_ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_idle3.wav", 1, PITDRONE_ATTN_IDLE);
		break;

	}
}

//=========================================================
// PainSound
//=========================================================
void CPitDrone::PainSound(void)
{
	int iPitch = RANDOM_LONG(85, 120);

	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_pain4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CPitDrone::AlertSound(void)
{
	int iPitch = RANDOM_LONG(140, 160);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}
//=========================================================
// DeathSound
//=========================================================
void CPitDrone::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_die3.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CPitDrone::AttackSound(void)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "pitdrone/pit_drone_melee_attack1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "pitdrone/pit_drone_melee_attack2.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void CPitDrone::StopTalking(void)
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);
}

//=========================================================
// ShouldSpeak - Should this PDrone be talking?
//=========================================================
BOOL CPitDrone::ShouldSpeak(void)
{
	if (m_flNextSpeakTime > gpGlobals->time)
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if (pev->spawnflags & SF_MONSTER_GAG)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time 
			// into the future a bit, so we don't talk immediately after 
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return FALSE;
		}
	}

	return TRUE;
}

void CPitDrone::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < PITDRONE_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlPDroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slPDroneRangeAttack1[] =
{
	{
		tlPDroneRangeAttack1,
		ARRAYSIZE(tlPDroneRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"PDrone Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlPDroneChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty PitDrone oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slPDroneChaseEnemy[] =
{
	{
		tlPDroneChaseEnemy1,
		ARRAYSIZE(tlPDroneChaseEnemy1),
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
		"PDrone Chase Enemy"
	},
};


// PitDrone walks to something tasty and eats it.
Task_t tlPDroneEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneEat[] =
{
	{
		tlPDroneEat,
		ARRAYSIZE(tlPDroneEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the PitDrone. This schedule plays a sniff animation before going to the source of food.
Task_t tlPDroneSniffAndEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the food
	{ TASK_PLAY_SEQUENCE, (float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, (float)50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneSniffAndEat[] =
{
	{
		tlPDroneSniffAndEat,
		ARRAYSIZE(tlPDroneSniffAndEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"PDroneSniffAndEat"
	}
};

// PitDrone does this to stinky things. 
Task_t tlPDroneWallow[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the PitDrone can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_INSPECT_FLOOR },
	{ TASK_EAT, (float)50 },// keeps PitDrone from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slPDroneWallow[] =
{
	{
		tlPDroneWallow,
		ARRAYSIZE(tlPDroneWallow),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"PDroneWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES(CPitDrone)
{
	slPDroneRangeAttack1,
		slPDroneChaseEnemy,
		slPDroneEat,
		slPDroneSniffAndEat,
		slPDroneWallow
};

IMPLEMENT_CUSTOM_SCHEDULES(CPitDrone, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CPitDrone::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_PDRONE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_PDRONE_EAT);
		}

		if (HasConditions(bits_COND_SMELL))
		{
			// there's something stinky. 
			CSound		*pSound;

			pSound = PBestScent();
			if (pSound)
				return GetScheduleOfType(SCHED_PDRONE_WALLOW);
		}

		break;
	}
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
			if (m_fCanThreatDisplay && IRelationship(m_hEnemy) == R_HT)
			{
				return GetScheduleOfType(SCHED_WAKE_ANGRY);
			}
		}

		if (HasConditions(bits_COND_SMELL_FOOD))
		{
			CSound		*pSound;

			pSound = PBestScent();

			if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
			{
				// scent is behind or occluded
				return GetScheduleOfType(SCHED_PDRONE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_PDRONE_EAT);
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
Schedule_t* CPitDrone::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slPDroneRangeAttack1[0];
		break;
	case SCHED_PDRONE_EAT:
		return &slPDroneEat[0];
		break;
	case SCHED_PDRONE_SNIFF_AND_EAT:
		return &slPDroneSniffAndEat[0];
		break;
	case SCHED_PDRONE_WALLOW:
		return &slPDroneWallow[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slPDroneChaseEnemy[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for PitDrone because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CPitDrone::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_melee_attack1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "pitdrone/pit_drone_melee_attack2.wav", 1, ATTN_NORM);
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
void CPitDrone::RunTask(Task_t *pTask)
{
	{
		CBaseMonster::RunTask(pTask);
	}
}


//=========================================================
// GetIdealState - Overridden for PDrone to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CPitDrone::GetIdealState(void)
{
	int	iConditions;

	iConditions = IScheduleFlags();

	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}
#endif