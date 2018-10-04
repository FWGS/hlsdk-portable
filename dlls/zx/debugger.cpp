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

class CDebugger: public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void EXPORT Commands( BOOL type );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
//	void WeaponIdle( void );
	void UpdateInfo (void);
	int command;
};

LINK_ENTITY_TO_CLASS( weapon_debug, CDebugger );


void CDebugger::Spawn( )
{
	Precache( );
	m_iId = WEAPON_DEBUG;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");
	m_iClip = -1;
	FallInit();// get ready to fall down.
}

int CDebugger::AddToPlayer( CBasePlayer *pPlayer )
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

void CDebugger::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	SendWeaponAnim( 8 );
}

void CDebugger::Precache( void )
{
	PRECACHE_MODEL("models/v_9mmhandgun.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/p_9mmhandgun.mdl");
	PRECACHE_SOUND ("buttons/blip1.wav");
}

int CDebugger::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = WEAPON_DEBUG;
	p->iWeight = -10;
	return 1;
}

BOOL CDebugger::Deploy( )
{
	switch (command){
		case 0:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_TOGGLE\n");
		}break;
		case 1:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_ON\n");
		}break;
		case 2:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_OFF\n");
		}break;
		case 3:{
			ALERT(at_debug,"WEAPON_DEBUG -> Give all weapons etc\n");
		}break;
		case 4:{
			ALERT(at_debug,"WEAPON_DEBUG -> Get AI State\n");
		}break;
		case 5:{
			ALERT(at_debug,"WEAPON_DEBUG -> List Globals\n");
		}break;
		case 6:{
			ALERT(at_debug,"WEAPON_DEBUG -> Player Silent On/Off\n");
		}break;
		case 7:{
			ALERT(at_debug,"WEAPON_DEBUG -> Entity Info\n");
		}break;
		case 8:{
			ALERT(at_debug,"WEAPON_DEBUG -> Show Texture Name\n" );
		}break;
		case 9:{
			ALERT(at_debug,"WEAPON_DEBUG -> node_viewer_fly\n" );
		}break;
		case 10:{
			ALERT(at_debug,"WEAPON_DEBUG -> node_viewer_human\n" );
		}break;
		case 11:{
			ALERT(at_debug,"WEAPON_DEBUG -> Show nearest node connections\n" );
		}break;
		case 12:{
			ALERT(at_debug, "WEAPON_DEBUG -> Random Splatter\n" );
		}break;
		case 13:{
			ALERT(at_debug, "WEAPON_DEBUG -> Remove Monster\n" );
		}break;
		case 14:{
			ALERT(at_debug, "WEAPON_DEBUG -> Spawn Grunt\n" );
		}break;
		}
	return DefaultDeploy( "models/v_9mmhandgun.mdl", "models/p_9mmhandgun.mdl", 7, "onehanded", /*UseDecrement() ? 1 : 0*/ 0 );
}

void CDebugger::PrimaryAttack( void )
{
 	Commands (TRUE);
}

void CDebugger::SecondaryAttack( void )
{
	Commands (FALSE);
}

void CDebugger::UpdateInfo (void)
{
	SERVER_COMMAND( "impulse 106\n" );
}

void CDebugger::Commands( BOOL type )
{
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "buttons/blip1.wav", 0.5, ATTN_NORM);
	if(type){
		switch (command){
		case 0:{
			SERVER_COMMAND( "impulse 93\n" );
		}break;
		case 1:{
			SERVER_COMMAND( "impulse 94\n" );
		}break;
		case 2:{
			SERVER_COMMAND( "impulse 95\n" );
		}break;
		case 3:{
			SERVER_COMMAND( "impulse 101\n" );
		}break;
		case 4:{
			SERVER_COMMAND( "impulse 103\n" );
		}break;
		case 5:{
			SERVER_COMMAND( "impulse 104\n" );
		}break;
		case 6:{
			SERVER_COMMAND( "impulse 105\n" );
		}break;
		case 7:{
			SERVER_COMMAND( "impulse 106\n" );
		}break;
		case 8:{
			SERVER_COMMAND( "impulse 107\n" );
		}break;
		case 9:{
			SERVER_COMMAND( "impulse 195\n" );
		}break;
		case 10:{
			SERVER_COMMAND( "impulse 197\n" );
		}break;
		case 11:{
			SERVER_COMMAND( "impulse 199\n" );
		}break;
		case 12:{
			SERVER_COMMAND( "impulse 202\n" );
		}break;
		case 13:{
			SERVER_COMMAND( "impulse 203\n" );
		}break;
		case 14:{
			SERVER_COMMAND( "impulse 76\n" );
		}break;
		}
		
	}else{
		command++;
		if (command == 15)command=0;
		
		switch (command){
		case 0:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_TOGGLE\n");
		}break;
		case 1:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_ON\n");
		}break;
		case 2:{
			ALERT(at_debug,"WEAPON_DEBUG -> USE_OFF\n");
		}break;
		case 3:{
			ALERT(at_debug,"WEAPON_DEBUG -> Give all weapons etc\n");
		}break;
		case 4:{
			ALERT(at_debug,"WEAPON_DEBUG -> Get AI State\n");
		}break;
		case 5:{
			ALERT(at_debug,"WEAPON_DEBUG -> List Globals\n");
		}break;
		case 6:{
			ALERT(at_debug,"WEAPON_DEBUG -> Player Silent On/Off\n");
		}break;
		case 7:{
			ALERT(at_debug,"WEAPON_DEBUG -> Entity Info\n");
		}break;
		case 8:{
			ALERT(at_debug,"WEAPON_DEBUG -> Show Texture Name\n" );
		}break;
		case 9:{
			ALERT(at_debug,"WEAPON_DEBUG -> node_viewer_fly\n" );
		}break;
		case 10:{
			ALERT(at_debug,"WEAPON_DEBUG -> node_viewer_human\n" );
		}break;
		case 11:{
			ALERT(at_debug,"WEAPON_DEBUG -> Show nearest node connections\n" );
		}break;
		case 12:{
			ALERT(at_debug, "WEAPON_DEBUG -> Random Splatter\n" );
		}break;
		case 13:{
			ALERT(at_debug, "WEAPON_DEBUG -> Remove Monster\n" );
		}break;
		case 14:{
			ALERT(at_debug, "WEAPON_DEBUG -> Spawn Grunt\n" );
		}break;
		}
	}
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.3;
}

/*void CDebugger::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	UpdateInfo ();

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
		
	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

	if (flRand <= 0.3 + 0 * 0.75)
	{
		iAnim = 2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
	}
	else if (flRand <= 0.6 + 0 * 0.875)
	{
		iAnim = 0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
	}
	else
	{
		iAnim = 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
	}
	SendWeaponAnim( iAnim, 1 );
}*/
