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


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	MELEEATTACK	( 1 )
#define	RUNATTACK	( 2 )

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CShieldBarn : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	float m_flNextFlinch;
	int m_iSpriteTexture;
	float charge;

	void PainSound(void);
	void AlertSound(void);
	void AttackSound(void);

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_shieldbarn, CShieldBarn);

const char *CShieldBarn::pAttackHitSounds[] =
{
	"buttons/latchunlocked1.wav",
};

const char *CShieldBarn::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CShieldBarn::pAttackSounds[] =
{
	"shieldbarn/taunt1.wav",
	"shieldbarn/taunt2.wav",
	"shieldbarn/taunt3.wav",
	"shieldbarn/taunt4.wav",
};

const char *CShieldBarn::pAlertSounds[] =
{
	"shieldbarn/taunt1.wav",
	"shieldbarn/taunt2.wav",
	"shieldbarn/taunt3.wav",
	"shieldbarn/taunt4.wav",
};

const char *CShieldBarn::pPainSounds[] =
{
	"shieldbarn/die1.wav",
	"shieldbarn/die2.wav",
	"shieldbarn/die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CShieldBarn::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CShieldBarn::SetYawSpeed(void)
{
	int ys;

	ys = 480;

#if 0
	switch (m_Activity)
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CShieldBarn::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CShieldBarn::PainSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 15) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CShieldBarn::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CShieldBarn::AttackSound(void)
{
	// Play a random attack sound
	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CShieldBarn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MELEEATTACK:
	{
						// do stuff for this event.
						CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
						if (pHurt)
						{
							if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
							{
								pHurt->pev->punchangle.x = 5;
								pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 350;
								pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 200;
							}
							EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
						}
						else
							EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

						if (RANDOM_LONG(0, 1))
							AttackSound();
	}
		break;

	case RUNATTACK:
	{
					  // do stuff for this event.
					  //CBaseEntity *pHurt = CheckTraceHullAttack( 20, gSkillData.zombieDmgBothSlash, DMG_SLASH );
					  CBaseEntity *pEntity = NULL;
					  while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 300)) != NULL)
					  {
						  if (pEntity->pev->takedamage != DAMAGE_NO)
						  {
							  if (!FClassnameIs(pEntity->pev, "monster_shieldbarn"))
							  {
								  pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_up * 350;
								  pEntity->TakeDamage(pev, pev, 30, DMG_SONIC | DMG_ALWAYSGIB);
							  }
						  }
					  }
					  // blast circles
					  MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
					  WRITE_BYTE(TE_BEAMCYLINDER);
					  WRITE_COORD(pev->origin.x);
					  WRITE_COORD(pev->origin.y);
					  WRITE_COORD(pev->origin.z + 32);
					  WRITE_COORD(pev->origin.x);
					  WRITE_COORD(pev->origin.y);
					  WRITE_COORD(pev->origin.z + 32 + 200 / .2); // reach damage radius over .3 seconds
					  WRITE_SHORT(m_iSpriteTexture);
					  WRITE_BYTE(0); // startframe
					  WRITE_BYTE(0); // framerate
					  WRITE_BYTE(5); // life
					  WRITE_BYTE(40);  // width
					  WRITE_BYTE(50);   // noise

					  WRITE_BYTE(255);
					  WRITE_BYTE(100);
					  WRITE_BYTE(100);

					  WRITE_BYTE(225); //brightness
					  WRITE_BYTE(0);		// speed
					  MESSAGE_END();
					  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
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
void CShieldBarn::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/shieldbarn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 80;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShieldBarn::Precache()
{
	PRECACHE_MODEL("models/shieldbarn.mdl");
	PRECACHE_SOUND("shieldbarn/step1.wav");
	PRECACHE_SOUND("shieldbarn/step2.wav");
	PRECACHE_SOUND("houndeye/he_blast1.wav");
	PRECACHE_SOUND("houndeye/he_blast2.wav");
	PRECACHE_SOUND("houndeye/he_blast3.wav");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}


BOOL CShieldBarn::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 100 && flDot >= 0.6 && m_hEnemy != NULL && charge > gpGlobals->time)
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CShieldBarn::CheckMeleeAttack2(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 100 && flDot >= 0.6 && m_hEnemy != NULL && charge < gpGlobals->time)
	{
		charge = gpGlobals->time + 5;
		pev->velocity = pev->velocity + gpGlobals->v_forward * 200;
		return TRUE;
	}
	return FALSE;
}


void CShieldBarn::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case 10:
	{
			   UTIL_Ricochet(ptr->vecEndPos, 1.0);
			   flDamage = 0.1;
	}
	}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CShieldBarn::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	/*
	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
	#if 0
	if (pev->health < 20)
	iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	else
	#endif
	if (m_flNextFlinch >= gpGlobals->time)
	iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
	if (m_flNextFlinch < gpGlobals->time)
	m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}
	*/
	return iIgnore;

}