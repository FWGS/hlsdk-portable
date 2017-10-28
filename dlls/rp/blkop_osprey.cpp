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

	void EXPORT FindAllThink(void);

	CBaseMonster *MakeGrunt(Vector vecSrc);
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


void CBlkopOsprey::FindAllThink(void)
{
	CBaseEntity *pEntity = NULL;

	m_iUnits = 0;
	while (m_iUnits < MAX_CARRY && (pEntity = UTIL_FindEntityByClassname(pEntity, "monster_male_assassin")) != NULL)
	{
		if (pEntity->IsAlive())
		{
			m_hGrunt[m_iUnits] = pEntity;
			m_vecOrigin[m_iUnits] = pEntity->pev->origin;
			m_iUnits++;
		}
	}

	if (m_iUnits == 0)
	{
		ALERT(at_console, "osprey error: no assassins to resupply\n");
		UTIL_Remove(this);
		return;
	}
	SetThink(&COsprey::FlyThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_startTime = gpGlobals->time;
}


CBaseMonster *CBlkopOsprey::MakeGrunt(Vector vecSrc)
{
	CBaseEntity *pEntity;
	CBaseMonster *pGrunt;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	if (tr.pHit && Instance(tr.pHit)->pev->solid != SOLID_BSP)
		return NULL;

	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i] == NULL || !m_hGrunt[i]->IsAlive())
		{
			if (m_hGrunt[i] != NULL && m_hGrunt[i]->pev->rendermode == kRenderNormal)
			{
				m_hGrunt[i]->SUB_StartFadeOut();
			}
			pEntity = Create("monster_male_assassin", vecSrc, pev->angles);
			pGrunt = pEntity->MyMonsterPointer();
			pGrunt->pev->movetype = MOVETYPE_FLY;
			pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
			pGrunt->SetActivity(ACT_GLIDE);

			CBeam *pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
			pBeam->PointEntInit(vecSrc + Vector(0, 0, 112), pGrunt->entindex());
			pBeam->SetFlags(BEAM_FSOLID);
			pBeam->SetColor(255, 255, 255);
			pBeam->SetThink(&CBeam::SUB_Remove);
			pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

			// ALERT( at_console, "%d at %.0f %.0f %.0f\n", i, m_vecOrigin[i].x, m_vecOrigin[i].y, m_vecOrigin[i].z );  
			pGrunt->m_vecLastPosition = m_vecOrigin[i];
			m_hGrunt[i] = pGrunt;
			return pGrunt;
		}
	}
	// ALERT( at_console, "none dead\n");
	return NULL;
}
