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

class CDart : public CBaseEntity
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

LINK_ENTITY_TO_CLASS(dart, CDart);

TYPEDESCRIPTION	CDart::m_SaveData[] =
{
	DEFINE_FIELD(CDart, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CDart, CBaseEntity);

void CDart::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CDart::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir)
{
	CDart *pSpit = GetClassPtr((CDart *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CDart::Touch(CBaseEntity *pOther)
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
		if (!FClassnameIs(pOther->pev, "monster_gonrider"))
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
#define	GONRIDER_SHOOT				1
#define	GONRIDER_ATTACK				2

#define ZOMBIE_FLINCH_DELAY			4		// at most one flinch every n secs

class CGonrider : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int LastSpoken;
	int m_flNextShootTime;
	void Shoot(void);

	float m_flNextFlinch;

	void AlertSound(void);
	void DeathSound(void);
	void PainSound(void);

	int		m_iShotgunShell;

	static const char *pAlertSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_gonrider, CGonrider);

const char *CGonrider::pPainSounds[] =
{
	"gonrider/grider_pain1.wav",
	"gonrider/grider_pain2.wav",
	"gonrider/grider_pain3.wav",
};

const char *CGonrider::pDeathSounds[] =
{
	"scientist/scream2.wav",
	"scientist/scream3.wav",
	"scientist/scream4.wav",
};

const char *CGonrider::pAlertSounds[] =
{
	"gonrider/grider_speak1.wav",
	"gonrider/grider_speak2.wav",
	"gonrider/grider_speak3.wav",
	"gonrider/grider_speak4.wav",
	"gonrider/grider_speak5.wav",
	"gonrider/grider_speak6.wav",
};

const char *CGonrider::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGonrider::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGonrider::Classify(void)
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGonrider::SetYawSpeed(void)
{
	pev->yaw_speed = 300;
}

int CGonrider::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CGonrider::AlertSound(void)
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		LastSpoken = gpGlobals->time + 5;
	}
}


void CGonrider::PainSound(void)
{
	if (RANDOM_LONG(0,2) == 0)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
	}
}

void CGonrider::DeathSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGonrider::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case GONRIDER_SHOOT:
	{
						  AlertSound();
						  Shoot();
	}
		break;

	case GONRIDER_ATTACK:
	{
								  // do stuff for this event.
								  CBaseEntity *pHurt = CheckTraceHullAttack(140, gSkillData.zombieDmgBothSlash, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 500 + gpGlobals->v_up * 250;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

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
void CGonrider::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gonrider.mdl");
	UTIL_SetSize(pev, Vector(-42, -42, 0), Vector(42, 42, 102));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.agruntHealth * 2;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	// White hands
	pev->skin = 0;

	pev->body = RANDOM_LONG(0, 3);// pick a head, any head

	// Luther is black, make his hands black
	if (pev->body == 2)
		pev->skin = 1;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGonrider::Precache()
{
	PRECACHE_MODEL("models/gonrider.mdl");
	PRECACHE_SOUND("weapons/sbarrel1.wav");
	PRECACHE_SOUND("weapons/scock1.wav");
	PRECACHE_SOUND("gonarch/gon_step1.wav");
	PRECACHE_SOUND("gonarch/gon_step2.wav");
	PRECACHE_SOUND("gonarch/gon_step3.wav");
	PRECACHE_MODEL("models/crossbow_bolt.mdl");

	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");

	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CGonrider::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK1))
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

BOOL CGonrider::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 100 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CGonrider::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1600 && flDist > 250 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}

void CGonrider::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector	vecSpitOffset;
	Vector	vecSpitDir;
	Vector anglesAim = pev->v_angle + pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(pev->angles);

	// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
	// we should be able to read the position of bones at runtime for this info.
	vecSpitOffset = (gpGlobals->v_forward * 37 + gpGlobals->v_up * 85);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 38) - vecSpitOffset).Normalize();

	vecSpitDir = vecSpitDir;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM);

	CDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, anglesAim);
	m_flNextShootTime = gpGlobals->time + 1;
}
