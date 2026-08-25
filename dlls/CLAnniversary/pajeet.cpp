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

//PAJEET POWER ATTACK WAVE

#define BOLT_AIR_VELOCITY	1000

class CPJWave : public CBaseEntity
{
	void Spawn(int type);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;
	int m_iSpriteTexture;
	int bolttype;
	int boltlife;

public:
	static CPJWave *BoltCreate(int type);
};
LINK_ENTITY_TO_CLASS(wave, CPJWave);

CPJWave *CPJWave::BoltCreate(int type)
{
	// Create a new entity with CCrossbowBolt private data
	CPJWave *pBolt = GetClassPtr((CPJWave *)NULL);
	pBolt->pev->classname = MAKE_STRING("pajeetweave");
	pBolt->Spawn(type);

	return pBolt;
}

void CPJWave::Spawn(int type)
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER;

	pev->gravity = 0.8;
	pev->speed = 1800;
	bolttype = type;
	SET_MODEL(ENT(pev), "sprites/shockwave.spr");
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	pev->frame = 0;
	pev->framerate = 8;
	pev->rendercolor.x = 188;
	pev->rendercolor.y = 220;
	pev->rendercolor.z = 255;
	pev->scale = 1;
	pev->angles.x = 90;

	UTIL_SetOrigin(pev, pev->origin);
	if (type == 0)
		UTIL_SetSize(pev, Vector(-3, -3, -3), Vector(3, 3, 3));
	else
	{
		UTIL_SetSize(pev, Vector(-32, -32, -2), Vector(32, 32, 2));
		boltlife = 15;
	}

	SetTouch(&CPJWave::BoltTouch);
	SetThink(&CPJWave::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.05;
}


void CPJWave::Precache()
{
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");
}


int	CPJWave::Classify(void)
{
	return	CLASS_NONE;
}

void CPJWave::BoltTouch(CBaseEntity *pOther)
{
	if (pOther->edict() == pev->owner)
		return;

	if (pOther->pev->classname == pev->classname)
		return;

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);
		pOther->TraceAttack(pevOwner, 85, pev->velocity.Normalize(), &tr, DMG_SONIC);
		ApplyMultiDamage(pev, pevOwner);
	}

	SetTouch(NULL);
	SetThink(NULL);
	UTIL_Remove(this);
}

void CPJWave::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.05;
	// blast circles
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 32);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 32 + 32 / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(2); // life
	WRITE_BYTE(40);  // width
	WRITE_BYTE(15);   // noise

	WRITE_BYTE(255);
	WRITE_BYTE(0);
	WRITE_BYTE(0);

	WRITE_BYTE(200); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();
	entvars_t *pevOwner = NULL;
	if (pev->owner)
		pevOwner = VARS(pev->owner);
	RadiusDamage(pev->origin, pevOwner, pevOwner, 8, 64, CLASS_HUMAN_MILITARY, DMG_SONIC);
}


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		1
#define	PAJEET_POWER_CHARGE			2
#define PAJEET_POWER_ATTACK			3
#define PAJEET_POWER_DASH			4

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CPajeet : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);
	float PowerAttackCoolDown;
	float DashCoolDown;
	int LastShitTalked;

	float m_flNextFlinch;

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
	BOOL CheckRangeAttack2(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist) { return FALSE; }
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_pajeet, CPajeet);

const char *CPajeet::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CPajeet::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CPajeet::pAttackSounds[] =
{
	"pajeet/pj_redeem1.wav",
	"pajeet/pj_redeem2.wav",
	"pajeet/pj_redeem3.wav",
	"pajeet/pj_redeem4.wav"
};

const char *CPajeet::pAlertSounds[] =
{
	"pajeet/pj_alert1.wav",
	"pajeet/pj_alert2.wav"
};

const char *CPajeet::pPainSounds[] =
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
int	CPajeet::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPajeet::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

int CPajeet::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CPajeet::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
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


void CPajeet::PainSound(void)
{
	if (m_Activity == ACT_RANGE_ATTACK1)
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 7) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}

void CPajeet::AlertSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CPajeet::AttackSound(void)
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
void CPajeet::HandleAnimEvent(MonsterEvent_t *pEvent)
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
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();

	}
	break;
	case PAJEET_POWER_CHARGE:
	{							
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "pajeet/powerattack.wav", 0.45, ATTN_NORM, 0, 100);
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "ambience/particle_suck1.wav", 1.0, ATTN_NORM, 0, 100);
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_IMPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 48);
		WRITE_BYTE(65);  // radius
		WRITE_BYTE(25); // count
		WRITE_BYTE(10); // life
		MESSAGE_END();
	}
		break;
	case PAJEET_POWER_ATTACK:
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		CPJWave *pBolt;
		pBolt = CPJWave::BoltCreate(0);

		Vector	vecSpitOffset;
		Vector	vecSpitDir;
		Vector anglesAim = pev->v_angle + pev->punchangle;
		UTIL_MakeVectors(anglesAim);
		anglesAim.x = -anglesAim.x;
		UTIL_MakeVectors(pev->angles);

		// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
		// we should be able to read the position of bones at runtime for this info.
		vecSpitOffset = (gpGlobals->v_forward * 12 + gpGlobals->v_up * 48);
		vecSpitOffset = (pev->origin + vecSpitOffset);
		if (m_hEnemy)
			vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 48) - vecSpitOffset).Normalize();
		else
			vecSpitDir = ((pev->origin + gpGlobals->v_forward * 32) - vecSpitOffset).Normalize();

		pBolt->pev->origin = vecSpitOffset;
		pBolt->pev->gravity = 0.1;
		pBolt->pev->friction = 0.8;

		pBolt->pev->owner = edict();

		pBolt->pev->velocity = gpGlobals->v_forward * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;

	}
		break;
	case PAJEET_POWER_DASH:
	{
		if (m_hEnemy)
		{
			UTIL_MakeVectors(m_hEnemy->pev->angles);
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10); // framerate
			MESSAGE_END();

			UTIL_MakeVectors(pev->v_angle);
			TraceResult tr;
			Vector trace_origin;

			trace_origin = pev->origin + gpGlobals->v_up * 8;
			UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 24, m_hEnemy->pev->origin + gpGlobals->v_up * 8, ignore_monsters, NULL, &tr);

			if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
			{
				pev->origin = m_hEnemy->pev->origin + gpGlobals->v_forward * 64;
			}

			EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "debris/beamstart8.wav", 1.0, ATTN_NORM, 0, 100);

			CBaseEntity *pHurt = CheckTraceHullAttack(120, gSkillData.zombieDmgOneSlash * 3, DMG_SLASH);
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
void CPajeet::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/pajeet.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 120;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	PowerAttackCoolDown = gpGlobals->time;
	DashCoolDown = gpGlobals->time;

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
void CPajeet::Precache()
{
	PRECACHE_MODEL("models/pajeet.mdl");

	PRECACHE_SOUND("pajeet/powerattack.wav");
	PRECACHE_SOUND("ambience/particle_suck1.wav");
	PRECACHE_SOUND("debris/beamstart8.wav");

	PRECACHE_SOUND("houndeye/he_blast1.wav");

	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CPajeet::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK1) || (m_Activity == ACT_RANGE_ATTACK2))
	{
		iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;

}

BOOL CPajeet::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CPajeet::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist >= 200 && flDist <= 850 && flDot >= 0.7 && gpGlobals->time > PowerAttackCoolDown)
	{
		PowerAttackCoolDown = gpGlobals->time + 10;
		return TRUE;
	}
	return FALSE;
}


BOOL CPajeet::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist >= 150 && flDist < 400 && flDot >= 0.7 && gpGlobals->time > DashCoolDown)
	{
		DashCoolDown = gpGlobals->time + 7;
		return TRUE;
	}
	return FALSE;
}