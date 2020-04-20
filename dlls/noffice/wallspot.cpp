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
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS( wall_spot, CWallSpot )

//=========================================================
//=========================================================
CWallSpot *CWallSpot::CreateSpot( void )
{
	CWallSpot *pSpot = GetClassPtr( (CWallSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING( "wall_spot" );

	return pSpot;
}

//=========================================================
//=========================================================
void CWallSpot::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->scale = 4.0f;
	pev->renderamt = 90;

	SET_MODEL( ENT( pev ), "sprites/beam2.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

void CWallSpot::Precache( void )
{
	PRECACHE_MODEL( "sprites/beam2.spr" );
}

