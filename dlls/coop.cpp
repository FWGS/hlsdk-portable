#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "coop_util.h"
#include "gravgunmod.h"

static float msglimittime1, msglimittime2;

cvar_t mp_coop = { "mp_coop", "0", FCVAR_SERVER };
cvar_t mp_coop_nofriendlyfire = { "mp_coop_nofriendlyfire", "0", FCVAR_SERVER };
cvar_t mp_coop_reconnect_hack = { "mp_coop_reconnect_hack", "0", FCVAR_SERVER };
cvar_t mp_coop_noangry = { "mp_coop_noangry", "0", FCVAR_SERVER };
cvar_t mp_coop_checkpoints = { "mp_coop_checkpoints", "1", FCVAR_SERVER };
cvar_t mp_skipdefaults = { "mp_skipdefaults", "0", FCVAR_SERVER };
cvar_t mp_coop_strongcheckpoints = { "mp_coop_strongcheckpoints", "0", FCVAR_SERVER };

cvar_t mp_unduck = { "mp_unduck", "0", FCVAR_SERVER };
cvar_t mp_semclip = { "mp_semclip", "0", FCVAR_SERVER };
cvar_t mp_spectator = { "mp_spectator", "0", FCVAR_SERVER };

cvar_t materials_txt = { "materials_txt", "sound/materials.txt", FCVAR_SERVER };
cvar_t sentences_txt = { "sentences_txt", "sound/sentences.txt", FCVAR_SERVER };

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
	if( gpGlobals->time < msglimittime1 )
		return;
	msglimittime1 = gpGlobals->time + 0.4;

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
	if( gpGlobals->time < msglimittime2 )
		return;
	msglimittime2 = gpGlobals->time + 0.4;

	va_list	argptr;
	char string[256] = "^2";

	va_start( argptr, format );
	int len = vsnprintf( string + 2, 253, format, argptr );
	va_end( argptr );
	string[len+2] = 0;
	UTIL_ClientPrintAll( HUD_PRINTTALK, string );
}


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

void UTIL_SpawnPlayer( CBasePlayer *pPlayer )
{
	//pPlayer->StopObserver();
	if( pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR )
		pPlayer->gravgunmod_data.m_state = STATE_SPAWNED;
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

char *UTIL_CoopPlayerName( CBaseEntity *pPlayer )
{
	if( !pPlayer )
		return (char*)"unnamed(NULL)";
	return (char*)( ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected" );
}


char *badlist[256] = {
"player", // does not even can set own name
"talat",
"hmse",
"mhmd",
"aeman",
"famas",
"danek",
"ame syia",
"melih",
"aliance",
"vladick"
};

void UTIL_CoopKickPlayer(CBaseEntity *pPlayer)
{
	int i;

	if( !pPlayer )
		return;

	char *name = UTIL_CoopPlayerName( pPlayer );
	SERVER_COMMAND( UTIL_VarArgs( "kick %d\n", ENTINDEX(pPlayer->pev->pContainingEntity) - 1 ) );

	if( strlen( name ) < 5 )
		return;

	// find last slot
	for( i = 0; badlist[i]; i++ );
	if( i > 254 )
		return;

	badlist[i] = strdup( name );
}
#ifdef __WIN32 // no strcasestr
#include <windows.h>
#include <string.h>
const char *strcasestr( const char *s1, const char *s2 )
{
	if( s1 == 0 || s2 == 0 )
		return 0;

	size_t n = strlen(s2);

	while(*s1)
		if(!strnicmp(s1++,s2,n))
			return (s1-1);

	return 0;
}
#endif

bool UTIL_CoopIsBadPlayer( CBaseEntity *plr )
{
	if( !plr )
		return false;

	for( int i = 0; badlist[i];i++ )
		if( strcasestr( (char*)UTIL_CoopPlayerName( plr ), badlist[i] ) )
			return true;

	return false;
}
bool g_fPause;
void COOP_ClearData( void )
{
	g_fPause = false;
	msglimittime1 = msglimittime2 = 0;
}


void COOP_ApplyData( void )
{
	ALERT( at_console, "^2CoopApplyData()\n" );
	msglimittime1 = msglimittime2 = 0;
	g_fPause = false;
}

// use this to translate GGMMapOffset during changelevel
struct GGMLandmarkTransition
{
	char sourceMap[32];
	char targetMap[32];
	char landmarkName[32];
	Vector vecLandmark;
	bool fTriggerUsed;
	bool fSavedPos;
	GGMPosition pos;
};


struct GGMCheckpoint
{
char szDisplayName[32];
float flTime;
struct GGMPosition pos;
};

// offset for all maps relative to current map
struct GGMMapState
{
	struct GGMMapState *pNext;
	char mapName[32];
	Vector vecOffset;
	struct GGMCheckpoint checkpoints[5];
};

GGMMapState *g_pMapStates;
GGMMapState *g_pCurrentMap;
GGMLandmarkTransition g_landmarkTransition;
struct GGMCoopState
{
	struct GGMPosition savedPos;
	bool fSaved;
} g_CoopState;
edict_t *COOP_FindLandmark( const char *pLandmarkName );

void COOP_MarkTriggers( void )
{
	CBaseEntity *pTrigger = NULL;

	while( pTrigger = UTIL_FindEntityByClassname( pTrigger, "trigger_changelevel" ) )
	{
		struct COOPChangelevelData *pData = COOP_GetTriggerData( pTrigger );
		pData->fIsBack = !strcmp( pData->pszMapName, g_landmarkTransition.sourceMap );
		//	if( gpGlobals->startspot && STRING(gpGlobals->startspot) && !strcmp(STRING(gpGlobals->startspot), m_szLandmarkName) )
		// m_coopData.fIsBack = true;

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

bool  COOP_ProcessTransition( void )
{
	bool fAddCurrent = true;
	edict_t *landmark;

	g_CoopState.savedPos = g_landmarkTransition.pos;
	g_CoopState.fSaved = g_landmarkTransition.fSavedPos;

	if( !mp_coop.value )
		return false;

	if( !g_landmarkTransition.landmarkName[0] )
		return false;

	if( strcmp( g_landmarkTransition.targetMap, STRING(gpGlobals->mapname) ) )
		return false;
	landmark = COOP_FindLandmark( g_landmarkTransition.landmarkName );
	if( !landmark )
		return false;
	Vector &lm = landmark->v.origin;

	for( struct GGMMapState *pMapState = g_pMapStates; pMapState; pMapState = pMapState->pNext )
	{
		if( !strcmp( pMapState->mapName, STRING(gpGlobals->mapname) ) )
		{
			pMapState->vecOffset = Vector( 0, 0, 0 );
			fAddCurrent = false;
			g_pCurrentMap = pMapState;
			continue;
		}
		pMapState->vecOffset = pMapState->vecOffset - g_landmarkTransition.vecLandmark + lm;
	}

	if( fAddCurrent )
	{
		GGMMapState *pNewState = (GGMMapState *)calloc(1, sizeof( struct GGMMapState ) );

		pNewState->pNext = g_pMapStates;
		pNewState->vecOffset = Vector(0, 0, 0);
		strncpy(pNewState->mapName, STRING(gpGlobals->mapname), 31);
		g_pMapStates = g_pCurrentMap = pNewState;
	}
	return true;
}

void COOP_SetupLandmarkTransition( const char *szNextMap, const char *szNextSpot, Vector vecLandmarkOffset, struct GGMPosition *pPos )
{
	strncpy(g_landmarkTransition.sourceMap, STRING(gpGlobals->mapname), 31 );
	strncpy(g_landmarkTransition.targetMap, szNextMap, 31 );
	strncpy(g_landmarkTransition.landmarkName, szNextSpot, 31 );
	g_landmarkTransition.vecLandmark = vecLandmarkOffset;
	if( pPos )
	{
		g_landmarkTransition.pos = *pPos;
		g_landmarkTransition.fSavedPos = true;
	}
}

void COOP_ServerActivate( void )
{
	memset( &g_CoopState, 0, sizeof( g_CoopState ) );
	COOP_MarkTriggers();
	if( !COOP_ProcessTransition() )
	{
		ALERT( at_console, "Transition failed, new game started\n");
		while( g_pMapStates )
		{
			GGMMapState *pMapState = g_pMapStates;
			g_pMapStates = pMapState->pNext;
			free( pMapState );
		}
		GGMMapState *pNewState = (GGMMapState *)calloc(1, sizeof( struct GGMMapState ) );

		pNewState->pNext = g_pMapStates;
		pNewState->vecOffset = Vector(0, 0, 0);
		strncpy(pNewState->mapName, STRING(gpGlobals->mapname), 31);
		g_pMapStates = g_pCurrentMap = pNewState;
		if( mp_coop.value )
			COOP_ClearData();
		GGM_ClearLists();
		g_WeaponList.Clear();
	}
	else if( mp_coop.value ) COOP_ApplyData();


	if( mp_coop.value )
	{
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

			// reset all players state
			if( plr )
			{
				plr->gravgunmod_data.m_state = STATE_UNINITIALIZED;
				plr->RemoveAllItems( TRUE );
				UTIL_BecomeSpectator( plr );
				//plr->Spawn();
			}
		}
		if( g_CoopState.fSaved && mp_coop_checkpoints.value )
		{
			memmove( &g_pCurrentMap->checkpoints[1], &g_pCurrentMap->checkpoints[0], sizeof ( g_pCurrentMap->checkpoints[0] ) * 3 );
			g_pCurrentMap->checkpoints[0].flTime = gpGlobals->time;
			snprintf( g_pCurrentMap->checkpoints[0].szDisplayName, 31,  "From %s", g_landmarkTransition.sourceMap );
			g_pCurrentMap->checkpoints[0].pos = g_CoopState.savedPos;
		}
	}
	memset( &g_landmarkTransition, 0, sizeof( GGMLandmarkTransition ) );
}

bool COOP_GetOrigin( Vector *pvecNewOrigin, const Vector &vecOrigin, const char *pszMapName )
{
	for( GGMMapState *pOffset = g_pMapStates; pOffset; pOffset = pOffset->pNext )
	{
		if( !strcmp( pOffset->mapName, pszMapName ) )
		{
			*pvecNewOrigin = vecOrigin + pOffset->vecOffset;
			return true;
		}
	}

	return false;
}

int g_iVote;

// Show to all spawned players: voting, etc..
class GlobalVote
{
public:

	int m_iConfirm;
	int m_iVoteCount;
	int m_iMaxCount;
	int m_iBanCount;
	float m_flTime;
	const char *maps[5];
	int votes[5];
	EHANDLE m_pTrigger;
	EHANDLE m_pPlayer;
	void ConfirmMenu( CBasePlayer *pPlayer, CBaseEntity *trigger, const char *mapname );
	void ShowGlobalMenu( const char *title, int count, const char **menu );
	void Process( CBasePlayer *pPlayer, int imenu );
};

GlobalVote g_GlobalVote;

void GlobalVote::Process( CBasePlayer *pPlayer, int imenu )
{
	if( pPlayer->pev->flags & FL_SPECTATOR )
		return;
	if( gpGlobals->time - m_flTime > 20 )
	{
		COOP_ResetVote();
		return;
	}

	//g_GlobalVote.m_flTime = gpGlobals->time;

	switch( g_iVote )
	{
	case 1: // touch blue trigger
		m_iVoteCount++;

		if( imenu == 0 ) // confirm
		{
			if( m_iBanCount >= 2 )
			{
				UTIL_CoopKickPlayer( pPlayer );
				m_iConfirm-= 5;
				m_iBanCount = 0;
				return;
			}
			m_iConfirm++;
			UTIL_CoopPrintMessage( "%s^7 confirmed map change\n", UTIL_CoopPlayerName( pPlayer ));

		}
		if( imenu == 1 ) // cancel
		{
			m_iConfirm--;
			if( pPlayer == m_pPlayer )
			{
				m_iConfirm -= 100; // player mistake
				g_iVote = 0;
			}
		}
		if( imenu == 2 )
		{
			m_iBanCount++;
			if( m_iBanCount >= 2 && m_iConfirm > -50 )
				UTIL_CoopKickPlayer( m_pPlayer );
			g_iVote = 0;
		}
		break;
	}
}
void GlobalVote::ShowGlobalMenu( const char *title, int count, const char **menu )
{
	int count2 = 0;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() && !UTIL_CoopIsBadPlayer( plr ) )
		{
			count2++;
			CBasePlayer *player = (CBasePlayer *) plr;
			GGM_PlayerMenu &m = player->gravgunmod_data.menu.New( title );
			for( int j = 0; j < count; j++ )
			{
				char cmd[32];
				sprintf(cmd, "votemenu %d", j );
				m.Add( menu[j], cmd );
			}
			m.Show();

		}
	}
	m_iMaxCount = count2;
	m_iBanCount = 0;
}

void GlobalVote::ConfirmMenu( CBasePlayer *pPlayer, CBaseEntity *trigger, const char *mapname )
{
	g_iVote = 1;
	m_flTime = gpGlobals->time;
	m_pTrigger = trigger;
	m_pPlayer = pPlayer;
	const char *menu[] = {
		"Confirm",
		"Cancel",
		"BAN"
	};

	UTIL_CoopPrintMessage( "%s^7 wants to change map ^1BACK to %s\n", UTIL_CoopPlayerName( pPlayer ), mapname );
	ShowGlobalMenu(UTIL_VarArgs("Confirm changing map BACK TO %s?", mapname), ARRAYSIZE(menu), menu);

}


void COOP_NewCheckpoint( entvars_t *pevPlayer )
{
	if( !pevPlayer->netname || pevPlayer->health <= 0 )
		return;
	memmove( &g_pCurrentMap->checkpoints[1], &g_pCurrentMap->checkpoints[0], sizeof ( g_pCurrentMap->checkpoints[0] ) * 3 );
	g_pCurrentMap->checkpoints[0].flTime = gpGlobals->time;
	snprintf( g_pCurrentMap->checkpoints[0].szDisplayName, 31,  "%5s %d", STRING( pevPlayer->netname ), (int)( gpGlobals->time / 60 ) );
	GGM_SavePosition( (CBasePlayer*)CBaseEntity::Instance( pevPlayer ), &g_pCurrentMap->checkpoints[0].pos );
	UTIL_CoopPrintMessage("New checkpoint by %s!\n", STRING( pevPlayer->netname ) );
}


bool COOP_PlayerDeath( CBasePlayer *pPlayer )
{
	static bool st_fSkipNext;
	if( st_fSkipNext )
	{
		st_fSkipNext = false;
		return false;
	}
	if( pPlayer->pev->health > 0 || pPlayer->gravgunmod_data.m_state != STATE_SPAWNED )
		return true;
//	if( pPlayer->gravgunmod_data.m_iMenuState == MENUSTATE_CHECKPOINT )
	//	return true;

	if( g_pCurrentMap->checkpoints[0].flTime )
	{
		COOP_CheckpointMenu( pPlayer );
		st_fSkipNext = true;
		return true;
	}

	return false;
}

bool COOP_SetDefaultSpawnPosition( CBasePlayer *pPlayer )
{
	if( !g_CoopState.fSaved )
		return false;
	return GGM_RestorePosition( pPlayer, &g_CoopState.savedPos );
}

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



// Collect all weapons tat player touchet in coop ant give to all players at spawn

CWeaponList g_WeaponList;

void CWeaponList::Clear()
{
	m_iWeapons = 0;
}
void CWeaponList::GiveToPlayer(CBasePlayer *pPlayer)
{
	for(int i = 0; i < m_iWeapons;i++)
		pPlayer->GiveNamedItem(weapons[i]);
}
void CWeaponList::AddWeapon( const char *classname )
{
	int i;

	//if( !strcmp( classname, "item_suit") )
		//return;

	if( !strcmp( classname, "item_healthkit") )
		return;

	for(i = 0; i < m_iWeapons;i++)
		if(!strcmp(weapons[i], classname))
			return;
	strcpy(weapons[m_iWeapons++], classname);
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *plr = (CBasePlayer*)UTIL_PlayerByIndex( i );

		// broadcast to active players
		if( plr && plr->pev->modelindex )
			plr->GiveNamedItem( classname );
	}

}

void COOP_ResetVote( void )
{
	g_iVote = 0;
	g_GlobalVote.m_iConfirm = 0;
	g_GlobalVote.m_iBanCount = 0;
	g_GlobalVote.m_flTime = gpGlobals->time;

}

bool COOP_ConfirmMenu(CBaseEntity *pTrigger, CBaseEntity *pActivator, int count2, char *mapname )
{
	if( gpGlobals->time - g_GlobalVote.m_flTime > 10 )
		COOP_ResetVote();
	//g_GlobalVote.m_flTime = gpGlobals->time;
	if( mp_coop_strongcheckpoints.value )
	{
		// do not allow go back if there are checkpoints, but not near changelevel
		if( g_pCurrentMap->checkpoints[0].flTime && (g_pCurrentMap->checkpoints[0].pos.vecOrigin - VecBModelOrigin(pTrigger->pev)).Length() > 150 )
		{
			COOP_ResetVote();
			//UTIL_CoopPlayerMessage( pActivator,  1, 5, 0xFF0000FF, 0xFF0000FF, 0, 0.7, "Changelevel back locked by checkpoint\nCheckpoint here to activate trigger!");
			ClientPrint( pActivator->pev, HUD_PRINTCENTER, "Changelevel back locked by checkpoint\nCheckpoint here to activate trigger!");
			return false;
		}
		//if( count2 < 2 )
			//return;
	}

	if( g_iVote != 1 )
	{
		if( !UTIL_CoopIsBadPlayer( pActivator ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)pActivator;

			if( pPlayer->gravgunmod_data.m_iLocalConfirm <= 0 )
				pPlayer->gravgunmod_data.m_iLocalConfirm = 1;
			if( pPlayer->gravgunmod_data.m_iLocalConfirm < 3 )
			{
				pPlayer->gravgunmod_data.menu.New("This will change map back", false)
						.Add("Confirm", "confirmchangelevel")
						.Add("Cancel", "")
						.Show();
			}
			else
			{
				g_GlobalVote.ConfirmMenu(pPlayer, pTrigger, mapname );
				pPlayer->gravgunmod_data.m_iLocalConfirm = 0;
			}
		}
		return false;
	}
	if( g_GlobalVote.m_iConfirm < count2 )
		return false;
	return true;
}

void COOP_CheckpointMenu( CBasePlayer *pPlayer )
{
	int i;

	if( !mp_coop_checkpoints.value )
		return;

	if( !g_pCurrentMap )
		return;

	GGM_PlayerMenu &m = pPlayer->gravgunmod_data.menu.New("Select checkpoint", false);

	if( pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR || pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR_BEGIN || pPlayer->pev->health <= 0 )
		m.Add("Default", "respawn");
	else
		m.Add("New checkpoint", "newcheckpoint");

	for( i = 1; g_pCurrentMap->checkpoints[i-1].flTime; i++ )
	{
		char cmd[32];
		sprintf(cmd, "loadcheckpoint %d", i-1 );
		m.Add(g_pCurrentMap->checkpoints[i-1].szDisplayName, cmd);
	}

	m.Show();
}

bool COOP_ClientCommand( edict_t *pEntity )
{
	const char *pcmd = CMD_ARGV(0);
	CBasePlayer *pPlayer = (CBasePlayer*)GET_PRIVATE(pEntity);
	entvars_t *pev = &pEntity->v;
	if( !mp_coop.value )
		return false;

	else if( FStrEq(pcmd, "unblock") )
	{
		if( pPlayer->gravgunmod_data.m_state != STATE_SPAWNED )
			return false;
		UTIL_CleanSpawnPoint( pev->origin, 150 );
		return true;
	}
	else if( FStrEq( pcmd, "joincoop" ) )
	{
		if( pPlayer->gravgunmod_data.m_state == STATE_SPAWNED )
			return false;
		if( mp_coop_checkpoints.value && g_pCurrentMap && g_pCurrentMap->checkpoints[0].szDisplayName[0] )
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
		if( pPlayer->gravgunmod_data.m_state == STATE_SPAWNED )
		{
			GGM_PlayerMenu &m = pPlayer->gravgunmod_data.menu.New( "COOP MENU" )
					.Add( "Force respawn", "respawn" )
					.Add( "Unblock", "unblock" )
					.Add( "Become spectator", "spectate" );
			if( mp_coop_checkpoints.value )
				m.Add( "Checkpoints", "checkpointmenu" );
			m.Add( "Cancel", "" );
			m.Show();
			return true;
		}
		else if( pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR )
		{
			pPlayer->gravgunmod_data.menu.New( "COOP MENU" )
					.Add( "Join game", "joincoop" )
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
		if( pPlayer->gravgunmod_data.m_state != STATE_SPAWNED || pPlayer->pev->health < 1 )
			UTIL_SpawnPlayer( pPlayer );
		GGM_RestorePosition( pPlayer, &g_pCurrentMap->checkpoints[i].pos );
		return true;
	}
	else if( FStrEq( pcmd, "newcheckpoint") )
	{
		if( !mp_coop_checkpoints.value )
			return false;
		if( pPlayer->gravgunmod_data.m_state != STATE_SPAWNED )
			return false;
		if( !UTIL_CoopIsBadPlayer( pPlayer ) )
			COOP_NewCheckpoint( pPlayer->pev );
		else
			return false;
		return true;
	}
	else if( FStrEq( pcmd, "confirmchangelevel" ) )
	{
		if( pPlayer->gravgunmod_data.m_iLocalConfirm )
			pPlayer->gravgunmod_data.m_iLocalConfirm++;
		else
			return false;
	}
	else if( FStrEq( pcmd, "votemenu" ) )
	{
		int i = atoi( CMD_ARGV(1) );
		g_GlobalVote.Process(pPlayer, i);
		return true;
	}

	return false;
}

void COOP_RegisterCVars()
{
	CVAR_REGISTER( &mp_coop );
	CVAR_REGISTER( &mp_coop_nofriendlyfire );
	CVAR_REGISTER( &mp_unduck );
	CVAR_REGISTER( &mp_semclip );
	CVAR_REGISTER( &mp_coop_reconnect_hack );
	CVAR_REGISTER( &mp_coop_noangry );
	CVAR_REGISTER( &mp_spectator );
	CVAR_REGISTER( &mp_coop_checkpoints );
	CVAR_REGISTER( &mp_skipdefaults );
	CVAR_REGISTER( &mp_coop_strongcheckpoints );

	CVAR_REGISTER( &sentences_txt );
	CVAR_REGISTER( &materials_txt );
}
