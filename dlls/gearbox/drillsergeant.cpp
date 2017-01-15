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
// Drill sergeant
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"gman.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CDrillSergeant : public CGMan
{
public:
	void Spawn(void);
	void Precache(void);

	void EXPORT DrillUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

LINK_ENTITY_TO_CLASS(monster_drillsergeant, CDrillSergeant);

//=========================================================
// Spawn
//=========================================================
void CDrillSergeant::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/drill.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->health = 100;
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
	SetUse(&CDrillSergeant::DrillUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDrillSergeant::Precache()
{
	PRECACHE_MODEL("models/drill.mdl");
}

//=========================================================
// Purpose:
//=========================================================
void CDrillSergeant::DrillUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	PlaySentence("DR_POK", 2, VOL_NORM, ATTN_NORM);
}