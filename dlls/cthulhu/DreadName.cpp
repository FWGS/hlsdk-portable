/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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


#define	DREAD_NAME_SPEAK_VOLUME 512

#include "DreadName.h"

LINK_ENTITY_TO_CLASS( weapon_dread_name, CDreadName );



enum dread_name_e {
	DREAD_NAME_OPEN = 0,
	DREAD_NAME_IDLE1,
	DREAD_NAME_IDLE2,
	DREAD_NAME_IDLE3,
	DREAD_NAME_CAST,
	DREAD_NAME_CLOSE
};


void CDreadName::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DREAD_NAME;
	SET_MODEL(ENT(pev), "models/w_dread_name.mdl");
	m_iClip = -1;
	m_iIsCasting = 0;

	FallInit();// get ready to fall down.
}


void CDreadName::Precache( void )
{
	PRECACHE_MODEL("models/v_dread_name.mdl");
	PRECACHE_MODEL("models/w_dread_name.mdl");
	PRECACHE_MODEL("models/p_dread_name.mdl");
	PRECACHE_SOUND("weapons/dreadname.wav");
}

int CDreadName::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = WEAPON_DREAD_NAME;
	p->iWeight = DREAD_NAME_WEIGHT;
	return 1;
}



BOOL CDreadName::Deploy( )
{
	return DefaultDeploy( "models/v_dread_name.mdl", "models/p_dread_name.mdl", DREAD_NAME_OPEN, "dread name" );
}

void CDreadName::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( DREAD_NAME_CLOSE );
}

int CDreadName::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CDreadName::PrimaryAttack()
{
	// send the weapon animation
	SendWeaponAnim( DREAD_NAME_CAST );
	m_iIsCasting = 1;
	m_flNextPrimaryAttack = gpGlobals->time + (42.0/30.0);
	m_flTimeWeaponIdle = gpGlobals->time + 1.0;
}

void CDreadName::WeaponIdle( void )
{
	ResetEmptySound( );

	if ( m_flTimeWeaponIdle > gpGlobals->time )
		return;

	if ( m_iIsCasting )
	{
		// find all monster in a sphere
		const int MAX_MONSTER = 1000;
		CBaseEntity* pEntInSphere[MAX_MONSTER];
		int iMonsters = UTIL_MonstersInSphere(pEntInSphere, MAX_MONSTER, pev->origin, 384);

		// send them the panic message
		int iClass;
		for (int i = 0; i < iMonsters; i++)
		{
			CBaseEntity* pEnt = pEntInSphere[i];

			// is this a monster?
			if (!(pEnt->pev->flags & FL_MONSTER)) continue;

			CBaseMonster* pMon = (CBaseMonster*)pEnt;

			// is it an alien monster (as opposed to a cultist, chicken, etc)?
			iClass = pMon->Classify();

			if (iClass != CLASS_ALIEN_MILITARY 
				&& iClass != CLASS_ALIEN_MONSTER 
				&& iClass != CLASS_ALIEN_PREY 
				&& iClass != CLASS_ALIEN_PREDATOR)
				continue;
			
			pMon->Panic(m_pPlayer->pev);
		}

		m_iIsCasting = 0;
		m_pPlayer->LoseSanity(5);
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
		return;
	}

	int iAnim;

	float flRand = RANDOM_FLOAT(0,1);

	if ( flRand <= 0.4 )
	{
		iAnim = DREAD_NAME_IDLE1;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT(2,3);
	}
	else if ( flRand <= 0.75 )
	{
		iAnim = DREAD_NAME_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}
	else 
	{
		iAnim = DREAD_NAME_IDLE3;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	SendWeaponAnim( iAnim );
}






