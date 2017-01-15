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
#include "effects.h"
#include "apache.h"

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix!
#define SF_NOWRECKAGE		0x08

class CBlkopApache : public CApache
{
public:

	void Spawn( void );
	void Precache( void );
};


LINK_ENTITY_TO_CLASS(monster_blkop_apache, CBlkopApache);

void CBlkopApache::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/blkop_apache.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, -64), Vector(32, 32, 0));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.apacheHealth;

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo();
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse(&CApache::StartupUse);
	}
	else
	{
		SetThink(&CApache::HuntThink);
		SetTouch(&CApache::FlyTouch);
		pev->nextthink = gpGlobals->time + 1.0;
	}

	m_iRockets = 10;
}


void CBlkopApache::Precache(void)
{
	PRECACHE_MODEL("models/blkop_apache.mdl");

	PRECACHE_SOUND("apache/ap_rotor1.wav");
	PRECACHE_SOUND("apache/ap_rotor2.wav");
	PRECACHE_SOUND("apache/ap_rotor3.wav");
	PRECACHE_SOUND("apache/ap_whine1.wav");

	PRECACHE_SOUND("weapons/mortarhit.wav");

	m_iSpriteTexture = PRECACHE_MODEL("sprites/white.spr");

	PRECACHE_SOUND("turret/tu_fire1.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

	m_iExplode = PRECACHE_MODEL("sprites/fexplo.spr");
	m_iBodyGibs = PRECACHE_MODEL("models/blkop_bodygibs.mdl");

	UTIL_PrecacheOther("hvr_rocket");
}