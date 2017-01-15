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
// Zombie Barney
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"zombie.h"

class CZBarney : public CZombie
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
};


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZBARNEY_AE_ATTACK_RIGHT		0x01
#define	ZBARNEY_AE_ATTACK_LEFT		0x02
#define	ZBARNEY_AE_ATTACK_BOTH		0x03

LINK_ENTITY_TO_CLASS(monster_zombie_barney, CZBarney);


const char *CZBarney::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZBarney::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZBarney::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZBarney::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZBarney::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZBarney::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};


//=========================================================
// Spawn
//=========================================================
void CZBarney::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/zombie_barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.zbarneyHealth;
	pev->view_ofs = VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZBarney::Precache()
{
	int i;

	PRECACHE_MODEL("models/zombie_barney.mdl");

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackSounds); i++)
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char *)pPainSounds[i]);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZBarney::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case ZBARNEY_AE_ATTACK_RIGHT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zbarneyDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
			// Play a random attack hit sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else // Play a random attack miss sound
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZBARNEY_AE_ATTACK_LEFT:
	{
		// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zbarneyDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = 18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
			}
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

		if (RANDOM_LONG(0, 1))
			AttackSound();
	}
	break;

	case ZBARNEY_AE_ATTACK_BOTH:
	{
		// do stuff for this event.
		CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.zbarneyDmgBothSlash, DMG_SLASH);
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

	default:
		CZombie::HandleAnimEvent(pEvent);
		break;
	}
}