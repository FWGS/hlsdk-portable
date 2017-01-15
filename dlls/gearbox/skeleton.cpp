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
#include	"weapons.h"

//=========================================================
// SKELETON PROP
//=========================================================
class CSkeleton : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue(KeyValueData *pkvd);

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[4];
};

char *CSkeleton::m_szPoses[] = { "s_onback", "s_sitting", "dead_against_wall", "dead_stomach" };

void CSkeleton::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_skeleton_dead, CSkeleton);

//=========================================================
// ********** Skeleton SPAWN **********
//=========================================================
void CSkeleton::Spawn(void)
{
	PRECACHE_MODEL("models/skeleton.mdl");
	SET_MODEL(ENT(pev), "models/skeleton.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Skeleton with bad pose\n");
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}

