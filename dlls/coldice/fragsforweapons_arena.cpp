/*
	Copyright (c) 1999, Cold Ice Modification. 
	
	This code has been written by SlimShady ( darcuri@optonline.net )

    Use, distribution, and modification of this source code and/or resulting
    object code is restricted to non-commercial enhancements to products from
    Valve LLC.  All other use, distribution, or modification is prohibited
    without written permission from Valve LLC and from the Cold Ice team.

    Please if you use this code in any public form, please give us credit.

*/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"items.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

//*********************************************************
// Rules for Cold Ice Frags for Weapons Arena
//*********************************************************

CFWArena :: CFWArena()
{
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	
	if ( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		char *servercfgfile = (char *)CVAR_GET_STRING( "servercfgfile" );

		if ( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing dedicated server config file\n" );
			sprintf( szCommand, "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
	else
	{
		// listen server
		char *lservercfgfile = (char *)CVAR_GET_STRING( "lservercfgfile" );

		if ( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	    
	}
}

//=========================================================
//=========================================================
void CFWArena::RefreshSkillData( void )
{
	CGameRules::RefreshSkillData();

// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	// Crowbar whack
	gSkillData.plrDmgCrowbar = 25;

	// Knife whack
	gSkillData.plrDmgKnife = 28;

	// Sword whack
	gSkillData.plrDmgSword = 32;

	// Glock Round
	gSkillData.plrDmg9MM = 12;

	// MP5 Round
	gSkillData.plrDmgMP5 = 12;

	// 765mm Round
	gSkillData.plrDmgRifle = 200;

	// Bolt
	gSkillData.plrDmgBoltgun = 15;

	// Contact grenade
	gSkillData.plrDmgContact = 120;

	// Cluster grenade
	gSkillData.plrDmgClusterGrenade = 50;

	// Rocket
	gSkillData.plrDmgRocket = 145;

	// Railgun Slug
	gSkillData.plrDmgRailgun = 100;

	// IRF Tripmine
	gSkillData.plrDmgTripmine = 130;

	// Buckshot
	gSkillData.plrDmgBuckshot = 8;

}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

//=========================================================
//=========================================================
void CFWArena :: Think ( void )
{
	///// Check game rules /////

	if ( g_fGameOver )   // someone else quit the game already
	{
		if ( m_flIntermissionEndTime < gpGlobals->time )
		{
			if ( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ((m_flIntermissionEndTime + MAX_INTERMISSION_TIME) < gpGlobals->time) ) 
				ChangeLevel(); // intermission is over
		}
		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;
	
	if ( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		// check if any player is over the frag limit
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if ( pPlayer && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
	}

}


//=========================================================
//=========================================================
BOOL CFWArena::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CFWArena::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CFWArena::IsFWArena( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CFWArena::IsCoOp( void )
{
	return gpGlobals->coop;
}

//=========================================================
//=========================================================
BOOL CFWArena::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if ( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CFWArena :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{

	CBasePlayerItem *pCheck;
	CBasePlayerItem *pBest;// this will be used in the event that we don't find a weapon in the same category.
	int iBestWeight;
	int i;

	iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
	pBest = NULL;

	if ( !pCurrentWeapon->CanHolster() )
	{
		// can't put this gun away right now, so can't switch.
		return FALSE;
	}

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		pCheck = pPlayer->m_rgpPlayerItems[ i ];

		while ( pCheck )
		{
			if ( pCheck->iWeight() > -1 && pCheck->iWeight() == pCurrentWeapon->iWeight() && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->CanDeploy() )
				{
					if ( pPlayer->SwitchWeapon( pCheck ) )
					{
						return TRUE;
					}
				}
			}
			else if ( pCheck->iWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//ALERT ( at_console, "Considering %s\n", STRING( pCheck->pev->classname ) );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->CanDeploy() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->iWeight();
					pBest = pCheck;
				}
			}

			pCheck = pCheck->m_pNext;
		}
	}

	// if we make it here, we've checked all the weapons and found no useable 
	// weapon in the same catagory as the current weapon. 
	
	// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
	// at least get the crowbar, but ya never know.
	if ( !pBest )
	{
		return FALSE;
	}

	pPlayer->SwitchWeapon( pBest );

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CFWArena :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CFWArena :: UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CFWArena :: InitHUD( CBasePlayer *pl )
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && STRING(pl->pev->netname)[0] != 0 ) ? STRING(pl->pev->netname) : "unconnected" ) );

	UTIL_LogPrintf( "\"%s<%i>\" has entered the game\n",  STRING( pl->pev->netname ), GETPLAYERUSERID( pl->edict() ) );

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
	MESSAGE_END();

	SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if ( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( plr->pev->frags );
				WRITE_SHORT( plr->m_iDeaths );
			MESSAGE_END();
		}
	}

	if ( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}

	pl->StartSpectator();
	pl->m_nMenu = Menu_Wep;
	ShowMenu(pl, 0x7, 0, 0,"Welcome to Cold Ice Beta1 2x\n\nCold-Ice Frags for Weapons Mode\n\nPlease choose a weapon.\n1. Knife\n2. Crowbar\n3. Sword");

}

//=========================================================
//=========================================================
void CFWArena :: ClientDisconnected( edict_t *pClient )
{
	if ( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if ( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );
			UTIL_LogPrintf( "\"%s<%i>\" left from FW Arena\n",  STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ) );

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CFWArena :: FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)CVAR_GET_FLOAT("mp_falldamage");

	switch ( iFallDamage )
	{
	case 1://progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
BOOL CFWArena::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CFWArena :: PlayerThink( CBasePlayer *pPlayer )
{
	if ( g_fGameOver )
	{
		// check for button presses
		if ( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}

}
//=========================================================
//=========================================================
BOOL CFWArena :: ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{

	if ( FStrEq( pcmd, "menuselect" ) )
	{

		int slot = atoi( CMD_ARGV(1) );

		//=============================================================
		//=============================================================
		switch(pPlayer->m_nMenu) 
		{
			case Menu_Spec: 
				if(slot == 1)
					pPlayer->StopSpectator();
				else if (slot == 2) 
				{
					pPlayer->StartSpectator();
					ShowMenu (pPlayer, 0x1, 0, 0, "Currently In Spectator Mode:\n\nHit Key 1 to exit and join the game.  " );
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_Wep: 
				if(slot == 1)
					pPlayer->m_cWeapon1 = "weapon_knife";
				else if (slot == 2) 
					pPlayer->m_cWeapon1 = "weapon_fwcrowbar";
				else if (slot == 3) 
					pPlayer->m_cWeapon1 = "weapon_sword";

				pPlayer->m_nMenu = Menu_Spec;
				ShowMenu(pPlayer, 0x3, 0, 0,"Choose a play option:\n\n1. Join Game\n2. Observe Game");
				break;
			//=============================================================
			//=============================================================
			case Menu_Change: 
				if(slot == 1)
					pPlayer->m_cWeapon1 = "weapon_knife";
				else if (slot == 2) 
					pPlayer->m_cWeapon1 = "weapon_crowbar";
				else if (slot == 3) 
					pPlayer->m_cWeapon1 = "weapon_sword";

				ShowMenu(pPlayer, 0x7, 3, 0,"Your manual weapon choice will be changed on\nthe next respawn");
				break;
			//=============================================================
			//=============================================================
			case Menu_WepSel: 
				if(slot == 1)
				{
					ShowMenu(pPlayer, 0x7, 0, 0,"1. Knife ( 1 frag )\n2. Crowbar ( 1 frag )\n3. Sword ( 1 frag )");
					pPlayer->m_nMenu = Menu_WepMan;
				}
				if(slot == 2)
				{
					ShowMenu(pPlayer, 0x3, 0, 0,"1. PPK ( 2 frags )\n2. Mag 60 ( 2 frags )");
					pPlayer->m_nMenu = Menu_WepHand;
				}
				if(slot == 3)
				{
					ShowMenu(pPlayer, 0x3F, 0, 0,"1. M-16 Assault Rifle ( 3 frags )\n2. 9mm Uzi ( 3 frags )\n3. Double Uzis ( 4 frags )\n4. Boltgun ( 4 frags )\n5. Chaingun ( 5 frags )\n6. Sniper Rifle ( 4 frags )");
					pPlayer->m_nMenu = Menu_WepAss;
				}
				if(slot == 4)
				{
					ShowMenu(pPlayer, 0x3, 0, 0,"1. 12 Gauge Shotgun ( 4 frags )\n2. Assault Shotgun ( 5 frags )");
					pPlayer->m_nMenu = Menu_WepShot;
				}
				if(slot == 5)
				{
					ShowMenu(pPlayer, 0xF, 0, 0,"1. Grenade Launcher ( 6 frags )\n2. Rocket Launcher ( 6 frags )\n3. IFR Tripmine ( 2 frags )\n4. Dynamite vest ( 4 frags )");
					pPlayer->m_nMenu = Menu_WepExp;
				}
				if(slot == 6)
				{
					ShowMenu(pPlayer, 0x7, 0, 0,"1. Chumtoad ( 4 frags )\n2. Railgun ( 7 frags )\n3. Nuke Launcher ( 10 frags )");
					pPlayer->m_nMenu = Menu_WepEper;
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_AmmoSel: 
				if(slot == 1)
				{
					ShowMenu(pPlayer, 0x7F, 0, 0,"1. .32 ACP ( 1 frag )\n2. .38 ACP ( 1 frag )\n3. 9mm ( 1 frag )\n4. 9mm Chain ( 1 frags )\n5. 5.56mm ( 1 frags )\n6. 7.65mm ( 1 frags )\n7. Buckshot ( 1 frags )");
					pPlayer->m_nMenu = Menu_AmmoBul;
				}
				if(slot == 2)
				{
					ShowMenu(pPlayer, 0xF, 0, 0,"1. Contact Grenades ( 2 frags )\n2. Timed Grenade ( 2 frags )\n3. Rockets ( 2 frags )\n4. Nuke ( 3 frags )");
					pPlayer->m_nMenu = Menu_AmmoExp;
				}
				if(slot == 3)
				{
					ShowMenu(pPlayer, 0x7, 0, 0,"1. Rail Slugs ( 1 frags )\n2. Bolts ( 1 frags )");
					pPlayer->m_nMenu = Menu_AmmoMisc;
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepMan: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 0 )
					{
						pPlayer->GiveNamedItem( "weapon_knife" );
						pPlayer->pev->frags --;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "weapon_crowbar" );
						pPlayer->pev->frags --;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "weapon_sword" );
						pPlayer->pev->frags --;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepHand: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 1)
					{
						pPlayer->GiveNamedItem( "weapon_ppk" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 1)
					{
						pPlayer->GiveNamedItem( "weapon_mag60" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepAss: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 2)
					{
						pPlayer->GiveNamedItem( "weapon_m16" );
						pPlayer->pev->frags -= 3;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 2)
					{
						pPlayer->GiveNamedItem( "weapon_uzi" );
						pPlayer->pev->frags -= 3;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 3)
					{
						pPlayer->GiveNamedItem( "weapon_doubleuzi" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 4)
				{
					if ( pPlayer->pev->frags > 3)
					{
						pPlayer->GiveNamedItem( "weapon_boltgun" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 5)
				{
					if ( pPlayer->pev->frags > 4)
					{
						pPlayer->GiveNamedItem( "weapon_chaingun" );
						pPlayer->pev->frags -= 5;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 6)
				{
					if ( pPlayer->pev->frags > 3)
					{
						pPlayer->GiveNamedItem( "weapon_rifle" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepShot: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 3)
					{
						pPlayer->GiveNamedItem( "weapon_sshotgun" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 4)
					{
						pPlayer->GiveNamedItem( "weapon_ashotgun" );
						pPlayer->pev->frags -= 5;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepExp: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 5)
					{
						pPlayer->GiveNamedItem( "weapon_grenadel" );
						pPlayer->pev->frags -= 6;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 5)
					{
						pPlayer->GiveNamedItem( "weapon_rocketl" );
						pPlayer->pev->frags -= 6;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 1)
					{
						pPlayer->GiveNamedItem( "weapon_ifrtripmine" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 4)
				{
					if ( pPlayer->pev->frags > 3)
					{
						pPlayer->GiveNamedItem( "weapon_tnt" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_WepEper: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 3 )
					{
						pPlayer->GiveNamedItem( "weapon_chumtoad" );
						pPlayer->pev->frags -= 4;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 6)
					{
						pPlayer->GiveNamedItem( "weapon_railgun" );
						pPlayer->pev->frags -= 7;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 9)
					{
						pPlayer->GiveNamedItem( "weapon_nuke" );
						pPlayer->pev->frags -= 10;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_AmmoBul: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 0 )
					{
						pPlayer->GiveNamedItem( "ammo_ppkclip" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_magclip" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_uziclip" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 4)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_chaingunbox" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 5)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_m16clip" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 6)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_rifleclip" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 7)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_buckshotbox" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				break;
			//=============================================================
			//=============================================================
			case Menu_AmmoExp: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 1 )
					{
						pPlayer->GiveNamedItem( "ammo_contact" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 1 )
					{
						pPlayer->GiveNamedItem( "ammo_timed" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 3)
				{
					if ( pPlayer->pev->frags > 1)
					{
						pPlayer->GiveNamedItem( "ammo_rocket" );
						pPlayer->pev->frags -= 2;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 4)
				{
					if ( pPlayer->pev->frags > 2 )
					{
						pPlayer->GiveNamedItem( "ammo_nukeclip" );
						pPlayer->pev->frags -= 3;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
			//=============================================================
			//=============================================================
			case Menu_AmmoMisc: 
				if(slot == 1)
				{
					if ( pPlayer->pev->frags > 0 )
					{
						pPlayer->GiveNamedItem( "ammo_railslug" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
				if(slot == 2)
				{
					if ( pPlayer->pev->frags > 0)
					{
						pPlayer->GiveNamedItem( "ammo_boltgun" );
						pPlayer->pev->frags--;

						MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
							WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
							WRITE_SHORT( pPlayer->pev->frags );
							WRITE_SHORT( pPlayer->m_iDeaths );
						MESSAGE_END();
					}	
					else 
					{
						ShowMenu(pPlayer, 0x1, 2, 0,"You do not have enough frags.");
					}
				}
			
		
		}
		return TRUE;
	}

		//=============================================================
		//=============================================================
		if (FStrEq(pcmd, "rune_status" ))
		{

			if ( pPlayer->m_iPlayerRune == RUNE_SPEED )
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have Rune: Speed" );
			else if ( pPlayer->m_iPlayerRune == RUNE_RESIST )
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have Rune: Resist" );
			else if ( pPlayer->m_iPlayerRune == RUNE_STRENGTH )
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have Rune: Strength" );
			else if ( pPlayer->m_iPlayerRune == RUNE_HEALTH )
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have Rune: Regeneration" );
			else if ( pPlayer->m_iPlayerRune == RUNE_ROCKETARENA )
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have Rune: Rocket Arena Special" );
			else
				ShowMenu (pPlayer, 0x1, 5, 0, "You Have No Runes." );

			
			return TRUE;	
		}
		//=============================================================
		//=============================================================
		if (FStrEq(pcmd, "changeweapons" ))
		{
			pPlayer->m_nMenu = Menu_Change;
			ShowMenu(pPlayer, 0x7, 0, 0,"Weapons Change:\n\nPlease choose a weapon.\n1. Knife\n2. Crowbar\n3. Sword");
				
			return TRUE;	
		}
	
		//=============================================================
		//=============================================================
		// Weapons and ammo
		if (FStrEq(pcmd, "weaponmenu" ))
		{
			if ( pPlayer->IsAlive() )
			{
				pPlayer->m_nMenu = Menu_WepSel;
				ShowMenu(pPlayer, 0x7F, 0, 0,"Choose a weapon class to buy:\n1. Manual\n2. Handguns\n3. Assault/Machineguns\n4. Shotguns\n5. High Explosive\n6. Experimental");
			}
			else
			{
				ShowMenu(pPlayer, 0x1, 1, 0,"Cant buy weapons while dead.");
			}

			return TRUE;	
		}
		//=============================================================
		//=============================================================
		else if (FStrEq(pcmd, "ammomenu" ))
		{	
			if ( pPlayer->IsAlive() )
			{	
				pPlayer->m_nMenu = Menu_AmmoSel;
				ShowMenu(pPlayer, 0x7, 0, 0,"Choose a ammo class to buy:\n1. Bullet Type\n2. High Explosive\n3. Misc");
			}
			else
			{
				ShowMenu(pPlayer, 0x1, 1, 0,"Cant buy weapons while dead.");
			}

			return TRUE;
		}
		//=============================================================
		//=============================================================
		else if ( FStrEq( pcmd, "stopobserv" ) ) 
		{
			pPlayer->StopSpectator();
			return TRUE;
		}
		//=============================================================
		//=============================================================
	return FALSE;

}

//=========================================================
//=========================================================
void CFWArena :: PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if ( addDefault )
	{
		pPlayer->GiveNamedItem( pPlayer->m_cWeapon1 );
	}
}

//=========================================================
//=========================================================
BOOL CFWArena :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return pPlayer->m_fWantRespawn;
}

//=========================================================
//=========================================================
float CFWArena :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CFWArena :: AllowAutoTargetCrosshair( void )
{
	return ( CVAR_GET_FLOAT( "mp_autocrosshair" ) != 0 );
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CFWArena :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}


//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CFWArena :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;


	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( ktmp && (ktmp->Classify() == CLASS_PLAYER) )
		peKiller = (CBasePlayer*)ktmp;

	if ( pVictim->pev == pKiller )  
	{  // killed self
		pKiller->frags -= 1;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );
		
		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else
	{  // killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX(PK->edict()) );
			WRITE_SHORT( PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
}

//=========================================================
// Deathnotice. 
//=========================================================
void CFWArena::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity *Killer = CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;

	if ( pKiller->flags & FL_CLIENT )
	{
		killer_index = ENTINDEX(ENT(pKiller));
		
		if ( pevInflictor )
		{
			if ( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
				
				if ( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
	}
	else
	{
		killer_weapon_name = STRING( pevInflictor->classname );
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if ( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );		// the victim
		WRITE_STRING( killer_weapon_name );				// what they were killed by (should this be a string?)
	MESSAGE_END();

	if ( pVictim->pev == pKiller )  
	{  // killed self
		UTIL_LogPrintf( "\"%s<%i>\" killed self with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}
	else if ( pKiller->flags & FL_CLIENT )
	{
		UTIL_LogPrintf( "\"%s<%i>\" killed \"%s<%i>\" with %s\n",  STRING( pKiller->netname ),
			GETPLAYERUSERID( ENT(pKiller) ),
			STRING( pVictim->pev->netname ),
			GETPLAYERUSERID( pVictim->edict() ),
			killer_weapon_name );
	}
	else
	{  // killed by the world
		UTIL_LogPrintf( "\"%s<%i>\" killed by world with %s\n",  STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ), killer_weapon_name );
	}

	return; 
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CFWArena :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CFWArena :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return FALSE;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CFWArena :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if ( pWeapon && pWeapon->m_iId  )
	{
			return 0;
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CFWArena :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CFWArena :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CFWArena::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if ( CVAR_GET_FLOAT("mp_weaponstay") > 0 )
	{
		if ( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for ( int i = 0 ; i < MAX_ITEM_TYPES ; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while ( it != NULL )
			{
				if ( it->m_iId == pItem->m_iId )
				{
					return FALSE;
				}

				it = it->m_pNext;
			}
		}
	}

	return CGameRules::CanHavePlayerItem( pPlayer, pItem );
}

//=========================================================
//=========================================================
BOOL CFWArena::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CFWArena::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CFWArena::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CFWArena::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CFWArena::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CFWArena::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CFWArena::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	if ( pEntity->pev->flags & FL_MONSTER )
		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CFWArena::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CFWArena::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CFWArena::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CFWArena::FlHealthChargerRechargeTime( void )
{
	return 60;
}


float CFWArena::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CFWArena::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}
//=========================================================
//=========================================================
int CFWArena::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CFWArena::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if ( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING(pentSpawnSpot->v.target), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}


//=========================================================
//=========================================================
int CFWArena::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

//=========================================================
//=========================================================
BOOL CFWArena :: PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if ( CVAR_GET_FLOAT( "mp_footsteps" ) == 0 )
		return FALSE;

	if ( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}
//=========================================================
//========================================================

BOOL CFWArena :: FAllowFlashlight( void ) 
{ 
	return CVAR_GET_FLOAT( "mp_flashlight" ) != 0; 
}

//=========================================================
//=========================================================

BOOL CFWArena :: FAllowMonsters( void )
{
	return ( CVAR_GET_FLOAT( "mp_allowmonsters" ) != 0 );
}

//=========================================================
//======== CFWArena private functions ===========
#define INTERMISSION_TIME		6

void CFWArena :: GoToIntermission( void )
{
	if ( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN(MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END();

	m_flIntermissionEndTime = gpGlobals->time + INTERMISSION_TIME;
	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;
}

void CFWArena :: ChangeLevel( void )
{
	char szNextMap[32];
	char szFirstMapInList[32];
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	// find the map to change to

	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );
	strcpy( szNextMap, STRING(gpGlobals->mapname) );
	strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( mapcfile, &length );
	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		sscanf( pFileList, " %32s", szNextMap );
		if ( IS_MAP_VALID( szNextMap ) )
			strcpy( szFirstMapInList, szNextMap );

		// keep pulling mapnames out of the list until the current mapname
		// if the current mapname isn't found,  load the first map in the list
		BOOL next_map_is_it = FALSE;
		while ( 1 )
		{
			while ( *pFileList && isspace( *pFileList ) ) pFileList++; // skip over any whitespace
			if ( !(*pFileList) )
				break;

			char cBuf[32];
			int ret = sscanf( pFileList, " %32s", cBuf );
			// Check the map name is valid
			if ( ret != 1 || *cBuf < 13 )
				break;

			if ( next_map_is_it )
			{
				// check that it is a valid map file
				if ( IS_MAP_VALID( cBuf ) )
				{
					strcpy( szNextMap, cBuf );
					break;
				}
			}

			if ( FStrEq( cBuf, STRING(gpGlobals->mapname) ) )
			{  // we've found our map;  next map is the one to change to
				next_map_is_it = TRUE;
			}

			pFileList += strlen( cBuf );
		}

		FREE_FILE( aFileList );
	}

	if ( !IS_MAP_VALID(szNextMap) )
		strcpy( szNextMap, szFirstMapInList );

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	
	CHANGE_LEVEL( szNextMap, NULL );
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   (MAX_MOTD_CHUNK * 4)

void CFWArena :: SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( "fw_arena_motd.txt", &length );

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while ( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK+1];
		
		if ( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if ( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( aFileList );
}
	

