#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "coop_util.h"
#include "gravgunmod.h"


struct SavedCoords g_SavedCoords, s_SavedCoords;

static float msglimittime1, msglimittime2;

cvar_t mp_coop = { "mp_coop", "0", FCVAR_SERVER };
cvar_t mp_coop_changelevel = { "mp_coop_changelevel", "0", FCVAR_SERVER };
cvar_t mp_coop_nofriendlyfire = { "mp_coop_nofriendlyfire", "0", FCVAR_SERVER };
cvar_t mp_coop_disabledmap = { "mp_coop_disabledmap", "", FCVAR_SERVER };
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


void COOP_ValidateOffset( void )
{
	if( !g_SavedCoords.validoffset )
	{
		edict_t *landmark = COOP_FindLandmark( g_SavedCoords.landmark );
		if(landmark)
			g_SavedCoords.offset = landmark->v.origin - g_SavedCoords.offset;
		else
			g_SavedCoords.offset = g_vecZero - g_SavedCoords.offset;
		g_SavedCoords.validoffset = true;
	}
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


Vector COOP_FixupSpawnPoint(Vector spawn)
{
	int i = 0;
	// predict that spawn point is almost correct
	while( i < 2 ) // 2 player heights
	{
		Vector point = spawn + Vector( 0, 0, 36 * i );
		TraceResult tr;
		UTIL_TraceHull( point, point, ignore_monsters, (mp_unduck.value&&g_fSavedDuck)?head_hull:human_hull, NULL, &tr );
		if( !tr.fStartSolid && !tr.fAllSolid )
			return point;
		i = -i;
		if( i >= 0 )
			i++;
	}
	return spawn;
}

void UTIL_CoopSaveTrain( CBaseEntity *pPlayer, SavedCoords *coords)
{
	if( coords->trainsaved )
		return;

	CBaseEntity *train = UTIL_CoopGetPlayerTrain(pPlayer);
	if( !train )
	{
		ALERT( at_console, "^1NO TRAIN!\n");
		return;
	}
	ALERT( at_console, "^1TRAIN IS %s\n", STRING( train->pev->classname ) );

	if( !pPlayer->IsPlayer() )
	{
		// it is trainnitself, try find player on it
		CBaseEntity *pList;
		Vector mins = pPlayer->pev->absmin;
		Vector maxs = pPlayer->pev->absmax;
		maxs.z += 72;
		int count = UTIL_EntitiesInBox( &pList, 1, mins, maxs, FL_ONGROUND );
		if( count && pList && pList->IsPlayer() )
			pPlayer = pList;
		else
		{
			ALERT( at_console, "Train without players\n" );
			return;
		}
	}

	strcpy( coords->trainglobal, STRING(train->pev->globalname) );
	coords->trainoffset = pPlayer->pev->origin - VecBModelOrigin(train->pev);
	coords->trainsaved = true;
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

char * UTIL_CoopPlayerName( CBaseEntity *pPlayer )
{
	if( !pPlayer )
		return (char*)"unnamed(NULL)";
	return (char*)( ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected" );
}

bool g_fSavedDuck;

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

struct checkpoint_s
{
	char str[32];
	float time;
	Vector origin;
	Vector angles;
} g_checkpoints[4];


void COOP_ClearData( void )
{
	// nullify
	SavedCoords l_SavedCoords = {0};
	g_SavedCoords = l_SavedCoords;
	memset( &g_checkpoints, 0, sizeof( g_checkpoints ) );
	msglimittime1 = msglimittime2 = 0;
}

bool g_fPause;
void COOP_ApplyData( void )
{
	if( s_SavedCoords.valid )
	{
		struct SavedCoords null1 = {0};
		g_SavedCoords = s_SavedCoords;
		s_SavedCoords = null1;
		g_fSavedDuck = g_SavedCoords.fDuck;
	}
	g_fPause = false;
	ALERT( at_console, "^2CoopApplyData()\n" );
	memset( &g_checkpoints, 0, sizeof( g_checkpoints ) );
	msglimittime1 = msglimittime2 = 0;

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
	if( !pevPlayer->netname )
		return;
	memmove( &g_checkpoints[1], &g_checkpoints[0], sizeof ( g_checkpoints[0] ) * 3 );
	g_checkpoints[0].time = gpGlobals->time;
	snprintf( g_checkpoints[0].str, 31,  "%5s %d", STRING( pevPlayer->netname ), (int)( gpGlobals->time / 60 ) );
	g_checkpoints[0].origin = pevPlayer->origin;
	g_checkpoints[0].angles = pevPlayer->angles;
	UTIL_CoopPrintMessage("New checkpoint by %s!\n", STRING( pevPlayer->netname ) );
}


bool COOP_PlayerDeath( CBasePlayer *pPlayer )
{

	if( pPlayer->gravgunmod_data.m_iMenuState == MENUSTATE_CHECKPOINT )
		return true;

	if( g_checkpoints[0].time )
	{
		COOP_CheckpointMenu( pPlayer );
		return true;
	}

	return false;
}

bool UTIL_CoopRestorePlayerCoords(CBaseEntity *player, Vector *origin, Vector *angles )
{
	if(!g_SavedCoords.valid)
		return false;
	COOP_ValidateOffset();
	// compute player by IQ
	char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ), "ip" );
	for( int i = 0;i < g_SavedCoords.iCount;i++)
	{
		if(ip && ip[0] && !strcmp(ip, g_SavedCoords.ip[i]) )
		{
			TraceResult tr;
			Vector point = g_SavedCoords.origin[i] + g_SavedCoords.offset;

			UTIL_TraceHull( point, point, missile, (mp_unduck.value&&g_fSavedDuck)?head_hull:human_hull, NULL, &tr );

			g_SavedCoords.ip[i][0] = 0;

			if( tr.fStartSolid || tr.fAllSolid )
				return false;
			*origin = point;
			*angles = g_SavedCoords.angles[i];
			return true;
		}
	}
	return false;
}

bool UTIL_CoopGetSpawnPoint( Vector *origin, Vector *angles)
{
	if(!g_SavedCoords.valid)
		return false;

	// spawn on elevator or train
	if( g_SavedCoords.trainsaved )
	{
		CBaseEntity *train = UTIL_FindEntityByString( NULL, "globalname", g_SavedCoords.trainglobal );
		if( !train ) train = UTIL_FindEntityByString( NULL, "classname", g_SavedCoords.trainglobal );
		if( train && ( !g_SavedCoords.trainuser1 || train->pev->iuser1 == g_SavedCoords.trainuser1 ) )
		{
			*angles = g_SavedCoords.triggerangles;
			*origin = VecBModelOrigin(train->pev) + g_SavedCoords.trainoffset;
			g_SavedCoords.trainuser1 = train->pev->iuser1;
			return true;
		}
		ALERT( at_console, "Failed to get train %s (map design error?)\n", g_SavedCoords.trainglobal );
	}
	Vector point = g_SavedCoords.triggerorigin;
	*angles = g_SavedCoords.triggerangles;
	if( !g_SavedCoords.validspawnpoint )
	{
		TraceResult tr;
		Vector angle;
		UTIL_MakeVectorsPrivate( *angles, (float*)&angle, NULL, NULL );
		COOP_ValidateOffset();
		point = point + g_SavedCoords.offset;
		//UTIL_TraceHull( point, point, ignore_monsters, human_hull, NULL, &tr );

		if( mp_unduck.value && g_fSavedDuck && !g_SavedCoords.fUsed )
			UTIL_TraceHull( point, point + angle * 100, missile, head_hull, NULL, &tr );
		else
			UTIL_TraceHull( point, point + angle * 100, ignore_monsters, human_hull, NULL, &tr );

		if( !tr.fStartSolid && !tr.fAllSolid || ENTINDEX( tr.pHit ) && ENTINDEX( tr.pHit ) <= gpGlobals->maxClients )
		{
			//g_SavedCoords.triggerorigin = tr.vecEndPos;
			//g_SavedCoords.validspawnpoint = true;
			if( tr.pHit && FClassnameIs( tr.pHit, "func_door" ) )
				tr.pHit->v.solid = SOLID_NOT;
			ALERT( at_console, "CoopGetSpawnPoint: ^2offset set\n");

		}
		else
		{
			//g_SavedCoords.valid = false;
			ALERT( at_console, "CoopGetSpawnPoint: ^2trace failed\n");
			return false;
		}
	}
	*origin = point;

	return true;
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
		if( g_checkpoints[0].time && (g_checkpoints[0].origin - VecBModelOrigin(pTrigger->pev)).Length() > 150 )
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

	GGM_PlayerMenu &m = pPlayer->gravgunmod_data.menu.New("Select checkpoint", false);

	if( pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR || pPlayer->gravgunmod_data.m_state == STATE_SPECTATOR_BEGIN || pPlayer->pev->health <= 0 )
		m.Add("Map begin", "respawn");
	else
		m.Add("New checkpoint", "newcheckpoint");

	for( i = 1; g_checkpoints[i-1].time; i++ )
	{
		char cmd[32];
		sprintf(cmd, "loadcheckpoint %d", i-1 );
		m.Add(g_checkpoints[i-1].str, cmd);
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
		if( mp_coop_checkpoints.value && g_checkpoints[0].str[0] )
			COOP_CheckpointMenu( pPlayer );
		else
		{
			pPlayer->RemoveAllItems( TRUE );
			pPlayer->gravgunmod_data.m_state = STATE_SPAWNED;
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
		if( i > 4 )
			return false;
		pPlayer->RemoveAllItems( TRUE );
		UTIL_SpawnPlayer( pPlayer );
		pPlayer->pev->origin = g_checkpoints[i].origin;
		pPlayer->pev->angles = g_checkpoints[i].angles;
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
	CVAR_REGISTER( &mp_coop_changelevel );
	CVAR_REGISTER( &mp_coop_nofriendlyfire );
	CVAR_REGISTER( &mp_coop_disabledmap );
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
