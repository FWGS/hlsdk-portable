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



class CRajDart : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir, int pevdmg);
	void Touch(CBaseEntity *pOther);
	int damage;

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS(rajdart, CRajDart);

TYPEDESCRIPTION	CRajDart::m_SaveData[] =
{
	DEFINE_FIELD(CRajDart, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CRajDart, CBaseEntity);

void CRajDart::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;
	pev->gravity = 0.1;
	damage = 15;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CRajDart::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir, int pevdmg)
{
	CRajDart *pSpit = GetClassPtr((CRajDart *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->damage = pevdmg;
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CRajDart::Touch(CBaseEntity *pOther)
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
		if (!FClassnameIs(pOther->pev, "monster_rajesh"))
		{
			pOther->TakeDamage(pev, pev, 15, DMG_NEVERGIB);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, iPitch);
		}
	}

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	RAJESH_MELEE				1
#define	RAJESH_SHOOT				2
#define	RAJESH_STRAFE_LEFT			3
#define	RAJESH_STRAFE_RIGHT			4

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CRajesh : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int LastShitTalked;
	void Shoot(void);

	float m_flNextFlinch;
	
	//For strafing
	bool justfired;
	float RollCooldown;

	int m_iTrail;

	void PainSound(void);
	void AlertSound(void);
	void AttackSound(void);

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_rajesh, CRajesh);

const char *CRajesh::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CRajesh::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CRajesh::pAttackSounds[] =
{
	"pajeet/pj_redeem1.wav",
	"pajeet/pj_redeem2.wav",
	"pajeet/pj_redeem3.wav",
	"pajeet/pj_redeem4.wav"
};

const char *CRajesh::pAlertSounds[] =
{
	"pajeet/pj_alert1.wav",
	"pajeet/pj_alert2.wav"
};

const char *CRajesh::pPainSounds[] =
{
	"pajeet/pj_pain1.wav",
	"pajeet/pj_pain2.wav",
	"pajeet/pj_redeem1.wav",
	"pajeet/pj_redeem2.wav",
	"pajeet/pj_redeem3.wav",
	"pajeet/pj_redeem4.wav"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CRajesh::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRajesh::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

int CRajesh::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CRajesh::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
		//case HITGROUP_CHEST:
		//case HITGROUP_STOMACH:
	case 9:
		flDamage -= 20;
		if (flDamage <= 0)
		{
			UTIL_Ricochet(ptr->vecEndPos, 1.0);
			flDamage = 0.01;
		}
		break;
	}

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CRajesh::PainSound(void)
{
	if (m_Activity == ACT_RANGE_ATTACK1)
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 7) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CRajesh::AlertSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CRajesh::AttackSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 4;
	else
		return;
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRajesh::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case RAJESH_SHOOT:
	{
		Shoot();
		if (RANDOM_LONG(0, 6) == 0)
			AttackSound();
	}
		break;
	case RAJESH_MELEE:
	{
		// do stuff for this event.
		CBaseEntity *pHurt = CheckTraceHullAttack(120, gSkillData.zombieDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
		break;
	case RAJESH_STRAFE_LEFT:
	{
		RollCooldown = gpGlobals->time + 7;
		justfired = false;
		//UTIL_MoveToOrigin(ENT(pev), pev->origin - gpGlobals->v_right * 64, 32, MOVE_STRAFE);
		pev->velocity = pev->velocity + gpGlobals->v_up * 50 - gpGlobals->v_right * 250;
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/fatroll.wav", 0.8, ATTN_NORM, 0, 100);
	}
		break;
	case RAJESH_STRAFE_RIGHT:
	{
		RollCooldown = gpGlobals->time + 7;
		justfired = false;
		pev->velocity = pev->velocity + gpGlobals->v_up * 50 + gpGlobals->v_right * 250;
		//UTIL_MoveToOrigin(ENT(pev), pev->origin + gpGlobals->v_right * 64, 32, MOVE_STRAFE);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/fatroll.wav", 0.8, ATTN_NORM, 0, 100);
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
void CRajesh::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rajesh.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 80;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	RollCooldown = gpGlobals->time;
	justfired = false;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRajesh::Precache()
{
	PRECACHE_MODEL("models/rajesh.mdl");

	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("common/fatroll.wav");
	PRECACHE_MODEL("models/crossbow_bolt.mdl");

	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CRajesh::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK2))
	{
		if (justfired && gpGlobals->time > RollCooldown)
		{
			if(RANDOM_LONG(0,1))
				m_IdealActivity = ACT_STRAFE_LEFT;
			else
				m_IdealActivity = ACT_STRAFE_RIGHT;
		}
		
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


BOOL CRajesh::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > 100 && flDist <= 850 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CRajesh::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 100 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}


void CRajesh::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	justfired = true;

	Vector	vecSpitOffset;
	Vector	vecSpitDir;
	Vector anglesAim = pev->v_angle + pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(pev->angles);

	vecSpitOffset = (gpGlobals->v_forward * 32 + gpGlobals->v_up * 42);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	Vector randDirOffset = gpGlobals->v_up * RANDOM_LONG(-24, 24) + gpGlobals->v_right * RANDOM_LONG(-24, 24);
	vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 15 + randDirOffset) - vecSpitOffset).Normalize();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM);
	CRajDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, anglesAim, 15);
}