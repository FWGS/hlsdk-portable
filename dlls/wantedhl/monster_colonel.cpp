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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CColonel : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
};
LINK_ENTITY_TO_CLASS(monster_colonel, CColonel);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CColonel::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// Spawn
//=========================================================
void CColonel::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/colonel.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CColonel::Precache()
{
	PRECACHE_MODEL("models/colonel.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
