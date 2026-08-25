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

int			   iCleanSpitSprite;


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SQUID_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SQUID_SMELLFOOD,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_SQUID_HOPTURN = LAST_COMMON_TASK + 1,
};

#ifndef CLIENT_DLL
//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CCleanSpit : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS(cleanspit, CCleanSpit);

TYPEDESCRIPTION	CCleanSpit::m_SaveData[] =
{
	DEFINE_FIELD(CCleanSpit, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CCleanSpit, CBaseEntity);

void CCleanSpit::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("cleanspit");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;

	SET_MODEL(ENT(pev), "sprites/bigspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CCleanSpit::Animate(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

void CCleanSpit::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CCleanSpit *pSpit = GetClassPtr((CCleanSpit *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CCleanSpit::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CCleanSpit::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = 100;

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (!pOther->pev->takedamage)
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 1));

		// make some flecks
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(tr.vecEndPos.x);	// pos
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_COORD(tr.vecPlaneNormal.x);	// dir
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(iCleanSpitSprite);	// model
		WRITE_BYTE(5);			// count
		WRITE_BYTE(30);			// speed
		WRITE_BYTE(80);			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	else
	{
		pOther->TakeDamage(pev, pev, gSkillData.bullsquidDmgSpit, DMG_GENERIC);
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
#endif

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSQUID_AE_SPIT		( 4 )
#define		BSQUID_AE_BITE		( 1 )

class CCleansuit : public CBaseMonster
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
	int LastShitTalked;

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
};
LINK_ENTITY_TO_CLASS(monster_cleansuit, CCleansuit);

TYPEDESCRIPTION	CCleansuit::m_SaveData[] =
{
	DEFINE_FIELD(CCleansuit, m_fCanThreatDisplay, FIELD_BOOLEAN),
	DEFINE_FIELD(CCleansuit, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CCleansuit, m_flNextSpitTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CCleansuit, CBaseMonster);

//=========================================================
// IgnoreConditions 
//=========================================================
int CCleansuit::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CCleansuit::IRelationship(CBaseEntity *pTarget)
{
	if (gpGlobals->time - m_flLastHurtTime < 5 && FClassnameIs(pTarget->pev, "monster_headcrab"))
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return R_NO;
	}

	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CCleansuit::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > SQUID_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	if (!FClassnameIs(pevAttacker, "monster_headcrab"))
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->time;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CCleansuit::CheckRangeAttack1(float flDot, float flDist)
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
BOOL CCleansuit::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 65 && flDot >= 0.7)
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
BOOL CCleansuit::CheckMeleeAttack2(float flDot, float flDist)
{
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CCleansuit::FValidateHintType(short sHint)
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
int CCleansuit::ISoundMask(void)
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
int	CCleansuit::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CCleansuit::IdleSound(void)
{
	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "cleansuit/cs_idle1.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "cleansuit/cs_idle2.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "cleansuit/cs_idle3.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "cleansuit/cs_idle4.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "cleansuit/cs_idle5.wav", 1, SQUID_ATTN_IDLE);
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CCleansuit::PainSound(void)
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
	int iPitch = 100;

	switch (RANDOM_LONG(0, 3))
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
	}
}

//=========================================================
// AlertSound
//=========================================================
void CCleansuit::AlertSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;

	int iPitch = 100;

	switch (RANDOM_LONG(0, 6))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert5.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 5:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert6.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 6:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleansuit/cs_alert7.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCleansuit::SetYawSpeed(void)
{
	pev->yaw_speed = 500;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCleansuit::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case BSQUID_AE_SPIT:
	{
		if (m_hEnemy)
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			AlertSound();

			UTIL_MakeVectors(pev->angles);

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 24 + gpGlobals->v_up * 64);
			vecSpitOffset = (pev->origin + vecSpitOffset);
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();


			// do stuff for this event.
			AttackSound();

#ifndef CLIENT_DLL
			// spew the spittle temporary ents.
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpitOffset);
			WRITE_BYTE(TE_SPRITE_SPRAY);
			WRITE_COORD(vecSpitOffset.x);	// pos
			WRITE_COORD(vecSpitOffset.y);
			WRITE_COORD(vecSpitOffset.z);
			WRITE_COORD(vecSpitDir.x);	// dir
			WRITE_COORD(vecSpitDir.y);
			WRITE_COORD(vecSpitDir.z);
			WRITE_SHORT(iCleanSpitSprite);	// model
			WRITE_BYTE(5);			// count
			WRITE_BYTE(210);			// speed
			WRITE_BYTE(25);			// noise ( client will divide by 100 )
			MESSAGE_END();

			CCleanSpit::Shoot(pev, vecSpitOffset, vecSpitDir * 900);
#endif
		}

	}
		break;

	case BSQUID_AE_BITE:
	{
						   // SOUND HERE!
						   CBaseEntity *pHurt = CheckTraceHullAttack(70, 30, DMG_POISON);
						   AlertSound();

						   if (pHurt)
						   {
							   //pHurt->pev->punchangle.z = -15;
							   //pHurt->pev->punchangle.x = -45;
							   pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
							   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
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
void CCleansuit::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/cleansuit_scientist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.bullsquidHealth;
	m_flFieldOfView = 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	pev->body = RANDOM_LONG(0, 3);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCleansuit::Precache()
{
	PRECACHE_MODEL("models/cleansuit_scientist.mdl");

	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.

	iCleanSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("bullchicken/bc_attack2.wav");
	PRECACHE_SOUND("bullchicken/bc_attack3.wav");

	PRECACHE_SOUND("bullchicken/bc_die1.wav");
	PRECACHE_SOUND("bullchicken/bc_die2.wav");
	PRECACHE_SOUND("bullchicken/bc_die3.wav");

	PRECACHE_SOUND("cleansuit/cs_alert1.wav");
	PRECACHE_SOUND("cleansuit/cs_alert2.wav");
	PRECACHE_SOUND("cleansuit/cs_alert3.wav");
	PRECACHE_SOUND("cleansuit/cs_alert4.wav");
	PRECACHE_SOUND("cleansuit/cs_alert5.wav");
	PRECACHE_SOUND("cleansuit/cs_alert6.wav");
	PRECACHE_SOUND("cleansuit/cs_alert7.wav");

	PRECACHE_SOUND("cleansuit/cs_idle1.wav");
	PRECACHE_SOUND("cleansuit/cs_idle2.wav");
	PRECACHE_SOUND("cleansuit/cs_idle3.wav");
	PRECACHE_SOUND("cleansuit/cs_idle4.wav");
	PRECACHE_SOUND("cleansuit/cs_idle5.wav");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");

	PRECACHE_SOUND("bullchicken/bc_attackgrowl.wav");
	PRECACHE_SOUND("bullchicken/bc_attackgrowl2.wav");
	PRECACHE_SOUND("bullchicken/bc_attackgrowl3.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND("bullchicken/bc_bite3.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

}

//=========================================================
// DeathSound
//=========================================================
void CCleansuit::DeathSound(void)
{
	return;
}

//=========================================================
// AttackSound
//=========================================================
void CCleansuit::AttackSound(void)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack3.wav", 1, ATTN_NORM);
		break;
	}
}


//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CCleansuit::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	/*if (pev->skin != 0)
	{
		// close eye if it was open.
		pev->skin = 0;
	}

	if (RANDOM_LONG(0, 39) == 0)
	{
		pev->skin = 1;
	}*/

	if (m_hEnemy == NULL && m_Activity == ACT_IDLE)
	{
		if (FStrEq(STRING(gpGlobals->mapname), "survival"))
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
			if (pPlayer)
			{
				m_hTargetEnt = pPlayer;
				m_hEnemy = pPlayer;
				m_IdealMonsterState = MONSTERSTATE_COMBAT;
				m_MonsterState = MONSTERSTATE_COMBAT;
				MoveToLocation(ACT_RUN, 0.0, pPlayer->pev->origin);
			}
		}
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
Task_t	tlCleanRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slCleanRangeAttack1[] =
{
	{
		tlCleanRangeAttack1,
		ARRAYSIZE(tlCleanRangeAttack1),
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
Task_t tlCleanChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty squid oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slCleanChaseEnemy[] =
{
	{
		tlCleanChaseEnemy1,
		ARRAYSIZE(tlCleanChaseEnemy1),
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

Task_t tlCleanHurtHop[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SOUND_WAKE, (float)0 },
	{ TASK_SQUID_HOPTURN, (float)0 },
	{ TASK_FACE_ENEMY, (float)0 },// in case squid didn't turn all the way in the air.
};

Schedule_t slCleanHurtHop[] =
{
	{
		tlCleanHurtHop,
		ARRAYSIZE(tlCleanHurtHop),
		0,
		0,
		"SquidHurtHop"
	}
};

Task_t tlCleanSeeCrab[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_SOUND_WAKE, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EXCITED },
	{ TASK_FACE_ENEMY, (float)0 },
};

Schedule_t slCleanSeeCrab[] =
{
	{
		tlCleanSeeCrab,
		ARRAYSIZE(tlCleanSeeCrab),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"SquidSeeCrab"
	}
};

// squid walks to something tasty and eats it.
Task_t tlCleanEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the squid can't get to the food
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

Schedule_t slCleanEat[] =
{
	{
		tlCleanEat,
		ARRAYSIZE(tlCleanEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"SquidEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the squid. This schedule plays a sniff animation before going to the source of food.
Task_t tlCleanSniffAndEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the squid can't get to the food
	{ TASK_PLAY_SEQUENCE, (float)ACT_DETECT_SCENT },
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

Schedule_t slCleanSniffAndEat[] =
{
	{
		tlCleanSniffAndEat,
		ARRAYSIZE(tlCleanSniffAndEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"SquidSniffAndEat"
	}
};

// squid does this to stinky things. 
Task_t tlCleanWallow[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the squid can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION, (float)0 },
	{ TASK_GET_PATH_TO_BESTSCENT, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_INSPECT_FLOOR },
	{ TASK_EAT, (float)50 },// keeps squid from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION, (float)0 },
	{ TASK_WALK_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
	{ TASK_CLEAR_LASTPOSITION, (float)0 },
};

Schedule_t slCleanWallow[] =
{
	{
		tlCleanWallow,
		ARRAYSIZE(tlCleanWallow),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"SquidWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES(CCleansuit)
{
	slCleanRangeAttack1,
		slCleanChaseEnemy,
		slCleanHurtHop,
		slCleanSeeCrab,
		slCleanEat,
		slCleanSniffAndEat,
		slCleanWallow
};

IMPLEMENT_CUSTOM_SCHEDULES(CCleansuit, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CCleansuit::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{
							   if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
							   {
								   return GetScheduleOfType(SCHED_SQUID_HURTHOP);
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
Schedule_t* CCleansuit::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slCleanRangeAttack1[0];
		break;
	case SCHED_SQUID_HURTHOP:
		return &slCleanHurtHop[0];
		break;
	case SCHED_SQUID_SEECRAB:
		return &slCleanSeeCrab[0];
		break;
	case SCHED_SQUID_EAT:
		return &slCleanEat[0];
		break;
	case SCHED_SQUID_SNIFF_AND_EAT:
		return &slCleanSniffAndEat[0];
		break;
	case SCHED_SQUID_WALLOW:
		return &slCleanWallow[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slCleanChaseEnemy[0];
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
void CCleansuit::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
							   switch (RANDOM_LONG(0, 2))
							   {
							   case 0:
								   EMIT_SOUND(ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl.wav", 1, ATTN_NORM);
								   break;
							   case 1:
								   EMIT_SOUND(ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl2.wav", 1, ATTN_NORM);
								   break;
							   case 2:
								   EMIT_SOUND(ENT(pev), CHAN_VOICE, "bullchicken/bc_attackgrowl3.wav", 1, ATTN_NORM);
								   break;
							   }

							   CBaseMonster::StartTask(pTask);
							   break;
	}
	case TASK_SQUID_HOPTURN:
	{
							   SetActivity(ACT_HOP);
							   MakeIdealYaw(m_vecEnemyLKP);
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
void CCleansuit::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SQUID_HOPTURN:
	{
							   MakeIdealYaw(m_vecEnemyLKP);
							   ChangeYaw(pev->yaw_speed);

							   if (m_fSequenceFinished)
							   {
								   m_iTaskStatus = TASKSTATUS_COMPLETE;
							   }
							   break;
	}
	default:
	{
			   CBaseMonster::RunTask(pTask);
			   break;
	}
	}
}


//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CCleansuit::GetIdealState(void)
{
	int	iConditions;

	iConditions = IScheduleFlags();

	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
	{
								if (m_hEnemy != NULL && (iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE) && FClassnameIs(m_hEnemy->pev, "monster_headcrab"))
								{
									// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
									m_hEnemy = NULL;
									m_IdealMonsterState = MONSTERSTATE_ALERT;
								}
								break;
	}
	}

	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}

