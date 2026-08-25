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
#define	GINA_MELEE					1
#define GINA_AIM					3
#define	GINA_SHOOT_ROCKET			2

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs


class CRocketGina : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);

	float m_flNextFlinch;
	float rocketfired;
	void RunAI(void);

	void PainSound(void);
	void AlertSound(void);
	void AttackSound(int sound);

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_rocketgina, CRocketGina);

const char *CRocketGina::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CRocketGina::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CRocketGina::pAttackSounds[] =
{
	"rocketgina/Chkatck1.wav",
	"rocketgina/Chkatck3.wav",
};

const char *CRocketGina::pAlertSounds[] =
{
	"rocketgina/alert1.wav",
	"rocketgina/alert2.wav",
	"rocketgina/alert3.wav",
	"rocketgina/alert4.wav",
	"rocketgina/alert5.wav",
	"rocketgina/alert6.wav",
	"rocketgina/alert7.wav",
	"rocketgina/alert8.wav",
	"rocketgina/alert9.wav",
};

const char *CRocketGina::pPainSounds[] =
{
	"rocketgina/Chkdeth1.wav",
	"rocketgina/Chkdeth2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRocketGina::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRocketGina::SetYawSpeed(void)
{
	pev->yaw_speed = 360;
}

int CRocketGina::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CRocketGina::PainSound(void)
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
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CRocketGina::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CRocketGina::AttackSound(int sound)
{
	// Play a random attack sound
	if (sound == 0)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rocketgina/Chkatck1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rocketgina/Chkatck3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRocketGina::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GINA_MELEE:
	{
								  // do stuff for this event.
								  CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

								  if (RANDOM_LONG(0, 1))
									  AttackSound(1);
	}
		break;
	case GINA_AIM:
	{
					 if (RANDOM_LONG(0, 1))
						 AttackSound(0);
					 else
						 EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, 100);
	}
		break;

	case GINA_SHOOT_ROCKET:
	{
							  Vector vecSrc = GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_right * 8 + gpGlobals->v_up * 48;

							  Vector vecShootOrigin;
							  UTIL_MakeVectors(pev->angles);
							  vecShootOrigin = pev->origin + Vector(0, 0, 55);

							  Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
							  vecShootDir.z = -vecShootDir.z;

							  Vector angDir = UTIL_VecToAngles(vecShootDir);
							  

							  CBaseEntity *pRocket = CBaseEntity::Create("rpg_rocket", vecSrc, angDir, edict());  
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
void CRocketGina::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rocketgina.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 90;
	pev->view_ofs = Vector(0, 0, 64);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	rocketfired = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRocketGina::Precache()
{
	PRECACHE_MODEL("models/rocketgina.mdl");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CRocketGina::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

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
}


int CRocketGina::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();
		
	iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);

	return iIgnore;

}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CRocketGina::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > 64 && flDist <= 1800 && flDot >= 0.5 && gpGlobals->time >= rocketfired)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		rocketfired = gpGlobals->time + 1.5;

		return TRUE;
	}

	return FALSE;
}