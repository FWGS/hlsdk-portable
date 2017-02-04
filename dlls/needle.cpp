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

#define	NEEDLE_BODYHIT_VOLUME 128
#define	NEEDLE_WALLHIT_VOLUME 512

class CNeedle : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	virtual BOOL UseDecrement( void )
	{
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
};


LINK_ENTITY_TO_CLASS( weapon_needle, CNeedle )

enum needle_e
{
	NEEDLE_IDLE1,
	NEEDLE_GIVESHOT,
	NEEDLE_DRAW
};


void CNeedle::Spawn( )
{
	Precache();
	m_iId = WEAPON_NEEDLE;
	SET_MODEL( ENT( pev ), "models/w_needle.mdl" );
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CNeedle::Precache( void )
{
	PRECACHE_MODEL( "models/v_needle.mdl" );
	PRECACHE_MODEL( "models/w_needle.mdl" );
	PRECACHE_MODEL( "models/p_needle.mdl" );
	PRECACHE_SOUND( "weapons/needleshot.wav" );
}

int CNeedle::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_NEEDLE;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}

BOOL CNeedle::Deploy()
{
	return DefaultDeploy( "models/v_needle.mdl", "models/p_needle.mdl", NEEDLE_DRAW, "needle" );
}

void CNeedle::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 1;
	SendWeaponAnim( NEEDLE_IDLE1 );
}

void CNeedle::PrimaryAttack()
{
	SendWeaponAnim( NEEDLE_GIVESHOT );
EMIT_SOUND( ENT( pev ), CHAN_ITEM, "weapons/needleshot.wav", 1, ATTN_NORM );
 
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 8;
}

