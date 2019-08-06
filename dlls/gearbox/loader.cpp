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
#include	"genericmonster.h"


// For holograms, make them not solid so the player can walk through them
#define	SF_LOADER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CLoader : public CGenericMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType) { return 1; }
};

LINK_ENTITY_TO_CLASS(monster_op4loader, CLoader)


//=========================================================
// Spawn
//=========================================================
void CLoader::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/loader.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->health = 8;
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();

	pev->takedamage = DAMAGE_NO;
	if (pev->spawnflags & SF_LOADER_NOTSOLID)
	{
		pev->solid = SOLID_NOT;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CLoader::Precache()
{
	PRECACHE_MODEL("models/loader.mdl");
}

void CLoader::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(1.0,2.0) );
}
