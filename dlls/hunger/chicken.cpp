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

class CChicken : public CHeadCrab
{
public:
	void Spawn(void);
	void Precache(void);
	void StartTask(Task_t *pTask);

	void PainSound(void);
	void DeathSound(void);
	void IdleSound(void);
	void AlertSound(void);
	void AttackSound(void);

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS(monster_th_chicken, CChicken);

const char *CChicken::pIdleSounds[] =
{
	"chicken/ch_idle1.wav",
	"chicken/ch_idle2.wav",
};
const char *CChicken::pAlertSounds[] =
{
	"chicken/ch_alert1.wav",
	"chicken/ch_alert1.wav",
};
const char *CChicken::pPainSounds[] =
{
	"chicken/ch_pain1.wav",
	"chicken/ch_pain2.wav",
};
const char *CChicken::pAttackSounds[] =
{
	"chicken/ch_attack1.wav",
	"chicken/ch_attack2.wav",
};

const char *CChicken::pDeathSounds[] =
{
	"chicken/ch_die1.wav",
	"chicken/ch_die2.wav",
};


//=========================================================
// Spawn
//=========================================================
void CChicken::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/chicken.mdl");
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
void CChicken::Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);

	PRECACHE_MODEL("models/chicken.mdl");
}

void CChicken::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CHeadCrab::LeapTouch);
		break;
	}
	default:
	{
		CHeadCrab::StartTask(pTask);
	}
	}
}

//=========================================================
// IdleSound
//=========================================================
void CChicken::IdleSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound 
//=========================================================
void CChicken::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// PainSound 
//=========================================================
void CChicken::PainSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// DeathSound 
//=========================================================
void CChicken::DeathSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AttackSound 
//=========================================================
void CChicken::AttackSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}