/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "decals.h"
#include "explode.h"
#include "effects.h"

// Fire Trail
class CFireTrail : public CBaseEntity
{
	void Spawn(void);
	void Think(void);
	void Touch(CBaseEntity *pOther);
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS(fire_trail, CFireTrail);

void CFireTrail::Spawn(void)
{
	pev->velocity = RANDOM_FLOAT(200, 300) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100.f, 100.f);
	pev->velocity.y += RANDOM_FLOAT(-100.f, 100.f);
	if (pev->velocity.z >= 0)
		pev->velocity.z += 200;
	else
		pev->velocity.z -= 200;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL(edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT(0.5, 1.5);
	pev->maxspeed = pev->speed;

	pev->angles = g_vecZero;
}


void CFireTrail::Think(void)
{
	CSprite* pSprite = CSprite::SpriteCreate("sprites/zerogxplode.spr", pev->origin, TRUE);
	pSprite->AnimateAndDie(RANDOM_FLOAT(15.0f, 20.0f));
	pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 192, kRenderFxNoDissipation );
	pSprite->SetScale(pev->speed);
	pSprite->pev->frame = pSprite->Frames() - ((max(0, pev->speed - (0.1 * pSprite->pev->framerate)) * pSprite->Frames()) / pev->maxspeed);

	pev->speed -= 0.1;
	if (pev->speed > 0)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove(this);

	pev->flags &= ~FL_ONGROUND;
}

void CFireTrail::Touch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ((pev->velocity.x*pev->velocity.x + pev->velocity.y*pev->velocity.y) < 10.0)
		pev->speed = 0;
}