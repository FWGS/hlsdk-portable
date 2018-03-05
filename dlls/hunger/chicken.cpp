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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CEinarChicken : public CHeadCrab
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();

	void PainSound();
	void DeathSound();
	void IdleSound();
	void AlertSound();
	void AttackSound();
	void StartAttackSound();
	void BiteSound();

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS( monster_th_chicken, CEinarChicken )

const char *CEinarChicken::pIdleSounds[] =
{
	"chicken/ch_idle1.wav",
	"chicken/ch_idle2.wav",
};

const char *CEinarChicken::pAlertSounds[] =
{
	"chicken/ch_alert1.wav",
	"chicken/ch_alert2.wav"
};

const char *CEinarChicken::pPainSounds[] =
{
	"chicken/ch_pain1.wav",
	"chicken/ch_pain2.wav",
};

const char *CEinarChicken::pAttackSounds[] =
{
	"chicken/ch_attack1.wav",
	"chicken/ch_attack2.wav",
};

const char *CEinarChicken::pDeathSounds[] =
{
	"chicken/ch_die1.wav",
	"chicken/ch_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CEinarChicken::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// Spawn
//=========================================================
void CEinarChicken::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/chicken.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.headcrabHealth;
	pev->view_ofs = Vector( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarChicken::Precache()
{
	PRECACHE_SOUND( "thehand/hnd_attack1.wav" );
        PRECACHE_SOUND( "thehand/hnd_headbite.wav" );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );

	PRECACHE_MODEL( "models/chicken.mdl" );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEinarChicken::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 30;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 60;
		break;
	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// AlertSound
//=========================================================
void CEinarChicken::AlertSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// IdleSound
//=========================================================
void CEinarChicken::IdleSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// BiteSound
//=========================================================
void CEinarChicken::BiteSound()
{
	if( !RANDOM_LONG( 0, 9 ) )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "chicken/ch_alert1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_headbite.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// PainSound 
//=========================================================
void CEinarChicken::PainSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CEinarChicken::DeathSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDeathSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AttackSound2
//=========================================================
void CEinarChicken::StartAttackSound()
{
	if( !RANDOM_LONG( 0, 9 ) )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AttackSound 
//=========================================================
void CEinarChicken::AttackSound()
{
	if( !RANDOM_LONG( 0, 1 ) )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "thehand/hnd_attack1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}
