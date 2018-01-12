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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum barneyshotgun_e
{
	BARNEYSHOTGUN_IDLE = 0,
	BARNEYSHOTGUN_FIRE,
	BARNEYSHOTGUN_FIRE2,
	BARNEYSHOTGUN_RELOAD,
	BARNEYSHOTGUN_PUMP,
	BARNEYSHOTGUN_START_RELOAD,
	BARNEYSHOTGUN_DRAW,
	BARNEYSHOTGUN_HOLSTER,
	BARNEYSHOTGUN_IDLE4,
	BARNEYSHOTGUN_IDLE_DEEP
};

LINK_ENTITY_TO_CLASS( weapon_barneyshotgun, CBarneyShotgun )

void CBarneyShotgun::Spawn()
{
	Precache();
	m_iId = WEAPON_BARNEYSHOTGUN;
	SET_MODEL( ENT( pev ), "models/w_shotgun.mdl" );

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CBarneyShotgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_barneyshotgun.mdl" );
	CShotgun::Precache();
}

int CBarneyShotgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BARNEYSHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CBarneyShotgun::Deploy()
{
	return DefaultDeploy( "models/v_barneyshotgun.mdl", "models/p_shotgun.mdl", BARNEYSHOTGUN_DRAW, "shotgun" );
}
