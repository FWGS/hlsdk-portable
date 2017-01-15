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
// voltigore
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

#define		VOLTIGORE_SPRINT_DIST	256 // how close the voltigore has to get before starting to sprint and refusing to swerve

#define		VOLTIGORE_MAX_BEAMS		12

#define		VOLTIGORE_CLASSNAME				"monster_alien_voltigore"
#define		VOLTIGORE_BABY_CLASSNAME		"monster_alien_babyvoltigore"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_VOLTIGORE_SMELLFOOD = LAST_COMMON_SCHEDULE + 1,
	SCHED_VOLTIGORE_EAT,
	SCHED_VOLTIGORE_SNIFF_AND_EAT,
	SCHED_VOLTIGORE_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_VOLTIGORE_NEWTASK = LAST_COMMON_TASK + 1,
};

//=========================================================
// Voltigore's energy ball projectile
//=========================================================
class CVoltigoreEnergyBall : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);
	void EXPORT BeamThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateBeams();
	void ClearBeams();
	void UpdateBeams();

	int  m_maxFrame;

	CBeam* m_pBeam[VOLTIGORE_MAX_BEAMS];
	int m_iBeams;

protected:

	void CreateBeam(int nIndex, const Vector& vecPos, int width, int brightness);
	void UpdateBeam(int nIndex, const Vector& vecPos);
	void ClearBeam(int nIndex);
};


LINK_ENTITY_TO_CLASS(voltigore_energy_ball, CVoltigoreEnergyBall);

TYPEDESCRIPTION	CVoltigoreEnergyBall::m_SaveData[] =
{
	DEFINE_ARRAY(CVoltigoreEnergyBall, m_pBeam, FIELD_CLASSPTR, VOLTIGORE_MAX_BEAMS),
	DEFINE_FIELD(CVoltigoreEnergyBall, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD(CVoltigoreEnergyBall, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CVoltigoreEnergyBall, CBaseEntity);

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("voltigore_energy_ball");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderGlow;
	pev->renderamt = 220;

	SET_MODEL(ENT(pev), "sprites/glow_prp.spr");
	pev->frame = 0;
	pev->scale = 0.05;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;

	m_iBeams = 0;

	// Create beams.
	CreateBeams();
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Animate(void)
{
#if 0
	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
#endif

	SetThink(&CVoltigoreEnergyBall::BeamThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CVoltigoreEnergyBall *pEnergyBall = GetClassPtr((CVoltigoreEnergyBall *)NULL);
	pEnergyBall->Spawn();

	UTIL_SetOrigin(pEnergyBall->pev, vecStart);
	pEnergyBall->pev->velocity = vecVelocity;
	pEnergyBall->pev->owner = ENT(pevOwner);

	pEnergyBall->SetThink(&CVoltigoreEnergyBall::Animate);
	pEnergyBall->pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	if (!pOther->pev->takedamage)
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 1));
	}
	else
	{
		pOther->TakeDamage(pev, pev, gSkillData.voltigoreDmgBeam, DMG_GENERIC);
	}

	// Clear beams.
	ClearBeams();

	SetThink(&CVoltigoreEnergyBall::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::BeamThink(void)
{
	// Update beams.
	UpdateBeams();

	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::CreateBeam(int nIndex, const Vector& vecPos, int width, int brightness)
{
	m_pBeam[nIndex] = CBeam::BeamCreate("sprites/lgtning.spr", width);
	if (!m_pBeam[nIndex])
		return;

	m_pBeam[nIndex]->PointEntInit(vecPos, entindex());
	m_pBeam[nIndex]->SetColor(125, 61, 177);
	m_pBeam[nIndex]->SetBrightness(brightness);
	m_pBeam[nIndex]->SetNoise(50);
	m_pBeam[nIndex]->SetFlags(SF_BEAM_SHADEIN | SF_BEAM_SHADEOUT );
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::UpdateBeam(int nIndex, const Vector& vecPos)
{
	if (!m_pBeam[nIndex])
		return;
	
	m_pBeam[nIndex]->SetStartPos(vecPos);
	m_pBeam[nIndex]->SetEndEntity(entindex());
	m_pBeam[nIndex]->RelinkBeam();
}

//=========================================================
// Purpose:
//=========================================================
void CVoltigoreEnergyBall::ClearBeam(int nIndex)
{
	if (m_pBeam[nIndex])
	{
		UTIL_Remove(m_pBeam[nIndex]);
		m_pBeam[nIndex] = NULL;
	}
}

//=========================================================
// CreateBeams - create all beams
//=========================================================
void CVoltigoreEnergyBall::CreateBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		CreateBeam(i, pev->origin, RANDOM_LONG( 2, 3 ) * 10, 250 + RANDOM_LONG(1, 5) );
	}
	m_iBeams = VOLTIGORE_MAX_BEAMS;
}

//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CVoltigoreEnergyBall::ClearBeams()
{
	for (int i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		ClearBeam( i );
	}
	m_iBeams = 0;
}


void CVoltigoreEnergyBall::UpdateBeams()
{
	int i, j;

	TraceResult tr;
	float flDist = 1.0f;

	Vector vecSrc, vecTarget;
	vecSrc = pev->origin;
	for (i = 0; i < VOLTIGORE_MAX_BEAMS; ++i)
	{
		for (j = 0; j < 3; ++j)
		{
			vecTarget = vecSrc;
			vecTarget = vecTarget + Vector(
				RANDOM_FLOAT(-1, 1)	* 128,
				RANDOM_FLOAT(-1, 1) * 128,
				RANDOM_FLOAT(-1, 1) * 128);


			TraceResult tr1;
			UTIL_TraceLine(vecSrc, vecTarget, ignore_monsters, ENT(pev), &tr1);
			if (flDist > tr1.flFraction)
			{
				tr = tr1;
				flDist = tr.flFraction;
			}
		}

		// Couldn't find anything close enough
		if (flDist == 1.0f)
			continue;

		// Update the target position of the beam.
		UpdateBeam(i, tr.vecEndPos);
	}
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		VOLTIGORE_AE_THROW			( 1 )
#define		VOLTIGORE_AE_PUNCH_BOTH		( 12 )
#define		VOLTIGORE_AE_PUNCH_SINGLE	( 13 )
#define		VOLTIGORE_AE_GIB			( 2002 )


//=========================================================
// CVoltigore
//=========================================================
class CVoltigore : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	virtual int  Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	virtual void IdleSound(void);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void AlertSound(void);
	virtual void AttackSound(void);
	virtual void StartTask(Task_t *pTask);
	virtual BOOL CheckMeleeAttack1(float flDot, float flDist);
	virtual BOOL CheckMeleeAttack2(float flDot, float flDist);
	virtual BOOL CheckRangeAttack1(float flDot, float flDist);
	virtual void RunAI(void);
	BOOL FValidateHintType(short sHint);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	virtual int IgnoreConditions(void);
	MONSTERSTATE GetIdealState(void);
	virtual void Killed(entvars_t *pevAttacker, int iGib);
	
	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flLastHurtTime;
	float m_flNextSpitTime; // last time the voltigore used the spit attack.
	BOOL m_fShouldUpdateBeam;
	CBeam* m_pBeam[3];
	CSprite* m_pBeamGlow;
	int m_glowBrightness;

	static const char* pAlertSounds[];
	static const char* pAttackMeleeSounds[];
	static const char* pMeleeHitSounds[];
	static const char* pMeleeMissSounds[];
	static const char* pComSounds[];
	static const char* pDeathSounds[];
	static const char* pFootstepSounds[];
	static const char* pIdleSounds[];
	static const char* pPainSounds[];
	static const char* pGruntSounds[];

	virtual BOOL	CanThrowEnergyBall(void)	const	{ return TRUE; }
	virtual BOOL	CanGib(void)				const	{ return TRUE; }
	virtual void	PrintBloodDecal(CBaseEntity* pHurt, const Vector& vecOrigin, const Vector& vecVelocity, float maxDist, int bloodColor = DONT_BLEED);


	void CreateBeams();
	void DestroyBeams();
	void UpdateBeams();

	void CreateGlow();
	void DestroyGlow();
	void GlowUpdate();
	void GlowOff(void);
	void GlowOn(int level);

};

LINK_ENTITY_TO_CLASS(monster_alien_voltigore, CVoltigore);


TYPEDESCRIPTION	CVoltigore::m_SaveData[] =
{
	DEFINE_FIELD(CVoltigore, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CVoltigore, m_flNextSpitTime, FIELD_TIME),
	DEFINE_FIELD(CVoltigore, m_fShouldUpdateBeam, FIELD_BOOLEAN),
	DEFINE_ARRAY(CVoltigore, m_pBeam, FIELD_CLASSPTR, 3),
	DEFINE_FIELD(CVoltigore, m_glowBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CVoltigore, m_pBeamGlow, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CVoltigore, CBaseMonster);

const char* CVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

const char* CVoltigore::pAttackMeleeSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char* CVoltigore::pMeleeHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CVoltigore::pMeleeMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CVoltigore::pComSounds[] =
{
	"voltigore/voltigore_communicate1.wav",
	"voltigore/voltigore_communicate2.wav",
	"voltigore/voltigore_communicate3.wav",
};


const char* CVoltigore::pDeathSounds[] =
{
	"voltigore/voltigore_die1.wav",
	"voltigore/voltigore_die2.wav",
	"voltigore/voltigore_die3.wav",
};

const char* CVoltigore::pFootstepSounds[] =
{
	"voltigore/voltigore_footstep1.wav",
	"voltigore/voltigore_footstep2.wav",
	"voltigore/voltigore_footstep3.wav",
};

const char* CVoltigore::pIdleSounds[] =
{
	"voltigore/voltigore_idle1.wav",
	"voltigore/voltigore_idle2.wav",
	"voltigore/voltigore_idle3.wav",
};

const char* CVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char* CVoltigore::pGruntSounds[] =
{
	"voltigore/voltigore_run_grunt1.wav",
	"voltigore/voltigore_run_grunt2.wav",
};


//=========================================================
// IgnoreConditions 
//=========================================================
int CVoltigore::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (gpGlobals->time - m_flLastHurtTime <= 20)
	{
		// haven't been hurt in 20 seconds, so let the voltigore care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

//=========================================================
// TakeDamage - overridden for voltigore so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CVoltigore::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the voltigore is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > VOLTIGORE_SPRINT_DIST)
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
BOOL CVoltigore::CheckRangeAttack1(float flDot, float flDist)
{
	if (!CanThrowEnergyBall())
	{
		return FALSE;
	}

	if (IsMoving() && flDist >= 512)
	{
		// voltigore will far too far behind if he stops running to spit at this distance from the enemy.
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
//=========================================================
void CVoltigore::RunAI(void)
{
	CBaseMonster::RunAI();

	if (m_fShouldUpdateBeam)
	{
		UpdateBeams();
	}

	GlowUpdate();
}

//=========================================================
// CheckMeleeAttack1 - voltigore is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CVoltigore::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= fabs(pev->mins.x - pev->maxs.x) && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - voltigore is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CVoltigore::CheckMeleeAttack2(float flDot, float flDist)
{
	if (!HasConditions(bits_COND_CAN_MELEE_ATTACK1))		// The player & voltigore can be as much as their bboxes 
	{													   // apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CVoltigore::FValidateHintType(short sHint)
{
	int i;

	static short sVoltigoreHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for (i = 0; i < ARRAYSIZE(sVoltigoreHints); i++)
	{
		if (sVoltigoreHints[i] == sHint)
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
int CVoltigore::ISoundMask(void)
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
int	CVoltigore::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// IdleSound 
//=========================================================
void CVoltigore::IdleSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), 1, ATTN_NORM);
}

//=========================================================
// PainSound 
//=========================================================
void CVoltigore::PainSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, RANDOM_LONG(85, 120));
}

//=========================================================
// AlertSound
//=========================================================
void CVoltigore::AlertSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1, ATTN_NORM, 0, RANDOM_LONG(140, 160));
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CVoltigore::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CVoltigore::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case VOLTIGORE_AE_THROW:
	{
		Vector	vecSpitOffset;
		Vector	vecSpitDir;

		UTIL_MakeVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23);
		vecSpitOffset = (pev->origin + vecSpitOffset);
		vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

		vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
		vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);


		// do stuff for this event.
		AttackSound();

		CVoltigoreEnergyBall::Shoot(pev, vecSpitOffset, vecSpitDir * 500);

		// turn the beam glow off.
		DestroyBeams();

		GlowOff();

		m_fShouldUpdateBeam = FALSE;
	}
	break;


	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(120, gSkillData.voltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = -20;
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -200;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			PrintBloodDecal(pHurt, pHurt->pev->origin, pHurt->pev->velocity, RANDOM_FLOAT(80, 90) );
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(120, gSkillData.voltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.x = 30;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			PrintBloodDecal(pHurt, pHurt->pev->origin, Vector(0, 0, -1), 90);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_GIB:
	{

	}
	break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CVoltigore::Spawn()
{
	pev->model = ALLOC_STRING("models/voltigore.mdl");
	Precache();

	SET_MODEL(ENT(pev), (char*)STRING(pev->model));
	UTIL_SetSize(pev, Vector(-80, -80, 0), Vector(80, 80, 90));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.voltigoreHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpitTime	= gpGlobals->time;

	pev->view_ofs		= Vector(0, 0, 90);

	m_fShouldUpdateBeam = FALSE;
	m_pBeamGlow = NULL;

	GlowOff();

	// Create glow.
	CreateGlow();

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CVoltigore::Precache()
{
	PRECACHE_MODEL((char*)STRING(pev->model));
	
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackMeleeSounds);
	PRECACHE_SOUND_ARRAY(pMeleeHitSounds);
	PRECACHE_SOUND_ARRAY(pMeleeMissSounds);
	PRECACHE_SOUND_ARRAY(pComSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pFootstepSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pGruntSounds);

	PRECACHE_SOUND("voltigore/voltigore_attack_shock.wav");
	PRECACHE_SOUND("voltigore/voltigore_eat.wav");

	PRECACHE_SOUND("debris/beamstart1.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/glow04.spr");

	PRECACHE_MODEL("sprites/glow_prp.spr");
	UTIL_PrecacheOther("voltigore_energy_ball");

}

//=========================================================
// DeathSound
//=========================================================
void CVoltigore::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_NORM);
}

//=========================================================
// AttackSound
//=========================================================
void CVoltigore::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "voltigore/voltigore_attack_shock.wav", 1, ATTN_NORM);
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slVoltigoreRangeAttack1[] =
{
	{
		tlVoltigoreRangeAttack1,
		ARRAYSIZE(tlVoltigoreRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Voltigore Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlVoltigoreChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },// !!!OEM - this will stop nasty voltigore oscillation.
	{ TASK_GET_PATH_TO_ENEMY, (float)0 },
	{ TASK_RUN_PATH, (float)0 },
	{ TASK_WAIT_FOR_MOVEMENT, (float)0 },
};

Schedule_t slVoltigoreChaseEnemy[] =
{
	{
		tlVoltigoreChaseEnemy1,
		ARRAYSIZE(tlVoltigoreChaseEnemy1),
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
		"Voltigore Chase Enemy"
	},
};

// Voltigore walks to something tasty and eats it.
Task_t tlVoltigoreEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the voltigore can't get to the food
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

Schedule_t slVoltigoreEat[] =
{
	{
		tlVoltigoreEat,
		ARRAYSIZE(tlVoltigoreEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"VoltigoreEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the voltigore. This schedule plays a sniff animation before going to the source of food.
Task_t tlVoltigoreSniffAndEat[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_EAT, (float)10 },// this is in case the voltigore can't get to the food
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

Schedule_t slVoltigoreSniffAndEat[] =
{
	{
		tlVoltigoreSniffAndEat,
		ARRAYSIZE(tlVoltigoreSniffAndEat),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_NEW_ENEMY,

		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT |
		bits_SOUND_CARCASS,
		"VoltigoreSniffAndEat"
	}
};

DEFINE_CUSTOM_SCHEDULES(CVoltigore)
{
	slVoltigoreRangeAttack1,
	slVoltigoreChaseEnemy,
	slVoltigoreEat,
	slVoltigoreSniffAndEat,
};

IMPLEMENT_CUSTOM_SCHEDULES(CVoltigore, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CVoltigore::GetSchedule(void)
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
				return GetScheduleOfType(SCHED_VOLTIGORE_SNIFF_AND_EAT);
			}

			// food is right out in the open. Just go get it.
			return GetScheduleOfType(SCHED_VOLTIGORE_EAT);
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
			return GetScheduleOfType(SCHED_WAKE_ANGRY);
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
Schedule_t* CVoltigore::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		return &slVoltigoreRangeAttack1[0];
		break;
	case SCHED_VOLTIGORE_EAT:
		return &slVoltigoreEat[0];
		break;
	case SCHED_VOLTIGORE_SNIFF_AND_EAT:
		return &slVoltigoreSniffAndEat[0];
		break;
	case SCHED_CHASE_ENEMY:
		return &slVoltigoreChaseEnemy[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		{
			CreateBeams();

			GlowOn( 255 );
			m_fShouldUpdateBeam = TRUE;

			// Play the beam 'glow' sound.
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/beamstart1.wav", 1, ATTN_NORM, 0, PITCH_HIGH);

			CBaseMonster::StartTask(pTask);
		}
		break;
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
		}
		break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}


//=========================================================
// GetIdealState - Overridden for Voltigore to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CVoltigore::GetIdealState(void)
{
#if 0
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
			if (m_hEnemy != NULL && (iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE))
			{
				// if the voltigore has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
				m_hEnemy = NULL;
				m_IdealMonsterState = MONSTERSTATE_ALERT;
			}
			break;
		}
	}
#endif

	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}

void CVoltigore::Killed(entvars_t *pevAttacker, int iGib)
{
	DestroyBeams();

	DestroyGlow();

	CBaseMonster::Killed(pevAttacker, iGib);
}

void CVoltigore::CreateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		m_pBeam[i] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
		if (!m_pBeam[i])
			return;

		GetAttachment(i, vecEnd, vecAngles);

		m_pBeam[i]->PointsInit(vecStart, vecEnd);
		m_pBeam[i]->SetColor(125, 61, 177);
		m_pBeam[i]->SetBrightness(255);
		m_pBeam[i]->SetNoise(0);
		m_pBeam[i]->SetFlags(SF_BEAM_SHADEIN /*| SF_BEAM_SHADEOUT*/);

	}
}

void CVoltigore::DestroyBeams()
{
	for (int i = 0; i < 3; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
			m_pBeam[i] = NULL;
		}
	}
}

void CVoltigore::UpdateBeams()
{
	Vector vecStart, vecEnd, vecAngles;
	GetAttachment(3, vecStart, vecAngles);

	for (int i = 0; i < 3; i++)
	{
		GetAttachment(i, vecEnd, vecAngles);
		m_pBeam[i]->SetStartPos(vecStart);
		m_pBeam[i]->SetEndPos(vecEnd);
		m_pBeam[i]->RelinkBeam();
	}
}

void CVoltigore::CreateGlow()
{
	m_pBeamGlow = CSprite::SpriteCreate("sprites/glow04.spr", pev->origin, FALSE);
	m_pBeamGlow->SetTransparency(kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pBeamGlow->SetAttachment(edict(), 4);
	m_pBeamGlow->SetScale(0.2f);
}

void CVoltigore::DestroyGlow()
{
	if (m_pBeamGlow)
	{
		UTIL_Remove(m_pBeamGlow);
		m_pBeamGlow = NULL;
	}
}

void CVoltigore::GlowUpdate()
{
	if (m_pBeamGlow)
	{
		m_pBeamGlow->pev->renderamt = UTIL_Approach(m_glowBrightness, m_pBeamGlow->pev->renderamt, 100);
		if (m_pBeamGlow->pev->renderamt == 0)
			m_pBeamGlow->pev->effects |= EF_NODRAW;
		else
			m_pBeamGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin(m_pBeamGlow->pev, pev->origin);
	}
}

void CVoltigore::GlowOff(void)
{
	m_glowBrightness = 0;
}

void CVoltigore::GlowOn(int level)
{
	m_glowBrightness = level;
}

void CVoltigore::PrintBloodDecal(CBaseEntity* pHurt, const Vector& vecOrigin, const Vector& vecVelocity, float maxDist, int bloodColor)
{
	if (!pHurt)
		return;

	if (bloodColor == DONT_BLEED)
	{
		CBaseMonster* pMonster = pHurt->MyMonsterPointer();
		if (pMonster)
			bloodColor = pMonster->m_bloodColor;
	}

	TraceResult tr;
	UTIL_TraceLine(vecOrigin, vecOrigin + vecVelocity * maxDist, ignore_monsters, ENT(pev), &tr);
	UTIL_BloodDecalTrace(&tr, bloodColor);
}


//=========================================================
// CBabyAlienVoltigore
//=========================================================

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BABY_VOLTIGORE_AE_RUN			( 14 )

class CBabyVoltigore : public CVoltigore
{
public:
	void	Spawn(void);
	void	Precache(void);
	void	HandleAnimEvent(MonsterEvent_t* pEvent);
	void	StartTask(Task_t *pTask);

	virtual BOOL	CanThrowEnergyBall(void)	const	{ return FALSE; }
	virtual BOOL	CanGib(void)				const	{ return FALSE; }
};

LINK_ENTITY_TO_CLASS(monster_alien_babyvoltigore, CBabyVoltigore);



//=========================================================
// Spawn
//=========================================================
void CBabyVoltigore::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/baby_voltigore.mdl");
	UTIL_SetSize(pev, Vector(-40, -40, 0), Vector(40, 40, 64));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	pev->health			= gSkillData.babyVoltigoreHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpitTime	= gpGlobals->time;

	pev->view_ofs		= Vector(0, 0, 64);

	MonsterInit();
}

//=========================================================
//=========================================================
void CBabyVoltigore::Precache(void)
{
	pev->model = ALLOC_STRING("models/baby_voltigore.mdl");
	CVoltigore::Precache();
}

void CBabyVoltigore::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BABY_VOLTIGORE_AE_RUN:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pFootstepSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NORM, 0, RANDOM_LONG(85, 120));
		break;

	case VOLTIGORE_AE_PUNCH_SINGLE:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.z = -10;
			pHurt->pev->punchangle.x = 10;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -100;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

			PrintBloodDecal(pHurt, pHurt->pev->origin, pHurt->pev->velocity, RANDOM_FLOAT(40, 45));
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;

	case VOLTIGORE_AE_PUNCH_BOTH:
	{
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.babyVoltigoreDmgPunch, DMG_CLUB | DMG_ALWAYSGIB);
		if (pHurt)
		{
			pHurt->pev->punchangle.x = 20;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;

			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeHitSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
			
			PrintBloodDecal(pHurt, pHurt->pev->origin, Vector(0, 0, -1), 50);
		}
		else
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pMeleeMissSounds), RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		}
	}
	break;
	default:
		CVoltigore::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for voltigore because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBabyVoltigore::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackMeleeSounds), RANDOM_FLOAT(0.5, 0.6), ATTN_NONE, 0, RANDOM_LONG(100, 105));
		CBaseMonster::StartTask(pTask);
	}
	break;

	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}
