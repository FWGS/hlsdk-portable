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

class CEinarHand : public CHeadCrab
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	float GetDamageAmount() { return gSkillData.headcrabDmgBite * 2; }

	void PainSound() {}
	void DeathSound() {}
	void IdleSound() {}
	void AlertSound() {}
	void StartAttackSound() {}
	void AttackSound();
	void BiteSound();

	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
};

LINK_ENTITY_TO_CLASS( einar_hand, CEinarHand );

const char *CEinarHand::pAttackSounds[] =
{
	"thehand/hnd_attack1.wav",
};

const char *CEinarHand::pBiteSounds[] =
{
	"thehand/hnd_headbite.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CEinarHand::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// Spawn
//=========================================================
void CEinarHand::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/thehand.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.headcrabHealth * 2;
	pev->view_ofs = Vector( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarHand::Precache()
{
	PRECACHE_MODEL( "models/thehand.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEinarHand::SetYawSpeed()
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
// AttackSound
//=========================================================
void CEinarHand::AttackSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AttackSound
//=========================================================
void CEinarHand::BiteSound()
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pBiteSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}
