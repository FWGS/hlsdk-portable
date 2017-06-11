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


//#define	CROWBAR_BODYHIT_VOLUME 128
//#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_jason, CJason );



enum gauss_e {
	JASON_IDLE = 0,
	JASON_DRAW,
	JASON_HOLSTER,
	JASON_ATTACK1HIT,
	JASON_ATTACK1MISS,
	JASON_ATTACK2MISS,
	JASON_ATTACK2HIT,
	JASON_ATTACK3MISS,
	JASON_ATTACK3HIT
};


void CJason::Spawn( )
{
	Precache( );
	m_iId = WEAPON_JASON;
	SET_MODEL(ENT(pev), "models/w_jason.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CJason::Precache( void )
{
	PRECACHE_MODEL("models/v_jason.mdl");
	PRECACHE_MODEL("models/w_jason.mdl");
	PRECACHE_MODEL("models/p_jason.mdl");

	m_usJason = PRECACHE_EVENT ( 1, "events/jason.sc" );
}

int CJason::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = WEAPON_JASON;
	p->iWeight = JASON_WEIGHT;
	return 1;
}



BOOL CJason::Deploy( )
{
	return DefaultDeploy( "models/v_jason.mdl", "models/p_jason.mdl", JASON_DRAW, "jason" );
}

void CJason::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( JASON_HOLSTER );
}

void CJason::PrimaryAttack()
{
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/jason1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/jason2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/jason3.wav", 1, ATTN_NORM); break;
				}

if (g_pGameRules->IsHeavyRain())  // is heavy rain enabled?
   {
		m_pPlayer->AddPoints(1,FALSE);
   }
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
}

void CJason::SecondaryAttack()
{
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shaun1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shaun2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shaun3.wav", 1, ATTN_NORM); break;
				}
if (g_pGameRules->IsHeavyRain())  // is heavy rain enabled?
   {
		m_pPlayer->AddPoints(-1,TRUE);
   }
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
}