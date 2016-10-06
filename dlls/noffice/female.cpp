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

#define		NUM_FEMALE_HEADS		4 // four heads available for female model
enum { HEAD_MICHELLE = 0, HEAD_MICHELLE2 = 1, HEAD_ADELE = 2, HEAD_HELEN = 3 };

//=========================================================
// Female
//=========================================================
class CFemale : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);

	BOOL	CanHeal(void) { return FALSE; }
};

LINK_ENTITY_TO_CLASS(monster_female, CFemale);

//=========================================================
// Spawn
//=========================================================
void CFemale::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/female.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_FEMALE_HEADS - 1);// pick a head, any head
	}

	MonsterInit();
	SetUse(NULL);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFemale::Precache(void)
{
	PRECACHE_MODEL("models/female.mdl");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}


//=========================================================
// Dead Female PROP
//=========================================================
class CDeadFemale : public CDeadScientist
{
public:
	void Spawn(void);

};

LINK_ENTITY_TO_CLASS(monster_Female_dead, CDeadFemale);

//
// ********** DeadCivilian SPAWN **********
//
void CDeadFemale::Spawn()
{
	PRECACHE_MODEL("models/female.mdl");
	SET_MODEL(ENT(pev), "models/female.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_FEMALE_HEADS - 1);// pick a head, any head
	}

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead female with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}
