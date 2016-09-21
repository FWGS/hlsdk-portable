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
#include	"animation.h"
#include	"soundent.h"
#include	"scientist.h"

class CScientist2 : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);

	BOOL	CanHeal(void) { return FALSE; }

	MONSTERSTATE GetIdealState(void);

	void DeathSound(void) {}
	void PainSound(void) {}
};

LINK_ENTITY_TO_CLASS(monster_scientist2, CScientist2);

void CScientist2::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/scientist2.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_SCIENTIST_HEADS - 1);// pick a head, any head
	}

	MonsterInit();
	SetUse(&CScientist2::FollowerUse);
}

void CScientist2::Precache(void)
{
	PRECACHE_MODEL("models/scientist2.mdl");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

MONSTERSTATE CScientist2::GetIdealState(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		return CScientist::GetIdealState();

	case MONSTERSTATE_COMBAT:
		m_IdealMonsterState = MONSTERSTATE_ALERT;
		return CScientist::GetIdealState();

	default:
		return CScientist::GetIdealState();
	}

	return CScientist::GetIdealState();
}