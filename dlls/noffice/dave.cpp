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
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"barney.h"

#define	DAVE_BODY_GUNHOLSTERED		0
#define	DAVE_BODY_GUNDRAWN			1
#define DAVE_BODY_GUNGONE			2

class CDave : public CBarney
{
public:
	void Spawn(void);
	void Precache(void);

	void DeathSound(void);
	void PainSound(void);
};

LINK_ENTITY_TO_CLASS(monster_dave, CDave);


//=========================================================
// Spawn
//=========================================================
void CDave::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/dave.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	if (pev->body == DAVE_BODY_GUNDRAWN)
	{
		m_fGunDrawn = TRUE;
	}
	else
	{
		m_fGunDrawn = FALSE; // gun in holster
	}

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(NULL);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDave::Precache()
{
	PRECACHE_MODEL("models/dave.mdl");

	PRECACHE_SOUND("dave/ba_attack2.wav");

	PRECACHE_SOUND("dave/ba_pain1.wav");
	PRECACHE_SOUND("dave/ba_pain2.wav");

	PRECACHE_SOUND("dave/ba_die1.wav");
	PRECACHE_SOUND("dave/ba_die2.wav");
	PRECACHE_SOUND("dave/ba_die3.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

//=========================================================
// PainSound
//=========================================================
void CDave::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dave/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dave/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CDave::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dave/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dave/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "dave/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}