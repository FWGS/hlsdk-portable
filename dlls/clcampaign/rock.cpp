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

enum rock_e
{
	ROCK_IDLE = 0,
	ROCK_THROW,
	ROCK_DRAW,
	ROCK_FIDGET
};

LINK_ENTITY_TO_CLASS( weapon_rock, CRock )

void CRock::Spawn()
{
	Precache();
	m_iId = WEAPON_ROCK;
	SET_MODEL( ENT( pev ), "models/w_rock.mdl" );

	//m_iDefaultAmmo = PEPSIGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CRock::Precache( void )
{
	PRECACHE_MODEL( "models/v_rock.mdl" );
	PRECACHE_MODEL( "models/w_rock.mdl" );
	PRECACHE_MODEL( "models/p_rock.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	m_usRock = PRECACHE_EVENT( 1, "events/rock.sc" );
}


int CRock::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ROCK;
	p->iWeight = ROCK_WEIGHT;

	return 1;
}

BOOL CRock::Deploy()
{
	return DefaultDeploy( "models/v_rock.mdl", "models/p_rock.mdl", ROCK_DRAW, "rock" );
}

void CRock::PrimaryAttack()
{
	SendWeaponAnim( ROCK_THROW );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
#if !CLIENT_DLL
	CGrenade::ShootRock( m_pPlayer->pev, m_pPlayer->pev->origin + gpGlobals->v_forward * 34 + Vector( 0, 0, 32 ), gpGlobals->v_forward * 800 );
#endif
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75f;
}


void CRock::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	SendWeaponAnim( ROCK_IDLE );
}

