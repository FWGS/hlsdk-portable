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
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "flashlightspot.h"

LINK_ENTITY_TO_CLASS(flashlight_spot, CFlashlightSpot);

//=========================================================
//=========================================================
CFlashlightSpot *CFlashlightSpot::CreateSpot(void)
{
	CFlashlightSpot *pSpot = GetClassPtr((CFlashlightSpot *)NULL);
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("flashlight_spot");

	return pSpot;
}

//=========================================================
//=========================================================
void CFlashlightSpot::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow; // kRenderGlow
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 128;
	pev->scale = 4;

	SET_MODEL(ENT(pev), "sprites/beam2.spr");
	UTIL_SetOrigin(pev, pev->origin);
};

void CFlashlightSpot::Precache(void)
{
	PRECACHE_MODEL("sprites/beam2.spr");
};