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


//===================cannon ball


LINK_ENTITY_TO_CLASS(cannonball, CCannonBall);

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001


void CCannonBall::Spawn(void)
{
	pev->movetype = MOVETYPE_BOUNCE;

	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/cannonball.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 100;
	m_fRegisteredSound = FALSE;
}


CGrenade *CCannonBall::ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CCannonBall *pCannonBall = GetClassPtr((CCannonBall *)NULL);
	pCannonBall->Spawn();
	// contact grenades arc lower
	pCannonBall->pev->gravity = 0.5;// lower gravity since grenade is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin(pCannonBall->pev, vecStart);
	pCannonBall->pev->velocity = vecVelocity;
	pCannonBall->pev->angles = UTIL_VecToAngles(pCannonBall->pev->velocity);
	pCannonBall->pev->owner = ENT(pevOwner);

	// make monsters afaid of it while in the air
	pCannonBall->SetThink(&CCannonBall::DangerSoundThink);
	pCannonBall->pev->nextthink = gpGlobals->time;

	// Tumble in air
	pCannonBall->pev->avelocity.x = RANDOM_FLOAT(-100, -500);

	// Explode on contact
	pCannonBall->SetTouch(&CCannonBall::ExplodeTouch);

	pCannonBall->pev->dmg = gSkillData.plrDmgM203Grenade;

	SET_MODEL(ENT(pCannonBall->pev), "models/cannonball.mdl");

	return pCannonBall;
}

//======================end cannon ball

