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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"islave.h"


class CBabyKelly : public CISlave
{
public:
	void Spawn(void);
	void Precache(void);

	void DeathSound(void);
	void PainSound(void);
	void AlertSound(void);
	void IdleSound(void);

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS(monster_th_babykelly, CBabyKelly);


const char *CBabyKelly::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CBabyKelly::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CBabyKelly::pPainSounds[] =
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char *CBabyKelly::pDeathSounds[] =
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};


//=========================================================
// ALertSound - scream
//=========================================================
void CBabyKelly::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "BKL_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch);

		CallForHelp("monster_alien_slave", 512, m_hEnemy, m_vecEnemyLKP);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CBabyKelly::IdleSound(void)
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "BKL_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// PainSound
//=========================================================
void CBabyKelly::PainSound(void)
{
	if (RANDOM_LONG(0, 2) == 0)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// DieSound
//=========================================================

void CBabyKelly::DeathSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pDeathSounds), 1.0, ATTN_NORM, 0, m_voicePitch);
}

//=========================================================
// Spawn
//=========================================================
void CBabyKelly::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/babykelly.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->effects = 0;
	pev->health = gSkillData.slaveHealth;
	pev->view_ofs = Vector(0, 0, 32);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch = RANDOM_LONG(85, 110);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBabyKelly::Precache()
{
	int i;

	PRECACHE_MODEL("models/babykelly.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for (i = 0; i < ARRAYSIZE(pDeathSounds); i++)
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	UTIL_PrecacheOther("test_effect");
}