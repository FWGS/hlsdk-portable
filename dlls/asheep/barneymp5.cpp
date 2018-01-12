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
#include "soundent.h"
#include "gamerules.h"

enum barneymp5_e
{
	BARNEYMP5_LONGIDLE = 0,
	BARNEYMP5_IDLE1,
	BARNEYMP5_LAUNCH,
	BARNEYMP5_RELOAD,
	BARNEYMP5_DEPLOY,
	BARNEYMP5_FIRE1,
	BARNEYMP5_FIRE2,
	BARNEYMP5_FIRE3,
	BARNEYMP5_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_barney9mmar, CBarneyMP5 )

void CBarneyMP5::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_9mmAR.mdl" );
	m_iId = WEAPON_BARNEYMP5;

	m_iDefaultAmmo = MP5_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CBarneyMP5::Precache( void )
{
	PRECACHE_MODEL( "models/v_barney9mmar.mdl" );
	CMP5::Precache();
}

int CBarneyMP5::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BARNEYMP5;
	p->iWeight = MP5_WEIGHT;

	return 1;
}

BOOL CBarneyMP5::Deploy()
{
	return DefaultDeploy( "models/v_barney9mmar.mdl", "models/p_9mmAR.mdl", BARNEYMP5_DEPLOY, "barneymp5" );
}
