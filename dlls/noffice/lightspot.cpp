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

LINK_ENTITY_TO_CLASS( light_spot2, CLightSpot )

//=========================================================
//=========================================================
CLightSpot *CLightSpot::CreateSpot( void )
{
	CLightSpot *pSpot = GetClassPtr( (CLightSpot *)NULL );
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING( "light_spot2" );

	return pSpot;
}

//=========================================================
//=========================================================
void CLightSpot::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderTransAdd;
	pev->renderfx = kRenderFxNoDissipation;

	SetBits( pev->effects, EF_DIMLIGHT );

	pev->renderamt = 185;

	SET_MODEL( ENT( pev ), "sprites/beam.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

void CLightSpot::Precache( void )
{
	PRECACHE_MODEL( "sprites/beam.spr" );
}

