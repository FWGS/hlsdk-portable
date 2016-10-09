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
#include	"schedule.h"
#include	"game.h"
#include	"headcrab.h"

class CHand : public CHeadCrab
{
public:
	void Spawn(void);
	void Precache(void);
	void StartTask(Task_t *pTask);
	void EXPORT LeapTouch(CBaseEntity *pOther);

	void PainSound(void) { }
	void DeathSound(void) { }
	void IdleSound(void) { }
	void AlertSound(void) { }
	void AttackSound(void) {  }

	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
};

LINK_ENTITY_TO_CLASS(einar_hand, CHand);

const char *CHand::pAttackSounds[] =
{
	"thehand/hnd_attack1.wav",
};

const char *CHand::pBiteSounds[] =
{
	"thehand/hnd_headbite.wav",
};

//=========================================================
// Spawn
//=========================================================
void CHand::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/thehand.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.headcrabHealth;
	pev->view_ofs = Vector(0, 0, 20);// position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHand::Precache()
{
	PRECACHE_MODEL("models/thehand.mdl");

	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_SOUND("headcrab/hc_attack1.wav");
	PRECACHE_SOUND("headcrab/hc_attack2.wav");
	PRECACHE_SOUND("headcrab/hc_attack3.wav");
}

//=========================================================
// LeapTouch - this is the hand's touch function when it
// is in the air
//=========================================================
void CHand::LeapTouch(CBaseEntity *pOther)
{
	if (!pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());

		pOther->TakeDamage(pev, pev, GetDamageAmount(), DMG_SLASH);
	}

	SetTouch(NULL);
}

void CHand::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CHand::LeapTouch);
		break;
	}
	default:
	{
		CHeadCrab::StartTask(pTask);
	}
	}
}