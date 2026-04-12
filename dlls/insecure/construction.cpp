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
// human scientist (passive lab worker)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "animation.h"
#include "soundent.h"

#define CONSTRUCTION_HEAD_GROUP 1
#define CONSTRUCTION_HEAD_BALD 0
#define CONSTRUCTION_HEAD_BEARD 1
#define CONSTRUCTION_HEAD_BLACK 2
#define CONSTRUCTION_HEAD_MUSTACHE 3

#define CONSTRUCTION_HELMET_GROUP 2
#define CONSTRUCTION_HELMET_WEAR 0
#define CONSTRUCTION_HELMET_NOT_WEAR 1

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadConstruction : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData* pkvd);

	int m_iPose; // which sequence to display	-- temporary, don't need to save
	int m_iHead;
	int m_iHelmet;

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	static const char* m_szPoses[4];
};

const char* CDeadConstruction::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "dead_sitting" };

TYPEDESCRIPTION CDeadConstruction::m_SaveData[] =
{
	DEFINE_FIELD(CDeadConstruction, m_iHead, FIELD_INTEGER),
	DEFINE_FIELD(CDeadConstruction, m_iHelmet, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CDeadConstruction, CBaseMonster);

void CDeadConstruction::KeyValue(KeyValueData* pkvd)
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
	else if (FStrEq(pkvd->szKeyName, "helmet"))
	{
		m_iHelmet = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}

	return CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_construction_dead, CDeadConstruction);

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadConstruction::Spawn()
{
	PRECACHE_MODEL("models/construction.mdl");
	SET_MODEL(ENT(pev), "models/construction.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead construction with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8; //gSkillData.barneyHealth;

	// Please don't hurt me.
	if (m_iHead == 1)
	{
		SetBodygroup(CONSTRUCTION_HEAD_GROUP, CONSTRUCTION_HEAD_BEARD);
	}
	else if (m_iHead == 2)
	{
		SetBodygroup(CONSTRUCTION_HEAD_GROUP, CONSTRUCTION_HEAD_BLACK);
	}
	else if (m_iHead == 3)
	{
		SetBodygroup(CONSTRUCTION_HEAD_GROUP, CONSTRUCTION_HEAD_MUSTACHE);
	}
	else if (m_iHead == -1)
	{
		SetBodygroup(CONSTRUCTION_HEAD_GROUP, RANDOM_LONG(0, 3));
	}
	else
	{
		SetBodygroup(CONSTRUCTION_HEAD_GROUP, CONSTRUCTION_HEAD_BALD);
	}

	if (m_iHelmet == 1)
	{
		SetBodygroup(CONSTRUCTION_HELMET_GROUP, CONSTRUCTION_HELMET_NOT_WEAR);
	}
	else
	{
		SetBodygroup(CONSTRUCTION_HELMET_GROUP, CONSTRUCTION_HELMET_WEAR);
	}

	MonsterInitDead();
}