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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "weapons.h"
#include "soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is otis dying for scripted sequences?
#define OTIS_GUN_GROUP		1
#define OTIS_GUN_HOLSTER	0
#define OTIS_GUN_DRAWN		1
#define OTIS_GUN_DONUT		2
#define OTIS_GUN_NONE		3

#define OTIS_HEAD_GROUP		2
#define OTIS_HEAD_HAIR		0
#define OTIS_HEAD_BALD		1

//=========================================================
// DEAD OTIS PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData* pkvd);

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	int m_iHead;
	static const char* m_szPoses[4];
};

const char* CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "dead_sitting" };

void CDeadOtis::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_otis_dead, CDeadOtis);

//=========================================================
// ********** DeadOtis SPAWN **********
//=========================================================
void CDeadOtis::Spawn()
{
	PRECACHE_MODEL("models/otis.mdl");
	SET_MODEL(ENT(pev), "models/otis.mdl");

	SetBodygroup(OTIS_GUN_GROUP, OTIS_GUN_NONE);
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	if (m_iHead == 1)
		SetBodygroup(OTIS_HEAD_GROUP, OTIS_HEAD_BALD);
	else
		SetBodygroup(OTIS_HEAD_GROUP, OTIS_HEAD_HAIR);

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead otis with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8; //gSkillData.otisHealth;

	MonsterInitDead();
}