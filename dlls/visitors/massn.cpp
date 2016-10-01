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
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"hgrunt.h"

class CMassn : public CHGrunt
{
public:
	void Spawn(void);
	void Precache(void);

	BOOL FOkToSpeak(void);

	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);

	int IRelationship(CBaseEntity *pTarget);
};

LINK_ENTITY_TO_CLASS(monster_human_massassin, CMassn);

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	CHGrunt::Spawn();

	SET_MODEL(ENT(pev), "models/massn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	CHGrunt::Precache();

	PRECACHE_MODEL("models/massn.mdl");
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CMassn::FOkToSpeak(void)
{
	return FALSE;
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound(void)
{
}

//=========================================================
// DeathSound 
//=========================================================
void CMassn::DeathSound(void)
{
}

//=========================================================
// IdleSound 
//=========================================================
void CMassn::IdleSound(void)
{
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CMassn::IRelationship(CBaseEntity *pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_human_grunt"))
	{
		return R_DL;
	}
	else if (FClassnameIs(pTarget->pev, "monster_hassassin"))
	{
		return R_AL;
	}

	return CHGrunt::IRelationship(pTarget);
}