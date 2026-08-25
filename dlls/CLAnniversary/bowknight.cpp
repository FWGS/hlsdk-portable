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

class CBowknightDart : public CBaseEntity
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

LINK_ENTITY_TO_CLASS(bowknightdart, CBowknightDart);

TYPEDESCRIPTION	CBowknightDart::m_SaveData[] =
{
	DEFINE_FIELD(CBowknightDart, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBowknightDart, CBaseEntity);

void CBowknightDart::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;
	pev->gravity = 0.1;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CBowknightDart::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecDir)
{
	CBowknightDart *pSpit = GetClassPtr((CBowknightDart *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles(vecVelocity);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CBowknightDart::Touch(CBaseEntity *pOther)
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
		if (!FClassnameIs(pOther->pev, "monster_bowknight"))
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
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	BOWKNIGHT_SHOOT				2
#define	ZOMBIE_AE_ATTACK_BOTH		0x03
#define	SCIENTIST_JUMP_ATTACK		4
#define SCIENTIST_TELEPORT			9

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CBowknight : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	float LastJumped;
	float LastTeleported;
	int LastShitTalked;
	int m_flNextShootTime;
	void Shoot(void);

	float m_flNextFlinch;

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
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	BOOL CheckRoll(void);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_bowknight, CBowknight);

const char *CBowknight::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CBowknight::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CBowknight::pAttackSounds[] =
{
	"whiteknight/wk_attack1.wav",
	"whiteknight/wk_attack2.wav",
	"whiteknight/wk_attack3.wav",
	"whiteknight/wk_attack4.wav",
	"whiteknight/wk_attack5.wav"
};

const char *CBowknight::pAlertSounds[] =
{
	"whiteknight/wk_alert1.wav",
	"whiteknight/wk_alert2.wav",
	"whiteknight/wk_alert3.wav"
};

const char *CBowknight::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBowknight::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBowknight::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

int CBowknight::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CBowknight::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
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


void CBowknight::PainSound(void)
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

void CBowknight::AlertSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CBowknight::AttackSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;
	// Play a random attack sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBowknight::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
								   // do stuff for this event.
								   CBaseEntity *pHurt = CheckTraceHullAttack(120, gSkillData.zombieDmgOneSlash, DMG_SLASH);
								   if (pHurt)
								   {
									   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									   {
										   pHurt->pev->punchangle.x = 5;
										   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
									   }
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
								   }
								   else
									   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

								   if (RANDOM_LONG(0, 1))
									   AttackSound();
	}
		break;
	case SCIENTIST_JUMP_ATTACK:
	{
								  pev->velocity = pev->velocity + gpGlobals->v_up * 50 + gpGlobals->v_forward * 600;
								  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "common/fatroll.wav", 0.8, ATTN_NORM, 0, 100);
	}
		break;
	case BOWKNIGHT_SHOOT:
	{
							if (RANDOM_LONG(0, 4))
								AttackSound();
						   Shoot();
	}
	case SCIENTIST_TELEPORT:
	{
							   //m_fSequenceFinished = TRUE;
							   //ResetSequenceInfo();
							   //pev->velocity = pev->velocity * 0;
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
void CBowknight::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/bowknight.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 90;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	LastJumped = gpGlobals->time;
	LastTeleported = gpGlobals->time;

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
void CBowknight::Precache()
{
	PRECACHE_MODEL("models/bowknight.mdl");
	PRECACHE_SOUND("common/fatroll.wav");

	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_MODEL("models/crossbow_bolt.mdl");

	PRECACHE_SOUND("common/knight_step1.wav");
	PRECACHE_SOUND("common/knight_step2.wav");
	PRECACHE_SOUND("common/knight_step3.wav");
	PRECACHE_SOUND("common/knight_step4.wav");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CBowknight::IgnoreConditions(void)
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


BOOL CBowknight::CheckRoll()
{
	TraceResult	tr;
	UTIL_MakeVectors(m_hEnemy->pev->angles);
	UTIL_TraceHull(m_hEnemy->pev->origin, m_hEnemy->pev->origin - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);
	return (tr.flFraction == 1.0);
}


BOOL CBowknight::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 2400 && flDist > 150 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		if (RANDOM_LONG(0, 1) && (gpGlobals->time > LastTeleported))
			return FALSE;
		return TRUE;
	}

	return FALSE;
}

BOOL CBowknight::CheckMeleeAttack2(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CBowknight::CheckMeleeAttack1(float flDot, float flDist)
{
	if (gpGlobals->time > LastTeleported && flDist > 150 && flDist < 350)
	{
		LastTeleported = gpGlobals->time + 3;
		m_flNextShootTime = gpGlobals->time + 3;
		return TRUE;
	}
	return FALSE;

	/*if (flDist > 50 && flDist <= 300 && flDot >= 0.7 && gpGlobals->time > LastTeleported)
	{
	UTIL_MakeVectors(pev->angles);
	pev->velocity = pev->velocity + gpGlobals->v_up * 50 + gpGlobals->v_forward * 400;
	pev->sequence = LookupSequence("fatroll");
	ResetSequenceInfo();
	pev->frame = 0;
	LastTeleported = gpGlobals->time + 4;
	pev->framerate = 1;
	pev->nextthink = gpGlobals->time + 0.2;
	return FALSE;
	}*/

	//if (flDist <= 200 && flDist >= 150 && flDot >= 0.7 && gpGlobals->time > LastJumped)
	//{
	//	LastJumped = gpGlobals->time + 5;
	//	return TRUE;
	//}
	return FALSE;
}

void CBowknight::Shoot(void)
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
	vecSpitOffset = (gpGlobals->v_forward * 12 + gpGlobals->v_up * 60 - gpGlobals->v_right * 5);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 24) - vecSpitOffset).Normalize();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM);

	CBowknightDart::Shoot(pev, vecSpitOffset, vecSpitDir * 1600, anglesAim);
	m_flNextShootTime = gpGlobals->time + 1;
}
