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
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03
#define	SCIENTIST_JUMP_ATTACK		6
#define SCIENTIST_TELEPORT			9

#define ZOMBIE_FLINCH_DELAY			10		// at most one flinch every n secs

class CFedoraSci : public CBaseMonster
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
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckTeleport(void);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS(monster_fedorasci, CFedoraSci);

const char *CFedoraSci::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CFedoraSci::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CFedoraSci::pAttackSounds[] =
{
	"scientist/fedora1.wav",
	"scientist/fedora4.wav",
	"scientist/fedora6.wav"
};

const char *CFedoraSci::pAlertSounds[] =
{
	"scientist/fedora2.wav",
	"scientist/fedora3.wav",	
	"scientist/fedora5.wav"
};

const char *CFedoraSci::pPainSounds[] =
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
int	CFedoraSci::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFedoraSci::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

int CFedoraSci::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
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

void CFedoraSci::PainSound(void)
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

void CFedoraSci::AlertSound(void)
{
	if (LastShitTalked < gpGlobals->time)
		LastShitTalked = gpGlobals->time + 9;
	else
		return;

	int pitch = 95 + RANDOM_LONG(0, 9);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}


void CFedoraSci::AttackSound(void)
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
void CFedoraSci::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
	{
								   // do stuff for this event.
								   CBaseEntity *pHurt = CheckTraceHullAttack(100, gSkillData.zombieDmgOneSlash*0.8, DMG_SLASH);
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
	case SCIENTIST_JUMP_ATTACK:
	{
								  UTIL_MakeVectors(pev->angles);
								  pev->velocity = pev->velocity + gpGlobals->v_up * 150 + gpGlobals->v_forward * 400;
	}
	break;
	case SCIENTIST_TELEPORT:
	{
		if (m_hEnemy)
		{
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10); // framerate
			MESSAGE_END();
			UTIL_MakeVectors(m_hEnemy->pev->angles);
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/fedora1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/beamstart8.wav", 0.8, ATTN_NORM, 0, 100);
			pev->origin = m_hEnemy->pev->origin - gpGlobals->v_forward * 50;
			pev->angles.y = m_hEnemy->pev->angles.y;
			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10); // framerate
			MESSAGE_END();
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
void CFedoraSci::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/fedorasci.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 90;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.9;// indicates the width of this monster's forward view cone ( as a dotproduct result )
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
void CFedoraSci::Precache()
{
	PRECACHE_MODEL("models/fedorasci.mdl");
	PRECACHE_SOUND("debris/beamstart8.wav");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CFedoraSci::IgnoreConditions(void)
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


BOOL CFedoraSci::CheckTeleport()
{
	TraceResult	tr;
	UTIL_MakeVectors(m_hEnemy->pev->angles);
	UTIL_TraceHull(m_hEnemy->pev->origin, m_hEnemy->pev->origin - gpGlobals->v_forward * 64, dont_ignore_monsters, head_hull, edict(), &tr);
	return (tr.flFraction == 1.0);
}


BOOL CFedoraSci::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 85 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CFedoraSci::CheckMeleeAttack2(float flDot, float flDist)
{
	if (CheckTeleport() && flDist > 200 && flDot >= 0.7 && gpGlobals->time > LastTeleported)
	{
		pev->sequence = LookupSequence("teleport");
		ResetSequenceInfo();
		pev->frame = 0;
		LastTeleported = gpGlobals->time + 8;
		pev->framerate = 1;
		pev->nextthink = gpGlobals->time + 0.2;
		return FALSE;
	}

	if (flDist <= 200 && flDist >= 150 && flDot >= 0.7 && gpGlobals->time > LastJumped)
	{
		LastJumped = gpGlobals->time + 5;
		return TRUE;
	}
	return FALSE;
}