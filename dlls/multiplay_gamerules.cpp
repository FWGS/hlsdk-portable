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
//
// teamplay_gamerules.cpp
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"maxcarry.h"
 
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"voice_gamemgr.h"
#include	"hltv.h"
#include	"trains.h"

extern DLL_GLOBAL CGameRules *g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgServerName;

extern int g_teamplay;

#define ITEM_RESPAWN_TIME	30
#define WEAPON_RESPAWN_TIME	20
#define AMMO_RESPAWN_TIME	20

float g_flIntermissionStartTime = 0;

CVoiceGameMgr	g_VoiceGameMgr;

class CMultiplayGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer(CBasePlayer *pListener, CBasePlayer *pTalker)
	{
		if( g_teamplay )
		{
			if( g_pGameRules->PlayerRelationship( pListener, pTalker ) != GR_TEAMMATE )
			{
				return false;
			}
		}

		return true;
	}
};

static CMultiplayGameMgrHelper g_GameMgrHelper;
//*********************************************************
// Rules for the half-life multiplayer game.
//*********************************************************
CHalfLifeMultiplay::CHalfLifeMultiplay()
{
	g_VoiceGameMgr.Init( &g_GameMgrHelper, gpGlobals->maxClients );
	RefreshSkillData();
	m_flIntermissionEndTime = 0;
	g_flIntermissionStartTime = 0;
	
	// 11/8/98
	// Modified by YWB:  Server .cfg file is now a cvar, so that 
	//  server ops can run multiple game servers, with different server .cfg files,
	//  from a single installed directory.
	// Mapcyclefile is already a cvar.

	// 3/31/99
	// Added lservercfg file cvar, since listen and dedicated servers should not
	// share a single config file. (sjb)
	if( IS_DEDICATED_SERVER() )
	{
		// dedicated server
		/*const char *servercfgfile = CVAR_GET_STRING( "servercfgfile" );

		if( servercfgfile && servercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing dedicated server config file\n" );
			sprintf( szCommand, "exec %s\n", servercfgfile );
			SERVER_COMMAND( szCommand );
		}
		*/
		// this code has been moved into engine, to only run server.cfg once
	}
	else
	{
		// listen server
		const char *lservercfgfile = CVAR_GET_STRING( "lservercfgfile" );

		if( lservercfgfile && lservercfgfile[0] )
		{
			char szCommand[256];
			
			ALERT( at_console, "Executing listen server config file\n" );
			sprintf( szCommand, "exec %s\n", lservercfgfile );
			SERVER_COMMAND( szCommand );
		}
	}
}

BOOL CHalfLifeMultiplay::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	long int TimeHSMP;

	pPlayer->m_iTimeMP = (int)CVAR_GET_FLOAT( "mp_buytime" );
	if ( pPlayer->m_iTimeMP <= 0)
	{
		pPlayer->m_iTimeMP = 1;
	}
	extern float g_flWeaponCheat;

	if ( FStrEq( pcmd, "zangle" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int angle = atoi( CMD_ARGV(1) );

		pPlayer->pev->v_angle.z = angle;

		return TRUE;
	}
	else if ( FStrEq( pcmd, "xangle" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int angle = atoi( CMD_ARGV(1) );

		pPlayer->pev->v_angle.x = angle;

		return TRUE;
	}
	else if ( FStrEq( pcmd, "yangle" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int angle = atoi( CMD_ARGV(1) );

		pPlayer->pev->v_angle.y = angle;

		return TRUE;
	}
	else if ( FStrEq(pcmd, "buy" ) )
	{
		TimeHSMP = (int)CVAR_GET_FLOAT( "mp_buytime" ); //Haunter
		if ( TimeHSMP <= 0)
			TimeHSMP = 1;

		ShowMenu(pPlayer, 0x27F, TimeHSMP, 0, "#Buy"); //default 0x3FF
		pPlayer->m_iMenu = 1;
		pPlayer->m_iBuyable = 1;
		return TRUE;
	}
	/*else if (FStrEq(pcmd, "bank" ))
	{
		ClientPrint(pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "Bank: $%i\n", pPlayer->m_iMoney ) );
		return TRUE;
	}*/
	else if ( FStrEq( pcmd, "menuselect" ) )
	{
		if ( CMD_ARGC() < 2 )
			return TRUE;

		int slot = atoi( CMD_ARGV(1) );

	if (pPlayer->m_iBuyable == 1)
	{
		if (pPlayer->m_iMenu == 1)
		{
			switch(slot)
			{
			case 1:
				ShowMenu(pPlayer, 0x23F, pPlayer->m_iTimeMP, 0, "#BuyPistol"); //default 0x20F
				pPlayer->m_iMenu = 2;
				break;
			case 2:
				ShowMenu(pPlayer, 0x203, pPlayer->m_iTimeMP, 0, "#BuyShotgun");
				pPlayer->m_iMenu = 3;
				break;
			case 3:
				ShowMenu(pPlayer, 0x21F, pPlayer->m_iTimeMP, 0, "#BuySubMachineGun"); //default 0x207
				pPlayer->m_iMenu = 4;
				break;
			case 4:
				ShowMenu(pPlayer, 0x203, pPlayer->m_iTimeMP, 0, "#ChooseType"); //default 0x277
				pPlayer->m_iMenu = 5;
				break;
			case 5:
				ShowMenu(pPlayer, 0x207, pPlayer->m_iTimeMP, 0, "#BuyHeavy");
				pPlayer->m_iMenu = 6;
				break;
			case 6:
				if ( pPlayer->m_iArmor == 1)
				{
					ShowMenu(pPlayer, 0x21F, pPlayer->m_iTimeMP, 0, "#BuyEquip1");
				}

				if ( pPlayer->m_iArmor == 0 )
				{
					ShowMenu(pPlayer, 0x21F, pPlayer->m_iTimeMP, 0, "#BuyEquip2");
				}
				pPlayer->m_iMenu = 7;
				break;
			case 7:
				ShowMenu(pPlayer, 0x23F, pPlayer->m_iTimeMP, 0, "#BuyAmmo");
				pPlayer->m_iMenu = 8;
			    break;
			}
		}
		else if (pPlayer->m_iMenu == 2) //Pistols
		{
			switch(slot)
			{
			case 1: //Glock18
				if(pPlayer->m_iMoney < 400)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_glock18" );
				pPlayer->m_iMoney -= 400;
				pPlayer->m_iBuyable = 0;
				break;
			case 2: //USP .45
				if(pPlayer->m_iMoney < 500)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_usp" );
				pPlayer->m_iMoney -= 500;
				pPlayer->m_iBuyable = 0;
				break;
			case 3: //P228
				if(pPlayer->m_iMoney < 600)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_p228" );
				pPlayer->m_iMoney -= 600;
				pPlayer->m_iBuyable = 0;
				break;
			case 4:
				if(pPlayer->m_iMoney < 650) //DEagle
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_deagle" );
				pPlayer->m_iMoney -= 650;
				pPlayer->m_iBuyable = 0;
				break;
			case 5:
				if(pPlayer->m_iMoney < 750) //FiveseveN
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_fiveseven" );
				pPlayer->m_iMoney -= 750;
				pPlayer->m_iBuyable = 0;
				break;
			case 6:
				if(pPlayer->m_iMoney < 800) //Elites
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_usp") | pPlayer->HasNamedPlayerItem("weapon_glock18") | pPlayer->HasNamedPlayerItem("weapon_deagle") | pPlayer->HasNamedPlayerItem("weapon_p228")
					| pPlayer->HasNamedPlayerItem("weapon_fiveseven") | pPlayer->HasNamedPlayerItem("weapon_elite"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a handgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_elite" );
				pPlayer->m_iMoney -= 800;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		else if (pPlayer->m_iMenu == 3) //Shotguns
		{
			switch(slot)
			{
			case 1:
				if(pPlayer->m_iMoney < 1700) //M3 Super 90
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_m3") | pPlayer->HasNamedPlayerItem("weapon_xm1014"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a shotgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_m3" );
				pPlayer->m_iMoney -= 1700;
				pPlayer->m_iBuyable = 0;
				break;
			case 2:
				if(pPlayer->m_iMoney < 3000) //M1014
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_m3") | pPlayer->HasNamedPlayerItem("weapon_xm1014"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a shotgun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_xm1014" );
				pPlayer->m_iMoney -= 3000;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		else if (pPlayer->m_iMenu == 4) //Sub-Machine guns
		{
			switch(slot)
			{
			case 1:
				if(pPlayer->m_iMoney < 1250) //TMP
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_mp5navy") | pPlayer->HasNamedPlayerItem("weapon_tmp") | pPlayer->HasNamedPlayerItem("weapon_p90") | pPlayer->HasNamedPlayerItem("weapon_mac10") | pPlayer->HasNamedPlayerItem("weapon_ump45"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_tmp" );
				pPlayer->m_iMoney -= 1250;
				pPlayer->m_iBuyable = 0;
				break;
			case 2:
				if(pPlayer->m_iMoney < 1500) //MP5
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_mp5navy") | pPlayer->HasNamedPlayerItem("weapon_tmp") | pPlayer->HasNamedPlayerItem("weapon_p90") | pPlayer->HasNamedPlayerItem("weapon_mac10") | pPlayer->HasNamedPlayerItem("weapon_ump45"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_mp5navy" );
				pPlayer->m_iMoney -= 1500;
				pPlayer->m_iBuyable = 0;
				break;
			case 3:
				if(pPlayer->m_iMoney < 1700) //UMP .45
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_mp5navy") | pPlayer->HasNamedPlayerItem("weapon_tmp") | pPlayer->HasNamedPlayerItem("weapon_p90") | pPlayer->HasNamedPlayerItem("weapon_mac10") | pPlayer->HasNamedPlayerItem("weapon_ump45"))
				{
					//UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_ump45" );
				pPlayer->m_iMoney -= 1700;
				pPlayer->m_iBuyable = 0;
				break;
			case 4:
				if(pPlayer->m_iMoney < 2350) //P90
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_mp5navy") | pPlayer->HasNamedPlayerItem("weapon_tmp") | pPlayer->HasNamedPlayerItem("weapon_p90") | pPlayer->HasNamedPlayerItem("weapon_mac10") | pPlayer->HasNamedPlayerItem("weapon_ump45"))
				{
					//UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_p90" );
				pPlayer->m_iMoney -= 2350;
				pPlayer->m_iBuyable = 0;
				break;
			case 5:
				if(pPlayer->m_iMoney < 1400) //Mac10
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_mp5navy") | pPlayer->HasNamedPlayerItem("weapon_tmp") | pPlayer->HasNamedPlayerItem("weapon_p90") | pPlayer->HasNamedPlayerItem("weapon_mac10") | pPlayer->HasNamedPlayerItem("weapon_ump45"))
				{
					//UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Sub-Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_mac10" );
				pPlayer->m_iMoney -= 1400;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		else if (pPlayer->m_iMenu == 5) //Rifles
		{
			switch(slot)
			{
			case 1:
				ShowMenu(pPlayer, 0x23F, pPlayer->m_iTimeSP, 0, "#BuyAssaultRifle");
				pPlayer->m_iMenu = 9;
				break;
			case 2:
				ShowMenu(pPlayer, 0x20F, pPlayer->m_iTimeSP, 0, "#BuySniperRifle");
				pPlayer->m_iMenu = 10;
				break;
			}
		}

		else if ( pPlayer->m_iMenu == 9)
		{
			switch(slot)
			{
			case 1:
				if(pPlayer->m_iMoney < 2000 ) //Galil
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}
					pPlayer->GiveNamedItem( "weapon_galil" );
				pPlayer->m_iMoney -= 2000;
				pPlayer->m_iBuyable = 0;
				break;

			case 2:
				if(pPlayer->m_iMoney < 2250) //FAMAS
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_famas" );
				pPlayer->m_iMoney -= 2250;
				pPlayer->m_iBuyable = 0;
				break;

			case 3:
				if(pPlayer->m_iMoney < 2500) //AK47
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_ak47" );
				pPlayer->m_iMoney -= 2500;
				pPlayer->m_iBuyable = 0;
				break;

			case 4:
				if(pPlayer->m_iMoney < 3500) //M4A1 w/M203
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_m4a1" );
				pPlayer->m_iMoney -= 3500;
				pPlayer->m_iBuyable = 0;
				break;

			case 5:
				if(pPlayer->m_iMoney < 3500) //AUG
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_aug" );
				pPlayer->m_iMoney -= 3500;
				pPlayer->m_iBuyable = 0;
				break;

			case 6:
				if(pPlayer->m_iMoney < 3500) //SG552
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_famas") | pPlayer->HasNamedPlayerItem("weapon_galil") |
					pPlayer->HasNamedPlayerItem("weapon_ak47") | pPlayer->HasNamedPlayerItem("weapon_aug") |
					pPlayer->HasNamedPlayerItem("weapon_m4a1") | pPlayer->HasNamedPlayerItem("weapon_sg552") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have an assault rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_sg552" );
				pPlayer->m_iMoney -= 3500;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}

		else if ( pPlayer->m_iMenu == 10)
		{
			switch(slot)
			{
			case 1:
				if(pPlayer->m_iMoney < 2750) //Scout
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_scout") | pPlayer->HasNamedPlayerItem("weapon_awp") |
					pPlayer->HasNamedPlayerItem("weapon_g3sg1") | pPlayer->HasNamedPlayerItem("weapon_sg550"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a sniper rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_scout" );
				pPlayer->m_iMoney -= 2750;
				pPlayer->m_iBuyable = 0;
				break;

			case 2:
				if(pPlayer->m_iMoney < 4750) //AW/M
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}

				if ( pPlayer->HasNamedPlayerItem("weapon_scout") | pPlayer->HasNamedPlayerItem("weapon_awp") |
					pPlayer->HasNamedPlayerItem("weapon_g3sg1") | pPlayer->HasNamedPlayerItem("weapon_sg550") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a sniper rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_awp" );
				pPlayer->m_iMoney -= 4750;
				pPlayer->m_iBuyable = 0;
				break;

			case 3:
				if(pPlayer->m_iMoney < 4200) //SG550
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_scout") | pPlayer->HasNamedPlayerItem("weapon_awp") |
					pPlayer->HasNamedPlayerItem("weapon_g3sg1") | pPlayer->HasNamedPlayerItem("weapon_sg550") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a sniper rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_sg550" );
				pPlayer->m_iMoney -= 4200;
				pPlayer->m_iBuyable = 0;
				break;

			case 4:
				if(pPlayer->m_iMoney < 5000) //G3SG1
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_scout") | pPlayer->HasNamedPlayerItem("weapon_awp") |
					pPlayer->HasNamedPlayerItem("weapon_g3sg1") | pPlayer->HasNamedPlayerItem("weapon_sg550"))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a sniper rifle\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_g3sg1" );
				pPlayer->m_iMoney -= 5000;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		else if (pPlayer->m_iMenu == 6) //Machine guns
		{
			switch(slot)
			{
			case 1:
				if(pPlayer->m_iMoney < 5750) //M249
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_m249") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a Machine Gun\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_m249" );
				pPlayer->m_iMoney -= 5750;
				pPlayer->m_iBuyable = 0;
				break;

			case 2:
				if(pPlayer->m_iMoney < 4500)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}
				if ( pPlayer->HasNamedPlayerItem("weapon_rpgrenade") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have a LAW\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_rpgrenade" );
				pPlayer->m_iMoney -= 4500;
				pPlayer->m_iBuyable = 0;
				break;

			case 3:
				int woot;
				woot = MP_MAXMONEY;
				if( pPlayer->m_iMoney < woot )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "Try getting $%i\n", woot ) );
					return TRUE;
				}

				if ( pPlayer->HasNamedPlayerItem("weapon_ultimate") )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "ERROR 404!*@#LOLZZ!!\n" ) );
					return TRUE;
				}

				pPlayer->GiveNamedItem( "weapon_ultimate" );
				pPlayer->m_iMoney -= woot;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		else if (pPlayer->m_iMenu == 7) //Equipment
		{
			switch(slot)
			{
			case 1:
				if ( pPlayer->m_iArmor == 1) //Sells Armor
				{
					if( pPlayer->m_iMoney < 650 )
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}

					if( pPlayer->pev->armorvalue >= 100)
					{
						return TRUE;
					}

						pPlayer->m_iMoney -= (((pPlayer->GiveArmor( 100 )) * 0) + 650);
						//ClientPrint(pPlayer->pev, HUD_PRINTTALK, UTIL_VarArgs( "Bank: $%i\n", pPlayer->m_iMoney ) );
				}

				if ( pPlayer->m_iArmor == 0) //Sells Health
				{
					if( pPlayer->m_iMoney <= 0 )
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}

					pPlayer->m_iMoney -= ((pPlayer->TakeHealth( 100, DMG_GENERIC )) * 0) + 100;
				}
				pPlayer->m_iBuyable = 0;
				break;
			case 2:
				if(pPlayer->m_iMoney < 300)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}

				if(pPlayer->m_iNumHEGrenades >= HELOOP)
				{
					pPlayer->m_iNumHEGrenades = HELOOP; //Just incase
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have 10 HEGrenades\n" ) );
					return TRUE;
				}

				pPlayer->m_iNumHEGrenades++;
				pPlayer->m_iHE++;
				pPlayer->GiveNamedItem( "weapon_hegrenade" );
				pPlayer->m_iMoney -= 300;
				pPlayer->m_iBuyable = 0;
				break;
			case 3:
				if(pPlayer->m_iMoney < 300)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}

				if(	pPlayer->m_iNumHEGrenades >= HELOOP)
				{
					pPlayer->m_iNumHEGrenades = HELOOP; //Just incase
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You cannot carry anymore\n" ) );
					return TRUE;
				}

				for ( pPlayer->m_iNumHEGrenades = pPlayer->m_iHE; pPlayer->m_iNumHEGrenades <= HELOOP; pPlayer->m_iNumHEGrenades++)
				{
					if ( (pPlayer->m_iMoney < 300) || (pPlayer->m_iNumHEGrenades == HELOOP ))
					{
						pPlayer->m_iHE = pPlayer->m_iNumHEGrenades;
						break;
					}
					else
					{
						pPlayer->GiveNamedItem( "weapon_hegrenade" );
					}
					pPlayer->m_iMoney -= 300;
				}
				pPlayer->m_iBuyable = 0;
				break;
			case 4:
				if(pPlayer->m_iMoney < 1500)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}

				if(pPlayer->m_iNumC4 >= C4LOOP)
				{
					pPlayer->m_iNumC4 = C4LOOP; //Just incase
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You already have 3 C4s\n" ) );
					return TRUE;
				}

				pPlayer->m_iNumC4++;
				pPlayer->m_iC4++;
				pPlayer->GiveNamedItem( "weapon_c4" );
				pPlayer->m_iMoney -= 1500;
				pPlayer->m_iBuyable = 0;
				break;
			case 5:
				if(pPlayer->m_iMoney < 1500)
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
					return TRUE;
				}

				if(	pPlayer->m_iNumC4 >= C4LOOP)
				{
					pPlayer->m_iNumC4 = C4LOOP; //Just incase
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You cannot carry anymore\n" ) );
					return TRUE;
				}

				for ( pPlayer->m_iNumC4 = pPlayer->m_iC4; pPlayer->m_iNumC4 <= C4LOOP; pPlayer->m_iNumC4++)
				{
					if ( (pPlayer->m_iMoney < 1500) || (pPlayer->m_iNumC4 == C4LOOP ))
					{
						pPlayer->m_iC4 = pPlayer->m_iNumC4;
						break;
					}
					else
					{
						pPlayer->GiveNamedItem( "weapon_c4" );
					}
					pPlayer->m_iMoney -= 1500;
				}

				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;// select the item from the current menu
			}
		else if (pPlayer->m_iMenu == 8) //ammunition
		{
				switch(slot)
			{
			case 1:
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_KNIFE || pPlayer->m_pActiveItem->m_iId == WEAPON_FLASHBANG || pPlayer->m_pActiveItem->m_iId == WEAPON_HEGRENADE )
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You do not need ammunition for this\n" ) );
					return TRUE;
				}

				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_P90 || pPlayer->m_pActiveItem->m_iId == WEAPON_FIVESEVEN)
				{
					if (pPlayer->m_iMoney < 50)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i57 == _57LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_P90clip");
						pPlayer->m_iMoney -= 50;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_DEAGLE )
				{
					if (pPlayer->m_iMoney < 40)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i50 == _50LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_DEclip");
						pPlayer->m_iMoney -= 40;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_USP || pPlayer->m_pActiveItem->m_iId == WEAPON_MAC10 || pPlayer->m_pActiveItem->m_iId == WEAPON_UMP45 )
				{
					if (pPlayer->m_iMoney < 25)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i45 == _45LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_USPclip");
						pPlayer->m_iMoney -= 25;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_GLOCK18 || pPlayer->m_pActiveItem->m_iId == WEAPON_MP5N || pPlayer->m_pActiveItem->m_iId == WEAPON_TMP || pPlayer->m_pActiveItem->m_iId == WEAPON_ELITE)
				{
					if (pPlayer->m_iMoney < 20)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i9mm == _9MMLOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_MP5clip");
						pPlayer->m_iMoney -= 20;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_P228)
				{
					if (pPlayer->m_iMoney < 50)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i357 == _357LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_P228clip");
						pPlayer->m_iMoney -= 50;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_AWP )
				{
					if (pPlayer->m_iMoney < 125)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i338 == _338LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_AWPclip");
						pPlayer->m_iMoney -= 125;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_AK47 || pPlayer->m_pActiveItem->m_iId == WEAPON_SCOUT || pPlayer->m_pActiveItem->m_iId == WEAPON_G3SG1 )
				{
					if (pPlayer->m_iMoney < 80)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i762 == _762LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_AK47clip");
					    pPlayer->m_iMoney -= 80;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_M4A1 || pPlayer->m_pActiveItem->m_iId == WEAPON_SG552 || pPlayer->m_pActiveItem->m_iId == WEAPON_AUG || pPlayer->m_pActiveItem->m_iId == WEAPON_SG550 || pPlayer->m_pActiveItem->m_iId == WEAPON_GALIL || pPlayer->m_pActiveItem->m_iId == WEAPON_FAMAS )
				{
					if (pPlayer->m_iMoney < 60)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i556 == _556LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_M4A1clip");
						pPlayer->m_iMoney -= 60;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_M249 )
				{
					if (pPlayer->m_iMoney < 60)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i5562 == _5562LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_M249clip");
					    pPlayer->m_iMoney -= 60;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_XM1014 || pPlayer->m_pActiveItem->m_iId == WEAPON_M3)
				{
					if (pPlayer->m_iMoney < 65)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_i12G == _12GLOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_XM4clip");
						pPlayer->m_iMoney -= 65;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_RPGRENADE)
				{
					if (pPlayer->m_iMoney < 250)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iRPG == RPGLOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_RPclip");
						pPlayer->m_iMoney -= 250;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_ULTIMATE)
				{
					if (pPlayer->m_iMoney < 300)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iJud == JudgmentLOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo1(), pPlayer->m_pActiveItem->iMaxAmmo1() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_judclip");
						EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, "weapons/ultimate_judgment.wav", 1, ATTN_NORM);
						pPlayer->m_iMoney -= 300;
					}
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_NONE)
					return TRUE;
				pPlayer->m_iBuyable = 0;
				break;
			case 2:
				// (full ammo bulk buy variants - mirrors case 1 above)
				pPlayer->m_iBuyable = 0;
				break;
			case 3:
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_M4A1)
				{
					if (pPlayer->m_iMoney < 30)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iM203 == M203LOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo2(), pPlayer->m_pActiveItem->iMaxAmmo2() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_M203Gren");
						pPlayer->m_iMoney -= 30;
					}
				}
				else if (!(pPlayer->m_pActiveItem->m_iId == WEAPON_M4A1))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "It's for the M4A1!\n" ) );
					return TRUE;
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_NONE )
					return TRUE;
				pPlayer->m_iBuyable = 0;
				break;
			case 4:
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_M4A1 )
				{
					if (pPlayer->m_iMoney < 30)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iM203 == M203LOOP )
						return TRUE;
					for ( pPlayer->m_iM203Ammo = pPlayer->m_iM203; pPlayer->m_iM203Ammo <= M203LOOP; pPlayer->m_iM203Ammo ++)
					{
						if ( (pPlayer->m_iMoney < 30) || (pPlayer->m_iM203Ammo == M203LOOP ) )
						{
							pPlayer->m_iM203 = pPlayer->m_iM203Ammo;
							break;
						}
						else
						{
							if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo2(), pPlayer->m_pActiveItem->iMaxAmmo2() ) > 0)
							{
								pPlayer->GiveNamedItem("ammo_M203Gren");
								pPlayer->m_iMoney -= 30;
							}
						}
					}
				}
				else if (!(pPlayer->m_pActiveItem->m_iId == WEAPON_M4A1))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "It's for the M4A1!\n" ) );
					return TRUE;
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_NONE )
					return TRUE;
				pPlayer->m_iBuyable = 0;
				break;
			case 5:
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_ULTIMATE)
				{
					if (pPlayer->m_iMoney < 400)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iJus == JusticeLOOP )
						return TRUE;
					if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo2(), pPlayer->m_pActiveItem->iMaxAmmo2() ) > 0)
					{
						pPlayer->GiveNamedItem("ammo_jusrock");
						pPlayer->m_iMoney -= 400;
						EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, "weapons/ultimate_justice.wav", 1, ATTN_NORM);
					}
				}
				else if (!(pPlayer->m_pActiveItem->m_iId == WEAPON_ULTIMATE))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "ERROR 404!*@#LOLZZ!!\n" ) );
					return TRUE;
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_NONE )
					return TRUE;
				pPlayer->m_iBuyable = 0;
				break;
			case 6:
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_ULTIMATE )
				{
					if (pPlayer->m_iMoney < 400)
					{
						ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "You have insufficient funds!\n" ) );
						return TRUE;
					}
					if ( pPlayer->m_iJus == JusticeLOOP )
						return TRUE;
					for ( pPlayer->m_iJusAmmo = pPlayer->m_iJus; pPlayer->m_iJusAmmo <= JusticeLOOP; pPlayer->m_iJusAmmo ++)
					{
						if ( (pPlayer->m_iMoney < 400) || (pPlayer->m_iJusAmmo == JusticeLOOP ) )
						{
							pPlayer->m_iJus = pPlayer->m_iJusAmmo;
							break;
						}
						else
						{
							if (pPlayer->GiveAmmo( 0, (char *)pPlayer->m_pActiveItem->pszAmmo2(), pPlayer->m_pActiveItem->iMaxAmmo2() ) > 0)
							{
								pPlayer->GiveNamedItem("ammo_jusrock");
								pPlayer->m_iMoney -= 400;
								EMIT_SOUND(ENT(pPlayer->pev), CHAN_VOICE, "weapons/ultimate_justice.wav", 1, ATTN_NORM);
							}
						}
					}
				}
				else if (!(pPlayer->m_pActiveItem->m_iId == WEAPON_ULTIMATE))
				{
					ClientPrint(pPlayer->pev, HUD_PRINTCENTER, UTIL_VarArgs( "ERROR 404!*@#LOLZZ!!\n" ) );
					return TRUE;
				}
				if ( pPlayer->m_pActiveItem->m_iId == WEAPON_NONE )
					return TRUE;
				pPlayer->m_iBuyable = 0;
				break;
			}
			pPlayer->m_iMenu = 0;
		}
		return TRUE;
	}
	}
	else if (pPlayer->m_iBuyable == 0)
	{
		if ( FStrEq( pcmd, "menuselect" ) )
		{
			if ( CMD_ARGC() < 2 )
			return TRUE;

		int slot = atoi( CMD_ARGV(1) );
		if (pPlayer->m_iMenu == 0)
		{
			switch(slot)
			{
			case 1:
				pPlayer->m_iMenu = 0;
				break;
			case 2:
				pPlayer->m_iMenu = 0;
				break;
			case 3:
				pPlayer->m_iMenu = 0;
				break;
			case 4:
				pPlayer->m_iMenu = 0;
				break;
			case 5:
				pPlayer->m_iMenu = 0;
				break;
			case 6:
				pPlayer->m_iMenu = 0;
				break;
			case 7:
				pPlayer->m_iMenu = 0;
				break;
			case 8:
				pPlayer->m_iMenu = 0;
				break;
			}
		}
		}
		return TRUE;
	}
	return FALSE;

	if( g_VoiceGameMgr.ClientCommand( pPlayer, pcmd ) )
		return TRUE;
	return CGameRules::ClientCommand( pPlayer, pcmd );
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::RefreshSkillData( void )
{
	// load all default values
	CGameRules::RefreshSkillData();

	// override some values for multiplay.

	// suitcharger
	gSkillData.suitchargerCapacity = 30;

	//Kept these few, just in case
	//Haunter

	// Tripmine
	gSkillData.plrDmgTripmine = 150;

	// hornet
	gSkillData.plrDmgHornet = 10;
	//Haunter
}

// longest the intermission can last, in seconds
#define MAX_INTERMISSION_TIME		120

extern cvar_t timeleft, fragsleft;

extern cvar_t mp_chattime;

//=========================================================
//=========================================================
void CHalfLifeMultiplay::Think( void )
{
	g_VoiceGameMgr.Update( gpGlobals->frametime );

	///// Check game rules /////
	static int last_frags;
	static int last_time;

	int frags_remaining = 0;
	int time_remaining = 0;

	if( g_fGameOver )   // someone else quit the game already
	{
		// bounds check
		int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
		if( time < 1 )
			CVAR_SET_STRING( "mp_chattime", "1" );
		else if( time > MAX_INTERMISSION_TIME )
			CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

		m_flIntermissionEndTime = g_flIntermissionStartTime + mp_chattime.value;

		// check to see if we should change levels now
		if( m_flIntermissionEndTime < gpGlobals->time )
		{
			if( m_iEndIntermissionButtonHit  // check that someone has pressed a key, or the max intermission time is over
				|| ( ( g_flIntermissionStartTime + MAX_INTERMISSION_TIME ) < gpGlobals->time ) ) 
				ChangeLevel(); // intermission is over
		}

		return;
	}

	float flTimeLimit = timelimit.value * 60;
	float flFragLimit = fraglimit.value;

	time_remaining = (int)( flTimeLimit ? ( flTimeLimit - gpGlobals->time ) : 0);

	if( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
	{
		GoToIntermission();
		return;
	}

	if( flFragLimit )
	{
		int bestfrags = 9999;
		int remain;

		// check if any player is over the frag limit
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );

			if( pPlayer && pPlayer->pev->frags >= flFragLimit )
			{
				GoToIntermission();
				return;
			}

			if( pPlayer )
			{
				remain = (int)( flFragLimit - pPlayer->pev->frags );
				if( remain < bestfrags )
				{
					bestfrags = remain;
				}
			}
		}
		frags_remaining = bestfrags;
	}

	// Updates when frags change
	if( frags_remaining != last_frags )
	{
		g_engfuncs.pfnCvar_DirectSet( &fragsleft, UTIL_VarArgs( "%i", frags_remaining ) );
	}

	// Updates once per second
	if( timeleft.value != last_time )
	{
		g_engfuncs.pfnCvar_DirectSet( &timeleft, UTIL_VarArgs( "%i", time_remaining ) );
	}

	last_frags = frags_remaining;
	last_time = time_remaining;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsMultiplayer( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsDeathmatch( void )
{
	return TRUE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsCoOp( void )
{
	return gpGlobals->coop ? TRUE : FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if( !pWeapon->CanDeploy() )
	{
		// that weapon can't deploy anyway.
		return FALSE;
	}

	if( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if( !pPlayer->m_iAutoWepSwitch )
	{
		return FALSE;
	}

	if( pPlayer->m_iAutoWepSwitch == 2
	    && pPlayer->m_afButtonLast & ( IN_ATTACK | IN_ATTACK2 ) )
	{
		return FALSE;
	}

	if( !pPlayer->m_pActiveItem->CanHolster() )
	{
		// can't put away the active item.
		return FALSE;
	}

	if( pWeapon->iWeight() > pPlayer->m_pActiveItem->iWeight() )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return HLGetNextBestWeapon( pPlayer, pCurrentWeapon );
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] )
{
	g_VoiceGameMgr.ClientConnected( pEntity );
	return TRUE;
}

extern int gmsgSayText;
extern int gmsgGameMode;

void CHalfLifeMultiplay::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 );  // game mode none
	MESSAGE_END();
}

void CHalfLifeMultiplay::InitHUD( CBasePlayer *pl )
{
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s has joined the game\n", 
		( pl->pev->netname && ( STRING( pl->pev->netname ) )[0] != 0 ) ? STRING( pl->pev->netname ) : "unconnected" ) );

	// team match?
	if( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERAUTHID( pl->edict() ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pl->edict() ), "model" ) );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" entered the game\n",  
			STRING( pl->pev->netname ), 
			GETPLAYERUSERID( pl->edict() ),
			GETPLAYERAUTHID( pl->edict() ),
			GETPLAYERUSERID( pl->edict() ) );
	}

	UpdateGameMode( pl );

	// sending just one score makes the hud scoreboard active;  otherwise
	// it is just disabled for single play
	//fix a bug in the information about the player's score when he left the server, so that his score would not be transferred to another player(seems to work)
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pl->edict()) );
		WRITE_SHORT( (int)pl->pev->frags );
		WRITE_SHORT( pl->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pl->m_szTeamName ) + 1 );
	MESSAGE_END();

	SendMOTDToClient( pl->edict() );

	// loop through all active players and send their score info to the new client
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		// FIXME:  Probably don't need to cast this just to read m_iDeaths
		CBasePlayer *plr = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if( plr )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgScoreInfo, NULL, pl->edict() );
				WRITE_BYTE( i );	// client number
				WRITE_SHORT( (int)plr->pev->frags );
				WRITE_SHORT( plr->m_iDeaths );
				WRITE_SHORT( 0 );
				WRITE_SHORT( GetTeamIndex( plr->m_szTeamName ) + 1 );
			MESSAGE_END();
		}
	}

	if( g_fGameOver )
	{
		MESSAGE_BEGIN( MSG_ONE, SVC_INTERMISSION, NULL, pl->edict() );
		MESSAGE_END();
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::ClientDisconnected( edict_t *pClient )
{
	if( pClient )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

		if( pPlayer )
		{
			FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

			// team match?
			if( g_teamplay )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" ) );
			}
			else
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%i>\" disconnected\n",  
					STRING( pPlayer->pev->netname ), 
					GETPLAYERUSERID( pPlayer->edict() ),
					GETPLAYERAUTHID( pPlayer->edict() ),
					GETPLAYERUSERID( pPlayer->edict() ) );
			}

			pPlayer->RemoveAllItems( TRUE );// destroy all of the players weapons and items
		}
	}
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	int iFallDamage = (int)falldamage.value;

	switch( iFallDamage )
	{
	case 1:
		//progressive
		pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
		break;
	default:
	case 0:
		// fixed
		return 10;
		break;
	}
} 

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerThink( CBasePlayer *pPlayer )
{
	if( g_fGameOver )
	{
		// check for button presses
		if( pPlayer->m_afButtonPressed & ( IN_DUCK | IN_ATTACK | IN_ATTACK2 | IN_USE | IN_JUMP ) )
			m_iEndIntermissionButtonHit = TRUE;

		// clear attack/use commands from player
		pPlayer->m_afButtonPressed = 0;
		pPlayer->pev->button = 0;
		pPlayer->m_afButtonReleased = 0;
	}
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;
	int 		iOldAutoWepSwitch;

	iOldAutoWepSwitch = pPlayer->m_iAutoWepSwitch;

	pPlayer->m_iAutoWepSwitch = 1;
	pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );

	addDefault = TRUE;

	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if( addDefault )
	{
		//Atomizer
		pPlayer->GiveNamedItem( "weapon_knife" );
		pPlayer->GiveNamedItem( "weapon_usp" );
		pPlayer->GiveAmmo( 60, "45ACP", _45ACP_MAXCARRY );//Haunter
		if ( pPlayer->m_iMoney > CVAR_GET_FLOAT("mp_startmoney") || 
			pPlayer->m_iMoney < CVAR_GET_FLOAT("mp_startmoney") )
		{
			pPlayer->m_iMoney = pPlayer->m_iMoney;
		}
		else
		{
			pPlayer->m_iMoney = CVAR_GET_FLOAT("mp_startmoney"); //Haunter
		}
		
		//Atom
	}

	pPlayer->m_iAutoWepSwitch = iOldAutoWepSwitch;
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

BOOL CHalfLifeMultiplay::AllowAutoTargetCrosshair( void )
{
	return ( aimcrosshair.value != 0 );
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CHalfLifeMultiplay::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	//Haunter
	pAttacker->m_iMoney += 1500;
	//Haunter
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CHalfLifeMultiplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if( ktmp && (ktmp->Classify() == CLASS_PLAYER ) )
		peKiller = (CBasePlayer*)ktmp;
	else if( ktmp && ktmp->Classify() == CLASS_VEHICLE )
	{
		CBasePlayer *pDriver = (CBasePlayer *)( (CFuncVehicle *)ktmp )->m_pDriver;

		if( pDriver != NULL )
		{
			pKiller = pDriver->pev;
			peKiller = (CBasePlayer *)pDriver;
		}
	}

	DeathNotice( pVictim, pKiller, pInflictor );

	pVictim->m_iDeaths += 1;

	FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

	if( pVictim->pev == pKiller )
	{
		// killed self
		pKiller->frags -= 1;
	}
	else if( ktmp && ktmp->IsPlayer() )
	{
		// if a player dies in a deathmatch game and the killer is a client, award the killer some points
		pKiller->frags += IPointsForKill( peKiller, pVictim );

		FireTargets( "game_playerkill", ktmp, ktmp, USE_TOGGLE, 0 );
	}
	else
	{
		// killed by the world
		pKiller->frags -= 1;
	}

	// update the scores
	// killed scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pVictim->edict()) );
		WRITE_SHORT( (int)pVictim->pev->frags );
		WRITE_SHORT( pVictim->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pVictim->m_szTeamName ) + 1 );
	MESSAGE_END();

	// killers score, if it's a player
	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
			WRITE_BYTE( ENTINDEX( PK->edict() ) );
			WRITE_SHORT( (int)PK->pev->frags );
			WRITE_SHORT( PK->m_iDeaths );
			WRITE_SHORT( 0 );
			WRITE_SHORT( GetTeamIndex( PK->m_szTeamName ) + 1 );
		MESSAGE_END();

		// let the killer paint another decal as soon as he'd like.
		PK->m_flNextDecalTime = gpGlobals->time;
	}
#ifndef HLDEMO_BUILD
	//Haunter
	if( pVictim->HasNamedPlayerItem( "weapon_c4" ) )
	{
		DeactivateC4( pVictim );
	}
	//Haunter
#endif
}

//=========================================================
// Deathnotice.
//=========================================================
void CHalfLifeMultiplay::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pevInflictor )
{
	// Work out what killed the player, and send a message to all clients about it
	CBaseEntity::Instance( pKiller );

	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_index = 0;

	// Hack to fix name change
	const char *tau = "tau_cannon";
	const char *gluon = "gluon gun";

	if( pevInflictor )
	{
		if( pKiller->flags & FL_CLIENT )
		{
			killer_index = ENTINDEX( ENT( pKiller ) );

			if( pevInflictor == pKiller )
			{
				// If the inflictor is the killer,  then it must be their current weapon doing the damage
				CBasePlayer *pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );

				if( pPlayer->m_pActiveItem )
				{
					killer_weapon_name = pPlayer->m_pActiveItem->pszName();
				}
			}
			else
			{
				killer_weapon_name = STRING( pevInflictor->classname );  // it's just that easy
			}
		}
		else
		{
			killer_weapon_name = STRING( pevInflictor->classname );
		}
	}

	// strip the monster_* or weapon_* from the inflictor's classname
	if( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		killer_weapon_name += 7;
	else if( strncmp( killer_weapon_name, "monster_", 8 ) == 0 )
		killer_weapon_name += 8;
	else if( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		killer_weapon_name += 5;

	MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
		WRITE_BYTE( killer_index );						// the killer
		WRITE_BYTE( ENTINDEX( pVictim->edict() ) );		// the victim
		WRITE_STRING( killer_weapon_name );		// what they were killed by (should this be a string?)
	MESSAGE_END();

	if( pVictim->pev == pKiller )
	{
		// killed self

		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" committed suicide with \"%s\"\n",  
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}
	else if( pKiller->flags & FL_CLIENT )
	{
		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERAUTHID( ENT(pKiller) ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( ENT(pKiller) ), "model" ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" killed \"%s<%i><%s><%i>\" with \"%s\"\n",  
				STRING( pKiller->netname ),
				GETPLAYERUSERID( ENT(pKiller) ),
				GETPLAYERAUTHID( ENT(pKiller) ),
				GETPLAYERUSERID( ENT(pKiller) ),
				STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ),
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );
		}
	}
	else
	{ 
		// killed by the world

		// team match?
		if( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERAUTHID( pVictim->edict() ),
				g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pVictim->edict() ), "model" ),
				killer_weapon_name );		
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" committed suicide with \"%s\" (world)\n",
				STRING( pVictim->pev->netname ), 
				GETPLAYERUSERID( pVictim->edict() ), 
				GETPLAYERAUTHID( pVictim->edict() ),
				GETPLAYERUSERID( pVictim->edict() ),
				killer_weapon_name );		
		}
	}

	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );	// command length in bytes
		WRITE_BYTE( DRC_CMD_EVENT );	// player killed
		WRITE_SHORT( ENTINDEX( pVictim->edict() ) );	// index number of primary entity
		if( pevInflictor )
			WRITE_SHORT( ENTINDEX( ENT( pevInflictor ) ) );	// index number of secondary entity
		else
			WRITE_SHORT( ENTINDEX( ENT( pKiller ) ) );	// index number of secondary entity
		WRITE_LONG( 7 | DRC_FLAG_DRAMATIC );   // eventflags (priority and flags)
	MESSAGE_END();

//  Print a standard message
	// TODO: make this go direct to console
	return; // just remove for now
/*
	char szText[128];

	if( pKiller->flags & FL_MONSTER )
	{
		// killed by a monster
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " was killed by a monster.\n" );
		return;
	}

	if( pKiller == pVictim->pev )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " commited suicide.\n" );
	}
	else if( pKiller->flags & FL_CLIENT )
	{
		strcpy( szText, STRING( pKiller->netname ) );

		strcat( szText, " : " );
		strcat( szText, killer_weapon_name );
		strcat( szText, " : " );

		strcat( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, "\n" );
	}
	else if( FClassnameIs( pKiller, "worldspawn" ) )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " fell or drowned or something.\n" );
	}
	else if( pKiller->solid == SOLID_BSP )
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " was mooshed.\n" );
	}
	else
	{
		strcpy( szText, STRING( pVictim->pev->netname ) );
		strcat( szText, " died mysteriously.\n" );
	}

	UTIL_ClientPrintAll( szText );
*/
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CHalfLifeMultiplay::PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHalfLifeMultiplay::FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	if( weaponstay.value > 0 )
	{
		// make sure it's only certain weapons
		if( !(pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
		{
			return gpGlobals->time + 0;		// weapon respawns almost instantly
		}
	}

	return gpGlobals->time + WEAPON_RESPAWN_TIME;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHalfLifeMultiplay::FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	if( pWeapon && pWeapon->m_iId && ( pWeapon->iFlags() & ITEM_FLAG_LIMITINWORLD ) )
	{
		if( NUMBER_OF_ENTITIES() < ( gpGlobals->maxEntities - ENTITY_INTOLERANCE ) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}

	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHalfLifeMultiplay::WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	if( pWeapon->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	return GR_WEAPON_RESPAWN_YES;
}

//=========================================================
// CanHaveWeapon - returns FALSE if the player is not allowed
// to pick up this weapon
//=========================================================
BOOL CHalfLifeMultiplay::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pItem )
{
	if( weaponstay.value > 0 )
	{
		if( pItem->iFlags() & ITEM_FLAG_LIMITINWORLD )
			return CGameRules::CanHavePlayerItem( pPlayer, pItem );

		// check if the player already has this weapon
		for( int i = 0; i < MAX_ITEM_TYPES; i++ )
		{
			CBasePlayerItem *it = pPlayer->m_rgpPlayerItems[i];

			while( it != NULL )
			{
				if( it->m_iId == pItem->m_iId )
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
BOOL CHalfLifeMultiplay::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::ItemShouldRespawn( CItem *pItem )
{
	if( pItem->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_ITEM_RESPAWN_NO;
	}

	return GR_ITEM_RESPAWN_YES;
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHalfLifeMultiplay::FlItemRespawnTime( CItem *pItem )
{
	return gpGlobals->time + ITEM_RESPAWN_TIME;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHalfLifeMultiplay::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
void CHalfLifeMultiplay::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::IsAllowedToSpawn( CBaseEntity *pEntity )
{
//	if( pEntity->pev->flags & FL_MONSTER )
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	if( pAmmo->pev->spawnflags & SF_NORESPAWN )
	{
		return GR_AMMO_RESPAWN_NO;
	}

	return GR_AMMO_RESPAWN_YES;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return gpGlobals->time + AMMO_RESPAWN_TIME;
}

//=========================================================
//=========================================================
Vector CHalfLifeMultiplay::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CHalfLifeMultiplay::FlHealthChargerRechargeTime( void )
{
	return 60;
}

float CHalfLifeMultiplay::FlHEVChargerRechargeTime( void )
{
	return 30;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_ACTIVE;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_ACTIVE;
}

edict_t *CHalfLifeMultiplay::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = CGameRules::GetPlayerSpawnSpot( pPlayer );	
	if( IsMultiplayer() && pentSpawnSpot->v.target )
	{
		FireTargets( STRING( pentSpawnSpot->v.target ), pPlayer, pPlayer, USE_TOGGLE, 0 );
	}

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
int CHalfLifeMultiplay::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life deathmatch has only enemies
	return GR_NOTTEAMMATE;
}

BOOL CHalfLifeMultiplay::PlayFootstepSounds( CBasePlayer *pl, float fvol )
{
	if( g_footsteps && g_footsteps->value == 0 )
		return FALSE;

	if( pl->IsOnLadder() || pl->pev->velocity.Length2D() > 220 )
		return TRUE;  // only make step sounds in multiplayer if the player is moving fast enough

	return FALSE;
}

BOOL CHalfLifeMultiplay::FAllowFlashlight( void )
{
	return flashlight.value != 0; 
}

//=========================================================
//=========================================================
BOOL CHalfLifeMultiplay::FAllowMonsters( void )
{
	return ( allowmonsters.value != 0 );
}

//=========================================================
//======== CHalfLifeMultiplay private functions ===========
#define INTERMISSION_TIME		6

void CHalfLifeMultiplay::GoToIntermission( void )
{
	if( g_fGameOver )
		return;  // intermission has already been triggered, so ignore.

	MESSAGE_BEGIN( MSG_ALL, SVC_INTERMISSION );
	MESSAGE_END();

	// bounds check
	int time = (int)CVAR_GET_FLOAT( "mp_chattime" );
	if( time < 1 )
		CVAR_SET_STRING( "mp_chattime", "1" );
	else if( time > MAX_INTERMISSION_TIME )
		CVAR_SET_STRING( "mp_chattime", UTIL_dtos1( MAX_INTERMISSION_TIME ) );

	m_flIntermissionEndTime = gpGlobals->time + ( (int)mp_chattime.value );
	g_flIntermissionStartTime = gpGlobals->time;

	g_fGameOver = TRUE;
	m_iEndIntermissionButtonHit = FALSE;
}

#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[32];
	int minplayers, maxplayers;
	char rulebuffer[MAX_RULE_BUFFER];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void DestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if( p )
	{
		start = p;
		p = p->next;
		while( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[1500];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
const char *COM_Parse( const char *data )
{
	int c;
	int len;

	len = 0;
	com_token[0] = 0;

	if( !data )
		return NULL;

// skip whitespace
skipwhite:
	while( ( c = *data ) <= ' ')
	{
		if( c == 0 )
			return NULL;                    // end of file;
		data++;
	}

	// skip // comments
	if( c=='/' && data[1] == '/' )
	{
		while( *data && *data != '\n' )
			data++;
		goto skipwhite;
	}

	// handle quoted strings specially
	if( c == '\"' )
	{
		data++;
		while( 1 )
		{
			c = *data++;
			if( c=='\"' || !c )
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	// parse single characters
	if( c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if( c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while ( c > 32 );

	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int COM_TokenWaiting( const char *buffer )
{
	const char *p;

	p = buffer;
	while( *p && *p!='\n')
	{
		if( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}

/*
==============
ReloadMapCycleFile

Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int ReloadMapCycleFile( const char *filename, mapcycle_t *cycle )
{
	char szMap[32];
	int length;
	const char *pFileList;
	const char *aFileList = pFileList = (const char *)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if( pFileList && length )
	{
		// the first map name in the file becomes the default
		while( 1 )
		{
			char szBuffer[MAX_RULE_BUFFER] = {0};
			hasbuffer = 0;

			pFileList = COM_Parse( pFileList );

			if( com_token[0] == '\0' )
				break;

			strlcpy( szMap, com_token, sizeof( szMap ));

			// Any more tokens on this line?
			if( COM_TokenWaiting( pFileList ) )
			{
				pFileList = COM_Parse( pFileList );

				if( com_token[0] != '\0' )
				{
					hasbuffer = 1;
					strlcpy( szBuffer, com_token, sizeof( szBuffer ));
				}
			}

			// Check map
			if( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = Q_max( item->minplayers, 0 );
						item->minplayers = Q_min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = Q_max( item->maxplayers, 0 );
						item->maxplayers = Q_min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}
		}

		FREE_FILE( (void*)aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if( !item )
	{
		return 0;
	}

	while( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;

	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int CountPlayers( void )
{
	int num = 0;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void ExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char pkey[512];
	char value[512];	// use two buffers so compares
				// work without stomping on each other
	char *o;
	
	if( *s == '\\' )
		s++;

	while( 1 )
	{
		o = pkey;
		while( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while( *s != '\\' && *s )
		{
			if( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );

		if( value[0] != '\0' )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if( !*s )
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void CHalfLifeMultiplay::ChangeLevel( void )
{
	static char szPreviousMapCycleFile[256];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[1500];
	char szRules[1500];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	const char *mapcfile = CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[0] = '\0';
	szRules[0] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		if( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING( gpGlobals->mapname ) );
		strcpy( szFirstMapInList, STRING( gpGlobals->mapname ) );

		// Traverse list
		for( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if( item->minplayers != 0 )
			{
				if( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if( item->maxplayers != 0 )
			{
				if( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if( !found )
		{
			item = mapcycle.next_item;
		}		

		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		ExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if( !IS_MAP_VALID( szNextMap ) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}

	if( szRules[0] != '\0' )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}

	CHANGE_LEVEL( szNextMap, NULL );

	if( szCommands[0] != '\0' )
	{
		SERVER_COMMAND( szCommands );
	}
}

#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)

void CHalfLifeMultiplay::SendMOTDToClient( edict_t *client )
{
	// read from the MOTD.txt file
	int length, char_count = 0;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( CVAR_GET_STRING( "motdfile" ), &length );

	// send the server name
	MESSAGE_BEGIN( MSG_ONE, gmsgServerName, NULL, client );
		WRITE_STRING( CVAR_GET_STRING( "hostname" ) );
	MESSAGE_END();

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		size_t size = strlcpy( chunk, pFileList, MAX_MOTD_CHUNK + 1 );
		char_count += Q_min( size, MAX_MOTD_CHUNK );
		if( char_count < MAX_MOTD_LENGTH )
			pFileList = aFileList + char_count; 
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, client );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE( (void*)aFileList );
}

