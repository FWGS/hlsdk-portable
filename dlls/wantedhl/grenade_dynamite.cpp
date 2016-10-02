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

//===================dynamite

LINK_ENTITY_TO_CLASS(dynamite, CDynamite);

void CDynamite::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_dynamite.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}

CGrenade * CDynamite::ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CDynamite *pDynamite = GetClassPtr((CDynamite *)NULL);
	pDynamite->Spawn();
	UTIL_SetOrigin(pDynamite->pev, vecStart);
	pDynamite->pev->velocity = vecVelocity;
	pDynamite->pev->angles = UTIL_VecToAngles(pDynamite->pev->velocity);
	pDynamite->pev->owner = ENT(pevOwner);

	pDynamite->SetTouch(&CDynamite::BounceTouch);	// Bounce if touched

	// Take one second off of the desired detonation time and set the think to PreDetonate. PreDetonate
	// will insert a DANGER sound into the world sound list and delay detonation for one second so that 
	// the grenade explodes after the exact amount of time specified in the call to ShootTimed(). 

	pDynamite->pev->dmgtime = gpGlobals->time + time;
	pDynamite->SetThink(&CDynamite::TumbleThink);
	pDynamite->pev->nextthink = gpGlobals->time + 0.1;
	if (time < 0.1)
	{
		pDynamite->pev->nextthink = gpGlobals->time;
		pDynamite->pev->velocity = Vector(0, 0, 0);
	}

	pDynamite->pev->sequence = RANDOM_LONG(3, 6);
	pDynamite->pev->framerate = 1.0;

	// Tumble through the air
	// pGrenade->pev->avelocity.x = -400;

	pDynamite->pev->gravity = 0.5;
	pDynamite->pev->friction = 0.8;

	SET_MODEL(ENT(pDynamite->pev), "models/w_dynamite.mdl");
	pDynamite->pev->dmg = 100;

	return pDynamite;
}

//======================end dynamite

