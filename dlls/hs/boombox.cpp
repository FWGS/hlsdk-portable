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
//#include "time.h"

LINK_ENTITY_TO_CLASS( weapon_boombox, CBoombox );

enum gauss_e {
	BOOMBOX_IDLE = 0,
	BOOMBOX_DRAW,
	BOOMBOX_HOLSTER,
	BOOMBOX_IDLE2,
	BOOMBOX_IDLE3,
};


const char *CBoombox::pBoomboxSongs[] =
{
	"bbox/song1.wav",
	"bbox/song2.wav",
	"bbox/song3.wav",
	"bbox/song4.wav",
	"bbox/song5.wav",
	"bbox/song6.wav",
	"bbox/song7.wav"
};

void CBoombox::Spawn( )
{
	Precache( );
	m_iId = WEAPON_BOOMBOX;
	SET_MODEL(ENT(pev), "models/w_boombox.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
}


void CBoombox::Precache( void )
{
	int i;
	PRECACHE_MODEL("models/v_boombox.mdl");
	PRECACHE_MODEL("models/w_boombox.mdl");
	PRECACHE_MODEL("models/p_boombox.mdl");
	for ( i = 0; i < ARRAYSIZE( pBoomboxSongs ); i++ )
		PRECACHE_SOUND(pBoomboxSongs[i]);
	PRECACHE_SOUND("bbox/xmassong.wav");
	PRECACHE_SOUND("bbox/songrc.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );

	m_usBoombox = PRECACHE_EVENT ( 1, "events/boombox.sc" );
}

int CBoombox::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_BOOMBOX;
	p->iWeight = BOOMBOX_WEIGHT;
	return 1;
}



BOOL CBoombox::Deploy( )
{
	return DefaultDeploy( "models/v_boombox.mdl", "models/p_boombox.mdl", BOOMBOX_DRAW, "rpg" );
}

void CBoombox::Holster( int skiplocal /* = 0 */ )
{
	int i;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( BOOMBOX_HOLSTER );
	for ( i = 0; i < ARRAYSIZE( pBoomboxSongs ); i++ )
	{
		STOP_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, pBoomboxSongs[i]);
	}
}

void CBoombox::PrimaryAttack()
{	
	if( IsChristmas( true ) )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "bbox/xmassong.wav", 1, ATTN_NORM);
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
		return;
	}
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, pBoomboxSongs[ RANDOM_LONG(0,ARRAYSIZE(pBoomboxSongs)-1) ], 1, ATTN_NORM);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CBoombox::SecondaryAttack()
{	
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "bbox/songrc.wav", 1, ATTN_NORM); //1.83
#ifndef CLIENT_DLL
		UTIL_ScreenShake( m_pPlayer->pev->origin, 25.0, 200.0, 2, 750 );
#endif
		RadiusDamage( m_pPlayer->pev->origin, pev, m_pPlayer->pev, 10, 750, CLASS_NONE, DMG_BILLNYE | DMG_ALWAYSGIB );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.75f;
}
