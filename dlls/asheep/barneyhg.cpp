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

#define	HANDGRENADE_PRIMARY_VOLUME		450

enum barneyhandgrenade_e
{
	BARNEYHANDGRENADE_IDLE = 0,
	BARNEYHANDGRENADE_FIDGET,
	BARNEYHANDGRENADE_PINPULL,
	BARNEYHANDGRENADE_THROW1,	// toss
	BARNEYHANDGRENADE_THROW2,	// medium
	BARNEYHANDGRENADE_THROW3,	// hard
	BARNEYHANDGRENADE_HOLSTER,
	BARNEYHANDGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_barneyhandgrenade, CBarneyHandGrenade )

void CBarneyHandGrenade::Spawn()
{
	Precache();
	m_iId = WEAPON_BARNEYHANDGRENADE;
	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif
	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CBarneyHandGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/v_barneygrenade.mdl" );
	CHandGrenade::Precache();
}

int CBarneyHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_BARNEYHANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CBarneyHandGrenade::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_barneygrenade.mdl", "models/p_grenade.mdl", BARNEYHANDGRENADE_DRAW, "crowbar" );
}
