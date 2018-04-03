// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"game.h"
#include	"BMOD_constants.h"

#define MAX_RULE_BUFFER 1024

extern DLL_GLOBAL	int		g_VoteStatus;
extern DLL_GLOBAL	float	g_VoteTimer;

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

extern DLL_GLOBAL BOOL	g_fGameOver;

extern int gmsgSpectator;
extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );
extern int CountPlayers( void );
extern void DestroyMapCycle( mapcycle_t *cycle );
extern int ReloadMapCycleFile( const char *filename, mapcycle_t *cycle );
extern void ExtractCommandString( char *s, char *szCommand );

extern cvar_t	bm_guns;
extern cvar_t	bm_ammo;
extern cvar_t	bm_g;
extern cvar_t	bm_cbar_mod;
extern cvar_t	bm_mp5_mod;
extern cvar_t	bm_shotty_mod;
extern cvar_t	bm_xbow_mod;
extern cvar_t	bm_rpg_mod;
extern cvar_t	bm_tau_mod;
extern cvar_t	bm_gluon_mod;
extern cvar_t	bm_hornet_mod;
extern cvar_t	bm_trip_mod;
extern cvar_t	bm_snarks_mod;
extern cvar_t	bm_spawneffects;
/*
extern cvar_t	bm_score_crowbar;
extern cvar_t	bm_score_throwncbar;
extern cvar_t	bm_score_9mm;
extern cvar_t	bm_score_357;
extern cvar_t	bm_score_mp5;
extern cvar_t	bm_score_shotgun;
extern cvar_t	bm_score_squidspit;
extern cvar_t	bm_score_zapgun;
extern cvar_t	bm_score_mp5grenade;
extern cvar_t	bm_score_gluon;
extern cvar_t	bm_score_tau;
extern cvar_t	bm_score_bolt;
extern cvar_t	bm_score_crossbow;
extern cvar_t	bm_score_satchel;
extern cvar_t	bm_score_handgrenade;
extern cvar_t	bm_score_rpg;
extern cvar_t	bm_score_snarks;
extern cvar_t	bm_score_tripmine;

int CGameRules :: BMOD_IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled, entvars_t *pInflictor) 
{
	if (g_pGameRules->IsTeamplay())
	{
		if ( !pKilled )
			return 0;

		if ( !pAttacker )
			return 1;

		if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
			return -1;
	}

	if (!strcmp(STRING(pInflictor->classname), "weapon_crowbar"))
		return bm_score_crowbar.value;
	else if (!strcmp(STRING(pInflictor->classname), "flying_crowbar"))
		return bm_score_throwncbar.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_9mmhandgun"))
		return bm_score_9mm.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_357"))
		return bm_score_357.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_9mmAR"))
		return bm_score_mp5.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_shotgun"))
		return bm_score_shotgun.value;
	else if (!strcmp(STRING(pInflictor->classname), "squidspit"))
		return bm_score_squidspit.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_hornetgun"))
		return bm_score_zapgun.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_egon"))
		return bm_score_gluon.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_gauss"))
		return bm_score_tau.value;
	else if (!strcmp(STRING(pInflictor->classname), "bolt"))
		return bm_score_bolt.value;
	else if (!strcmp(STRING(pInflictor->classname), "weapon_crossbow"))
		return bm_score_crossbow.value;
	else if (!strcmp(STRING(pInflictor->classname), "monster_satchel"))
		return bm_score_satchel.value;
	else if (!strcmp(STRING(pInflictor->classname), "hand_grenade"))
		return bm_score_handgrenade.value;
	else if (!strcmp(STRING(pInflictor->classname), "rpg_rocket"))
		return bm_score_rpg.value;
	else if (!strcmp(STRING(pInflictor->classname), "monster_snark"))
		return bm_score_snarks.value;
	else
		return 1;
}
*/
void CHalfLifeMultiplay :: BMOD_ClientDisconnected(edict_t *pClient, CBasePlayer *pPlayer)
{
	// Disconnecting observers
	//MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
	//	WRITE_BYTE( ENTINDEX(pClient) );
	//	WRITE_BYTE( 0 );
	//MESSAGE_END();

	CBasePlayer *client = NULL;
	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		if ( !client->pev )
			continue;
		if ( client == pPlayer )
			continue;

		// If a spectator was chasing this player, move him/her onto the next player
		if ( client->m_hObserverTarget == pPlayer )
		{
			int iMode = client->pev->iuser1;
			client->pev->iuser1 = 0;
			client->m_hObserverTarget = NULL;
			client->Observer_SetMode( iMode );
		}
	}

	// Destroy all satchels, trips, and snark mines owned by this player. 
	DeactivateSatchels( pPlayer );
	DeactivateTrips( pPlayer );
	DeactivateSnarkTrips( pPlayer );

	pPlayer->m_bIsConnected = FALSE;
	UTIL_SaveRestorePlayer(pPlayer, 1, 0);
}

void CHalfLifeMultiplay :: BMOD_PlayerSpawn( CBasePlayer *pPlayer )
{
	// Send new players into observer mode.
	if (!pPlayer->m_iFirstSpawn)
	{
		pPlayer->m_iFirstSpawn = TRUE;
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
		pPlayer->StartObserver( VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);
		pPlayer->m_iSpawnKills = 0;
		pPlayer->m_iTypeKills = 0;
		pPlayer->m_bBanMe = FALSE;
		UTIL_SaveRestorePlayer(pPlayer, 0, 0);
	}

	// Else stuff for a new player.
	else if (bm_spawneffects.value) {
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_TELEPORT ); //This denotes which temporary intity we want to use
		WRITE_COORD(pPlayer->pev->origin.x);// Location-X
		WRITE_COORD(pPlayer->pev->origin.y);// Location-Y
		WRITE_COORD(pPlayer->pev->origin.z);// Location-Z
		MESSAGE_END();

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_BODY, "debris/beamstart8.wav", 1.0, ATTN_NORM);

	}
}

void CHalfLifeMultiplay :: BMOD_InitHUD( CBasePlayer *pl )
{
	pl->m_bIsConnected = TRUE;
	pl->m_iKillsThisFrame = 0;
	pl->m_iSpamSay = 0;
	pl->m_LocateMode = FALSE;
	pl->m_LeetSpeak = FALSE;
	// pl->m_iHLHack = 0;
}

void CGameRules :: BMOD_PreChangeLevel( void )
{
	return;
}

void CHalfLifeMultiplay :: BMOD_PreChangeLevel( void )
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "hldm1" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = CountPlayers();

	// Has the map cycle filename changed?
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		DestroyMapCycle( &mapcycle );

		if ( !ReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING(gpGlobals->mapname) );
		strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
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

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	CVAR_SET_STRING("bm_map", STRING(gpGlobals->mapname) );
	CVAR_SET_STRING("bm_nextmap", szNextMap );
}

// Bmod version of Changelevel
void CHalfLifeMultiplay :: ChangeLevel( void )
{
	char szNextMap[32];

	// Confirm what map we need.
	// BMOD_PreChangeLevel();

	strcpy( szNextMap, (char*)CVAR_GET_STRING( "bm_nextmap" ) );

	g_fGameOver = TRUE;

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	
	CHANGE_LEVEL( szNextMap, NULL );
}

void CHalfLifeMultiplay :: BMOD_Think ( void )
{
	static float setVarsTime = 0;

	if (setVarsTime < gpGlobals->time ) 
	{
		g_engfuncs.pfnCvar_DirectSet( &bm_ver, UTIL_VarArgs( "%s", BMOD_VERSION ) );
		g_engfuncs.pfnCvar_DirectSet( &bm_bname, UTIL_VarArgs( "%s", BMOD_BRANCH_NAME ) );
		g_engfuncs.pfnCvar_DirectSet( &bm_bver, UTIL_VarArgs( "%s", BMOD_BRANCH_VERSION ) );
		g_engfuncs.pfnCvar_DirectSet( &bm_burl, UTIL_VarArgs( "%s", BMOD_BRANCH_URL ) );
		
		BMOD_UpdateGuns();
		BMOD_UpdateMods();
	    //UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> updated at %f", gpGlobals->time));

		setVarsTime = gpGlobals->time + 1;
	}

	// BMod voting
	if ((g_VoteStatus == 1) && (g_VoteTimer < gpGlobals->time))
	{
		int players = CountPlayers();
		int votesNeeded = (players * 3 / 4) + ((players < 3) ? 1 : 0);

		char winner[81];
		strcpy(winner, UTIL_CountVotes());
		int winvotes = winner[0];
		char map[81];
		strcpy(map, &winner[1]);

		UTIL_SpeakAll("dadeda");
		if (winvotes >= votesNeeded) {
			UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> *** We have a winner! ***\n");
		}
		else {
			UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> *** Voting is closed! ***\n");
		}

		/*if (winvotes)
			UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> \"%s\" got %i vote(s). (needed %i to win with %i players)\n",
				&(winner[1]),
				winvotes,
				votesNeeded,
				players ) );*/

		if (winvotes == 0)
			UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> No votes were cast!\n");

		if (winvotes >= votesNeeded)
		{
			if (!strcmp(map, "extend")) {
				UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> Extend won the vote! This map will be extended 30 mins & 25 kills.\n");
				int newTime = CVAR_GET_FLOAT("mp_timelimit") + 30;
				int newFrags = CVAR_GET_FLOAT("mp_fraglimit") + 25;
				CVAR_SET_FLOAT( "mp_timelimit", (newTime > bm_maxtime.value) ? bm_maxtime.value : newTime);
				CVAR_SET_FLOAT( "mp_fraglimit", (newFrags > bm_maxfrags.value) ? bm_maxfrags.value : newFrags);
				g_VoteStatus = 0;
			}
			else {
				UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> \"%s\" Won the vote! Changing map... please wait.\n",
					&(winner[1]) 
					));
				CVAR_SET_STRING("bm_nextmap", &(winner[1]));
				g_VoteStatus = 2;
				g_VoteTimer = gpGlobals->time + 10;
			}
			UTIL_LogPrintf( "// Map \"%s\" won the map vote.\n", map);
		}
		else
		{
			if (winvotes)
				UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> \"%s\" got %i vote(s). (needed %i to win with %i players)\n",
					&(winner[1]),
					winvotes,
					votesNeeded,
					players ) );
			UTIL_LogPrintf( "// Map vote ended.\n", map);
			g_VoteStatus = 0;
		}
	}
	if ((g_VoteStatus == 2) && (g_VoteTimer < gpGlobals->time))
	{
		g_VoteStatus = 0;
		CVAR_SET_FLOAT( "mp_timelimit", 1); 
	}
	// BMod
}

void CHalfLifeMultiplay :: BMOD_GiveGunsAndAmmo ( CBasePlayer *pPlayer )
{
	char	*pName;
	char	itemlist[1025];
	char	szTemp[80];
	int		iszItem;

	// Give ammo
	// make a copy because strtok is destructive
	strncpy( itemlist, bm_ammo.string, 1024 );
	pName = itemlist;
	pName = strtok( pName, ";" );
	while ( pName != NULL && *pName )
	{
		strcpy( szTemp, "ammo_" );
		strcat( szTemp, pName );
		iszItem = ALLOC_STRING( szTemp );
		pPlayer->GiveNamedItem( STRING(iszItem) );
		pName = strtok( NULL, ";" );
	}

	// Give guns
	// make a copy because strtok is destructive
	strncpy( itemlist, bm_guns.string, 1024 );
	pName = itemlist;
	pName = strtok( pName, ";" );
	int guns = 0;
	while ( pName != NULL && *pName )
	{
		strcpy( szTemp, "weapon_" );
		strcat( szTemp, pName );

		if (!strcmp(szTemp, "weapon_glock"))
		{
			strcpy( szTemp, "weapon_9mmhandgun" );
		}
		else if (!strcmp(szTemp, "weapon_mp5"))
		{
			strcpy( szTemp, "weapon_9mmAR" );
		}
		else if (!strcmp(szTemp, "weapon_python"))
		{
			strcpy( szTemp, "weapon_357" );
		}

		iszItem = ALLOC_STRING( szTemp );
		pPlayer->GiveNamedItem( STRING(iszItem) );
		pPlayer->SelectItem( STRING(iszItem) );
		pName = strtok( NULL, ";" );
	}
}

void CHalfLifeMultiplay :: BMOD_UpdateGuns ( void )
{
	char	*pName;
	char	itemlist[1024];

	int		guns = 0;

	// guns
	// make a copy because strtok is destructive
	strncpy( itemlist, bm_guns.string, 1024 );
	pName = itemlist;
	pName = strtok( pName, ";" );
	while ( pName != NULL && *pName )
	{
		if (!strcmp(pName, "crowbar")) 
		{
			guns = guns | (1 << 0);
		}
		else if (!strcmp(pName, "9mmhandgun") || !strcmp(pName, "glock")) 
		{
			guns = guns | (1 << 1);
		}
		else if (!strcmp(pName, "357") || !strcmp(pName, "python")) 
		{
			guns = guns | (1 << 2);
		}
		else if (!strcmp(pName, "9mmAR") || !strcmp(pName, "mp5")) 
		{
			guns = guns | (1 << 3);
		}
		else if (!strcmp(pName, "shotgun")) 
		{
			guns = guns | (1 << 4);
		}
		else if (!strcmp(pName, "egon")) 
		{
			guns = guns | (1 << 5);
		}
		else if (!strcmp(pName, "gauss")) 
		{
			guns = guns | (1 << 6);
		}
		else if (!strcmp(pName, "rpg")) 
		{
			guns = guns | (1 << 7);
		}
		else if (!strcmp(pName, "hornetgun")) 
		{
			guns = guns | (1 << 8);
		}
		else if (!strcmp(pName, "tripmine")) 
		{
			guns = guns | (1 << 9);
		}
		else if (!strcmp(pName, "satchel")) 
		{
			guns = guns | (1 << 10);
		}
		else if (!strcmp(pName, "handgrenade")) 
		{
			guns = guns | (1 << 11);
		}
		else if (!strcmp(pName, "snark")) 
		{
			guns = guns | (1 << 12);
		}
		else if (!strcmp(pName, "crossbow")) 
		{
			guns = guns | (1 << 13);
		}
		
		pName = strtok( NULL, ";" );
	}
		g_engfuncs.pfnCvar_DirectSet( &bm_g, UTIL_VarArgs( "%d", guns ) );
}

void CHalfLifeMultiplay :: BMOD_UpdateMods ( void )
{

	int		mods = 0;

	// mods
	// bm_cbar_mod
 	// bm_mp5_mod
 	// bm_shotty_mod
 	// bm_xbow_mod
 	// bm_rpg_mod
 	// bm_tau_mod
 	// bm_gluon_mod
 	// bm_hornet_mod
 	// bm_trip_mod
 	// bm_snarks_mod

	if (bm_cbar_mod.value) 
	{
		mods = mods | (1 << 0);
	}
	
	/*
	if (!strcmp(pName, "9mmhandgun") || !strcmp(pName, "glock")) 
	{
		mods = mods | (1 << 1);
	}
	*/

	/*
	if (!strcmp(pName, "357") || !strcmp(pName, "python")) 
	{
		mods = mods | (1 << 2);
	}
	*/

	if (bm_mp5_mod.value) 
	{
		mods = mods | (1 << 3);
	}

	if (bm_shotty_mod.value) 
	{
		mods = mods | (1 << 4);
	}

	if (bm_gluon_mod.value) 
	{
		mods = mods | (1 << 5);
	}

	if (bm_tau_mod.value) 
	{
		mods = mods | (1 << 6);
	}

	if (bm_rpg_mod.value) 
	{
		mods = mods | (1 << 7);
	}

	if (bm_hornet_mod.value) 
	{
		mods = mods | (1 << 8);
	}

	if (bm_trip_mod.value) 
	{
		mods = mods | (1 << 9);
	}

	/*
	if (!strcmp(pName, "satchel")) 
	{
		mods = mods | (1 << 10);
	}
	*/

	/*
	if (!strcmp(pName, "handgrenade")) 
	{
		mods = mods | (1 << 11);
	}
	*/

	if (bm_snarks_mod.value) 
	{
		mods = mods | (1 << 12);
	}

	if (bm_xbow_mod.value) 
	{
		mods = mods | (1 << 13);
	}
	

	g_engfuncs.pfnCvar_DirectSet( &bm_mods, UTIL_VarArgs( "%d", mods ) );
}
