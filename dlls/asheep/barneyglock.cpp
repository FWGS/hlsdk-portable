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

enum barneyglock_e
{
	BARNEYGLOCK_IDLE1 = 0,
	BARNEYGLOCK_IDLE2,
	BARNEYGLOCK_IDLE3,
	BARNEYGLOCK_SHOOT,
	BARNEYGLOCK_SHOOT_EMPTY,
	BARNEYGLOCK_RELOAD,
	BARNEYGLOCK_RELOAD_NOT_EMPTY,
	BARNEYGLOCK_DRAW,
	BARNEYGLOCK_HOLSTER,
	BARNEYGLOCK_ADD_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_barney9mmhg, CBarneyGlock )

void CBarneyGlock::Spawn()
{
	Precache();
	m_iId = WEAPON_BARNEYGLOCK;
	SET_MODEL( ENT( pev ), "models/w_9mmhandgun.mdl" );

	m_iDefaultAmmo = GLOCK_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CBarneyGlock::Precache( void )
{
	PRECACHE_MODEL( "models/v_barney9mmhg.mdl" );
	CGlock::Precache();
}

int CBarneyGlock::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GLOCK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_BARNEYGLOCK;
	p->iWeight = GLOCK_WEIGHT;

	return 1;
}

BOOL CBarneyGlock::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_barney9mmhg.mdl", "models/p_9mmhandgun.mdl", BARNEYGLOCK_DRAW, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}
