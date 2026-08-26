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
#define	EXTRACLEAN_SLAP			1
#define	EXTRACLEAN_SLAM			2
#define	EXTRACLEAN_HOP  		3

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CExtraClean : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	int m_iSpriteTexture;

	float m_flNextFlinch;
	float lastspecialdone;
	float lasttalked;

	void RunAI(void);

	void DeathSound(void);
	void AttackSound(void);
	void PainSound(void);

	static const char *pAttackSounds[];
	static const char *pAngrySounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_extraclean, CExtraClean);

const char *CExtraClean::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CExtraClean::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CExtraClean::pAttackSounds[] =
{
	"extraclean/ec_attack1.wav",
	"extraclean/ec_attack2.wav",
	"extraclean/ec_attack3.wav",
	"extraclean/ec_attack4.wav",
	"extraclean/ec_attack5.wav",
	"extraclean/ec_attack6.wav",
};

const char *CExtraClean::pAngrySounds[] =
{
	"extraclean/ec_angry1.wav",
	"extraclean/ec_angry2.wav",
	"extraclean/ec_angry3.wav",
	"extraclean/ec_angry4.wav",
	"extraclean/ec_angry5.wav",
	"extraclean/ec_angry6.wav",
};

const char *CExtraClean::pDeathSounds[] =
{
	"extraclean/ec_death1.wav",
	"extraclean/ec_death2.wav",
	"extraclean/ec_death3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CExtraClean::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CExtraClean::SetYawSpeed(void)
{
	pev->yaw_speed = 360;
}

int CExtraClean::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CExtraClean::DeathSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pDeathSounds[RANDOM_LONG(0, ARRAYSIZE(pDeathSounds) - 1)], 1.0, ATTN_NORM, 0, 100);
}

void CExtraClean::AttackSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100);
}

void CExtraClean::PainSound(void)
{
	if (lasttalked <= gpGlobals->time)
	{
		lasttalked = gpGlobals->time + 6;
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAngrySounds[RANDOM_LONG(0, ARRAYSIZE(pAngrySounds) - 1)], 1.0, ATTN_NORM, 0, 100);
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CExtraClean::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case EXTRACLEAN_HOP:
	{
						   pev->velocity = pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 180;
	}
	break;
	case EXTRACLEAN_SLAM:
	{
							// do stuff for this event.
							//CBaseEntity *pHurt = CheckTraceHullAttack( 20, gSkillData.zombieDmgBothSlash, DMG_SLASH );
							CBaseEntity *pEntity = NULL;
							while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 300)) != NULL)
							{
								if (pEntity->pev->takedamage != DAMAGE_NO)
								{
									if (!FClassnameIs(pEntity->pev, "monster_extraclean"))
									{
										pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_up * 350;
										pEntity->TakeDamage(pev, pev, 60, DMG_SONIC | DMG_ALWAYSGIB);
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

							if (lasttalked <= gpGlobals->time)
							{
								lasttalked = gpGlobals->time + 8;
								AttackSound();
							}
	}
		break;
	case EXTRACLEAN_SLAP:
	{
								  // do stuff for this event.
								  CBaseEntity *pHurt = CheckTraceHullAttack(95, gSkillData.zombieDmgBothSlash * 3, DMG_SLASH);
								  if (pHurt)
								  {
									  if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									  {
										  pHurt->pev->punchangle.x = 5;
										  pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 200;
									  }
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 50);
								  }
								  else
									  EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 50);

								  if (lasttalked <= gpGlobals->time)
								  {
									  lasttalked = gpGlobals->time + 8;
									  AttackSound();
								  }	  
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
void CExtraClean::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/extraclean.mdl");
	UTIL_SetSize(pev, Vector(-26,-26,0), Vector(26,26,96));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 800;
	pev->view_ofs = Vector(0,0,90);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.4;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	lastspecialdone = gpGlobals->time;
	lasttalked = gpGlobals->time;

	MonsterInit();
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CExtraClean::Precache()
{
	PRECACHE_MODEL("models/extraclean.mdl");
	PRECACHE_SOUND("gonarch/gon_step1.wav");
	PRECACHE_SOUND("gonarch/gon_step2.wav");
	PRECACHE_SOUND("gonarch/gon_step3.wav");
	PRECACHE_SOUND("houndeye/he_blast1.wav");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAngrySounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CExtraClean::RunAI(void)
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
				m_MonsterState = MONSTERSTATE_COMBAT;
				MoveToLocation(ACT_RUN, 0.0, pPlayer->pev->origin);
			}
		}
	}
}


int CExtraClean::IgnoreConditions(void)
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


BOOL CExtraClean::CheckMeleeAttack1(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist <= 160 && flDot >= 0.6 && m_hEnemy != NULL && lastspecialdone > gpGlobals->time)
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CExtraClean::CheckMeleeAttack2(float flDot, float flDist)
{
	if (HasConditions(bits_COND_SEE_ENEMY) && flDist > 90 && flDist <= 300 && flDot >= 0.6 && m_hEnemy != NULL && lastspecialdone < gpGlobals->time)
	{
		lastspecialdone = gpGlobals->time + 8;
		return TRUE;
	}
	return FALSE;
}