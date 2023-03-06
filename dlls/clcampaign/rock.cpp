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
#define WEAPON_ROCK 20

enum shotgun_e
{
	PEPSIGUN_IDLE = 0,
	PEPSIGUN_THROW,
	PEPSIGUN_DRAW,
	PEPSIGUN_FIDGET
};

class CRock : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	void PrimaryAttack( void );
	BOOL Deploy( void );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void ){ return FALSE; }
};


LINK_ENTITY_TO_CLASS( weapon_rock, CRock )

void CRock::Spawn()
{
	Precache();
	m_iId = WEAPON_ROCK;
	SET_MODEL( ENT( pev ), "models/w_rock.mdl" );

	m_iDefaultAmmo = PEPSIGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CRock::Precache( void )
{
	PRECACHE_MODEL( "models/v_rock.mdl" );
	PRECACHE_MODEL( "models/w_rock.mdl" );
	PRECACHE_MODEL( "models/p_rock.mdl" );
}


int CRock::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 8;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ROCK;
	p->iWeight = 16;

	return 1;
}

BOOL CRock::Deploy()
{
	return DefaultDeploy( "models/v_rock.mdl", "models/p_rock.mdl", PEPSIGUN_DRAW, "crowbar" );
}

void CRock::PrimaryAttack()
{
	Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

	if( angThrow.x < 0 )
		angThrow.x = -10 + angThrow.x * ( ( 90 - 10 ) / 90.0 );
	else
		angThrow.x = -10 + angThrow.x * ( ( 90 + 10 ) / 90.0 );

	float flVel = ( 90 - angThrow.x ) * 4;
	if( flVel > 500 )
		flVel = 1300;

	UTIL_MakeVectors( angThrow );
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

	Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;	
	CGrenadeRock::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow * 1.5, 300000000000 );
	SendWeaponAnim( PEPSIGUN_THROW );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->time + 0.5;
	m_flNextSecondaryAttack = gpGlobals->time + 0.75;
	m_flTimeWeaponIdle = gpGlobals->time + 0.75;
	m_fInSpecialReload = 0;
}


void CRock::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		m_flPumpTime = 0;
	}

	if( m_flTimeWeaponIdle <  gpGlobals->time )
	{
		if( m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			Reload();
		}
		else if( m_fInSpecialReload != 0 )
		{
			if( m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			{
				Reload();
			}
			else
			{
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );

				iAnim = PEPSIGUN_IDLE;
				m_flTimeWeaponIdle = gpGlobals->time + ( 20.0 / 9.0 );
			SendWeaponAnim( iAnim );
		}
	}
}

