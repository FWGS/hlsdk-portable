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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "osprey.h"

#define SF_WAITFORTRIGGER	0x40

#define MAX_CARRY	OSPREY_MAX_CARRY

class CBlkopOsprey : public COsprey
{
public:
	void Spawn( void );
	void Precache( void );
};

LINK_ENTITY_TO_CLASS(monster_blkop_osprey, CBlkopOsprey);

void CBlkopOsprey::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/blkop_osprey.mdl");
	UTIL_SetSize(pev, Vector(-400, -400, -100), Vector(400, 400, 32));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_YES;
	m_flRightHealth = 200;
	m_flLeftHealth = 200;
	pev->health = 400;

	m_flFieldOfView = 0; // 180 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	SetThink(&CBlkopOsprey::FindAllThink);
	SetUse(&COsprey::CommandUse);

	if (!(pev->spawnflags & SF_WAITFORTRIGGER))
	{
		pev->nextthink = gpGlobals->time + 1.0;
	}

	m_pos2 = pev->origin;
	m_ang2 = pev->angles;
	m_vel2 = pev->velocity;
}


void CBlkopOsprey::Precache(void)
{
	UTIL_PrecacheOther("monster_male_assassin");

	PRECACHE_MODEL("models/blkop_osprey.mdl");
	PRECACHE_MODEL("models/HVR.mdl");

	PRECACHE_SOUND("apache/ap_rotor4.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");

	m_iExplode = PRECACHE_MODEL("sprites/fexplo.spr");
	m_iTailGibs = PRECACHE_MODEL("models/blkop_tailgibs.mdl");
	m_iBodyGibs = PRECACHE_MODEL("models/blkop_bodygibs.mdl");
	m_iEngineGibs = PRECACHE_MODEL("models/blkop_enginegibs.mdl");
}

