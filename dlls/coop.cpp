#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "coop_util.h"
#include "gravgunmod.h"

bool g_fPause;

// offset for all maps relative to current map
struct COOPMapState
{
	struct COOPMapPersist
	{
		char szMapName[32];
		Vector vecOffset;
		struct COOPCheckpoint {
			char szDisplayName[32];
			float flTime;
			struct GGMPosition pos;
		} rgCheckpoints[5];
	} p;

	struct COOPMapState *pNext;
};

struct COOPLandmarkTransition {
	char szSourceMap[32];
	char szTargetMap[32];
	char szLandmarkName[32];
	Vector vecLandmark;
	bool fTriggerUsed;
	bool fSavedPos;
	struct GGMPosition pos;
	bool fLoading;
};

enum COOPSaveSlot
{
	COOP_SAVE_START1 = 0,
	COOP_SAVE_START2,
	COOP_SAVE_AUTO1,
	COOP_SAVE_AUTO2,
	COOP_SAVE_COUNT
};

struct COOPState
{
	// will be saved
	struct COOPPersist
	{
		// weapon list
		char rgszWeapons[64][32];
		int iWeaponCount;

		// data for spawnpoint
		struct GGMPosition savedPos;
		bool fSaved;
		char rgszSaveSlots[COOP_SAVE_COUNT][32];
		char iLastAutoSave;
	} p;

	// runtime state
	struct COOPMapState *pMapStates;
	struct COOPMapState *pCurrentMap;

	// translate GGMMapState during changelevel
	struct COOPLandmarkTransition landmarkTransition;

	int iVote;
	float flMsgLimit1, flMsgLimit2;

} g_CoopState;

cvar_t mp_coop = { "mp_coop", "0", FCVAR_SERVER };
cvar_t mp_coop_nofriendlyfire = { "mp_coop_nofriendlyfire", "0", FCVAR_SERVER };
cvar_t mp_coop_reconnect_hack = { "mp_coop_reconnect_hack", "0", FCVAR_SERVER };
cvar_t mp_coop_noangry = { "mp_coop_noangry", "0", FCVAR_SERVER };
cvar_t mp_coop_checkpoints = { "mp_coop_checkpoints", "1", FCVAR_SERVER };
cvar_t mp_coop_strongcheckpoints = { "mp_coop_strongcheckpoints", "0", FCVAR_SERVER };
cvar_t mp_semclip = { "mp_semclip", "0", FCVAR_SERVER };
cvar_t mp_coop_pause = { "mp_coop_pause", "1", FCVAR_SERVER };


cvar_t materials_txt = { "materials_txt", "sound/materials.txt", FCVAR_SERVER };
cvar_t sentences_txt = { "sentences_txt", "sound/sentences.txt", FCVAR_SERVER };
void COOP_CheckpointMenu( CBasePlayer *pPlayer );

/*
=========================
COOP_WriteState

Write COOP state to file
=========================
*/
void COOP_WriteState( const char *path )
{
	FILE *f = fopen( path, "wb" );
	unsigned int tsize = sizeof( g_CoopState.p );
	struct COOPMapState *pState = g_CoopState.pMapStates;

	if( !f )
		return;

	// keep size to allow extent struct
	fwrite( &tsize, 4, 1, f );
	fwrite( &g_CoopState.p, tsize, 1, f );

	tsize = sizeof( pState->p );
	fwrite( &tsize, 4, 1, f );

	while( pState )
	{
		fwrite( &pState->p, tsize, 1, f );
		pState = pState->pNext;
	}
	fclose( f );
}

/*
=========================
COOP_ReadState

Read COOP state from file
=========================
*/
bool COOP_ReadState( const char *path )
{
	FILE *f = fopen( path, "rb" );
	unsigned int tsize;

	if( !f )
		return false;

	g_CoopState.landmarkTransition.fLoading = true;

	if( !fread( &tsize, 4, 1, f ) )
		return false;

	// do not allow shrink structure
	if( tsize > sizeof( g_CoopState.p ) )
		return false;

	if( !fread( &g_CoopState.p, tsize, 1, f ) )
		return false;

	if( mp_coop_pause.value )
		g_fPause = true;

	if( !fread( &tsize, 4, 1, f ) )
	{
		ALERT( at_error, "Could not read map states from %s\n", path );
		return true;
	}

	g_CoopState.pCurrentMap = NULL;
	while( g_CoopState.pMapStates )
	{
		struct COOPMapState *pState = g_CoopState.pMapStates;

		g_CoopState.pMapStates = pState->pNext;
		free( pState );
	}

	while( true )
	{
		struct COOPMapState *pState = (struct COOPMapState *)calloc( 1, sizeof( struct COOPMapState ) );

		memset( pState, 0, sizeof( struct COOPMapState ) );

		fread( &pState->p, tsize, 1, f );

		if( feof(f) || ferror( f ) )
		{
			free( pState );
			break;
		}

		pState->pNext = g_CoopState.pMapStates;
		g_CoopState.pMapStates = pState;
	}

	fclose( f );
	return true;
}

/*
=========================
COOP_AutoSave

Helper for trigger_autosave
=========================
*/
void COOP_AutoSave( CBaseEntity *pPlayer )
{
	strncpy( g_CoopState.p.rgszSaveSlots[COOP_SAVE_AUTO2], g_CoopState.p.rgszSaveSlots[COOP_SAVE_AUTO1], 31 );
	g_CoopState.p.iLastAutoSave ^= 1;
	snprintf( g_CoopState.p.rgszSaveSlots[COOP_SAVE_AUTO1], 31, "auto%d-%s", g_CoopState.p.iLastAutoSave, STRING( gpGlobals->mapname ) );
	GGM_Save( g_CoopState.p.rgszSaveSlots[COOP_SAVE_AUTO1] );
	memmove( &g_CoopState.pCurrentMap->p.rgCheckpoints[1], &g_CoopState.pCurrentMap->p.rgCheckpoints[0], sizeof ( g_CoopState.pCurrentMap->p.rgCheckpoints[0] ) * 3 );
	g_CoopState.pCurrentMap->p.rgCheckpoints[0].flTime = gpGlobals->time;
	snprintf( g_CoopState.pCurrentMap->p.rgCheckpoints[0].szDisplayName, 31,  "autosave %d", (int)( gpGlobals->time / 60 ) );
	GGM_SavePosition( (CBasePlayer*)pPlayer, &g_CoopState.pCurrentMap->p.rgCheckpoints[0].pos );
	UTIL_CoopPrintMessage("%s activated autosave!\n", GGM_PlayerName( pPlayer ) );
}

#include <dirent.h>
#ifdef _WIN32
#define PATHSEP "\\"
#else
#define PATHSEP "/"
#endif
/*
=========================
COOP_CheckSaveSlots

Nullify slots that are not exist in filesystem
=========================
*/
static void COOP_CheckSaveSlots( void )
{
	char path[256];
	int i;

	// check save slots (may be cleaned by newunit)
	GET_GAME_DIR(path);
	strcat( path, PATHSEP"save" );
	for( i = 0; i < ARRAYSIZE( g_CoopState.p.rgszSaveSlots ); i++ )
	{
		char fpath[256] = "";
		FILE *f;

		if( !g_CoopState.p.rgszSaveSlots[i] )
			continue;

		snprintf( fpath, 255, "%s"PATHSEP"%s.coop", path, g_CoopState.p.rgszSaveSlots[i] );
		if( f = fopen( fpath, "rb" ) )
			fclose( f );
		else
			g_CoopState.p.rgszSaveSlots[i][0] = 0;
	}
}

/*
=========================
COOP_ClearSaves

Clear all save files except of last map (newunit)
=========================
*/
void COOP_ClearSaves( void )
{
	char path[256];
	DIR *dir;
	struct dirent *entry;
	const char *pszOldMap = NULL;

	GET_GAME_DIR(path);
	strcat( path, PATHSEP"save" );

	if( g_CoopState.landmarkTransition.szSourceMap[0] )
		pszOldMap = g_CoopState.landmarkTransition.szSourceMap;

	ALERT( at_console, "COOP_ClearSaves\n" );

	dir = opendir( path );
	if( !dir )
	{
		ALERT( at_error, "opendir failed, cannot clean save files!\n");
		return;
	}

	while( entry = readdir( dir) )
	{
		if( Q_stricmpext("auto0-*", entry->d_name ) ||
			Q_stricmpext("auto1-*", entry->d_name ) ||
			Q_stricmpext("start-*", entry->d_name )	)
		{
			char fpath[256] = "";
			if( pszOldMap && !strncmp( entry->d_name + 6, pszOldMap, strlen( pszOldMap ) ) )
				continue;

			snprintf( fpath, 255, "%s"PATHSEP"%s", path, entry->d_name );
			ALERT( at_console, "Removing %s\n", fpath );
			remove( fpath );
		}
	}
	closedir(dir);
	COOP_CheckSaveSlots();
}

/*
=========================
COOP_MapStartSave

Create save when reaching new map
=========================
*/
void COOP_MapStartSave( void )
{
	char szSavename[32] = "";

	snprintf( szSavename, 31, "start-%s", STRING( gpGlobals->mapname ) );

	// moving to previous map and returning back should not trigger save
	if( !strcmp( g_CoopState.p.rgszSaveSlots[COOP_SAVE_START1], szSavename ) || !strcmp( g_CoopState.p.rgszSaveSlots[COOP_SAVE_START2], szSavename ) )
		return;

	strncpy( g_CoopState.p.rgszSaveSlots[COOP_SAVE_START2], g_CoopState.p.rgszSaveSlots[COOP_SAVE_START1], 31 );
	strncpy( g_CoopState.p.rgszSaveSlots[COOP_SAVE_START1], szSavename, 31 );
	SERVER_COMMAND( UTIL_VarArgs( "wait;wait;ggm_save %s\n", g_CoopState.p.rgszSaveSlots[COOP_SAVE_START1] ) );
}

/*
=========================
COOP_FindLandmark

return info_landmark pointer
=========================
*/
edict_t *COOP_FindLandmark( const char *pLandmarkName )
{
	edict_t	*pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING( NULL, "targetname", pLandmarkName );
	while( !FNullEnt( pentLandmark ) )
	{
		// Found the landmark
		if( FClassnameIs( pentLandmark, "info_landmark" ) )
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING( pentLandmark, "targetname", pLandmarkName );
	}
	ALERT( at_error, "Can't find landmark %s\n", pLandmarkName );
	return NULL;
}

void UTIL_CoopPlayerMessage( CBaseEntity *pPlayer, int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... )
{
	if( !pPlayer )
		return;

	hudtextparms_t params;
	params.x = x, params.y = y;
	params.fadeinTime = params.fadeoutTime = .5f;
	params.holdTime = time;
	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = channel;
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;
	char *pstr = string;

	// set line breaks
	for( int i = 0; *pstr; pstr++,i++ )
	{
		if( *pstr == '\n' )
			i = 0;
		if( i >= 79 )
			*pstr = '\n', i = 0;
	}

	UTIL_HudMessage( pPlayer, params, string );
}

void UTIL_CoopHudMessage( int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... )
{
	if( gpGlobals->time < g_CoopState.flMsgLimit1 )
		return;
	g_CoopState.flMsgLimit1 = gpGlobals->time + 0.4;

	hudtextparms_t params;
	params.x = x, params.y = y;
	params.fadeinTime = params.fadeoutTime = .5f;
	params.holdTime = time;
	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = channel;
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;
	char *pstr = string;

	// set line breaks
	for( int i = 0; *pstr; pstr++,i++ )
	{
		if( *pstr == '\n' )
			i = 0;
		if( i >= 79 )
			*pstr = '\n', i = 0;
	}

	UTIL_HudMessageAll( params, string );
}

void UTIL_CoopPrintMessage( const char *format, ... )
{
	if( gpGlobals->time < g_CoopState.flMsgLimit2 )
		return;
	g_CoopState.flMsgLimit2 = gpGlobals->time + 0.4;

	va_list	argptr;
	char string[256] = "^2";

	va_start( argptr, format );
	int len = vsnprintf( string + 2, 253, format, argptr );
	va_end( argptr );
	string[len+2] = 0;
	UTIL_ClientPrintAll( HUD_PRINTTALK, string );
}

/*
=========================
COOP_CleanSpawnPoint

Move all players near specified origin to prevent stucking
=========================
*/
void UTIL_CleanSpawnPoint( Vector origin, float dist )
{
	CBaseEntity *ent = NULL;
	while( ( ent = UTIL_FindEntityInSphere( ent, origin, dist ) ) != NULL )
	{
		if( ent->IsPlayer() )
		{
			TraceResult tr;
			UTIL_TraceHull( ent->pev->origin + Vector( 0, 0, 36), ent->pev->origin + Vector( RANDOM_FLOAT( -150, 150 ), RANDOM_FLOAT( -150, 150 ), 0 ), dont_ignore_monsters, human_hull, ent->edict(), &tr);
			//UTIL_TraceModel( ent->pev->origin + Vector( 0, 0, 36), ent->pev->origin + Vector( RANDOM_FLOAT( -150, 150 ), RANDOM_FLOAT( -150, 150 ), 0 ), 0, ent->edict(), &tr);
			if( !tr.fAllSolid )
				UTIL_SetOrigin(ent->pev, tr.vecEndPos);
		}
	}
}

/*
=========================
COOP_FixupSpawnPoint

Trace and fix origin to prevent stuch (unused)
=========================
*/
Vector COOP_FixupSpawnPoint( Vector vecOrigin, bool fDuck )
{
	int i = 0;
	// predict that spawn point is almost correct
	while( i < 2 ) // 2 player heights
	{
		Vector point = vecOrigin + Vector( 0, 0, 36 * i );
		TraceResult tr;
		UTIL_TraceHull( point, point, ignore_monsters, (mp_unduck.value&&fDuck)?head_hull:human_hull, NULL, &tr );
		if( !tr.fStartSolid && !tr.fAllSolid )
			return point;
		i = -i;
		if( i >= 0 )
			i++;
	}
	return vecOrigin;
}

/*
=========================
UTIL_BecomeSpectator

Set noclip and invisibility
=========================
*/
void UTIL_BecomeSpectator( CBasePlayer *pPlayer )
{
	//pPlayer->m_bDoneFirstSpawn = true;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->flags |= FL_NOTARGET;
	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->modelindex = 0;
	pPlayer->pev->health = 1;
	pPlayer->m_pGoalEnt = NULL;
	//pPlayer->StopObserver();
	//while( !pPlayer->IsObserver() )
		//pPlayer->StartObserver(pPlayer->pev->origin, pPlayer->pev->angles);
	return;
}

/*
=========================
UTIL_SpawnPlayer

Spawn player which is marked as spectator
=========================
*/
void UTIL_SpawnPlayer( CBasePlayer *pPlayer )
{
	//pPlayer->StopObserver();
	if( pPlayer->m_ggm.iState == STATE_LOAD_FIX )
		return;

	if( !pPlayer->m_ggm.pState )
		return;

	if( pPlayer->m_ggm.iState == STATE_SPECTATOR )
		pPlayer->m_ggm.iState = STATE_SPAWNED;

	pPlayer->m_iRespawnFrames = 0;
	pPlayer->pev->effects &= ~EF_NODRAW;

	pPlayer->pev->takedamage = DAMAGE_YES;
	pPlayer->pev->flags &= ~FL_SPECTATOR;
	pPlayer->pev->movetype = MOVETYPE_WALK;
	pPlayer->Spawn();
	//pPlayer->StopObserver();
	if( mp_coop.value )
		CLIENT_COMMAND( pPlayer->edict(), "touch_show _coopm*\n" );

}

/*
=========================
COOP_GiveDefaultWeapons

Give each item in weapon list
=========================
*/
void COOP_GiveDefaultWeapons(CBasePlayer *pPlayer)
{
	for(int i = 0; i < g_CoopState.p.iWeaponCount;i++)
		pPlayer->GiveNamedItem(g_CoopState.p.rgszWeapons[i]);
}

/*
=========================
COOP_AddDefaultWeapon

Write weapon name to weapon list
=========================
*/
void COOP_AddDefaultWeapon( const char *classname )
{
	int i;

	//if( !strcmp( classname, "item_suit") )
		//return;

	if( !strcmp( classname, "item_healthkit") )
		return;

	for(i = 0; i < g_CoopState.p.iWeaponCount;i++)
		if(!strcmp(g_CoopState.p.rgszWeapons[i], classname))
			return;
	strcpy(g_CoopState.p.rgszWeapons[g_CoopState.p.iWeaponCount++], classname);
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		// broadcast to active players
		if( plr && plr->pev->modelindex )
			plr->GiveNamedItem( classname );
	}

}

/*
=========================
COOP_MarkTriggers

Setup trigger_changelevel color and direction
=========================
*/
void COOP_MarkTriggers( void )
{
	CBaseEntity *pTrigger = NULL;

	while( pTrigger = UTIL_FindEntityByClassname( pTrigger, "trigger_changelevel" ) )
	{
		struct COOPChangelevelData *pData = COOP_GetTriggerData( pTrigger );
		//pData->fIsBack = !strcmp( pData->pszMapName, g_CoopState.landmarkTransition.szSourceMap );
		pData->fIsBack = false;
		if( !strcmp(pData->pszLandmarkName, g_CoopState.landmarkTransition.szLandmarkName) )
			pData->fIsBack = true;

		pTrigger->pev->renderamt = 127;
		pTrigger->pev->effects &= ~EF_NODRAW;
		pTrigger->pev->rendermode = kRenderTransColor;
		pTrigger->pev->rendercolor = g_vecZero;
		if( pData->fIsBack )
			pTrigger->pev->rendercolor.z = 255;
		else
			pTrigger->pev->rendercolor.y = 255;
	}
}

/*
=========================
COOP_ProcessTransition

Process landmark transition
return false to start new game
=========================
*/
bool COOP_ProcessTransition( void )
{
	bool fAddCurrent = true;
	edict_t *landmark;

	// only set pCurrentMap when loading
	if( g_CoopState.landmarkTransition.fLoading )
	{
		COOP_CheckSaveSlots();

		for( struct COOPMapState *pMapState = g_CoopState.pMapStates; pMapState; pMapState = pMapState->pNext )
		{
			if( !strcmp( pMapState->p.szMapName, STRING(gpGlobals->mapname) ) )
			{
				g_CoopState.pCurrentMap = pMapState;
				return true;
			}
		}
		return false;
	}

	memset( &g_CoopState.p.savedPos, 0, sizeof( struct GGMPosition ) );
	g_CoopState.p.fSaved = false;

	COOP_MarkTriggers();

	g_CoopState.p.savedPos = g_CoopState.landmarkTransition.pos;
	g_CoopState.p.fSaved = g_CoopState.landmarkTransition.fSavedPos;

	if( !mp_coop.value )
		return false;

	if( !g_CoopState.landmarkTransition.szLandmarkName[0] )
		return false;

	if( strcmp( g_CoopState.landmarkTransition.szTargetMap, STRING(gpGlobals->mapname) ) )
		return false;
	landmark = COOP_FindLandmark( g_CoopState.landmarkTransition.szLandmarkName );
	if( !landmark )
		return false;
	Vector &lm = landmark->v.origin;

	for( struct COOPMapState *pMapState = g_CoopState.pMapStates; pMapState; pMapState = pMapState->pNext )
	{
		if( !strcmp( pMapState->p.szMapName, STRING(gpGlobals->mapname) ) )
		{
			pMapState->p.vecOffset = Vector( 0, 0, 0 );
			fAddCurrent = false;
			g_CoopState.pCurrentMap = pMapState;
			continue;
		}
		pMapState->p.vecOffset = pMapState->p.vecOffset - g_CoopState.landmarkTransition.vecLandmark + lm;
	}

	if( fAddCurrent )
	{
		COOPMapState *pNewState = (COOPMapState *)calloc(1, sizeof( struct COOPMapState ) );

		pNewState->pNext = g_CoopState.pMapStates;
		pNewState->p.vecOffset = Vector(0, 0, 0);
		strncpy(pNewState->p.szMapName, STRING(gpGlobals->mapname), 31);
		g_CoopState.pMapStates = g_CoopState.pCurrentMap = pNewState;
	}
	return true;
}


/*
=========================
COOP_SetupLandmarkTransition

set cross-level state
=========================
*/
void COOP_SetupLandmarkTransition( const char *szNextMap, const char *szNextSpot, Vector vecLandmarkOffset, struct GGMPosition *pPos )
{
	g_CoopState.landmarkTransition.fLoading = false;
	strncpy(g_CoopState.landmarkTransition.szSourceMap, STRING(gpGlobals->mapname), 31 );
	strncpy(g_CoopState.landmarkTransition.szTargetMap, szNextMap, 31 );
	strncpy(g_CoopState.landmarkTransition.szLandmarkName, szNextSpot, 31 );
	g_CoopState.landmarkTransition.vecLandmark = vecLandmarkOffset;
	if( pPos )
	{
		g_CoopState.landmarkTransition.pos = *pPos;
		g_CoopState.landmarkTransition.fSavedPos = true;
	}
}

static float g_flDupCheck;

/*
=========================
COOP_PlayerSpawn

handle load fix. return true to refuse state change
=========================
*/
bool COOP_PlayerSpawn( CBasePlayer *pPlayer )
{
	if( !g_CoopState.landmarkTransition.fLoading )
		g_flDupCheck = 0.0f;
	if( g_CoopState.landmarkTransition.fLoading && pPlayer )
		pPlayer->m_ggm.iState = STATE_LOAD_FIX;
	if( pPlayer->m_ggm.iState == STATE_LOAD_FIX )
		return true;
	return false;
}

/*
=========================
COOP_ServerActivate

Handle landmark transition, load or game restart
=========================
*/
void COOP_ServerActivate( void )
{
	if( !mp_coop.value )
		return;

	if( !gpGlobals->mapname )
	{
		ALERT( at_error, "NULL mapname\n" );
		return;
	}

	ALERT( at_console, "COOP_ServerActivate: %s\n", STRING( gpGlobals->mapname ) );

	if( g_flDupCheck && gpGlobals->time && ( fabs( g_flDupCheck -gpGlobals->time ) < 1 || g_flDupCheck == 1 )  )
	{
		g_flDupCheck = 0.0f;
		return;
	}

	if( g_CoopState.landmarkTransition.fLoading )
	{
		g_flDupCheck = gpGlobals->time;
	}

	GGM_ConnectSaveBot();

	if( !COOP_ProcessTransition() )
	{
		ALERT( at_console, "Transition failed, new game started\n");

		COOP_ClearSaves();

		while( g_CoopState.pMapStates )
		{
			COOPMapState *pMapState = g_CoopState.pMapStates;
			g_CoopState.pMapStates = pMapState->pNext;
			free( pMapState );
		}
		COOPMapState *pNewState = (COOPMapState *)calloc(1, sizeof( struct COOPMapState ) );

		memset( &g_CoopState.p, 0, sizeof( g_CoopState.p ) );
		pNewState->pNext = g_CoopState.pMapStates;
		pNewState->p.vecOffset = Vector(0, 0, 0);
		strncpy(pNewState->p.szMapName, STRING(gpGlobals->mapname), 31);
		g_CoopState.pMapStates = g_CoopState.pCurrentMap = pNewState;
		GGM_ClearLists();
	}

	g_fPause = (mp_coop_pause.value == 1);
	g_CoopState.flMsgLimit1 = g_CoopState.flMsgLimit2 = 0;


	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );
		// reset all players state
		if( plr )
		{


			plr->m_ggm.iState = STATE_UNINITIALIZED;
			plr->m_ggm.pState = NULL;
			plr->RemoveAllItems( TRUE );
			UTIL_BecomeSpectator( plr );
			if( g_CoopState.landmarkTransition.fLoading )
			{
//				plr->Spawn();
//				CLIENT_COMMAND( plr->edict(), "reconnect\n");
				plr->m_ggm.iState = STATE_LOAD_FIX;

			}
			//plr->Spawn();
		}
	}
	if( !g_CoopState.landmarkTransition.fLoading && g_CoopState.p.fSaved && mp_coop_checkpoints.value )
	{
		memmove( &g_CoopState.pCurrentMap->p.rgCheckpoints[1], &g_CoopState.pCurrentMap->p.rgCheckpoints[0], sizeof ( g_CoopState.pCurrentMap->p.rgCheckpoints[0] ) * 3 );
		g_CoopState.pCurrentMap->p.rgCheckpoints[0].flTime = gpGlobals->time;
		snprintf( g_CoopState.pCurrentMap->p.rgCheckpoints[0].szDisplayName, 31,  "From %s", g_CoopState.landmarkTransition.szSourceMap );
		g_CoopState.pCurrentMap->p.rgCheckpoints[0].pos = g_CoopState.p.savedPos;
	}
	if( !g_CoopState.landmarkTransition.fLoading )
		COOP_MapStartSave();
	memset( &g_CoopState.landmarkTransition, 0, sizeof( struct COOPLandmarkTransition ) );

}

/*
=========================
COOP_GetOrigin

Compute origin by landmarks
=========================
*/
bool COOP_GetOrigin( Vector *pvecNewOrigin, const Vector &vecOrigin, const char *pszMapName )
{
	if( !mp_coop.value )
	{
		if( !strcmp( STRING( gpGlobals->mapname ), pszMapName ) )
		{
			*pvecNewOrigin = vecOrigin;
			return true;
		}
		return false;
	}
	for( COOPMapState *pOffset = g_CoopState.pMapStates; pOffset; pOffset = pOffset->pNext )
	{
		if( !strcmp( pOffset->p.szMapName, pszMapName ) )
		{
			*pvecNewOrigin = vecOrigin + pOffset->p.vecOffset;
			return true;
		}
	}

	return false;
}

/*
=========================
COOP_NewCheckpoint

Check if player allowed to make checkpoint and create it
=========================
*/
void COOP_NewCheckpoint( entvars_t *pevPlayer )
{
	if( !pevPlayer->netname || pevPlayer->health <= 0 )
		return;
	memmove( &g_CoopState.pCurrentMap->p.rgCheckpoints[1], &g_CoopState.pCurrentMap->p.rgCheckpoints[0], sizeof ( g_CoopState.pCurrentMap->p.rgCheckpoints[0] ) * 3 );
	g_CoopState.pCurrentMap->p.rgCheckpoints[0].flTime = gpGlobals->time;
	snprintf( g_CoopState.pCurrentMap->p.rgCheckpoints[0].szDisplayName, 31,  "%5s %d", STRING( pevPlayer->netname ), (int)( gpGlobals->time / 60 ) );
	GGM_SavePosition( (CBasePlayer*)CBaseEntity::Instance( pevPlayer ), &g_CoopState.pCurrentMap->p.rgCheckpoints[0].pos );
	UTIL_CoopPrintMessage("New checkpoint by %s!\n", STRING( pevPlayer->netname ) );
}

/*
=========================
COOP_PlayerDeath

Show checkpoints menu for dead player
=========================
*/
bool COOP_PlayerDeath( CBasePlayer *pPlayer )
{
	static bool st_fSkipNext;

	if( !mp_coop.value )
		return false;

	if( st_fSkipNext )
	{
		st_fSkipNext = false;
		return false;
	}
	if( pPlayer->pev->health > 0 || pPlayer->m_ggm.iState != STATE_SPAWNED )
		return true;
//	if( pPlayer->gravgunmod_data.m_iMenuState == MENUSTATE_CHECKPOINT )
	//	return true;

	if( g_CoopState.pCurrentMap->p.rgCheckpoints[0].flTime )
	{
		COOP_CheckpointMenu( pPlayer );
		st_fSkipNext = true;
		return true;
	}

	return false;
}

/*
=========================
COOP_SetDefaultSpawnPosition

Set position got from landmark transition
=========================
*/
bool COOP_SetDefaultSpawnPosition( CBasePlayer *pPlayer )
{
	if( !g_CoopState.p.fSaved )
		return false;
	return GGM_RestorePosition( pPlayer, &g_CoopState.p.savedPos );
}

/*
=========================
UTIL_CoopGetPlayerTrain

Check if player on global platform and return pointer
=========================
*/
CBaseEntity *UTIL_CoopGetPlayerTrain( CBaseEntity *pPlayer)
{
	CBaseEntity *train = NULL;

	if( !pPlayer)
		return NULL;

	if( !pPlayer->IsPlayer() )
	{
		// activated by path track
		train = pPlayer;
	}
	else
	{
		if( FNullEnt(pPlayer->pev->groundentity))
			return NULL;

		train = CBaseEntity::Instance(pPlayer->pev->groundentity);
	}

	if( !train )
		return NULL;
	if( !train->pev->globalname ||!STRING(train->pev->globalname) || !STRING(train->pev->globalname)[0] )
		return NULL;
	// doors are elevators
	if( strcmp( STRING( train->pev->classname ), "func_train") && strcmp( STRING( train->pev->classname ), "func_tracktrain") && strcmp( STRING( train->pev->classname ), "func_door")   )
		return NULL;
	return train;
}

/*
=========================
COOP_ConfirmMenu

Check blue triggers and ask for changelevel confirmation
=========================
*/
bool COOP_ConfirmMenu( CBaseEntity *pTrigger, CBaseEntity *pActivator, int count2, char *mapname )
{
	if( mp_coop_strongcheckpoints.value )
	{
		// do not allow go back if there are checkpoints, but not near changelevel
		if( g_CoopState.pCurrentMap->p.rgCheckpoints[0].flTime && (g_CoopState.pCurrentMap->p.rgCheckpoints[0].pos.vecOrigin - VecBModelOrigin(pTrigger->pev)).Length() > 150 )
		{
			ClientPrint( pActivator->pev, HUD_PRINTCENTER, "Changelevel back locked by checkpoint\nCheckpoint here to activate trigger!");
			return false;
		}
	}

	if( GGM_ChangelevelVote((CBasePlayer*)pActivator, pTrigger->edict(), mapname ) < count2 )
		return false;

	return true;
}

/*
=========================
COOP_CheckPointMenu

Build and show checkpoint menu for player
=========================
*/
void COOP_CheckpointMenu( CBasePlayer *pPlayer )
{
	int i;

	if( !mp_coop_checkpoints.value )
		return;

	if( !g_CoopState.pCurrentMap )
		return;

	GGM_PlayerMenu &m = pPlayer->m_ggm.menu.New("Select checkpoint", false);

	if( pPlayer->m_ggm.iState == STATE_SPECTATOR || pPlayer->m_ggm.iState == STATE_SPECTATOR_BEGIN || pPlayer->pev->health <= 0 )
		m.Add("Default", "respawn");
	else
		m.Add("New checkpoint", "newcheckpoint");

	for( i = 1; g_CoopState.pCurrentMap->p.rgCheckpoints[i-1].flTime; i++ )
	{
		char cmd[32];
		sprintf(cmd, "loadcheckpoint %d", i-1 );
		m.Add(g_CoopState.pCurrentMap->p.rgCheckpoints[i-1].szDisplayName, cmd);
	}

	m.Show();
}

/*
=========================
COOP_ClientCommand
=========================
*/
bool COOP_ClientCommand( edict_t *pEntity )
{
	const char *pcmd = CMD_ARGV(0);
	CBasePlayer *pPlayer = (CBasePlayer*)GET_PRIVATE(pEntity);
	entvars_t *pev = &pEntity->v;
	if( !mp_coop.value )
		return false;

	else if( FStrEq(pcmd, "unblock") )
	{
		if( pPlayer->m_ggm.iState != STATE_SPAWNED )
			return false;
		UTIL_CleanSpawnPoint( pev->origin, 150 );
		return true;
	}
	else if( FStrEq( pcmd, "joincoop" ) )
	{
		if( pPlayer->m_ggm.iState == STATE_SPAWNED )
			return false;
		if( mp_coop_checkpoints.value && g_CoopState.pCurrentMap && g_CoopState.pCurrentMap->p.rgCheckpoints[0].szDisplayName[0] )
			COOP_CheckpointMenu( pPlayer );
		else
		{
			pPlayer->RemoveAllItems( TRUE );
			UTIL_SpawnPlayer( pPlayer );
		}
		return true;
	}
	else if( FStrEq( pcmd, "coopmenu" ) )
	{
		//UTIL_CoopMenu( pPlayer );
		if( pPlayer->m_ggm.iState == STATE_SPAWNED )
		{
			GGM_PlayerMenu &m = pPlayer->m_ggm.menu.New( "COOP MENU" )
					.Add( "Unblock", "unblock" )
					/// TODO: statistics button here
					.Add( "Respawn", "respawn" )
					.Add( "Other", "coopmenu1");
			if( mp_coop_checkpoints.value )
				m.Add( "Checkpoints", "checkpointmenu" );
			m.Add( "Cancel", "" );
			m.Show();
			return true;
		}
		else if( pPlayer->m_ggm.iState == STATE_SPECTATOR )
		{
			pPlayer->m_ggm.menu.New( "COOP MENU" )
					.Add( "Join game", "joincoop" )
					.Add( "Cancel", "" )
					.Show();
			return true;

		}
	}
	else if( FStrEq( pcmd, "coopmenu1" ) )
	{
		//UTIL_CoopMenu( pPlayer );
		if( pPlayer->m_ggm.iState == STATE_SPAWNED )
		{
			pPlayer->m_ggm.menu.New( "COOP MENU" )
					.Add( "Force respawn", "respawn" )
					.Add( "Spectate", "spectate" )
					.Add( "Vote load", "voteload")
					.Add( "Cancel", "" )
					.Show();
			return true;
		}
	}
	else if( FStrEq( pcmd, "respawn" ) )
	{
		pPlayer->RemoveAllItems( TRUE );
		UTIL_SpawnPlayer( pPlayer );
		return true;
	}
	else if( FStrEq( pcmd, "checkpointmenu") )
	{
		if( !mp_coop_checkpoints.value )
			return false;
		COOP_CheckpointMenu( pPlayer );

		return true;
	}
	else if( FStrEq( pcmd, "loadcheckpoint") )
	{
		int i = atoi(CMD_ARGV(1));
		if( i > 4 || i < 0 )
			return false;
		if( pPlayer->m_ggm.iState != STATE_SPAWNED || pPlayer->pev->health < 1 )
			UTIL_SpawnPlayer( pPlayer );
		GGM_RestorePosition( pPlayer, &g_CoopState.pCurrentMap->p.rgCheckpoints[i].pos );
		return true;
	}
	else if( FStrEq( pcmd, "newcheckpoint") )
	{
		if( !mp_coop_checkpoints.value )
			return false;
		if( pPlayer->m_ggm.iState != STATE_SPAWNED )
			return false;
		if( !GGM_IsTempBanned( pPlayer ) )
			COOP_NewCheckpoint( pPlayer->pev );
		else
			return false;
		return true;
	}
	else if( FStrEq( pcmd, "voteload") )
	{
		if( !mp_coop_checkpoints.value )
			return false;
		if( pPlayer->m_ggm.iState != STATE_SPAWNED )
			return false;
		if( !GGM_IsTempBanned( pPlayer ) )
		{
			if( CMD_ARGC() == 1)
			{
				GGM_PlayerMenu &menu = pPlayer->m_ggm.menu.New("Select save", false );

				for( int i = 0; i < 4; i++ )
					if( g_CoopState.p.rgszSaveSlots[i][0] )
						menu.Add( g_CoopState.p.rgszSaveSlots[i], UTIL_VarArgs("voteload %s",g_CoopState.p.rgszSaveSlots[i]) );

				menu.Add( "Cancel", "" ).Show();
			}
			else
			{
				char szCommand[32];
				snprintf( szCommand, 31, "ggm_load %s", CMD_ARGV(1) );

				GGM_StartVoteCommand( pPlayer, szCommand, UTIL_VarArgs("Load save %s", CMD_ARGV(1) ));
			}
		}
		else
			return false;
		return true;
	}
	else if( FStrEq( pcmd, "confirmchangelevel" ) )
	{
		if( pPlayer->m_ggm.iLocalConfirm )
		{
			pPlayer->m_ggm.iLocalConfirm++;
			if( pPlayer->m_ggm.pChangeLevel )
			{
				edict_t *pChangeLevel = pPlayer->m_ggm.pChangeLevel;
				pPlayer->m_ggm.pChangeLevel = NULL;
				DispatchTouch( pChangeLevel, pPlayer->edict() );
			}
			return true;

		}
		else
			return false;
	}

	return false;
}

/*
=========================
COOP_RegisterCVars

Register console vars and commands for COOP
=========================
*/
void COOP_RegisterCVars()
{
	CVAR_REGISTER( &mp_coop );
	CVAR_REGISTER( &mp_coop_nofriendlyfire );
	CVAR_REGISTER( &mp_coop_pause );


	CVAR_REGISTER( &mp_semclip );
	CVAR_REGISTER( &mp_coop_reconnect_hack );
	CVAR_REGISTER( &mp_coop_noangry );
	CVAR_REGISTER( &mp_coop_checkpoints );
	CVAR_REGISTER( &mp_coop_strongcheckpoints );

	CVAR_REGISTER( &sentences_txt );
	CVAR_REGISTER( &materials_txt );
}
