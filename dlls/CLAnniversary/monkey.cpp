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
#define	MONKEY_SLAP	1
#define MONKEY_THROW 3

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

#define BOLT_AIR_VELOCITY	900
#define BOLT_WATER_VELOCITY	600

#ifndef CLIENT_DLL
class CMonkeyPoop : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	int touchcounter = 0;
	int iPoopSprite;

	int m_iTrail;

public:
	static CMonkeyPoop *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(monkeypoop, CMonkeyPoop);

CMonkeyPoop *CMonkeyPoop::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CMonkeyPoop *pBolt = GetClassPtr((CMonkeyPoop *)NULL);
	pBolt->pev->classname = MAKE_STRING("monkeypoop");
	pBolt->Spawn();
	pBolt->touchcounter = 0;

	return pBolt;
}

void CMonkeyPoop::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	
	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/poop.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));

	SetTouch(&CMonkeyPoop::BoltTouch);
	pev->nextthink = gpGlobals->time + 0.2;

	pev->body = RANDOM_LONG(0, 2);
}


void CMonkeyPoop::Precache()
{
	PRECACHE_MODEL("models/poop.mdl");
	iPoopSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.
}


int	CMonkeyPoop::Classify(void)
{
	return	CLASS_NONE;
}

void CMonkeyPoop::BoltTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	// play body "thwack" sound
	switch (RANDOM_LONG(0, 6))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh1.wav", 0.9, ATTN_NORM); break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh2.wav", 0.9, ATTN_NORM); break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh3.wav", 0.9, ATTN_NORM); break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh4.wav", 0.9, ATTN_NORM); break;
	case 4:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh5.wav", 0.9, ATTN_NORM); break;
	case 5:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh6.wav", 0.9, ATTN_NORM); break;
	case 6:
		EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh7.wav", 0.9, ATTN_NORM); break;
	}

	if (pOther->pev->takedamage && (pOther->IsBSPModel() == false))
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		if (pOther)
		{
			//pOther->TraceAttack(pevOwner, 12, pev->velocity.Normalize(), &tr, DMG_ACID);
			pOther->TakeDamage(pevOwner, pevOwner, 12, DMG_ACID);
		}
	}

	// spew the spittle temporary ents.
	Vector vecSpitDir = ((pev->origin + pev->view_ofs) - pev->origin).Normalize();
	
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD(pev->origin.x);	// pos
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(vecSpitDir.x);	// dir
	WRITE_COORD(vecSpitDir.y);
	WRITE_COORD(vecSpitDir.z);
	WRITE_SHORT(iPoopSprite);	// model
	WRITE_BYTE(8);			// count
	WRITE_BYTE(45);			// speed
	WRITE_BYTE(25);			// noise ( client will divide by 100 )
	MESSAGE_END();

	UTIL_Remove(this);
}
#endif


class CMonkey : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	float LastJumped;
	float LastPooped;
	int IgnoreConditions(void);
	void RunAI(void);
	void Killed(entvars_t *pevAttacker, int iGib);
	int IRelationship(CBaseEntity *pTarget);

	float m_flNextFlinch;

	void PainSound(void);
	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_monkey, CMonkey);

const char *CMonkey::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CMonkey::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CMonkey::pAttackSounds[] =
{
	"monkey/monke_alert1.wav",
	"monkey/monke_alert2.wav",
	"monkey/monke_pain1.wav",
	"monkey/monke_pain2.wav",
	"monkey/monke_pain3.wav",
	"monkey/monke_pain4.wav",
	"monkey/monke_pain5.wav"
};

const char *CMonkey::pIdleSounds[] =
{
	"monkey/monke_idle1.wav",
	"monkey/monke_idle2.wav",
	"monkey/monke_idle3.wav"
};

const char *CMonkey::pAlertSounds[] =
{
	"monkey/monke_alert1.wav",
	"monkey/monke_alert2.wav"
	"monkey/monke_alert3.wav"
};

const char *CMonkey::pPainSounds[] =
{
	"monkey/monke_pain1.wav",
	"monkey/monke_pain2.wav",
	"monkey/monke_pain3.wav",
	"monkey/monke_pain4.wav",
	"monkey/monke_pain5.wav"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMonkey::Classify(void)
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMonkey::SetYawSpeed(void)
{
	pev->yaw_speed = 360;
}

int CMonkey::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CMonkey::PainSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CMonkey::AlertSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CMonkey::IdleSound(void)
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	// Play a random idle sound
	if (RANDOM_LONG(0,2) == 0)
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 0.6, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}

void CMonkey::AttackSound(void)
{
	// Play a random attack sound
	if (RANDOM_LONG(0, 5) < 2)
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMonkey::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case MONKEY_SLAP:
	{
		// do stuff for this event.
		CBaseEntity *pHurt = CheckTraceHullAttack(85, gSkillData.zombieDmgOneSlash / 2, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -50;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
		break;
	case MONKEY_THROW:
	{
			Vector vecSrc = pev->origin + gpGlobals->v_forward * 32 + gpGlobals->v_right * 8 + gpGlobals->v_up * 48;

			Vector vecShootOrigin;
			UTIL_MakeVectors(pev->angles);
			vecShootOrigin = pev->origin + Vector(0, 0, 55);

			Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
			vecShootDir.z = -vecShootDir.z;

			Vector angDir = UTIL_VecToAngles(vecShootDir);

			CMonkeyPoop *pBolt = CMonkeyPoop::BoltCreate();
			pBolt->pev->origin = vecSrc;
			pBolt->pev->movetype = MOVETYPE_BOUNCE;
			pBolt->pev->gravity = 0.5;
			pBolt->pev->friction = 0.8;
			pBolt->pev->owner = edict();
			pBolt->pev->velocity = vecShootDir * 1300;
			pBolt->pev->speed = 1300;
			pBolt->pev->avelocity.x = -600;
			pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
			pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
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
void CMonkey::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/monkey.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	LastJumped = gpGlobals->time;
	LastPooped = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMonkey::Precache()
{
	PRECACHE_MODEL("models/monkey.mdl");
	UTIL_PrecacheOther("monkeypoop");

	PRECACHE_SOUND("debris/flesh1.wav");
	PRECACHE_SOUND("debris/flesh2.wav");
	PRECACHE_SOUND("debris/flesh3.wav");
	PRECACHE_SOUND("debris/flesh4.wav");
	PRECACHE_SOUND("debris/flesh5.wav");
	PRECACHE_SOUND("debris/flesh6.wav");
	PRECACHE_SOUND("debris/flesh7.wav");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


int CMonkey::IRelationship(CBaseEntity *pTarget)
{
	if (isRandomized)
		return R_DL;

	if (FClassnameIs(pTarget->pev, "monster_monkey"))
		return R_AL;
	else
		return R_HT;

	return CBaseMonster::IRelationship(pTarget);
}


//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CMonkey::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	/*if (isRandomized)
	{
		if (m_hEnemy == NULL && m_Activity == ACT_IDLE)
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
	}*/
}

void CMonkey::Killed(entvars_t *pevAttacker, int iGib)
{
	if (isRandomized)
	{
		if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
		{
			UTIL_MakeVectors(pev->angles);
			CBaseEntity *pGun = DropItem("weapon_money", pev->origin, pev->angles);
			pGun->pev->origin = pGun->pev->origin + gpGlobals->v_up * 16;
			pGun->pev->velocity = pev->velocity + gpGlobals->v_up * 80 + gpGlobals->v_right*RANDOM_LONG(-150, 150) + gpGlobals->v_forward*RANDOM_LONG(-150, 150);
			pGun->pev->avelocity = Vector(RANDOM_FLOAT(-100, 100), 0, 0);
			pGun->pev->gravity = 0.6;
			
		}
		isRandomized = false;
	}

	CBaseMonster::Killed(pevAttacker, iGib);
}


BOOL CMonkey::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 80 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CMonkey::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist > 60 && flDist < 200 && gpGlobals->time > LastJumped)
	{
		LastJumped = gpGlobals->time + 5;
		pev->velocity = pev->velocity + gpGlobals->v_up * 250 + gpGlobals->v_forward * 350;
		return TRUE;
	}
	return FALSE;
}

BOOL CMonkey::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist > 200 && flDist < 600 && gpGlobals->time > LastPooped)
	{
		LastPooped = gpGlobals->time + 3;
		return TRUE;
	}
	return FALSE;
}

int CMonkey::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);

	return iIgnore;

}