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
/*

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"

LINK_ENTITY_TO_CLASS(tnt, CTnt);

void CTnt::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_tnt.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}

CGrenade * CTnt::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CTnt *pTnt = GetClassPtr((CTnt *)NULL);
	pTnt->Spawn();
	UTIL_SetOrigin(pTnt->pev, vecStart);
	pTnt->pev->velocity = vecVelocity;
	pTnt->pev->angles = UTIL_VecToAngles(pTnt->pev->velocity);
	pTnt->pev->owner = ENT(pevOwner);

	pTnt->SetTouch(&CTnt::BounceTouch);	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pTnt->pev->dmgtime = gpGlobals->time + time;
	pTnt->SetThink(&CTnt::TumbleThink);
	pTnt->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pTnt->pev->nextthink = gpGlobals->time;
		pTnt->pev->velocity = Vector(0, 0, 0);
	}

	pTnt->pev->sequence = RANDOM_LONG(3, 6);
	pTnt->pev->framerate = 1.0;

	// Tumble through the air
	// pTnt->pev->avelocity.x = -400;

	pTnt->pev->gravity = 0.5;
	pTnt->pev->friction = 0.8;

	SET_MODEL(ENT(pTnt->pev), "models/w_tnt.mdl");
	pTnt->pev->dmg = 100;

	return pTnt;
}