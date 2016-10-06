/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "func_break.h"

LINK_ENTITY_TO_CLASS(weapon_clip, CWeaponClip);

void CWeaponClip::Spawn(const char* szModel)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.35;

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or rendermode! bad!
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	// pev->solid = SOLID_SLIDEBOX;/// hopefully this will fix the VELOCITY TOO LOW crap
	pev->solid = SOLID_TRIGGER;
	pev->classname = MAKE_STRING("weapon_clip");
	m_bloodColor = DONT_BLEED;

	PRECACHE_MODEL((char*)szModel);
	SET_MODEL(ENT(pev), szModel);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	// pev->nextthink = gpGlobals->time + 4;
	m_lifeTime = 25;
	SetThink(&CWeaponClip::WaitTillLand);
	SetTouch(&CWeaponClip::ClipTouch);

	m_bloodColor = DONT_BLEED;
	m_cBloodDecals = 0;// how many blood decals this gib can place (1 per bounce until none remain). 
}

//=========================================================
// WaitTillLand - in order to emit their meaty scent from
// the proper location, gibs should wait until they stop 
// bouncing to emit their scent. That's what this function
// does.
//=========================================================
void CWeaponClip::WaitTillLand(void)
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->velocity == g_vecZero && pev->avelocity == g_vecZero)
	{
		pev->angles = UTIL_VecToAngles(pev->velocity);
		pev->angles[0] = 0;
		pev->movetype = MOVETYPE_NONE;

		SetThink(&CGib::SUB_StartFadeOut);
		pev->nextthink = gpGlobals->time + m_lifeTime;
	}
	else
	{
		// wait and check again in another half second.
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CWeaponClip::ClipTouch(CBaseEntity *pOther)
{
	if (!FClassnameIs(pOther->pev, "worldspawn"))
	{
		pev->nextthink = gpGlobals->time;
		return;
	}
}