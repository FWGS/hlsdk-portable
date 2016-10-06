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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "scripted.h"

class CPassableFurniture : public CFurniture
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(monster_passable_furniture, CPassableFurniture);

//=========================================================
// This used to have something to do with bees flying, but 
// now it only initializes moving furniture in scripted sequences
//=========================================================
void CPassableFurniture::Spawn()
{
	PRECACHE_MODEL((char *)STRING(pev->model));
	SET_MODEL(ENT(pev), STRING(pev->model));

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->health = 80000;
	pev->takedamage = DAMAGE_AIM;
	pev->effects = 0;
	pev->yaw_speed = 0;
	pev->sequence = 0;
	pev->frame = 0;

	//	pev->nextthink += 1.0;
	//	SetThink (WalkMonsterDelay);

	ResetSequenceInfo();
	pev->frame = 0;
	MonsterInit();
}