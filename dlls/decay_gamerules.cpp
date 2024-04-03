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
// decay_gamerules.cpp
// Author: Vyacheslav Dzhura ( slava.dzhura@gmail.com ; slava.dzhura@protonmail.com )
// (C) 2008
//
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"items.h"
#include	"client.h"
#include	"bot.h"
#include	"triggers.h"

extern DLL_GLOBAL CGameRules	*g_pGameRules;
extern DLL_GLOBAL BOOL	g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgScoreInfo;
extern int gmsgMOTD;
extern int gmsgChangePlayer;
extern int gmsgGameMode;
extern int gmsgSparePlayer;
extern int gmsgUpdateDecayPlayerName;
extern respawn_t bot_respawn[32];
extern bool bSlaveCoop;

bool bStatsLoaded = false;
int	m_iMagicWord1 = 159123512;
int m_iMagicWord2 = 241245;
byte m_bMagicShift = 23;

struct mapEntry       //structure definition
{ 
     char szName[128];
     int m_iId;
     int m_iNextId;
	 bool m_bLocked;
	 t_playerStats statsBest1;
	 t_playerStats statsLast1;
	 t_playerStats statsBest2;
	 t_playerStats statsLast2;
} ;

static t_playerStats blankStats = 
{
	0, // kills;
	0, // int damage;
	0, // int shots;
	0, // int hits;
	0, // int lastShotId;
	m_iMagicWord1, 
	0, // lastShotCounted;
	m_iMagicWord2,
	0, // float accuracy;
	3, // int gradeKills;
	3, // int gradeDamage;
	3, // int gradeAccuracy;
	3 // int gradeFinal;
};

mapEntry decayMaps[] = {
	{ "dy_accident1", 1, 2, false, blankStats, blankStats, blankStats, blankStats },
	{ "dy_accident2", 2, 3, false, blankStats, blankStats, blankStats, blankStats },
	{ "dy_hazard", 3, 4, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_uplink", 4, 5, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_dampen", 5, 6, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_dorms", 6, 7, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_signal", 7, 8, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_focus", 8, 9, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_lasers", 9, 10, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_fubar", 10, 11, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_outro", 11, -1, true, blankStats, blankStats, blankStats, blankStats },
	{ "dy_alien", 12, -1, true, blankStats, blankStats, blankStats, blankStats }
};

inline char *GET_INFOBUFFER( edict_t *e )
{
   return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
   (*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}

int getMapEntryId( int desiredId )
{
    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);
	for ( int i = 0; i < mapCount; i++ )
	{
		if ( decayMaps[i].m_iId == desiredId )
			return i;
	}
	return -1;
}

int findMapId( static char* szMapName )
{
    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);

	int mapId = -1;

	//char szMapName[ 128 ];
	//strcpy( szMapName, szMapName );

	for ( int i = 0; i < mapCount; i++ )
	{
		if ( strcmp( szMapName, decayMaps[i].szName ) == 0 )
		{
			mapId = decayMaps[i].m_iId;
			break;
		}
	}

	return mapId;
}

bool canUnlockBonusMission()
{
    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);
	for ( int i = 0; i < mapCount; i++ )
	{
		if (( decayMaps[i].statsBest1.gradeFinal != 0 )  || ( decayMaps[i].statsBest2.gradeFinal != 0 ))
			return false;
	}
	return true;
}

char *CDecayRules::getDecayMapName( int mapId )
{
	static char szMap[128];
	sprintf( szMap, "null" );

	int entryId = getMapEntryId( mapId );
	if ( entryId == -1 )
		return szMap;

	sprintf( szMap, decayMaps[entryId].szName );
	return szMap;
}

void CDecayRules::SetAlienMode( bool bMode )
{
	this->m_bAlienMode = bMode;
//	ALERT( at_console, "Decay (alien mode = %d)\n", bSlaveCoop);
}

//=========================================================
//=========================================================
CDecayRules::CDecayRules( void )
{
	char buffer[MAX_COMPUTERNAME_LENGTH+1];
	DWORD size;
	size=sizeof(buffer);
	GetComputerName(buffer,&size);

	m_iMagicWord1 = 0;

	int cnl = strlen(buffer);
	for (int i=1; i<cnl; i++)
	{
		m_iMagicWord1 += byte(buffer[i]);
	}
	/*######################*/

	this->m_bAlienMode = false;
	PlayersCount = 0;
	RefreshSkillData();

	statsLoad();

	memset( &this->pStats[0], 0, sizeof t_playerStats );
	memset( &this->pStats[1], 0, sizeof t_playerStats );
	memset( &this->pStats[2], 0, sizeof t_playerStats );
	memset( &this->pStats[3], 0, sizeof t_playerStats );	

	int curMapId = this->getDecayMapId();
	if ( curMapId != -1 )
	{
		int mapEntryId = getMapEntryId( curMapId );
		if ( decayMaps[mapEntryId].m_bLocked == true )
		{
			ALERT( at_console, "Loading LOCKED MISSION!!!\n" );
		} else
			ALERT( at_console, "Current mission is UNLOCKED!!!\n" );			
	}

	//pLoopBack = NULL;
}

//=========================================================
//=========================================================
void CDecayRules::statsLoad()
{
	byte    *aMemFile;
	byte    *pMemFile;
	int	length;

	char	szDirName[MAX_PATH];
	GET_GAME_DIR( szDirName );
	strcat( szDirName, "/save" );

	char	szFilename[MAX_PATH];
	strcpy ( szFilename, "save/save0.sv2" );

	pMemFile = aMemFile = LOAD_FILE_FOR_ME(szFilename, &length);

	if ( ( !pMemFile ) || ( length == 0 ) )
	{
		ALERT( at_console, "(load game) failed, file is null or does not exists!!!\n" );
		statsSave(); // create blank stats file
		return;
	}

	int iVersion;
	byte bTrash[50];
	bool bHasNext = true;
	bool bInvalid = false;
	
	// read version
	memcpy(&iVersion, pMemFile, sizeof(int));
	pMemFile += sizeof(int);

	// read map entry

	mapEntry m_mapEntry;
	byte bEntryCount = 0;

	while ( bHasNext == true )
	{
		if ( bEntryCount > 12 )
		{
			ALERT( at_console, "(load game) probably infinite loop - stopped reading game save!!!\n" );
			bInvalid = true;
			break;
		}

		memcpy(&bTrash[0], pMemFile, m_bMagicShift );
		pMemFile += m_bMagicShift;

		memcpy(&m_mapEntry, pMemFile, sizeof(mapEntry));
		pMemFile += sizeof(mapEntry);

		if (( m_mapEntry.statsLast1.magicWord1 != m_iMagicWord1 ) || ( m_mapEntry.statsLast1.magicWord2 != m_iMagicWord2 ))
		{
			bInvalid = true; // INVALID FILE!!!
			break;
		}

		int m_iReadMapId = findMapId( m_mapEntry.szName );
		int m_iReadMapEntryId = getMapEntryId( m_iReadMapId );
		if (( m_iReadMapId == -1 ) || ( m_iReadMapEntryId == -1 ))
		{
			bInvalid = true; // INVALID FILE!!!		
			break;
		}

		decayMaps[m_iReadMapEntryId].m_bLocked = m_mapEntry.m_bLocked;
		decayMaps[m_iReadMapEntryId].statsLast1 = m_mapEntry.statsLast1;
		decayMaps[m_iReadMapEntryId].statsBest1 = m_mapEntry.statsBest1;
		decayMaps[m_iReadMapEntryId].statsLast2 = m_mapEntry.statsLast2;
		decayMaps[m_iReadMapEntryId].statsBest2 = m_mapEntry.statsBest2;

		// read next entry
		memcpy(&bHasNext, pMemFile, sizeof(bHasNext));
		pMemFile += sizeof(bHasNext);

		bEntryCount++;
	}

	FREE_FILE(aMemFile);

	if ( bInvalid == true )
		ALERT( at_console, "(load game) failed, file is invalid!!!\n" );
}

void CDecayRules::statsSave()
{
	FILE	*file;
	int	length;

	char	szFilename[MAX_PATH];
	GET_GAME_DIR( szFilename );
	strcat( szFilename, "/save/save0.sv2" );

	file = fopen ( szFilename, "wb" );

	if ( !file )
	{
		ALERT( at_console, "(save game) failed to create!!!\n" );
		return;
	}

	int iVersion = 692008;
	byte bTrash[50];
	bool bHasNext = true;
	bool bInvalid = false;
	
	// write version
	fwrite ( &iVersion, sizeof ( int ), 1, file );

	// read map entry
	mapEntry m_mapEntry;
	byte bEntryCount = 0;

/*********/
		char buffer[MAX_COMPUTERNAME_LENGTH+1];
	DWORD size;
	size=sizeof(buffer);
	GetComputerName(buffer,&size);

	m_iMagicWord1 = 0;

	int cnl = strlen(buffer);
	for (int i=1; i<cnl; i++)
	{
		m_iMagicWord1 += byte(buffer[i]);
	}
/**********/

    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);
	for ( int i = 0; i < mapCount; i++ )
	{
		for (int j = 0; j < m_bMagicShift; j++)
			bTrash[j] = RANDOM_LONG( 0, 255 );
		fwrite( &bTrash[0], sizeof ( byte ), m_bMagicShift, file );

		m_mapEntry = decayMaps[i];
		m_mapEntry.statsLast1.magicWord1 = m_iMagicWord1;
		fwrite( &m_mapEntry, sizeof( mapEntry ), 1, file );

		// read next entry
		bHasNext = true;
		if ( m_mapEntry.m_iId == 12 ) // dy_alien
			bHasNext = false;
		fwrite( &bHasNext, sizeof ( bool ), 1, file );
	}

	fclose( file );

	statsExportXml();

//	if ( bInvalid == true )
//		ALERT( at_console, "(load game) failed, file is invalid!!!\n" );
}

void CDecayRules::unlockMissions( bool unlockAlien )
{
	bool bNotifyUnlock = false;

	int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);
	for ( int i = 0; i < mapCount; i++ )
	{
		if ( decayMaps[i].m_iId == 12 )
		{
			if ( unlockAlien == true )
			{
				decayMaps[i].m_bLocked = false;
				bNotifyUnlock = true;
			}
		} else
			decayMaps[i].m_bLocked = false;
		decayMaps[i].statsBest1.gradeFinal = 0;
		decayMaps[i].statsBest2.gradeFinal = 0;
	}

	if ( bNotifyUnlock == true )
		UTIL_ShowMessageAll( "Alien mission unlocked!!!" );
}

char CDecayRules::getGradeChar( int grade )
{
	char gradeChar;
	switch ( grade ) 
	{
		case 3: 
			gradeChar = 'D';
			break;
		case 2:
			gradeChar = 'C';
			break;
		case 1:
			gradeChar = 'B';
			break;
		case 0:
			gradeChar = 'A';
			break;
		default:
			gradeChar = 'D';
			break;
	}
	return gradeChar;
}

void CDecayRules::printXmlPlayerStats( FILE *fp, t_playerStats playerStats, bool bBest, int playerId )
{
	if ( bBest )
		fprintf( fp, "<best>\n" );
	else
		fprintf( fp, "<last>\n" );

	fprintf( fp, "<id>%d</id>\n", playerId );

	fprintf( fp, "<accuracy>%f</accuracy>\n", playerStats.accuracy );
	fprintf( fp, "<damage>%d</damage>\n", playerStats.damage );
	fprintf( fp, "<hits>%d</hits>\n", playerStats.hits );
	fprintf( fp, "<kills>%d</kills>\n", playerStats.kills );

	fprintf( fp, "<grades>\n" );
	fprintf( fp, "<final>%c</final>\n", getGradeChar( playerStats.gradeFinal ) );
	fprintf( fp, "<accuracy>%c</accuracy>\n", getGradeChar( playerStats.gradeAccuracy ) );
	fprintf( fp, "<damage>%c</damage>\n", getGradeChar( playerStats.gradeDamage ) );
	fprintf( fp, "<kills>%c</kills>\n", getGradeChar( playerStats.gradeKills ) );
	fprintf( fp, "</grades>\n" );

	if ( bBest )
		fprintf( fp, "</best>\n" );
	else
		fprintf( fp, "</last>\n" );
}

void CDecayRules::statsExportXml( void )
{
	char	szFilename[MAX_PATH];
	GET_GAME_DIR( szFilename );
	strcat( szFilename, "/manual" );
	CreateDirectory( szFilename, NULL );
	strcat( szFilename, "/stats.xml" );

	FILE *fp;

	fp = fopen( szFilename, "w" );
	if ( !fp )
		return;

    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);

	fprintf( fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
	fprintf( fp, "<?xml-stylesheet type=\"text/xsl\" href=\"template.xsl\"?>\n" );
	fprintf( fp, "<decaystats>\n" );
	for ( int i = 0; i < mapCount; i++ )
	{
		fprintf( fp, "<map>\n" );

		fprintf( fp, "<id>%d</id>\n", decayMaps[i].m_iId );
		fprintf( fp, "<name>%s</name>\n", decayMaps[i].szName );
		fprintf( fp, "<locked>%d</locked>\n", decayMaps[i].m_bLocked );
		if ( decayMaps[i].m_iId == 12 )
			fprintf( fp, "<alien>1</alien>\n" );
		
		printXmlPlayerStats( fp, decayMaps[i].statsBest1, true, 1 );
		printXmlPlayerStats( fp, decayMaps[i].statsLast1, false, 1 );
		printXmlPlayerStats( fp, decayMaps[i].statsBest2, true, 2 );
		printXmlPlayerStats( fp, decayMaps[i].statsLast2, false, 2 );
			
		fprintf( fp, "</map>\n" );
	}
	fprintf( fp, "</decaystats>\n" );

	//fprintf( fp, "%6.2f, %6.2f, %6.2f, %s, %2d\n", dataTime, health, ammo, pMapname, skillLevel );
	fclose( fp );

}

//=========================================================
//=========================================================
void CDecayRules::Think ( void )
{
}

//=========================================================
//=========================================================
BOOL CDecayRules::IsMultiplayer( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CDecayRules::IsDeathmatch ( void )
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CDecayRules::IsCoOp( void )
{
	int mapId = getDecayMapId();
	ALERT( at_console, "Decay IsCoOp = True, %d\n", mapId);

	return TRUE;
}


//=========================================================
//=========================================================
BOOL CDecayRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	if ( !pPlayer->m_pActiveItem )
	{
		// player doesn't have an active item!
		return TRUE;
	}

	if ( !pPlayer->m_pActiveItem->CanHolster() )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
//=========================================================
BOOL CDecayRules :: GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon )
{
	return FALSE;
}

void CDecayRules ::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
	if ( m_bAlienMode )
		WRITE_BYTE( 2 );  // alien mode
	else
		WRITE_BYTE( 3 );  // turn off alien mode
	MESSAGE_END();
}

//=========================================================
//=========================================================
BOOL CDecayRules :: ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] )
{
	// pPlayers array of 2 (now 8) stores Decay players starting from 0
	// so first player is in pPlayer[0] (PlayersCount = 1) Gina
	// second is in pPlayer[1]			(PlayersCount = 2) Colette
	// only second player could be a bot

	if (PlayersCount >= 2)
	{
		if (FBitSet(pPlayers[1]->pev->flags, FL_FAKECLIENT) && strcmp(pszAddress, "127.0.0.1") != 0)
		{
			bot_respawn[0].state = 0;
			//SERVER_COMMAND("kick Colette\n");

			char cmd[128];
			sprintf( cmd, "kick \"%s\"\n", STRING( pPlayers[1]->pev->netname ) );
			SERVER_COMMAND( cmd );
			return TRUE;
		} else
			return FALSE; // even if we return FALSE clien still gets connected
	}

	// Players DecayId is assigned in GameRules->OnPlayerSpawn code which is called via ClientPutInServer
	// PlayersCount variable is also incremented there
	// thus code below is obsolete

	/*
	pPlayers[PlayersCount] = GetClassPtr( (CBasePlayer *)VARS(pEntity));
	pPlayers[PlayersCount]->SetDecayPlayerIndex( PlayersCount+1 );	// Gina 1, Collete 2
	PlayersCount++;
	
	UTIL_LogPrintf( "\"%s<%i>\" has entered the game\n",  STRING( pPlayers[PlayersCount]->pev->netname ), GETPLAYERUSERID( pPlayers[PlayersCount]->edict() ) );
	ALERT( at_console, "\"%s<%i>\" has entered the game\n",  STRING( pPlayers[PlayersCount]->pev->netname ), GETPLAYERUSERID( pPlayers[PlayersCount]->edict() ) );
	*/
	return TRUE;
}

void CDecayRules :: InitHUD( CBasePlayer *pl )
{
	//
	// pop-up spare player message
	//

	if ( pl->m_iDecayId >= 3)
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSparePlayer, NULL, pl->edict() );
			WRITE_BYTE( 0 );
		MESSAGE_END();
	}

	//
	// update alien\human game mode
	//

	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pl->edict() );
	    if ( m_bAlienMode )
		  WRITE_BYTE( 2 );  // alien mode
		else
		  WRITE_BYTE( 3 );  // turn off alien mode
	MESSAGE_END();

	//
	// update HUD color
	//

	MESSAGE_BEGIN( MSG_ONE, gmsgChangePlayer, NULL, pl->edict() );
		WRITE_BYTE( pl->m_iDecayId );
	MESSAGE_END();

	//
	// update pPlayer name
	//

	MESSAGE_BEGIN( MSG_ONE, gmsgUpdateDecayPlayerName, NULL, pl->edict() );
		WRITE_BYTE( pl->m_iDecayId );
	MESSAGE_END();

	UTIL_LogPrintf( "\"%s<%i>\" has entered the game\n",  STRING( pl->pev->netname ), GETPLAYERUSERID( pl->edict() ) );

	//
	// check if mission is locked!!!
	//

	int curMapId = this->getDecayMapId();
	if ( curMapId != -1 )
	{
		if ( decayMaps[ getMapEntryId(curMapId) ].m_bLocked == true )
		{
			// fade screen
			// show message
			// call end section

			CTriggerLockedMission *pKicker = (CTriggerLockedMission *)CBaseEntity::Create( "trigger_lockedmission", g_vecZero, g_vecZero );
			if ( !pKicker )
				ALERT( at_aiconsole, "Locked mission entity was not created!\n");
			pKicker->Lock();
		}			
	}

	//
	// fire all trigger_clientspawn entities
	//

	CBaseEntity *pEnt = NULL;
	while ((pEnt = UTIL_FindEntityByClassname( pEnt, "trigger_clientspawn" )) != NULL)
		pEnt->Use( pl, pl, USE_ON, 0.0 );
}

//=========================================================
//=========================================================
void CDecayRules :: ClientDisconnected( edict_t *pClient )
{
	PlayersCount--;
}

//=========================================================
//=========================================================
float CDecayRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
	// subtract off the speed at which a player is allowed to fall without being hurt,
	// so damage will be based on speed beyond that, not the entire fall
	pPlayer->m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
	return pPlayer->m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

void CDecayRules ::RemoveNewPlayer( int PlayerIdInArray )
{
	CTriggerKicker *pKicker = (CTriggerKicker *)CBaseEntity::Create( "trigger_kicker", g_vecZero, g_vecZero );
	if ( !pKicker )
		ALERT( at_aiconsole, "Kicker entity was not created!\n");

	pKicker->m_flDelay = 3;
	pKicker->KickPlayer( pPlayers[PlayerIdInArray] );
}

//=========================================================
//=========================================================
void CDecayRules :: PlayerSpawn( CBasePlayer *pPlayer )
{
	// UpdateGameMode( pPlayer );
	if ( PlayersCount >= 2)
	{
		pPlayers[PlayersCount] = pPlayer; // also store extra player in our array
		PlayersCount++;
		pPlayer->m_iDecayId = PlayersCount;
		ALERT( at_console, "Spare player joined the game!\n");
		
		// Vyacheslav Dzhura TODO
		RemoveNewPlayer( PlayersCount-1 ); // in 5 seconds

		// all these won't normally remove the player:
		//	ClientDisconnect( pPlayer->edict( ) );
		//	ClientKill( pPlayer->edict( ) );
		//	UTIL_Remove( pPlayer );
		return;
	}


	pPlayers[PlayersCount] = pPlayer;
	PlayersCount++;
	pPlayer->m_iDecayId = PlayersCount;

	ALERT( at_console, "Player spawned with DecayID = %i \n", pPlayer->m_iDecayId );

	if ( m_bAlienMode )
		pPlayer->GiveNamedItem( "weapon_vorti" );
/*
	//
	// update pPlayerayer's models
	//

	if ( !m_bAlienMode )
		SET_MODEL(ENT(pPlayer->pev), "models/player.mdl");
	else
		SET_MODEL(ENT(pPlayer->pev), "models/player/dm_slave/dm_slave.mdl");

    char *infobuffer = GET_INFOBUFFER( pPlayer->edict( ) );
    int clientIndex = pPlayer->entindex( );

	if ( !m_bAlienMode )
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "ginacol" );
	else
		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "player/dm_slave/dm_slave" );

	//
	// update heads (for human mode)
	//

	pPlayer->SetBodygroup( 0, 0 );
	if ( !m_bAlienMode )
	{
		if ( pPlayer->m_iDecayId == 1 )
		{
			pPlayer->SetBodygroup( 1, 1 );
			pPlayer->pev->skin = 1;
		} else
			if ( pPlayer->m_iDecayId == 2 )
			{
				pPlayer->SetBodygroup( 1, 0 );
				pPlayer->pev->skin = 0;
			}
	}

	//
	// update various pPlayerayer's properties
    //

	if ( !m_bAlienMode )
		pPlayer->m_bloodColor	= BLOOD_COLOR_RED;
	else
		pPlayer->m_bloodColor	= BLOOD_COLOR_YELLOW;

	pPlayer->UpdateClientData();
*/
}

//=========================================================
//=========================================================
BOOL CDecayRules :: AllowAutoTargetCrosshair( void )
{
	return ( g_iSkillLevel == SKILL_EASY );
}

//=========================================================
//=========================================================
void CDecayRules :: PlayerThink( CBasePlayer *pPlayer )
{
}


//=========================================================
//=========================================================
BOOL CDecayRules :: FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return TRUE;
}

//=========================================================
//=========================================================
float CDecayRules :: FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
	return gpGlobals->time;//now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CDecayRules :: IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	return 1;
}

void CDecayRules :: MonsterKilled( entvars_t *pKiller, entvars_t *pVictim )
{
//   CBaseEntity *pAttacker = CBaseEntity::Instance(pevAttacker);
	CBaseEntity *pKillerEnt = CBaseEntity::Instance( pKiller );
	if ( pKillerEnt->IsPlayer() )
	{
		CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pKillerEnt->pev );
		CBaseEntity *pVictimEnt = CBaseEntity::Instance( pVictim );
		//ALERT( at_console, "Player %d killed %s entity!\n", pl->m_iDecayId, STRING( pVictimEnt->pev->classname ) );
		pStats[pl->m_iDecayId].kills++;
	}
}

void CDecayRules :: PlayerDamaged( CBasePlayer *pPlayer, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	//ALERT( at_console, "Player %d damaged for %f hp (dt: %d)\n", pPlayer->m_iDecayId, flDamage, bitsDamageType );
	pStats[pPlayer->m_iDecayId].damage += flDamage;
}

void CDecayRules :: BulletsFired( entvars_t *pevAttacker, ULONG cShots, int iBulletType, int iShotId )
{
	//lastShotAction
	CBaseEntity *pShooterEnt = CBaseEntity::Instance( pevAttacker );
	if ( pShooterEnt->IsPlayer() )
	{
		CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pShooterEnt->pev );
		pStats[pl->m_iDecayId].lastShotId = iShotId;
		pStats[pl->m_iDecayId].lastShotCounted = false;
		pStats[pl->m_iDecayId].shots++;
	}
}

void CDecayRules :: BulletHit( CBaseEntity *pEntity, entvars_t *pevAttacker, int iShotId )
{
	CBaseEntity *pShooterEnt = CBaseEntity::Instance( pevAttacker );
	if ( pShooterEnt->IsPlayer() )
	{
		CBasePlayer *pl = (CBasePlayer*) CBasePlayer::Instance( pShooterEnt->pev );
		if ( ( iShotId == pStats[pl->m_iDecayId].lastShotId ) & (!pStats[pl->m_iDecayId].lastShotCounted) )
		{
			// yes! we hit something
			pStats[pl->m_iDecayId].lastShotCounted = true;
	
			if ( !FClassnameIs( pEntity->pev, "worldspawn" ) )
				pStats[pl->m_iDecayId].hits++;

			//ALERT( at_console, "Bullet hit stats - player hit %s \n", STRING(pEntity->pev->classname) );
		}
	}
}

char* CDecayRules :: getDecayNextMap()
{
    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);

	static char szNextMap[128];
	sprintf( szNextMap, "null" );

	char szMapName[ 128 ];
	strcpy( szMapName, STRING(gpGlobals->mapname) );

	for ( int i = 0; i < mapCount; i++ )
	{
		if ( strcmp( szMapName, decayMaps[i].szName ) == 0 )
		{
			if ( decayMaps[i].m_iNextId != -1 )
			{
  			    int nextMapEntryId = getMapEntryId( decayMaps[i].m_iNextId );
			
				if ( nextMapEntryId != -1 )
			    {
			        sprintf( szNextMap, decayMaps[nextMapEntryId].szName );
					break;
			    }
			}
		}
	}

	return szNextMap;
}

int CDecayRules :: getDecayMapId()
{
    int mapCount = sizeof(decayMaps)/sizeof(decayMaps[0]);

	int mapId = -1;

	char szMapName[ 128 ];
	strcpy( szMapName, STRING(gpGlobals->mapname) );

	for ( int i = 0; i < mapCount; i++ )
	{
		if ( strcmp( szMapName, decayMaps[i].szName ) == 0 )
		{
			mapId = decayMaps[i].m_iId;
			break;
		}
	}

	return mapId;
/*
	if ( strcmp( szMapName, "dy_accident1" ) == 0 )
		mapId = 0;
*/
}

void CDecayRules :: savePlayerStats( int playerId, int finalGrade, int damageGrade, int killsGrade, int accuracyGrade )
{
	char buffer[MAX_COMPUTERNAME_LENGTH+1];
	DWORD size;
	size=sizeof(buffer);
	GetComputerName(buffer,&size);

	m_iMagicWord1 = 0;

	int cnl = strlen(buffer);
	for (int i=1; i<cnl; i++)
	{
		m_iMagicWord1 += byte(buffer[i]);
	}

	pStats[playerId].gradeFinal = finalGrade;
	pStats[playerId].gradeDamage = damageGrade;
	pStats[playerId].gradeKills = killsGrade;
	pStats[playerId].gradeAccuracy = accuracyGrade;

//	ALERT( at_console, "Player %d grade: %d\n", playerId, finalGrade );
//	ALERT( at_console, "Player %d damage: %d\n", playerId, damageGrade );
//	ALERT( at_console, "Player %d kills: %d\n", playerId, killsGrade );
//	ALERT( at_console, "Player %d accuracy: %d\n", playerId, accuracyGrade );

	int mapId = this->getDecayMapId();
	int mapEntryId = getMapEntryId( mapId );

	pStats[playerId].magicWord1 = m_iMagicWord1;
	pStats[playerId].magicWord2 = m_iMagicWord2;

	if ( mapEntryId != -1 )
	{
		if ( playerId == 1 )
		{
			//if ( !decayMaps[mapEntryId].statsBest1.gradeFinal || decayMaps[mapEntryId].statsBest1.gradeFinal <= finalGrade )
			if ( decayMaps[mapEntryId].statsBest1.gradeFinal >= finalGrade )
			{
				decayMaps[mapEntryId].statsBest1 = pStats[playerId];
				decayMaps[mapEntryId].statsLast1 = pStats[playerId];
			} else
				decayMaps[mapEntryId].statsLast1 = pStats[playerId];
		} else
		if ( playerId == 2 )
		{
			//if ( !decayMaps[mapEntryId].statsBest2.gradeFinal || decayMaps[mapEntryId].statsBest2.gradeFinal <= finalGrade )
			if ( decayMaps[mapEntryId].statsBest2.gradeFinal >= finalGrade )
			{
				decayMaps[mapEntryId].statsBest2 = pStats[playerId];
				decayMaps[mapEntryId].statsLast2 = pStats[playerId];
			} else
				decayMaps[mapEntryId].statsLast2 = pStats[playerId];
		}

		int nextMapId = decayMaps[mapEntryId].m_iNextId;
		if ( nextMapId != -1 )
		{
			decayMaps[getMapEntryId(nextMapId)].m_bLocked = false; // TODO: what if -1 here?
			if ( mapId == 10 )
				if ( canUnlockBonusMission() == true )
				{
					int bonusEntryId = getMapEntryId( 12 ); // dy_alien
					if ( bonusEntryId != -1 )
					{
						decayMaps[bonusEntryId].m_bLocked = false;
						UTIL_ShowMessageAll( "CONGRATULATIONS!!!!! :D Alien mission unlocked!!!" );
					}
				}
		}
	}

	statsSave();
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CDecayRules :: PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if (PlayersCount > 2) // player count will be decreased afterwards in ClientDisconnected function
	{
		ALERT( at_console, "Removed player with id > 2 !!!\n" );
		return;
	}

	char enddecayname[32];
	sprintf( enddecayname, "decay_player%d_dead", pVictim->m_iDecayId ); // decay_player2_dead

	//edict_t	*pentTarget = NULL;
	//pentTarget = FIND_ENTITY_BY_TARGETNAME(pentTarget, enddecayname);
	FireTargets( enddecayname, pVictim, NULL, USE_ON, 1);

	// Could not find trigger_enddecay for player death!
}

//=========================================================
// Deathnotice
//=========================================================
void CDecayRules::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CDecayRules :: PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
}

//=========================================================
// CanHavePlayerItem - the player is touching an CBasePlayerItem, 
// do I give it to him?
//=========================================================

BOOL CDecayRules :: CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	// only living players can have items
	if ( pPlayer->pev->deadflag != DEAD_NO )
		return FALSE;

	if (this->m_bAlienMode == true)
	{
		return (pWeapon->m_iId == WEAPON_VORTI);
	}

	if (pWeapon->DecayPlayerIndex !=0)
	{
		return (pWeapon->DecayPlayerIndex == pPlayer->m_iDecayId);
	}

	return true;
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CDecayRules :: FlWeaponRespawnTime( CBasePlayerItem *pWeapon )
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CDecayRules :: FlWeaponTryRespawn( CBasePlayerItem *pWeapon )
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CDecayRules :: VecWeaponRespawnSpot( CBasePlayerItem *pWeapon )
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CDecayRules :: WeaponShouldRespawn( CBasePlayerItem *pWeapon )
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
BOOL CDecayRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	if ( this->m_bAlienMode )
		return FALSE;
	else
		return TRUE;
}

//=========================================================
//=========================================================
void CDecayRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
{
}

//=========================================================
//=========================================================
int CDecayRules::ItemShouldRespawn( CItem *pItem )
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CDecayRules::FlItemRespawnTime( CItem *pItem )
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CDecayRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CDecayRules::IsAllowedToSpawn( CBaseEntity *pEntity )
{
	return TRUE;
}

//=========================================================
//=========================================================
void CDecayRules::PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount )
{
}

//=========================================================
//=========================================================
int CDecayRules::AmmoShouldRespawn( CBasePlayerAmmo *pAmmo )
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CDecayRules::FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo )
{
	return -1;
}

//=========================================================
//=========================================================
Vector CDecayRules::VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo )
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CDecayRules::FlHealthChargerRechargeTime( void )
{
	return 0;// don't recharge
}

//=========================================================
//=========================================================
int CDecayRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CDecayRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CDecayRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// why would a single player in half life need this? 
	return GR_TEAMMATE;
}

//=========================================================
//=========================================================
BOOL CDecayRules :: FAllowMonsters( void )
{
	return TRUE;
}

void CDecayRules::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
//	ALERT( at_console, "client info (player %d) changed!\n", pPlayer->m_iDecayId );
}

void CopyWeaponsAndAmmo(CBasePlayer *PlayerFrom, CBasePlayer *PlayerTo, bool bDummyDest = false)
{
	int i;
	CBasePlayerItem *pWeapon;
	CBasePlayerItem *pNextWeapon;
	CBasePlayerItem *pActiveWeapon = NULL;
	int viewmodel, weaponmodel, sequence = 0;

	//
	// AMMO
	//

	for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	{
		PlayerTo->m_rgAmmo[i] = PlayerFrom->m_rgAmmo[ i ];
		//  String below prevents sending to HUD (gmsgAmmoX) new ammo values (applied after weapon change)
		//PlayerTo->m_rgAmmoLast[i] = PlayerFrom->m_rgAmmoLast[i];
		PlayerTo->m_rgAmmoLast[i] = 0;
	}
	if ( !bDummyDest )
		PlayerTo->SendAmmoUpdate();

	//
	// SELECTED (ACTIVE) WEAPON
	//

	pActiveWeapon = NULL;
	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( PlayerFrom->m_rgpPlayerItems[ i ] )
		{
			pWeapon = PlayerFrom->m_rgpPlayerItems[i];
			while ( pWeapon ) //rgpPackWeapons[ iPW ]
			{
    			if ( pWeapon == PlayerFrom->m_pActiveItem )
				{
					pActiveWeapon = pWeapon;
					break;
				}
				pWeapon = pWeapon->m_pNext;
			} // while
		}
	}

	sequence = PlayerFrom->pev->sequence;
	if (pActiveWeapon)
	{
		viewmodel = PlayerFrom->pev->viewmodel;
		weaponmodel = PlayerFrom->pev->weaponmodel;
	} else
	{
		viewmodel = 0;
		weaponmodel = 0;
	}

	//
	// WEAPONS
	//

	for ( i = 0 ; i < MAX_ITEM_TYPES ; i++ )
	{
		if ( PlayerFrom->m_rgpPlayerItems[ i ] )
		{
			pWeapon = PlayerFrom->m_rgpPlayerItems[i];  //(CBasePlayerWeapon *)

			while ( pWeapon ) //rgpPackWeapons[ iPW ]
			{
				if ( pWeapon->m_pPlayer )
				{
    				//if ( pWeapon == PlayerFrom->m_pActiveItem )
					//{
					//	PlayerTo->m_pActiveItem = pWeapon;
					//}

					pNextWeapon = pWeapon->m_pNext;

					// problem here moving from tmpPlayer to player 2!!!
					if ( !pWeapon->m_pPlayer->RemovePlayerItem( pWeapon ) )
					{
						ALERT( at_console, "problems removing weapon\n" );
					}

					int iWeaponSlot = pWeapon->iItemSlot();
					
					if ( PlayerTo->m_rgpPlayerItems[ iWeaponSlot ] )
					{
						// there's already one weapon in this slot, so link this into the slot's column
						pWeapon->m_pNext = PlayerTo->m_rgpPlayerItems[ iWeaponSlot ];	
						PlayerTo->m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
					}
					else
					{
						// first weapon we have for this slot
						PlayerTo->m_rgpPlayerItems[ iWeaponSlot ] = pWeapon;
						pWeapon->m_pNext = NULL;	
					}

					if ( bDummyDest == true )
					{
						pWeapon->m_pPlayer = PlayerTo; //NULL;
						pWeapon->pev->owner = PlayerTo->edict();
					}
					else
					{
						pWeapon->AddToPlayer( PlayerTo );  
						pWeapon->AttachToPlayer( PlayerTo );
					}
				}

				pWeapon = pNextWeapon;
			} // while
		}
	}

	if ( pActiveWeapon != NULL )
	{
		PlayerTo->m_pActiveItem = pActiveWeapon;
		PlayerTo->TabulateAmmo();
		if ( !bDummyDest )
		{
			PlayerTo->m_pActiveItem->UpdateClientData( PlayerTo ); // should send gmsgCurWeapon, then should call SendAmmoUpdate which sends gmsgAmmox 
			PlayerTo->SwitchWeapon(pActiveWeapon);     // calls m_pActiveWeapon->Deploy()
			//PlayerTo->SendAmmoUpdate();
			PlayerTo->m_pActiveItem->UpdateItemInfo(); // updates HUD state
		}
	} else
		PlayerTo->m_pActiveItem = NULL;
		
	//PlayerTo->m_pActiveItem = pActiveWeapon;
	
	PlayerTo->pev->viewmodel = viewmodel;
	PlayerTo->pev->weaponmodel = weaponmodel;
	PlayerTo->pev->weapons = PlayerFrom->pev->weapons;
	PlayerTo->pev->sequence = sequence;

	if ( !bDummyDest )
		PlayerTo->UpdateClientData();
}

void CDecayRules :: ChangePlayer( void )
{
	if (PlayersCount < 2) // was !=
	{
	  	ALERT( at_console, "There is no second player!\n" );
		return;
	}

	// CREATE TEMP PLAYER TO STORE SETTINGS TO COPY
	edict_t	*pent;
	pent = CREATE_NAMED_ENTITY( MAKE_STRING( "player" ) );

	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "Can't create tmpPlayer for ChangePlayer function!\n" );
		return;
	}
	CBasePlayer *tmpPlayer = (CBasePlayer*)CBasePlayer::Instance( VARS( pent ));
	//ClearBits(tmpPlayer->pev->flags, FL_FAKECLIENT);
	tmpPlayer->pev->flags |= FL_IMMUNE_WATER;  // Hoaxer: HACK-HACK-HACK 

	// ************** INFOBUFFERS **********************

	char *infobuffer1;
	char *infobuffer2;
    char *pName;
	char sName1[256];
	char sName2[256];

	infobuffer1 = GET_INFOBUFFER(  pPlayers[0]->edict( ) );
	pName = g_engfuncs.pfnInfoKeyValue( infobuffer1, "model" );
	strncpy( sName1, pName, sizeof(sName1) - 1 );
	sName1[ sizeof(sName1) - 1 ] = '\0';

    infobuffer2 = GET_INFOBUFFER(  pPlayers[1]->edict( ) );
	pName = g_engfuncs.pfnInfoKeyValue( infobuffer2, "model" );
	strncpy( sName2, pName, sizeof(sName2) - 1 );
	sName2[ sizeof(sName2) - 1 ] = '\0';

	//sNameX contains model name without .mdl extensions
	
	// Vyacheslav Dzhura TODO: set "name" property as Gina and Colette manually here
	//char sName3[256] = "Gina";
	//sName3[ sizeof(sName3) - 1 ] = '\0';
	//char sName4[256] = "Colette";
	//sName4[ sizeof(sName4) - 1 ] = '\0';

    //SET_CLIENT_KEY_VALUE( pPlayers[0]->entindex(), infobuffer1, "name", sName3 );
    //SET_CLIENT_KEY_VALUE( pPlayers[1]->entindex(), infobuffer2, "name", sName4 );

    SET_CLIENT_KEY_VALUE( pPlayers[0]->entindex(), infobuffer1, "model", sName2 );
    SET_CLIENT_KEY_VALUE( pPlayers[1]->entindex(), infobuffer2, "model", sName1 ); 
	// *************************************************

	bool bP1Ducked = FBitSet(pPlayers[0]->pev->flags, FL_DUCKING);
	bool bP2Ducked = FBitSet(pPlayers[1]->pev->flags, FL_DUCKING);

	// ************** FLASHLIGHT **********************
	bool tmpPlayer1UseFlashligh = pPlayers[0]->FlashlightIsOn(); // to avoid NOT CLIENT bug
	bool tmpPlayer2UseFlashligh = pPlayers[1]->FlashlightIsOn(); // to avoid NOT CLIENT bug

	if (tmpPlayer2UseFlashligh)
		pPlayers[0]->FlashlightTurnOn();
	else
		pPlayers[0]->FlashlightTurnOff();

	if (tmpPlayer1UseFlashligh)
		pPlayers[1]->FlashlightTurnOn();
	else
		pPlayers[1]->FlashlightTurnOff();

	tmpPlayer->m_flFlashLightTime = pPlayers[0]->m_flFlashLightTime;
	tmpPlayer->m_iFlashBattery = pPlayers[0]->m_iFlashBattery;

	pPlayers[0]->m_flFlashLightTime = pPlayers[1]->m_flFlashLightTime;
	pPlayers[0]->m_iFlashBattery = pPlayers[1]->m_iFlashBattery;

	pPlayers[1]->m_flFlashLightTime = tmpPlayer->m_flFlashLightTime;
	pPlayers[1]->m_iFlashBattery = tmpPlayer->m_iFlashBattery;
	// *************************************************

	//for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	//	ALERT( at_console, "BE4 p1 ammo %d is %d | p2 ammo %d is %d \n", i, pPlayers[0]->m_rgAmmo[ i ], i, pPlayers[1]->m_rgAmmo[ i ]); 

	//
	// weapons switching stuff
	//

	// store items from PLAYER 1 to TEMP PLAYER
	//ALERT( at_console, "(CopyWeaponsAndAmmo) from pPlayers[0] to tmpPlayer\n" );
	CopyWeaponsAndAmmo( pPlayers[0], tmpPlayer, true );
	pPlayers[0]->RemoveAllItems( FALSE );

	// now from PLAYER 2 to PLAYER 1
	//ALERT( at_console, "(CopyWeaponsAndAmmo) from pPlayers[1] to pPlayers[0]\n" );
    CopyWeaponsAndAmmo( pPlayers[1], pPlayers[0] );
	pPlayers[1]->RemoveAllItems( FALSE );

	// and finally from TEMP PLAYER to PLAYER 2
	//ALERT( at_console, "(CopyWeaponsAndAmmo) from tmpPlayer to pPlayers[1]\n" );
	CopyWeaponsAndAmmo( tmpPlayer, pPlayers[1] );

	//ALERT( at_console, "DONE WITH WEAPOSN COPYING!!!\n" );

	//for ( i = 0 ; i < MAX_AMMO_SLOTS ; i++ )
	//	ALERT( at_console, "AFTER p1 ammo %d is %d | p2 ammo %d is %d \n", i, pPlayers[0]->m_rgAmmo[ i ], i, pPlayers[1]->m_rgAmmo[ i ]); 

	// ******* BASIC PARAMS FROM PLAYER 1 TO TEMP PLAYER *******
	tmpPlayer->pev->origin = pPlayers[0]->pev->origin;
	tmpPlayer->pev->v_angle = pPlayers[0]->pev->v_angle;
	tmpPlayer->pev->angles = pPlayers[0]->pev->angles;

	tmpPlayer->m_iDecayId = pPlayers[0]->m_iDecayId;
	tmpPlayer->pev->skin = pPlayers[0]->pev->skin;
	tmpPlayer->pev->body = pPlayers[0]->pev->body;
	tmpPlayer->pev->view_ofs = pPlayers[0]->pev->view_ofs;
	tmpPlayer->pev->health = pPlayers[0]->pev->health;
	tmpPlayer->pev->armorvalue = pPlayers[0]->pev->armorvalue;

	// ******* BASIC PARAMS FROM PLAYER 1 TO PLAYER 2 *******
	pPlayers[0]->pev->origin = pPlayers[1]->pev->origin;
	pPlayers[0]->pev->v_angle = pPlayers[1]->pev->v_angle;
	pPlayers[0]->pev->angles = pPlayers[1]->pev->angles;
	pPlayers[0]->pev->fixangle = TRUE;

	pPlayers[0]->pev->view_ofs = pPlayers[1]->pev->view_ofs;
	pPlayers[0]->m_iDecayId = pPlayers[1]->m_iDecayId;
	pPlayers[0]->pev->skin = pPlayers[1]->pev->skin;
	pPlayers[0]->pev->body= pPlayers[1]->pev->body;
	pPlayers[0]->pev->health = pPlayers[1]->pev->health;
	pPlayers[0]->pev->armorvalue = pPlayers[1]->pev->armorvalue;

	//pPlayers[0]->m_hEnemy = NULL;
	//SetBits(pPlayers[0]->pev->flags, FL_FAKECLIENT);

	// ******* BASIC PARAMS FROM PLAYER 2 TO TEMP PLAYER *******
	pPlayers[1]->pev->origin = tmpPlayer->pev->origin;
	pPlayers[1]->pev->v_angle = tmpPlayer->pev->v_angle;
	pPlayers[1]->pev->angles = tmpPlayer->pev->angles;
	pPlayers[1]->pev->fixangle = TRUE;

	pPlayers[1]->pev->view_ofs = tmpPlayer->pev->view_ofs;
	pPlayers[1]->m_iDecayId = tmpPlayer->m_iDecayId;
	pPlayers[1]->pev->skin = tmpPlayer->pev->skin;
	pPlayers[1]->pev->body = tmpPlayer->pev->body;
	pPlayers[1]->pev->health = tmpPlayer->pev->health;
	pPlayers[1]->pev->armorvalue = tmpPlayer->pev->armorvalue;

	// AI
	pPlayers[0]->m_hEnemy = NULL;
	pPlayers[1]->m_hEnemy = NULL;

	//ClearBits(pPlayers[1]->pev->flags, FL_FAKECLIENT);

	// ******* FINAL PREPARATIONS *******
	// remove temp player
	//ALERT( at_console, "REMOVING TMPPLAYER!!!\n" );
	UTIL_Remove( tmpPlayer );
	//ALERT( at_console, "TMPPLAYER SUCCESFULLY REMOVED!!!\n" );

	// ******* DUCK FIX ************
	//if ((pev->button & IN_DUCK) || FBitSet(pev->flags,FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING) )
	//	Duck();
	
	if (bP1Ducked == true) //= FBitSet(pPlayers[0]->pev->flags, FL_DUCKING);
	{
		SetBits(pPlayers[1]->pev->flags, FL_DUCKING);
		pPlayers[1]->Duck();
	}
	if (bP2Ducked == true)
	{
		SetBits(pPlayers[0]->pev->flags, FL_DUCKING);
		pPlayers[0]->Duck();
	}

	// ******** END OF DUCK FIX ****

	// ****** HEADS CHANGE *********
	for ( int i = 0; i < 2; i++ )
	{
		pPlayers[i]->SetBodygroup( 0, 0 );
		if ( pPlayers[i]->m_iDecayId == 1 )
		{
			pPlayers[i]->SetBodygroup( 1, 1 );
			pPlayers[i]->pev->skin = 1;
		} else
			if ( pPlayers[i]->m_iDecayId == 2 )
			{
				pPlayers[i]->SetBodygroup( 1, 0 );
				pPlayers[i]->pev->skin = 0;
			}
	}

	// ****** END OF HEADS CHANGE ******

	if ( gmsgChangePlayer != 0)
	{
		//MESSAGE_BEGIN( MSG_ALL, gmsgChangePlayer);
		MESSAGE_BEGIN( MSG_ONE, gmsgChangePlayer, NULL, pPlayers[0]->edict() );
		  WRITE_BYTE( pPlayers[0]->m_iDecayId );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ONE, gmsgChangePlayer, NULL, pPlayers[1]->edict() );
		  WRITE_BYTE( pPlayers[1]->m_iDecayId );
		MESSAGE_END();
	} else
		ALERT( at_console, "Message gmsgChangePlayer not found in client.dll! Invalid CLIENT.DLL!!!\n" );

	// update client data... again :)
	//pPlayers[0]->UpdateClientData();
	//pPlayers[1]->UpdateClientData();

	//ALERT( at_console, "Checking if second player is bot...\n" );
	if (!pPlayers[1]->IsNetClient()) // if second player is bot
	{
		CBot *pBot;
		pBot = (CBot*)pPlayers[1];
		pBot->pBotEnemy = NULL;
		pBot->BotWeaponInventory();
	}
	// BotWeaponInventory
}
