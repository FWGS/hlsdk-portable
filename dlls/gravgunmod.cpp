#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gravgunmod.h"
#include "player.h"
#include "coop_util.h"
#include "gamerules.h"
#include "weapons.h"


cvar_t cvar_allow_gravgun = { "mp_allow_gravgun","2", FCVAR_SERVER };
cvar_t cvar_allow_ar2 = { "mp_allow_ar2","0", FCVAR_SERVER };
cvar_t cvar_ar2_mp5 = { "mp_ar2_mp5","0", FCVAR_SERVER };
cvar_t cvar_ar2_balls = { "mp_ar2_balls","0", FCVAR_SERVER };
cvar_t cvar_ar2_bullets = { "mp_ar2_bullets","0", FCVAR_SERVER };
cvar_t cvar_allow_bigcock = { "mp_allow_bigcock","0", FCVAR_SERVER };
cvar_t cvar_allow_gateofbabylon = { "mp_allow_gateofbabylon","0", FCVAR_SERVER };

cvar_t cvar_wresptime = { "mp_wresptime","20", FCVAR_SERVER };
cvar_t cvar_iresptime = { "mp_iresptime","30", FCVAR_SERVER };
cvar_t cvar_gibtime = { "mp_gibtime","250", FCVAR_SERVER };
cvar_t cvar_hgibcount = { "mp_hgibcount","12", FCVAR_SERVER };
cvar_t cvar_agibcount = { "mp_agibcount","8", FCVAR_SERVER };
cvar_t mp_gravgun_players = { "mp_gravgun_players", "0", FCVAR_SERVER };
cvar_t mp_skipdefaults = { "mp_skipdefaults", "0", FCVAR_SERVER };
cvar_t mp_spectator = { "mp_spectator", "0", FCVAR_SERVER };
cvar_t mp_unduck = { "mp_unduck", "0", FCVAR_SERVER };

cvar_t mp_fixhornetbug = { "mp_fixhornetbug", "0", FCVAR_SERVER };
cvar_t mp_fixsavetime = { "mp_fixsavetime", "0", FCVAR_SERVER };
cvar_t mp_checkentities = { "mp_checkentities", "0", FCVAR_SERVER };
cvar_t mp_touchmenu = { "mp_touchmenu", "1", FCVAR_SERVER };
cvar_t mp_touchname = { "mp_touchname", "", FCVAR_SERVER };
cvar_t mp_touchcommand = { "mp_touchcommand", "", FCVAR_SERVER };
cvar_t mp_serverdistclip = { "mp_serverdistclip", "0", FCVAR_SERVER};
cvar_t mp_maxbmodeldist = { "mp_maxbmodeldist", "4096", FCVAR_SERVER};
cvar_t mp_maxtrashdist = { "mp_maxtrashdist", "4096", FCVAR_SERVER};
cvar_t mp_maxwaterdist = { "mp_maxwaterdist", "4096", FCVAR_SERVER};
cvar_t mp_maxotherdist = { "mp_maxotherdist", "4096", FCVAR_SERVER};
cvar_t mp_maxmonsterdist = { "mp_maxmonsterdist", "4096", FCVAR_SERVER};
cvar_t mp_servercliptents = { "mp_servercliptents", "0", FCVAR_SERVER};
cvar_t mp_maxtentdist = { "mp_maxtentdist", "4096", FCVAR_SERVER};
cvar_t mp_maxdecals = { "mp_maxdecals", "-1", FCVAR_SERVER };
cvar_t mp_enttools_checkmodels = { "mp_enttools_checkmodels", "0", FCVAR_SERVER };
cvar_t mp_errormdl = { "mp_errormdl", "0", FCVAR_SERVER };
cvar_t mp_errormdlpath = { "mp_errormdlpath", "models/error.mdl", FCVAR_SERVER };

cvar_t *zombietime = NULL;
static char gamedir[MAX_PATH];
void Ent_RunGC_f( void );
static bool g_fCmdUsed;
enum GGMVoteMode
{
	VOTE_NONE = 0,
	VOTE_COOP_CHANGELEVEL,
	VOTE_COMMAND
};

// cancel vote after 15 seconds
#define VOTE_INACTIVE_TIME 15.0f
// ignore vote first 2 seconds
#define VOTE_MISCLICK_TIME 2.0f

// global vote state
struct GGMVote
{
	int iMode;
	EHANDLE pPlayer;
	// changelevel
	edict_t *pTrigger;
	char szCommand[256];
	char szMessage[256];
	float flStartTime;
	float flLastActiveTime;
	int iMaxCount;
	int iTempBanCount;
	int iConfirm;
} g_Vote;

/*
=====================
GGM_IsTempBanned

Return true if tempban active for player
=====================
*/
bool GGM_IsTempBanned( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = (CBasePlayer*)pEnt;

	if( !pPlayer || !pPlayer->IsPlayer() )
		return false;

	if( !pPlayer->m_ggm.pState )
		return false;

	return pPlayer->m_ggm.pState->t.fIsTempBanned;
}

/*
=====================
GGM_ClearVote

Reset vote state
=====================
*/
void GGM_ClearVote( void )
{
	memset( &g_Vote, 0, sizeof( g_Vote ) );
}

/*
=====================
GGM_SendVote

Send menu for current vote for specified player
=====================
*/
void GGM_SendVote( CBasePlayer *pPlayer )
{
	pPlayer->m_ggm.menu.New( g_Vote.szMessage )
			.Add( "Confirm", "voteconfirm" )
			.Add( "Cancel", "votecancel" )
			.Add( "BAN", "votetempban" )
			.Show();
}

/*
=====================
GGM_BroadcastVote

Send menu for current vote for all players
=====================
*/
void GGM_BroadcastVote( void )
{
	int iCount = 0;

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() && !GGM_IsTempBanned( plr ) )
		{
			CBasePlayer *pPlayer = (CBasePlayer *) plr;

			if( pPlayer->m_ggm.iState != STATE_SPAWNED )
				continue;

			iCount++;
			GGM_SendVote( pPlayer );
		}
	}
	g_Vote.flLastActiveTime = g_Vote.flStartTime = gpGlobals->time;
	g_Vote.iTempBanCount = 0;
	g_Vote.iConfirm = 0;
	g_Vote.iMaxCount = iCount;
}

/*
=====================
GGM_VoteCommand

Start vote for specified command and message
=====================
*/
void GGM_StartVoteCommand( CBasePlayer *pPlayer, const char *pszCommand, const char *pszMessage )
{
	if( gpGlobals->time - g_Vote.flLastActiveTime > VOTE_INACTIVE_TIME )
		GGM_ClearVote();

	// vote pending
	if( g_Vote.iMode )
		return;

	g_Vote.iMode = VOTE_COMMAND;
	g_Vote.pPlayer = pPlayer;
	snprintf( g_Vote.szCommand, 255, "%s\n", pszCommand);
	strncpy( g_Vote.szMessage, pszMessage, 255 );

	GGM_BroadcastVote();
}

void GGM_VoteCommand_f( void )
{
	GGM_StartVoteCommand( NULL, CMD_ARGV(1), CMD_ARGV(2));
}

/*
=====================
GGM_ChangelevelVote

Start changelevel vote or get vote confirm count
=====================
*/
int GGM_ChangelevelVote( CBasePlayer *pPlayer, edict_t *pTrigger, const char *pszMapName )
{
	if( gpGlobals->time - g_Vote.flLastActiveTime > VOTE_INACTIVE_TIME )
		GGM_ClearVote();

	if( g_Vote.iMode == VOTE_COOP_CHANGELEVEL )
	{
		if( g_Vote.pTrigger == pTrigger )
			return g_Vote.iConfirm;
		else
			return -1;
	}

	if( g_Vote.iMode )
		return -1;

	if( !GGM_IsTempBanned( pPlayer ) )
	{
		if( pPlayer->m_ggm.iLocalConfirm <= 0 )
			pPlayer->m_ggm.iLocalConfirm = 1;
		if( pPlayer->m_ggm.iLocalConfirm < 3 )
		{
			pPlayer->m_ggm.pChangeLevel = pTrigger;
			pPlayer->m_ggm.menu.New("This will change map back", false)
					.Add("Confirm", "confirmchangelevel")
					.Add("Cancel", "")
					.Show();
			return -1;
		}
		else
		{
			g_Vote.iMode = VOTE_COOP_CHANGELEVEL;
			g_Vote.pTrigger = pTrigger;
			g_Vote.pPlayer = pPlayer;
			ALERT( at_logged, "coop: %s started vote changelevel for %s\n", GGM_PlayerName( pPlayer ), pszMapName );
			UTIL_CoopPrintMessage( "%s^7 wants to change map ^1BACK to %s\n", GGM_PlayerName( pPlayer ), pszMapName );
			snprintf(g_Vote.szMessage, 255, "Change map BACK TO %s?", pszMapName );
			GGM_BroadcastVote();
			pPlayer->m_ggm.iLocalConfirm = 0;
			pPlayer->m_ggm.pChangeLevel = NULL;
			return 0;
		}
	}
	return -1;
}

/*
=====================
GGM_VoteProcess

Handle vote commands, do actions
=====================
*/
bool GGM_VoteProcess( CBasePlayer *pPlayer, const char *pszStr )
{
	if( !g_Vote.iMode )
		return false;

	if( gpGlobals->time - g_Vote.flLastActiveTime > VOTE_INACTIVE_TIME )
	{
		GGM_ClearVote();
		return false;
	}

	if( gpGlobals->time - g_Vote.flStartTime < VOTE_MISCLICK_TIME )
	{
		GGM_SendVote( pPlayer );
		return false;
	}

	if( !strcmp( pszStr, "voteconfirm" ) )
	{
		if( g_Vote.iTempBanCount >= 3 )
		{
			GGM_TempBan( pPlayer );
			g_Vote.iConfirm -= 5;
			g_Vote.iTempBanCount = 0;
			return true;
		}
		g_Vote.iConfirm++;

		if( g_Vote.iMode == VOTE_COOP_CHANGELEVEL )
		{
			UTIL_CoopPrintMessage( "%s^7 confirmed map change\n", GGM_PlayerName( pPlayer ) );
			ALERT( at_logged, "coop: %s confirmed map change\n", GGM_PlayerName( pPlayer ) );
			
			DispatchTouch( g_Vote.pTrigger, g_Vote.pPlayer.Get() );
		}
		else if( g_Vote.iMode == VOTE_COMMAND )
		{
			UTIL_CoopPrintMessage( "%s^7 confirmed vote\n", GGM_PlayerName( pPlayer ) );
			ALERT( at_logged, "ggm: %s confirmed vote for \"%s\"\n", GGM_PlayerName( pPlayer ), g_Vote.szCommand );
			if( g_Vote.iConfirm > g_Vote.iMaxCount / 2 )
			{
				g_fCmdUsed = false;
				SERVER_COMMAND( g_Vote.szCommand );
				SERVER_EXECUTE();
				GGM_ClearVote();
			}
		}
		return true;
	}
	else if( !strcmp( pszStr, "votecancel" ) )
	{
		g_Vote.iConfirm--;
		if( g_Vote.pPlayer == pPlayer )
			GGM_ClearVote();
		return true;
	}
	else if( !strcmp( pszStr, "votetempban" ) )
	{
		g_Vote.iTempBanCount++;
		if( g_Vote.iTempBanCount >= 3 )
		{
			GGM_TempBan( g_Vote.pPlayer );
			GGM_ClearVote();
		}
		return true;
	}

	return false;
}

/*
=====================
GGM_PlayerName

Return readable player name
=====================
*/
const char *GGM_PlayerName( CBaseEntity *pPlayer )
{
	if( !pPlayer )
		return "unnamed(NULL)";
	return (const char*)( ( pPlayer->pev->netname && STRING( pPlayer->pev->netname )[0] != 0 ) ? STRING( pPlayer->pev->netname ) : "unconnected" );
}

/*
=====================
GGM_TempBan

Mark player as temprary banned and kick from server
This bans player from votes and changelevel activation
=====================
*/
void GGM_TempBan( CBaseEntity *pEnt )
{
	CBasePlayer *pPlayer = (CBasePlayer*)pEnt;

	if( !pEnt || pEnt->IsPlayer() )
		return;

	if( !pPlayer->m_ggm.pState )
		return;

	pPlayer->m_ggm.pState->t.fIsTempBanned = true;

	SERVER_COMMAND( UTIL_VarArgs( "kick #%d\n", GETPLAYERUSERID( pPlayer->edict() ) ) );
}

/*
=====================
Q_starcmp

string comparing with pattern (from Xash3D)
=====================
*/
static bool Q_starcmp( const char *pattern, const char *text )
{
	char		c, c1;
	const char	*p = pattern, *t = text;

	while(( c = *p++ ) == '?' || c == '*' )
	{
		if( c == '?' && *t++ == '\0' )
			return false;
	}

	if( c == '\0' ) return true;

	for( c1 = (( c == '\\' ) ? *p : c ); ; )
	{
		if( tolower( *t ) == c1 && Q_stricmpext( p - 1, t ))
			return true;
		if( *t++ == '\0' ) return false;
	}
}
/*
=====================
Q_stricmpext

string comparing with pattern (from Xash3D)
=====================
*/
bool Q_stricmpext( const char *pattern, const char *text )
{
	char	c;

	while(( c = *pattern++ ) != '\0' )
	{
		switch( c )
		{
		case '?':
			if( *text++ == '\0' )
				return false;
			break;
		case '\\':
			if( tolower( *pattern++ ) != tolower( *text++ ))
				return false;
			break;
		case '*':
			return Q_starcmp( pattern, text );
		default:
			if( tolower( c ) != tolower( *text++ ))
				return false;
		}
	}
	return true;
}

/*
=====================
GGM_LightStyle_f

Allow server admin to change lightstyles
=====================
*/
void GGM_LightStyle_f( void )
{
	if( CMD_ARGC() != 3 )
	{
		ALERT( at_error, "Usage: mp_lightstyle <number> <pattern>\n" );
		return;
	}

	int style = atoi( CMD_ARGV(1) );
	if( style < 0 || style > 256 )
		return;

	LIGHT_STYLE( style, CMD_ARGV(2) );
}

/*
=====================
Ent_RunGC

Run Garbage Collector
Clean trash that we can remove without breaking game
Remove entities by uid or pattern
=====================
*/
void Ent_RunGC( int flags, const char *userid, const char *pattern )
{
	int i, count = 0, removed = 0;
	edict_t *ent = g_engfuncs.pfnPEntityOfEntIndex( gpGlobals->maxClients + 5 );

	ALERT( at_warning, "Running garbage collector\n" );

	for( i = gpGlobals->maxClients + 5; i < gpGlobals->maxEntities; i++, ent++ )
	{
		const char *classname = STRING( ent->v.classname );

		if( ent->free )
			continue;

		if( !classname || !ent->v.classname || !classname[0] )
			continue;

		count++;

		if( ent->v.flags & FL_KILLME )
			continue;

		if( !strcmp( classname, "spark_shower" ) )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		if( !strncmp( classname, "weapon_", 7 ) || !strncmp( classname, "ammo_", 5 ) || !strncmp( classname, "item_", 5 ) || !strcmp( classname, "prop" ) )
		{
			if( ent->v.velocity.z < -1600 )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}

		if( flags & GC_COMMON )
		{
			if( !strcmp( classname, "gib" ) || !strcmp( classname, "gateofbabylon_bolt" ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}

			if( !strncmp( classname, "monster_", 8 ) && ent->v.health <= 0 || ent->v.deadflag != DEAD_NO )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}
		if( !(flags & GC_ENTTOOLS) && !pattern )
		{
			if( strncmp( classname, "monster_", 8 ) || strncmp( classname, "weapon_", 7 ) || strncmp( classname, "ammo_", 5 ) || strncmp( classname, "item_", 5 ) )
				continue;
		}

		if( !ent->v.owner && ent->v.spawnflags & SF_NORESPAWN )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		CBaseEntity *entity = CBaseEntity::Instance( ent );

		if( !entity )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		if( (flags & GC_ENTTOOLS) && entity->enttools_data.enttools == 1 )
		{
			if( !userid || !strcmp( userid, entity->enttools_data.ownerid ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}

		if( (flags & GC_COMMON) && !entity->IsInWorld() )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}

		if( pattern )
		{
			const char *targetname = STRING( ent->v.targetname );
			if( !targetname || !ent->v.targetname )
				targetname = "";

			if( Q_stricmpext( pattern, classname ) || Q_stricmpext( pattern, targetname ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}
	}

	ALERT( at_notice, "Total %d entities, %d removed\n", count, removed );

}

/*
=====================
Ent_RunGC_f

Run some GC modes manually
=====================
*/
void Ent_RunGC_f()
{
	int enttools = atoi(CMD_ARGV(1));
	const char *pattern = CMD_ARGV( 2 );
	if( enttools != 2 || !pattern[0] )
		pattern = NULL;
	int flags = 0;
	if( !enttools )
		flags |= GC_COMMON;
	if( enttools == 1 )
		flags |= GC_ENTTOOLS;
	Ent_RunGC( flags, NULL, pattern );
}

/*
=====================
CREATE_NAMED_ENTITY wrapper

return NULL when out of edicts instead of Host_Error
use timer to detect spawn in same frame
=====================
*/
edict_t *CREATE_NAMED_ENTITY( string_t name )
{
	static int lastindex;
	static float time;

	// if entities overflowed in this frame, do not allow create new one
	if( gpGlobals->maxEntities - lastindex < 10 && time == gpGlobals->time )
		return NULL;

	edict_t *pent = g_engfuncs.pfnCreateNamedEntity(name);
	lastindex = ENTINDEX( pent );
	time = gpGlobals->time;
	return pent;
}

/*
=====================
Ent_ChangeOwner

Change owner and enttools state by pattern or old owner id
=====================
*/
void Ent_ChangeOwner( const char *szOld, const char *pattern, const char *szNew, int oldstate, int newstate )
{
	edict_t *ent = g_engfuncs.pfnPEntityOfEntIndex( gpGlobals->maxClients + 5 );
	int i;

	for( i = gpGlobals->maxClients + 5; i < gpGlobals->maxEntities; i++, ent++ )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( ent );

		if( !pEntity )
			continue;

		if( pEntity->enttools_data.enttools == oldstate )
		{
			const char *classname = STRING( ent->v.classname );
			const char *targetname = STRING( ent->v.targetname );

			if( !ent->v.classname ) classname = 0;
			if( !ent->v.targetname ) targetname = 0;

			if( pattern && pattern[0] && classname && targetname && !Q_stricmpext( pattern, classname ) && !Q_stricmpext( pattern, targetname ) )
				continue;

			if( szOld && szOld[0] && strcmp( szOld, pEntity->enttools_data.ownerid ) )
				continue;

			pEntity->enttools_data.enttools = newstate;
			strcpy( pEntity->enttools_data.ownerid, szNew );
		}
	}
}

/*
=====================
Ent_ChangeOwner_f
=====================
*/
void Ent_Chown_f()
{
	if( CMD_ARGC() != 6 )
	{
		ALERT( at_console, "ent_chown <oldowner> <pattern> <newowner> <oldstate> <newstate>\n");
	}
	Ent_ChangeOwner( CMD_ARGV(1), CMD_ARGV(2), CMD_ARGV(3), atoi(CMD_ARGV(4)), atoi(CMD_ARGV(5)) );
}


/*
=====================
Ent_CheckEntitySpawn

Refuse some entities to spawn
=====================
*/
int Ent_CheckEntitySpawn( edict_t *pent )
{

	if( mp_checkentities.value )
	{
		int index = ENTINDEX( pent );
		static unsigned int counter, lastgc;
		counter++;


		if( gpGlobals->maxEntities - index < 10 )
		{
			ALERT( at_error, "REFUSING CREATING ENTITY %s\n", STRING( pent->v.classname ) );
			Ent_RunGC( GC_COMMON, NULL );
			return 1;
		}

		if( gpGlobals->maxEntities - index < 100 )
		{
			if( !strncmp( STRING(pent->v.classname), "env_", 4) )
				return 1;

			if( !strcmp( STRING(pent->v.classname), "gib" ) )
				return 1;


			Ent_RunGC( GC_COMMON, NULL );

			return 0;
		}

		if( index > gpGlobals->maxEntities / 2 && counter - lastgc > 256 )
		{
			lastgc = counter;
			Ent_RunGC( GC_COMMON, NULL );
			return 0;
		}
		else if( counter - lastgc > gpGlobals->maxEntities )
		{
			lastgc = counter;
			Ent_RunGC( GC_COMMON, NULL );
			return 0;
		}
	}

	return 0;
}

/*
=====================
GGM_ChatPrintf

Print message to player's chat
=====================
*/
void GGM_ChatPrintf( CBasePlayer *pPlayer, const char *format, ... )
{
	va_list	argptr;
	char string[256];

	va_start( argptr, format );
	int len = vsnprintf( string, 256, format, argptr );
	va_end( argptr );
	string[len] = 0;

	//ClientPrint( &player->v, HUD_PRINTCONSOLE, string );
	CLIENT_PRINTF( pPlayer->edict(), print_chat, string );
}

/*
=====================
GGM_FilterFileName

Allow only safe characters
=====================
*/
bool GGM_FilterFileName( const char *name )
{
	while( name && *name )
	{
		if( *name >= 'A' && *name <= 'z' || *name >= '0' && *name <= '9' || *name == '_' )
		{
			name++;
			continue;
		}
		return false;
	}

	return true;
}

/*
=====================
GGM_GetAuthId

Calculate auth id for connected client
This does not require player state filled
=====================
*/
const char *GGM_GetAuthID( CBasePlayer *pPlayer )
{
	static char uid[33];
	const char *authid = GETPLAYERAUTHID( pPlayer->edict() );

	if( !authid || strstr(authid, "PENDING") )
	{
		const char *ip = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "ip" );
		if( ip )
		{
			char *pUid;

			snprintf( uid, 32, "IP_%s", ip );

			for( pUid = uid; *pUid; pUid++ )
				if( *pUid == '.' ) *pUid = '_';
		}
		else
			return "UNKNOWN";
	}
	else strncpy( uid, authid, 32 );

	if( GGM_FilterFileName( uid ) )
		return uid;

	return "UNKNOWN";
}

/*
=====================
GGM_ClientPutinServer

Let The Emperor connect to the server
Mark player as connected and setup pState
=====================
*/
void GGM_ClientPutinServer( edict_t *pEntity, CBasePlayer *pPlayer )
{
	if( pPlayer->m_ggm.iState == STATE_LOAD_FIX )
		return;
	if( mp_touchmenu.value && pPlayer->m_ggm.iState == STATE_UNINITIALIZED )
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "touch_enable", 111 );

	pPlayer->m_ggm.iState = STATE_CONNECTED;

	pPlayer->m_ggm.flEntTime = 0;
	pPlayer->m_ggm.flEntScore = 0;
	pPlayer->m_ggm.menu.m_pPlayer = pPlayer;
	pPlayer->m_ggm.menu.Clear();
	pPlayer->m_ggm.pState = GGM_GetState( GGM_GetAuthID(pPlayer), STRING(pEntity->v.netname) );
	// restore frags
	GGM_RestoreState( pPlayer );
}

/*
=====================
GGM_ClientFirstSpawn

Handle first spawn in deathmatch
=====================
*/
void GGM_ClientFirstSpawn(CBasePlayer *pPlayer)
{
	// AGHL-like spectator
	if( mp_spectator.value && g_pGameRules->IsMultiplayer()  )
	{
		pPlayer->RemoveAllItems( TRUE );
		UTIL_BecomeSpectator( pPlayer );
	}

}

/*
=====================
GGM_PlayerByID

Find connected player with specified UID
=====================
*/
edict_t *GGM_PlayerByID( const char *id )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() )
		{
			CBasePlayer *player = (CBasePlayer *) plr;

			if( !player->m_ggm.pState )
				continue;

			if( !strcmp( player->m_ggm.pState->szUID, id ) )
				return player->edict();
		}
	}

	return NULL;
}

/*
=====================
GGM_GetPlayerID

Check player pointed and return UID or NULL
=====================
*/
const char *GGM_GetPlayerID( edict_t *player )
{
	CBasePlayer *plr = (CBasePlayer*)CBaseEntity::Instance( player );

	if( !plr->IsPlayer() )
		return NULL;

	if( !plr->m_ggm.pState )
		return NULL;

	return plr->m_ggm.pState->szUID;
}

struct GGMPlayerState *registered_list;
struct GGMPlayerState *anonymous_list;
struct GGMLogin *login_list;

/*
=====================
GGM_FindState

Find state in given list
=====================
*/
struct GGMPlayerState *GGM_FindState( GGMPlayerState *list, const char *uid )
{
	struct GGMPlayerState *pState;

	for( pState = list; pState; pState = pState->pNext )
	{
		if( !strncmp( uid, pState->szUID, 32 ) )
			return pState;
	}
	return NULL;
}

/*
=====================
GGM_WritePersist

Write persistent cross-server state
This enabled only for registered players
=====================
*/
void GGM_WritePersist( GGMPlayerState *pState )
{
	FILE *f;
	char path[64] = "";

	if( !pState->fRegistered )
		return;

	pState->fNeedWrite = false;

	snprintf( path, 63, "%s/ggm/registrations/%s", gamedir, pState->szUID );

	f = fopen( path, "wb" );

	if( !f )
		return;

	fwrite( &pState->p, 1, sizeof( pState->p ), f );
	fclose( f );
}

/*
=====================
GGM_ReadPersist

Read persistent cross-server state
This enabled only for registered players
=====================
*/
void GGM_ReadPersist( GGMPlayerState *pState )
{
	FILE *f;
	char path[64] = "";

	if( !pState->fRegistered )
		return;

	snprintf( path, 63, "%s/ggm/registrations/%s", gamedir, pState->szUID );

	f = fopen( path, "rb" );

	if( !f )
		return;

	fread( &pState->p, 1, sizeof( pState->p ), f );
	fclose( f );
}

/*
=====================
GGM_ClearLists

Clear all GGMPlayerState records
This used before loading player states
or when starting new game
=====================
*/
void GGM_ClearLists( void )
{
	// unlink from all players
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( pPlayer && pPlayer->IsPlayer() )
			pPlayer->m_ggm.pState = NULL;
	}

	while( login_list )
	{
		struct GGMLogin *pLogin = login_list;

		login_list = login_list->pNext;
		free( pLogin );
	}

	while( registered_list )
	{
		struct GGMPlayerState *pState = registered_list;

		registered_list = registered_list->pNext;
		free( pState );
	}

	while( anonymous_list )
	{
		struct GGMPlayerState *pState = anonymous_list;

		anonymous_list = anonymous_list->pNext;
		free( pState );
	}
}

/*
=====================
GGM_WritePlayers

Write players non-persistent state
This used during ggm_save to fully save game
=====================
*/
void GGM_WritePlayers( const char *path )
{
	FILE *f = fopen( path, "wb" );
	unsigned int tsize = sizeof( struct GGMTempState );
	struct GGMPlayerState *pState = registered_list;
	bool reg_completed = false;

	if( !f )
		return;

	// make state actual
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( pPlayer && pPlayer->IsPlayer() )
			GGM_SaveState( pPlayer );
	}

	// keep size to allow extent struct
	fwrite( &tsize, 4, 1, f );

	while( true )
	{
		if( !pState && !reg_completed )
		{
			pState = anonymous_list;
			reg_completed = true;
		}
		if( !pState )
			break;

		fwrite( &pState->fRegistered, 1, 1, f );
		fwrite( &pState->szUID, 33, 1, f );
		fwrite( &pState->t, sizeof( struct GGMTempState ), 1, f );

		pState = pState->pNext;
	}
	fclose( f );
}

/*
=====================
GGM_ReadPlayers

Read players non-persistent state
This used during ggm_load to fully restore game
=====================
*/
bool GGM_ReadPlayers( const char *path )
{
	FILE *f = fopen( path, "rb" );
	unsigned int tsize;

	if( !f )
		return false;

	GGM_ClearLists();

	if( !fread( &tsize, 4, 1, f ) )
		return false;

	// do not allow shrink structure
	if( tsize > sizeof( struct GGMTempState ) )
		return false;

	while( true )
	{
		struct GGMPlayerState *pState = (struct GGMPlayerState *)calloc( 1, sizeof( struct GGMPlayerState ) );

		memset( pState, 0, sizeof( struct GGMPlayerState ) );

		fread( &pState->fRegistered, 1, 1, f );
		fread( &pState->szUID, 33, 1, f );
		fread( &pState->t, tsize, 1, f );

		if( feof(f) || ferror( f ) )
		{
			free( pState );
			break;
		}

		if( pState->fRegistered )
		{
			pState->pNext = registered_list;
			registered_list = pState;
			GGM_ReadPersist( pState );
		}
		else
		{
			pState->pNext = anonymous_list;
			anonymous_list = pState;
		}
	}

	fclose( f );
	return true;
}

void GGM_SavePlayers_f( void )
{
	GGM_WritePlayers( CMD_ARGV(1) );
}


void GGM_LoadPlayers_f( void )
{
	if( !GGM_ReadPlayers( CMD_ARGV(1) ) )
		ALERT( at_error, "Failed to load player states from %s\n", CMD_ARGV( 1 ) );
}

/*
=====================
GGM_ConnectSaveBot

if need to do automatic saves,
connect bot to reserve first slot
======================
*/
void GGM_ConnectSaveBot( void )
{
	edict_t *client0 = INDEXENT( 1 );
	edict_t *bot = NULL;
	char cmd[33] = "";
	float health = client0->v.health;
	int deadflag = client0->v.deadflag;
	float zombietime_old;
	SERVER_EXECUTE();

	// save even with dead player
	if( health <= 0 )
		client0->v.health = 1;

	client0->v.deadflag = 0;

	if( g_engfuncs.pfnGetInfoKeyBuffer( client0 )[0] )
		return;

	snprintf( cmd, 32, "kick #%d\n", GETPLAYERUSERID( client0 ) );
	SERVER_COMMAND(cmd);
	SERVER_EXECUTE();
	bot = g_engfuncs.pfnCreateFakeClient("_save_bot");
	if( bot != client0 )
		ALERT( at_warning, "Bot is not player 1\n" );
	bot->v.health = 1;
	bot->v.deadflag = 0;
	bot->v.effects |= EF_NODRAW;
	bot->free = true;

	client0->v.deadflag = deadflag;
	client0->v.health = health;
	if( zombietime )
		zombietime->value = zombietime_old;
}

/*
=====================
GGM_Save

wrapper to save that saves all GGM states
if mp_coop disabled, connect bot if client0 is not alive
to force engine allow save without player 1
=====================
*/
void GGM_Save( const char *savename )
{
	edict_t *client0 = INDEXENT( 1 );
	edict_t *bot = NULL;
	char cmd[33] = "";
	float health = client0->v.health;
	int deadflag = client0->v.deadflag;
	float zombietime_old;
	bool fNeedKick = false;
	SERVER_EXECUTE();

	// save even with dead player
	if( health <= 0 )
		client0->v.health = 1;

	client0->v.deadflag = 0;

	if( zombietime )
		zombietime_old = zombietime->value;
	if( !(g_engfuncs.pfnGetInfoKeyBuffer( client0 )[0]))
		fNeedKick = true;
	if( !(g_engfuncs.pfnGetPhysicsInfoString( client0 )[0]))
		fNeedKick = true;
	if( !client0->v.netname )
		fNeedKick = true;
	if( !strcmp( GETPLAYERAUTHID( client0 ), "VALVE_ID_LOOPBACK" ) )
		fNeedKick = false;

	if( mp_coop.value )
		fNeedKick = false;

	// hack to make save work when client 0 not connected
	if( fNeedKick )
	{
		snprintf( cmd, 32, "kick #%d\n", GETPLAYERUSERID( client0 ) );
		SERVER_COMMAND(cmd);
		SERVER_EXECUTE();
		bot = g_engfuncs.pfnCreateFakeClient("_save_bot");
		if( bot != client0 )
			ALERT( at_warning, "Bot is not player 1\n" );
		bot->v.health = 1;
		bot->v.deadflag = 0;
	}

	snprintf( cmd, 32, "save %s\n", savename);
	SERVER_COMMAND(cmd);
	if( bot )
		SERVER_COMMAND( "kick _save_bot\n");
	SERVER_EXECUTE();
	client0->v.deadflag = deadflag;
	client0->v.health = health;
	if( zombietime )
		zombietime->value = zombietime_old;
	snprintf( cmd, 32, "%s/save/%s.players", gamedir, savename );
	GGM_WritePlayers( cmd );
	if( mp_coop.value )
	{
		snprintf( cmd, 32, "%s/save/%s.coop", gamedir, savename );
		COOP_WriteState( cmd );
	}
}

/*
=====================
GGM_Save_f
=====================
*/
void GGM_Save_f( void )
{
	char savename[33] = "";
	strncpy( savename, CMD_ARGV(1), 32);
	GGM_Save( savename );
}

/*
=====================
GGM_Load

wrapper to load that loads all GGM states
=====================
*/
void GGM_Load( const char *savename )
{
	char cmd[33] = "";

	if( mp_coop.value )
	{

		snprintf( cmd, 32, "%s/save/%s.coop", gamedir, savename );
		if( !COOP_ReadState( cmd ) )
		{
			ALERT( at_error, "Failed to read gravgunmod coop state %s\n", cmd );
			return;
		}
	}

	snprintf( cmd, 32, "%s/save/%s.players", gamedir, savename );
	if( !GGM_ReadPlayers( cmd ) )
	{
		ALERT( at_error, "Failed to read gravgunmod players state %s\n", cmd );
		return;
	}

	snprintf( cmd, 32, "load %s\n", savename);
	SERVER_COMMAND( cmd );
	SERVER_EXECUTE();

}

/*
=====================
GGM_Load_f
=====================
*/
void GGM_Load_f( void )
{
	char savename[33] = "";
	strncpy( savename, CMD_ARGV(1), 32);
	GGM_Load( savename );
}

/*
=====================
GGM_GetRegistration

find/load registration record by name
=====================
*/
struct GGMPlayerState *GGM_GetRegistration( const char *name )
{
	struct GGMPlayerState *pState = GGM_FindState( registered_list, name );

	if( pState )
	{
		GGM_ReadPersist( pState );
		return pState;
	}
	else
	{
		FILE *f;
		char path[64] = "";

		snprintf( path, 63, "%s/ggm/registrations/%s", gamedir, name );

		f = fopen( path, "rb" );

		if( !f )
			return NULL;

		pState = (struct GGMPlayerState*)calloc( 1, sizeof( struct GGMPlayerState ) );
		memset( pState, 0, sizeof( struct GGMPlayerState ) );

		fread( &pState->p, 1, sizeof( pState->p ), f );
		fclose( f );
		pState->pNext = registered_list;
		pState->fRegistered = true;
		registered_list = pState;
		strncpy( pState->szUID, name, 32 );
		return pState;
	}
}

/*
=====================
GGM_HashString

Numeric string replacement to replace strigns with bad symbols
=====================
*/
unsigned int GGM_HashString( const char *s )
{
	unsigned int hashval;

	for( hashval = 0; *s; s++ )
		hashval = *s + 31*hashval;
	return hashval;
}


/*
=====================
GGM_WriteLogin

Save login to filesystem to share it between servers
=====================
*/
void GGM_WriteLogin( struct GGMLogin *pLogin )
{
	FILE *f;
	char path[64] = "";

	if( !pLogin->pState )
		return;

	if( !GGM_FilterFileName( pLogin->f.szUID ) || !GGM_FilterFileName( pLogin->f.szName ) )
		snprintf( path, 63,  "%s/ggm/logins/%d.%d", gamedir, GGM_HashString( pLogin->f.szUID ), GGM_HashString( pLogin->f.szName ) );
	else
		snprintf( path, 63, "%s/ggm/logins/%s.%s", gamedir, pLogin->f.szUID, pLogin->f.szName );

	f = fopen( path, "wb" );

	if( !f )
		return;

	fwrite( &pLogin->f, 1, sizeof( pLogin->f ), f );
	fwrite( &pLogin->pState->szUID, 1, 33, f );
	fclose( f );
}

/*
=====================
GGM_LoadLogin

Find or read GGMLogin
=====================
*/
struct GGMLogin *GGM_LoadLogin( const char *uid, const char *name )
{
	FILE *f;
	char path[64] = "";
	struct GGMLogin *pLogin;

	for( pLogin = login_list; pLogin; pLogin = pLogin->pNext )
	{
		if( !strcmp( name, pLogin->f.szName ) && !strcmp(uid, pLogin->f.szUID ) )
		{
			return pLogin;
		}
	}

	if( !GGM_FilterFileName( uid ) || !GGM_FilterFileName( name ) )
		snprintf( path, 63,  "%s/ggm/logins/%d.%d", gamedir, GGM_HashString( uid ), GGM_HashString( name ) );
	else
		snprintf( path, 63, "%s/ggm/logins/%s.%s", gamedir, uid, name );

	f = fopen( path, "rb" );

	if( !f )
		return NULL;

	pLogin = (struct GGMLogin*)calloc(1, sizeof( struct GGMLogin ) );
	fread( &pLogin->f, 1, sizeof( pLogin->f ), f );
	fread( path, 1, 33, f );
	path[32] = 0;
	fclose( f );
	pLogin->pState = GGM_GetRegistration(path);
	pLogin->pNext = login_list;
	login_list = pLogin;
	return pLogin;
}

/*
=====================
GGM_GetState

Find or read GGMPlayerState
=====================
*/
struct GGMPlayerState *GGM_GetState( const char *uid, const char *name )
{
	struct GGMPlayerState *pState;
	struct GGMLogin *pLogin = GGM_LoadLogin( uid, name );
	char *rgpszBadNames[] = {
	"player*", // does not even can set own name
	"*talat*",
	"*hmse*",
	"*mhmd*",
	"*aeman*",
	"*famas*",
	"*danek*",
	"ame syia*",
	"*melih*",
	"*aliance*",
	"*alliance*",
	"*vladick*",
	};

	if( pLogin )
	{
		return pLogin->pState;
	}
	else
	{
		for( struct GGMLogin *login = login_list; login; login = login->pNext )
		{
			if( !strcmp( name, login->f.szName ) )
			{
				return NULL;
			}
		}
	}

	if( ( pState = GGM_FindState( anonymous_list, uid ) ) )
			return pState;

	pState = (struct GGMPlayerState*)calloc( 1, sizeof( struct GGMPlayerState ) );
	memset( pState, 0, sizeof( struct GGMPlayerState ) );
	strncpy( pState->szUID, uid, 32 );
	pState->szUID[32] = 0;
	pState->pNext = anonymous_list;

	for( int i = 0; i < ARRAYSIZE( rgpszBadNames ); i++ )
	{
		if( Q_stricmpext( rgpszBadNames[i], name ) )
		{
			pState->t.fIsTempBanned = true;
			break;
		}
	}

	return anonymous_list = pState;
}

/*
=====================
Activate server
Clear states and handle changelevel
=====================
*/
void GGM_ServerActivate( void )
{
	COOP_ServerActivate();
	GGM_ClearVote();
}
/*
=====================
GGM_SavePostiton

Build GGMPostition from player
save platform-relative and landmark-relative postitions
=====================
*/
void GGM_SavePosition( CBasePlayer *pPlayer, struct GGMPosition *pos )
{
	pos->vecOrigin = pPlayer->pev->origin;
	pos->vecAngles = pPlayer->pev->angles;
	pos->fDuck = !!(pPlayer->pev->flags & FL_DUCKING);
	strncpy( pos->szMapName, STRING(gpGlobals->mapname), 31 );
	CBaseEntity *pTrain = UTIL_CoopGetPlayerTrain(pPlayer);
	if( pTrain )
	{
		strcpy( pos->szTrainGlobal, STRING( pTrain->pev->globalname ) );
		if( pTrain->pev->angles == g_vecZero )
			pos->vecTrainOffset = pPlayer->pev->origin - VecBModelOrigin(pTrain->pev);
		else
			pos->vecTrainOffset = pPlayer->pev->origin - pTrain->pev->origin;

		pos->vecTrainAngles = pTrain->pev->angles;
	}
	else pos->szTrainGlobal[0] = 0;

}

/*
=====================
GGM_SaveState

Update GGMPlayerState record
=====================
*/
void GGM_SaveState( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;
	GGMPlayerState *pState = pPlayer->m_ggm.pState;
	int i, j = 0;

	if( !pState )
		return;

	pState->t.iFrags = pPlayer->pev->frags;
	pState->t.iDeaths = pPlayer->m_iDeaths;

	if( pPlayer->m_ggm.iState != STATE_SPAWNED )
		return;

	if( pPlayer->pev->health <= 0 )
		return;

	GGM_SavePosition( pPlayer, &pState->t.pos );

	pState->t.flHealth = pPlayer->pev->health;
	pState->t.flBattery = pPlayer->pev->armorvalue;
	if(pPlayer->m_pActiveItem.Get())
		strncpy( pState->t.szWeaponName, STRING(pPlayer->m_pActiveItem.Get()->v.classname), 31);


	for( i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		CBasePlayerWeapon *pWeapon = pPlayer->m_rgpPlayerItems[i];

		while( pWeapon )
		{
			strncpy( pState->t.rgszWeapons[j], STRING(pWeapon->pev->classname), 31);
			pState->t.rgiClip[j] = pWeapon->m_iClip;
			j++;
			pWeapon = pWeapon->m_pNext;
		}
	}
	pState->t.rgszWeapons[j][0] = 0;
	for( i = 0; i < MAX_AMMO_SLOTS; i++ )
		pState->t.rgszAmmo[i] = pPlayer->m_rgAmmo[i];

	if( pState->fNeedWrite )
		GGM_WritePersist( pState );
}

/*
=====================
GGM_RestorePostiton

Copy position from GGMPostition to real player
Restore platform-relative and landmark-relative postitions
=====================
*/
bool GGM_RestorePosition( CBasePlayer *pPlayer, struct GGMPosition *pos )
{
	bool fOriginSet = COOP_GetOrigin( &pPlayer->pev->origin, pos->vecOrigin, pos->szMapName );
	//pPlayer->pev->origin = pState->t.pos.vecOrigin;

	if( pos->szTrainGlobal[0] )
	{
		CBaseEntity *pTrain =  UTIL_FindEntityByString( NULL, "globalname", pos->szTrainGlobal );
		if( pTrain )
		{
			Vector vecTrainOrigin;
			if( pos->vecTrainAngles == g_vecZero )
				vecTrainOrigin = VecBModelOrigin( pTrain->pev );
			else
				vecTrainOrigin = pTrain->pev->origin;

			Vector angleDiff = pTrain->pev->angles - pos->vecAngles;
			if( angleDiff != g_vecZero )
			{
				float length = pos->vecTrainOffset.Length();
				Vector newAngles = UTIL_VecToAngles( pos->vecTrainOffset) + angleDiff;
				//newAngles[0] = -newAngles[0];
				Vector newOffset;
				UTIL_MakeVectorsPrivate( newAngles, newOffset, NULL, NULL );
				pPlayer->pev->origin = vecTrainOrigin - newOffset * length;
			}
			else
				pPlayer->pev->origin = vecTrainOrigin + pos->vecTrainOffset;
			fOriginSet = true;
		}
	}
	if( mp_coop.value && !fOriginSet )
	{
		if( !COOP_SetDefaultSpawnPosition( pPlayer ) )
			g_pGameRules->GetPlayerSpawnSpot( pPlayer );
		if( pPlayer->m_ggm.iState == STATE_POINT_SELECT )
			return false;
	}

	if( pos->fDuck )
	{
		pPlayer->pev->view_ofs.z = 12;
		pPlayer->pev->flags |= FL_DUCKING;
		UTIL_SetSize( pPlayer->pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	}
	pPlayer->pev->angles = pos->vecAngles;
	pPlayer->pev->fixangle = TRUE;
	return true;
}

extern int gmsgScoreInfo;

/*
=====================
GGM_RestoreState

Copy data from GGMPlayerState to player, update persist part
=====================
*/
bool GGM_RestoreState( CBasePlayer *pPlayer )
{
	GGMPlayerState *pState = pPlayer->m_ggm.pState;
	int i;

	if( !pState )
		return false;

	GGM_ReadPersist( pState );

	pPlayer->pev->frags = pState->t.iFrags;
	pPlayer->m_iDeaths = pState->t.iDeaths;

	// update the scores
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( ENTINDEX(pPlayer->edict()) );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
	MESSAGE_END();

	if( pState->t.flHealth < 1 )
		return false;


	if( pPlayer->m_ggm.iState != STATE_SPAWNED )
		return false;

	pPlayer->pev->armorvalue = pState->t.flBattery;
	pPlayer->pev->health = pState->t.flHealth;

	if( !GGM_RestorePosition( pPlayer, &pState->t.pos ) )
		return false;


	pPlayer->RemoveAllItems( FALSE );

	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		if( !pState->t.rgszWeapons[i][0] )
			break;

		CBasePlayerWeapon *pWeapon = (CBasePlayerWeapon*)CBaseEntity::Create(pState->t.rgszWeapons[i], pPlayer->pev->origin, pPlayer->pev->angles );

		if( !pWeapon )
			continue;

		pWeapon->pev->spawnflags |= SF_NORESPAWN;
		pWeapon->m_iDefaultAmmo = 0;
		pWeapon->m_iClip = pState->t.rgiClip[i];
		if (pPlayer->AddPlayerItem(pWeapon)) {
			pWeapon->AttachToPlayer(pPlayer);
		}
	}
	for( i = 0; i < MAX_AMMO_SLOTS; i++ )
		 pPlayer->m_rgAmmo[i] = pState->t.rgszAmmo[i];
	pPlayer->SelectItem(pState->t.szWeaponName);

	return true;
}


void ClientPutInServer( edict_t *client );

/*
=====================
GGM_PlayerSpawn

Handle state change when game wants player to spawn
return true if spawn handled here
return false to allow gamerules to handle spawn
=====================
*/
bool GGM_PlayerSpawn( CBasePlayer *pPlayer )
{
	if( mp_coop.value )
		if( COOP_PlayerSpawn( pPlayer ) )
			return true;

	if( pPlayer->m_ggm.iState == STATE_LOAD_FIX )
		return true;
	if( pPlayer->m_ggm.iState == STATE_UNINITIALIZED )
	{
		ClientPutInServer( pPlayer->edict() );
		return true;
	}

	if( ( mp_coop.value || mp_spectator.value ) && pPlayer->m_ggm.iState == STATE_CONNECTED || !pPlayer->m_ggm.pState  )
	{
		if( !pPlayer->m_ggm.pState )
			GGM_ChatPrintf( pPlayer, "This nickname busy! Please login or change nickname\n" );

		pPlayer->m_ggm.iState = STATE_SPECTATOR_BEGIN;
		pPlayer->RemoveAllItems( TRUE );
		UTIL_BecomeSpectator( pPlayer );
		if( !COOP_SetDefaultSpawnPosition( pPlayer ) )
			g_pGameRules->GetPlayerSpawnSpot( pPlayer );
		return true;
	}

	if( mp_coop.value && pPlayer->m_ggm.iState == STATE_POINT_SELECT && !(pPlayer->pev->flags & FL_SPECTATOR) )
	{
		pPlayer->RemoveAllItems( TRUE );
		UTIL_BecomeSpectator( pPlayer );
		return true;
	}

	if( pPlayer->pev->flags & FL_SPECTATOR )
		return true;

	if( mp_coop.value )
	{
		if( pPlayer->m_ggm.iState != STATE_SPAWNED )
		{
			pPlayer->m_ggm.iState = STATE_SPAWNED;
			g_fPause = false;
			if( GGM_RestoreState( pPlayer ) )
			{
				pPlayer->pev->weapons |= (1 << WEAPON_SUIT);
				return true;
			}
			else
			{
				/*edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
				pPlayer->pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
				pPlayer->pev->v_angle  = g_vecZero;
				pPlayer->pev->velocity = g_vecZero;
				pPlayer->pev->angles = VARS( pentSpawnSpot )->angles;
				pPlayer->pev->punchangle = g_vecZero;
				pPlayer->pev->fixangle = TRUE;
				if( !(pPlayer->pev->flags & FL_SPECTATOR ) )
				if( !UTIL_CoopGetSpawnPoint( &pPlayer->pev->origin, &pPlayer->pev->angles ) )
				{
					ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Server cannot select a spawnpoint\nplease fly to it manually\nand press attack button");
					pPlayer->m_afButtonPressed = 0;
					if(pPlayer->pev->origin.Length() > 8192)
						pPlayer->pev->origin = g_vecZero;
					pPlayer->gravgunmod_data.m_state = STATE_POINT_SELECT;
					pPlayer->m_afButtonPressed = 0;
				}*/
			}
		}
		else
		{

		}
		if( !COOP_SetDefaultSpawnPosition( pPlayer ) )
			g_pGameRules->GetPlayerSpawnSpot( pPlayer );
	}
	else
	{
		if( pPlayer->m_ggm.iState != STATE_SPAWNED )
		{
			pPlayer->m_ggm.iState = STATE_SPAWNED;
			g_fPause = false;
			if( GGM_RestoreState( pPlayer ) )
			{
				pPlayer->pev->weapons |= (1 << WEAPON_SUIT);
				return true;
			}
			else return false;
		}
	}

	g_fPause = false;

	return pPlayer->m_ggm.iState != STATE_SPAWNED;
}

/*
=====================
GGM_Logout

Remove login record. This does not free state
just unlogs current nickname
=====================
*/
void GGM_Logout( CBasePlayer *pPlayer )
{
	struct GGMLogin *pLogin, *pPrevLogin = NULL;
	const char *uid = GGM_GetAuthID( pPlayer );
	const char *name = STRING( pPlayer->pev->netname );
	char path[64] = "";

	// unlink from list and free
	for( pLogin = login_list; pLogin; pLogin = pLogin->pNext )
	{
		if( strcmp( uid, pLogin->f.szUID ) || strcmp( name, pLogin->f.szName ) )
		{
			pPrevLogin = pLogin;
			continue;
		}

		if( pLogin == login_list )
		{
			login_list = login_list->pNext;
			free( pLogin );
			break;
		}

		if( pPrevLogin )
			pPrevLogin->pNext = pLogin->pNext;
		free( pLogin );

		break;
	}

	if( pPlayer->m_ggm.iState == STATE_SPAWNED )
		GGM_SaveState( pPlayer );

	pPlayer->m_ggm.pState = GGM_GetState(uid, name);
	GGM_RestoreState( pPlayer );

	// remove login record
	if( !GGM_FilterFileName( uid ) || !GGM_FilterFileName( name ) )
		snprintf( path, 63,  "%s/ggm/logins/%d.%d", gamedir, GGM_HashString( uid ), GGM_HashString( name ) );
	else
		snprintf( path, 63, "%s/ggm/logins/%s.%s", gamedir, uid, name );
	remove(path);
}

/*
=====================
GGM_FreeState

Free temporary anonymous state
Used on login or register
=====================
*/
void GGM_FreeState( const char *uid )
{
	struct GGMPlayerState *pState, *pPrevState = NULL;
	CBasePlayer *pPlayer;

	// unlink from all anonymous players
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if( pPlayer && pPlayer->IsPlayer() && pPlayer->m_ggm.pState )
		{
			if( !pPlayer->m_ggm.pState->fRegistered && !strcmp( uid, pPlayer->m_ggm.pState->szUID ) )
				pPlayer->m_ggm.pState = NULL;
		}
	}

	// unlink from list and free
	for( pState = anonymous_list; pState; pState = pState->pNext )
	{
		if( strcmp( uid, pState->szUID ) )
		{
			pPrevState = pState;
			continue;
		}

		if( pState == anonymous_list )
		{
			anonymous_list = anonymous_list->pNext;
			free( pState );
			return;
		}

		if( pPrevState )
			pPrevState->pNext = pState->pNext;
		free( pState );

		return;
	}
}

/*
=====================
GGM_CheckUserName

Login record must be valid filename
=====================
*/
bool GGM_CheckUserName( CBasePlayer *pPlayer, const char *name, bool exist )
{
	int len = strlen( name );

	if( len < 3 )
	{
		GGM_ChatPrintf( pPlayer, "Name %s too short (2 characters min)!\n", name );
		return false;
	}

	if( len > 31 )
	{
		GGM_ChatPrintf( pPlayer, "Name %s too long (31 characters max)!\n", name );
		return false;
	}

	if( !GGM_FilterFileName( name ) )
	{
		GGM_ChatPrintf( pPlayer, "Name %s contains bad characters!\n", name );
		return false;
	}

	if( exist && GGM_GetRegistration( name ) )
	{
		GGM_ChatPrintf( pPlayer, "Name %s busy!\n", name );
		return false;
	}

	return true;
}

/*
=====================
GGM_Munge

Munge password to prevent accidentally reading
=====================
*/
static void GGM_Munge( char *pStr )
{
	int len = strlen(pStr);

	for( int i = 0; i < 32; i++ )
	{
		pStr[i] ^= 'a' + len - ((i*5*len)%32);
		if( pStr[i] == 0 ) pStr[i] = 'b';
	}
}

/*
=====================
GGM_Register

Add new registration record and login automaticly
=====================
*/
void GGM_Register( CBasePlayer *pPlayer, const char *name, const char *password )
{
	struct GGMPlayerState *pState;
	struct GGMLogin *pLogin;

	if( !pPlayer || !pPlayer->m_ggm.pState )
		return;

	if( pPlayer->m_ggm.pState->t.fIsTempBanned )
		return;

	if( pPlayer->m_ggm.pState->fRegistered )
	{
		GGM_ChatPrintf( pPlayer, "Cannot register, when logged in\n" );
		return;
	}

	if( !GGM_CheckUserName( pPlayer, name, true ) )
		return;

	pState = (struct GGMPlayerState*)calloc( 1, sizeof( struct GGMPlayerState ) );
	memset( pState, 0, sizeof( struct GGMPlayerState ) );
	strncpy( pState->szUID, name, 32 );
	pState->szUID[32] = 0;
	pState->fRegistered = true;
	strncpy( pState->p.szPassword, password, 32 );
	GGM_Munge( pState->p.szPassword );
	pState->p.szPassword[32] = 0;
	pState->t = pPlayer->m_ggm.pState->t;
	pState->pNext = registered_list;
	registered_list = pState;
	GGM_WritePersist( pState );
	pLogin = (struct GGMLogin*)calloc(1, sizeof( struct GGMLogin ) );
	pLogin->pState = pState;
	strncpy( pLogin->f.szName, STRING(pPlayer->pev->netname ), 32 );
	strncpy( pLogin->f.szUID, pPlayer->m_ggm.pState->szUID, 32 );
	pLogin->pNext = login_list;
	login_list = pLogin;
	GGM_WriteLogin( pLogin );
	GGM_FreeState( pPlayer->m_ggm.pState->szUID );
	pPlayer->m_ggm.pState = pState;
	GGM_ChatPrintf( pPlayer, "Successfully registered as %s!\n", name );
}

/*
=====================
GGM_RegName_f
=====================
*/
void GGM_RegName_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() != 2 )
		return;

	if( !GGM_CheckUserName( pPlayer, CMD_ARGV(1), true ) )
		return;

	strncpy( pPlayer->m_ggm.fRegisterInput, CMD_ARGV(1), 31 );

	CLIENT_COMMAND( pPlayer->edict(), "messagemode reg_Password\n");
}

/*
=====================
GGM_Register_f
=====================
*/
void GGM_Register_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() == 3 )
		GGM_Register( pPlayer, CMD_ARGV(1), CMD_ARGV(2) );
	else if( CMD_ARGC() == 2 )
		GGM_RegName_f(pPlayer);
	else
		CLIENT_COMMAND( pPlayer->edict(), "messagemode reg_Name\n");

}

/*
=====================
GGM_RegPassword_f
=====================
*/
void GGM_RegPassword_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() != 2 )
		return;

	GGM_Register( pPlayer, pPlayer->m_ggm.fRegisterInput, CMD_ARGV(1) );
}

/*
=====================
GGM_Login

Auth user and create login record
=====================
*/
void GGM_Login( CBasePlayer *pPlayer, const char *name, const char *password )
{
	struct GGMPlayerState *pState;
	struct GGMLogin *pLogin;
	char mpassword[33] = "";

	if( !pPlayer )
		return;

	if( pPlayer->m_ggm.pState && pPlayer->m_ggm.pState->fRegistered )
	{
		GGM_ChatPrintf( pPlayer, "Cannot login, already logged in\n" );
		return;
	}

	if( !GGM_CheckUserName( pPlayer, name, false ) )
		return;

	pState = GGM_GetRegistration( name );

	if( !pPlayer->m_ggm.pState )
	{
		for( pLogin = login_list; pLogin; pLogin = pLogin->pNext )
		{
			if( !strcmp( pLogin->f.szName, STRING(pPlayer->pev->netname ) ) )
			{
				if( pState == pLogin->pState ) // same person
					break;
				else
				{
					GGM_ChatPrintf( pPlayer, "Other user logged in with this name!\n" );
					return;
				}

			}
		}
	}

	strncpy( mpassword, password, 32 );
	GGM_Munge( mpassword );

	if( !pState || strncmp( mpassword, pState->p.szPassword, 32 ) )
	{
		GGM_ChatPrintf( pPlayer, "Login failed!\n" );
		return;
	}
	pLogin = (struct GGMLogin*)calloc(1, sizeof( struct GGMLogin ) );
	pLogin->pState = pState;
	strncpy( pLogin->f.szName, STRING(pPlayer->pev->netname ), 32 );
	strncpy( pLogin->f.szUID, GGM_GetAuthID(pPlayer), 32 );
	pLogin->pNext = login_list;
	login_list = pLogin;
	GGM_WriteLogin( pLogin );
	if( pPlayer->m_ggm.pState )
		GGM_FreeState( pPlayer->m_ggm.pState->szUID );
	pPlayer->m_ggm.pState = pState;
	GGM_ChatPrintf( pPlayer, "Successfully logged in as %s\n", name );

	GGM_RestoreState( pPlayer );
}

/*
=====================
GGM_LoginPassword_f
=====================
*/
void GGM_LoginPassword_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() != 2 )
		return;

	GGM_Login( pPlayer, pPlayer->m_ggm.fRegisterInput, CMD_ARGV(1) );
}

/*
=====================
GGM_LoginName_f
======================
*/
void GGM_LoginName_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() != 2 )
		return;

	if( !GGM_CheckUserName( pPlayer, CMD_ARGV(1), false ) )
		return;

	strncpy( pPlayer->m_ggm.fRegisterInput, CMD_ARGV(1), 31 );

	CLIENT_COMMAND( pPlayer->edict(), "messagemode login_Password\n");
}

/*
=====================
GGM_Login_f
=====================
*/
void GGM_Login_f( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;

	if( CMD_ARGC() == 3 )
		GGM_Login( pPlayer, CMD_ARGV(1), CMD_ARGV(2) );
	else if( CMD_ARGC() == 2 )
		GGM_LoginName_f(pPlayer);
	else
		CLIENT_COMMAND( pPlayer->edict(), "messagemode login_Name\n");
}

/*
=====================
GGM_ChangePassword_f
=====================
*/
void GGM_ChangePassword_f( CBasePlayer *pPlayer )
{
	if( !pPlayer || !pPlayer->m_ggm.pState || !pPlayer->m_ggm.pState->fRegistered )
		return;

	if( !pPlayer->m_ggm.pState->fRegistered )
	{
		GGM_ChatPrintf( pPlayer, "Cannot register, when logged in\n" );
		return;
	}
	else if( CMD_ARGC() == 2 )
	{
		strncpy( pPlayer->m_ggm.pState->p.szPassword, CMD_ARGV(1), 32 );
		GGM_Munge( pPlayer->m_ggm.pState->p.szPassword );
		GGM_WritePersist( pPlayer->m_ggm.pState );
	}
	else
		CLIENT_COMMAND( pPlayer->edict(), "messagemode New_Password\n");
}


/*
===============================

CMD_ARGV replacement

We do not have access to engine's COM_TokenizeString,
so perform all parsing here, allowing to change CMD_ARGV result

===============================
*/


// CMD_ARGS replacement
namespace GGM
{
static int		cmd_argc;
static char *cmd_args;
static char		*cmd_argv[80];

static char *COM_ParseFile( char *data, char *token )
{
	int	c, len;

	if( !token )
		return NULL;

	len = 0;
	token[0] = 0;

	if( !data )
		return NULL;
// skip whitespace
skipwhite:
	while(( c = ((byte)*data)) <= ' ' )
	{
		if( c == 0 )
			return NULL;	// end of file;
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
			c = (byte)*data;

			// unexpected line end
			if( !c )
			{
				token[len] = 0;
				return data;
			}
			data++;

			if( c == '\\' && *data == '"' )
			{
				token[len++] = *data++;
				continue;
			}

			if( c == '\"' )
			{
				token[len] = 0;
				return data;
			}
			token[len] = c;
			len++;
		}
	}

	// parse single characters
	if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
	{
		token[len] = c;
		len++;
		token[len] = 0;
		return data + 1;
	}

	// parse a regular word
	do
	{
		token[len] = c;
		data++;
		len++;
		c = ((byte)*data);

		if( c == '{' || c == '}' || c == ')' || c == '(' || c == '\'' || c == ',' )
			break;
	} while( c > 32 );

	token[len] = 0;

	return data;
}

static void Cmd_TokenizeString( const char *text )
{
	char	cmd_token[256];
	int	i;

	// clear the args from the last string
	for( i = 0; i < cmd_argc; i++ )
	{
		delete [] cmd_argv[i];
		cmd_argv[i] = 0;
	}

	cmd_argc = 0; // clear previous args
	cmd_args = NULL;

	g_fCmdUsed = true;

	if( !text ) return;

	while( 1 )
	{
		// skip whitespace up to a /n
		while( *text && ((byte)*text) <= ' ' && *text != '\r' && *text != '\n' )
			text++;

		if( *text == '\n' || *text == '\r' )
		{
			// a newline seperates commands in the buffer
			if( *text == '\r' && text[1] == '\n' )
				text++;
			text++;
			break;
		}

		if( !*text )
			return;

		if( cmd_argc == 1 )
			 cmd_args = (char*)text;
		text = COM_ParseFile( (char*)text, cmd_token );
		if( !text ) return;

		if( cmd_argc < 80 )
		{
			cmd_argv[cmd_argc] = new char[strlen(cmd_token)+1];
			strcpy(cmd_argv[cmd_argc], cmd_token);
			cmd_argc++;
		}
	}
}

static void Cmd_Reset( void )
{
	g_fCmdUsed = false;
}

}
/*
=====================
Command wrappers
When g_fCmdUsed set, use internal
arguments istead of engine calls
Used for GGM_Say and internal menu processing
=====================
*/
extern "C" int CMD_ARGC()
{
	if( g_fCmdUsed )
	{
		return GGM::cmd_argc;
	}
	return g_engfuncs.pfnCmd_Argc();
}

extern "C" const char *CMD_ARGS()
{
	if( g_fCmdUsed )
	{
		if(!GGM::cmd_args)
			return "";
		return GGM::cmd_args;
	}
	return g_engfuncs.pfnCmd_Args();
}
extern "C" const char *CMD_ARGV( int i )
{
	if( g_fCmdUsed )
	{
		if( i < 0 || i >= GGM::cmd_argc|| !GGM::cmd_argv[i] )
			return "";
		return GGM::cmd_argv[i];
	}
	return g_engfuncs.pfnCmd_Argv( i );
}



// client.cpp
void ClientCommand( edict_t *pEntity );
/*
=====================
GGM_Say

Redirect chat commands to ClientCommand
This allows say /command as alias to command
=====================
*/
void GGM_Say( edict_t *pEntity )
{
	const char *args = CMD_ARGS();

	if( !args || strlen( args ) < 2 )
		return;

	if( args[1] != '/' )
		return;

	GGM::Cmd_TokenizeString( args + 2 );
	ClientCommand( pEntity );
	GGM::Cmd_Reset();
}

/*
=====================
GGM_ClearHelpMessage

Hide help message when hiding menu
=====================
*/
bool GGM_ClearHelpMessage( CBasePlayer *pPlayer )
{

	hudtextparms_t params;

	params.x = 0.5, params.y = 0.3;
	params.fadeinTime = params.fadeoutTime = 0;
	params.holdTime = 0;
	int color2 = 0xFF0000FF, color1 = 0x00FF00FF;

	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = 7;
	params.effect = 0;

	UTIL_HudMessage( pPlayer, params, "" );

	return true;
}

/*
=====================
GGM_PlayerMenu

Build and show menu
You may use method chunks
=====================
*/

GGM_PlayerMenu &GGM_PlayerMenu::Add(const char *name, const char *command)
{
	if( m_fShow )
		return *this;

	if( m_iCount > 4 )
	{
		ALERT( at_error, "GGM_PlayerMenu::Add: Only 5 menu items supported\n" );
		return *this;
	}

	strncpy( m_rgItems[m_iCount].szName, name, sizeof(m_rgItems[m_iCount].szName) - 1 );
	strncpy( m_rgItems[m_iCount].szCommand, command, sizeof(m_rgItems[m_iCount].szCommand) - 1 );
	m_iCount++;
	return *this;
}
GGM_PlayerMenu &GGM_PlayerMenu::Clear( void )
{
	m_fShow = false;
	m_iCount = 0;
	return *this;
}

GGM_PlayerMenu &GGM_PlayerMenu::SetTitle( const char *title )
{
	if( m_fShow )
		return *this;
	strncpy( m_sTitle, title, sizeof(m_sTitle) - 1);
	return *this;
}
GGM_PlayerMenu &GGM_PlayerMenu::New( const char *title, bool force )
{
	if( m_fShow && !force )
		return *this;

	m_fShow = false;

	SetTitle(title);
	return Clear();
}
extern int gmsgShowMenu;
void GGM_PlayerMenu::Show()
{
	if( !m_pPlayer )
		return;
	if( m_pPlayer->m_ggm.fTouchMenu)
	{
		char buf[256];

		#define MENU_STR(VAR) (#VAR)
		sprintf( buf, MENU_STR(slot10\ntouch_hide _sm*\ntouch_show _sm\ntouch_addbutton "_smt" "#%s" "" 0.16 0.11 0.41 0.3 0 255 0 255 78 1.5\n), m_sTitle);

		if( m_pPlayer )
			CLIENT_COMMAND( m_pPlayer->edict(), buf);

		for( int i = 0; i < m_iCount; i++ )
		{
			sprintf( buf, MENU_STR(touch_settexture _sm%d "#%d. %s"\ntouch_show _sm%d\n), i+1, i+1, m_rgItems[i].szName, i + 1 );

			if( m_pPlayer )
				CLIENT_COMMAND( m_pPlayer->edict(), buf);
		}
	}
	else
	{
		char buf[128], *pbuf = buf;
		short int flags = 1<<9;

		pbuf += sprintf( pbuf, "^2%s:\n", m_sTitle );

		for( int i = 0; i < m_iCount; i++ )
		{
			pbuf += sprintf( pbuf, "^3%d.^7 %s\n", i+1, m_rgItems[i].szName);
			flags |= 1<<i;
		}

		MESSAGE_BEGIN( MSG_ONE, gmsgShowMenu, NULL, m_pPlayer->pev );
		WRITE_SHORT( flags ); // slots
		WRITE_CHAR( 255 ); // show time
		WRITE_BYTE( 0 ); // need more
		WRITE_STRING( buf );
		MESSAGE_END();
	}

	m_fShow = true;
}

bool GGM_PlayerMenu::MenuSelect( int select )
{
	m_fShow = false;

	if( select > m_iCount || select < 1 )
		return false;


	if( !m_rgItems[select-1].szCommand[0] )
	{
		// cancel menu item
		GGM_ClearHelpMessage( m_pPlayer );
		return true;
	}

	GGM::Cmd_TokenizeString( m_rgItems[select-1].szCommand );
	ClientCommand( m_pPlayer->edict() );
	GGM::Cmd_Reset();

	return true;
}

/*
=====================
GGM_MenuCommand

Show menu defined in file
=====================
*/
bool GGM_MenuCommand( CBasePlayer *player, const char *name )
{
	char buf[256] = "ggm/menus/";
	char *file, *pFile;

	if( !GGM_FilterFileName( name ) )
		return false;

	strncat( buf, name, sizeof(buf) - 1 );

	file = pFile = (char*)LOAD_FILE_FOR_ME( buf, NULL );

	if( !file )
		return false;

	pFile = GGM::COM_ParseFile( pFile, buf );

	// no title
	if( !pFile )
	{
		FREE_FILE( file );
		return false;
	}

	GGM_PlayerMenu &m = player->m_ggm.menu.New( buf );

	while( pFile = GGM::COM_ParseFile( pFile, buf ) )
	{
		char cmd[256];
		if( !(pFile = GGM::COM_ParseFile( pFile, cmd )) )
			break;

		m.Add( buf, cmd );
	}

	m.Show();
	return true;
}

// half-life special math?
#define MAX_MOTD_CHUNK	  60
#define MAX_MOTD_LENGTH   1536 // (MAX_MOTD_CHUNK * 4)
extern int gmsgMOTD;
/*
=====================
GGM_MOTDCommand

Send custom text as motd
=====================
*/
bool GGM_MOTDCommand( CBasePlayer *player, const char *name )
{
	char buf[256] = "ggm/motd/";
	char *file, *pFileList;
	int char_count = 0;

	if( !GGM_FilterFileName( name ) )
		return false;

	strncat( buf, name, sizeof(buf) - 1 );

	file = pFileList = (char*)LOAD_FILE_FOR_ME( buf, NULL );

	if( !file )
		return false;

	// Send the message of the day
	// read it chunk-by-chunk,  and send it in parts

	while( pFileList && *pFileList && char_count < MAX_MOTD_LENGTH )
	{
		char chunk[MAX_MOTD_CHUNK + 1];

		if( strlen( pFileList ) < MAX_MOTD_CHUNK )
		{
			strcpy( chunk, pFileList );
		}
		else
		{
			strncpy( chunk, pFileList, MAX_MOTD_CHUNK );
			chunk[MAX_MOTD_CHUNK] = 0;		// strncpy doesn't always append the null terminator
		}

		char_count += strlen( chunk );
		if( char_count < MAX_MOTD_LENGTH )
			pFileList = file + char_count;
		else
			*pFileList = 0;

		MESSAGE_BEGIN( MSG_ONE, gmsgMOTD, NULL, player->edict() );
			WRITE_BYTE( *pFileList ? FALSE : TRUE );	// FALSE means there is still more message to come
			WRITE_STRING( chunk );
		MESSAGE_END();
	}

	FREE_FILE(file);
	return true;
}

/*
=====================
GGM_HelpCommand

Send help messages which will show parallel to menu
=====================
*/
bool GGM_HelpCommand( CBasePlayer *pPlayer, const char *name )
{
	char buf[256] = "ggm/help/";
	char *file, *pFileList;
	int char_count = 0;

	if( !pPlayer )
		return false;

	if( !GGM_FilterFileName( name ) )
		return false;

	hudtextparms_t params;

	strncat( buf, name, sizeof(buf) - 1 );

	file = (char*)LOAD_FILE_FOR_ME( buf, NULL );

	if( !file )
		return false;
	params.x = 0.5, params.y = 0.3;
	params.fadeinTime = params.fadeoutTime = 0.05f;
	params.holdTime = 30;
	int color2 = 0xFF0000FF, color1 = 0x00FF00FF;

	params.r1 = (color1 >> 24) & 0xFF, params.g1 = (color1 >> 16) & 0xFF, params.b1 = (color1 >> 8) & 0xFF, params.a1 = color1 & 0xFF;
	params.r2 = (color2 >> 24) & 0xFF, params.g2 = (color2 >> 16) & 0xFF, params.b2 = (color2 >> 8) & 0xFF, params.a2 = color2 & 0xFF;
	params.channel = 7;
	params.effect = 2;
	char *pstr = file;

	// set line breaks
	for( int i = 0; *pstr; pstr++,i++ )
	{
		if( *pstr == '\n' )
			i = 0;
		if( i >= 79 )
			*pstr = '\n', i = 0;
	}

	UTIL_HudMessage( pPlayer, params, file );

	FREE_FILE(file);
	return true;
}

/*
=====================
GGM_InitialMenus

When touch menu loading done, or it not needed, send initial menu to client
=====================
*/
void GGM_InitialMenus( CBasePlayer *pPlayer )
{
	pPlayer->m_ggm.fTouchLoading = false;

	if( !GGM_MenuCommand( pPlayer, "init" ) && mp_coop.value )
		pPlayer->m_ggm.menu.New( "COOP SERVER" )
				.Add("Join coop", "joincoop")
				.Add("Spectate", "spectate")
				.Show();

	GGM_HelpCommand( pPlayer, "init" );

	if( mp_maxdecals.value >= 0 )
		CLIENT_COMMAND( pPlayer->edict(), UTIL_VarArgs("r_decals %f\n", mp_maxdecals.value ) );
}

/*
=====================
GGM_TouchCommand

Handle touch menu loading
=====================
*/
bool GGM_TouchCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	edict_t *pEntity = pPlayer->edict();

	if( FStrEq(pcmd, "tb") )
	{
		CLIENT_COMMAND( pEntity, "touch_show _sb*\n");
		return true;
	}

	if( !pPlayer->m_ggm.fTouchLoading )
		return false;

	if( FStrEq(pcmd, "m1"))
	{
		if( mp_touchmenu.value )
			CLIENT_COMMAND( pEntity,
				MENU_STR(touch_addbutton "_sm" "*black" "" 0.15 0.1 0.4 0.72 0 0 0 128 335\ntouch_addbutton "_smt" "#" "" 0.16 0.11 0.41 0.3 0 255 0 255 79 1.5\nm2\n)
				);
	}
	else if( FStrEq(pcmd, "m2"))
	{
		if( mp_touchmenu.value )
			CLIENT_COMMAND( pEntity,
				MENU_STR(touch_addbutton "_sm1" "#" "menuselect 1;touch_hide _sm*" 0.16 0.21 0.39 0.3 255 255 255 255 335 1.5\ntouch_addbutton "_sm2" "#" "menuselect 2;touch_hide _sm*" 0.16 0.31 0.39 0.4 255 255 255 255 335 1.5\nm3\n)
				);
	}
	else if( FStrEq(pcmd, "m3"))
	{
		if( mp_touchmenu.value )
			CLIENT_COMMAND( pEntity,
				MENU_STR(touch_addbutton "_sm3" "#" "menuselect 3;touch_hide _sm*" 0.16 0.41 0.39 0.5 255 255 255 255 335 1.5\ntouch_addbutton "_sm4" "#" "menuselect 4;touch_hide _sm*" 0.16 0.51 0.39 0.6 255 255 255 255 335 1.5\nm4\n)
				);
	}
	else if( FStrEq(pcmd, "m4"))
	{
		if( mp_touchmenu.value )
			CLIENT_COMMAND( pEntity,
				MENU_STR(touch_addbutton "_sm5" "#" "menuselect 5;touch_hide _sm*" 0.16 0.61 0.39 0.7 255 255 255 255 335 1.5;wait;slot10\n)
				);
		GGM_InitialMenus( pPlayer );
	}
	else return false;
	return true;
}

extern float g_flWeaponCheat;

void DumpProps(); // prop.cpp


/*
=====================
GGM_ClientCommand

Handle client commands and redirect to related functions
=====================
*/
bool GGM_ClientCommand( CBasePlayer *pPlayer, const char *pCmd )
{
	bool ret = GGM_HelpCommand( pPlayer, pCmd );

	if( FStrEq( pCmd, "menuselect" ) )
	{
		int imenu = atoi( CMD_ARGV( 1 ) );

		return pPlayer->m_ggm.menu.MenuSelect(imenu);
	}
	else if( GGM_MOTDCommand( pPlayer, pCmd ) )
		return true;
	else if( GGM_MenuCommand( pPlayer, pCmd ) )
		return true;
	else if( GGM_TouchCommand( pPlayer, pCmd ) )
		return true;
	else if( GGM_VoteProcess( pPlayer, pCmd ) )
		return true;
	else if( FStrEq(pCmd, "dumpprops") )
	{
		if ( g_flWeaponCheat != 0.0 )
			DumpProps();
		return true;
	}
	else if( FStrEq(pCmd, "reg") )
	{
		GGM_Register_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "reg_Name") )
	{
		GGM_RegName_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "reg_Password") )
	{
		GGM_RegPassword_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "login") )
	{
		GGM_Login_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "login_Name") )
	{
		GGM_LoginName_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "login_Password") )
	{
		GGM_LoginPassword_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "New_Password") || FStrEq(pCmd, "chpwd") )
	{
		GGM_ChangePassword_f(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "qsave") )
	{
		GGM_SaveState( pPlayer );
		return true;
	}
	else if( FStrEq(pCmd, "qload") )
	{
		GGM_RestoreState( pPlayer );
		return true;
	}
	else if( FStrEq(pCmd, "ent_import" ) )
	{
		if( !pPlayer->m_ggm.pState || !pPlayer->m_ggm.pState->fRegistered )
			return false;

		Ent_ChangeOwner( GGM_GetAuthID(pPlayer), NULL, pPlayer->m_ggm.pState->szUID, 1, 2 );
		return true;
	}
	else if( FStrEq(pCmd, "logout") )
	{
		GGM_Logout(pPlayer);
		return true;
	}
	else if( FStrEq(pCmd, "client") )
	{
		char args[256] = {0};
		strncpy(args, CMD_ARGS(),254);
		strcat(args,"\n");
		CLIENT_COMMAND( pPlayer->edict(), args );
		return true;
	}
	else if( COOP_ClientCommand( pPlayer->edict() ) )
		return true;
	else if( Ent_ProcessClientCommand( pPlayer->edict() ) )
		return true;

	return ret;

}

/*
=====================
GGM_CvarValue2

Handle touch_enable cvar request
=====================
*/
void GGM_CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value )
{
	if( pEnt && requestID == 111  && FStrEq( cvarName , "touch_enable" ) && atoi( value) )
	{
		CBasePlayer *player = (CBasePlayer * ) CBaseEntity::Instance( (edict_t*)pEnt );
		player->m_ggm.fTouchMenu = !!atof( value );

		CLIENT_COMMAND((edict_t*)pEnt, "m1\n");
		player->m_ggm.fTouchLoading = true;

		const char *name = NULL, *command = NULL;
		if( mp_coop.value )
			name = "COOP MENU", command = "coopmenu";
		else if( mp_touchname.string[0] && mp_touchcommand.string[0] )
			name = mp_touchname.string, command = mp_touchcommand.string;
		else
			return;
		CLIENT_COMMAND( (edict_t*)pEnt,
			UTIL_VarArgs(MENU_STR(touch_addbutton "_sb" "*black" "%s;touch_hide _sb*;tb" 0 0.05 0.15 0.11 0 0 0 128 334\ntouch_addbutton "_sbt" "#%s" "" 0 0.05 0.16 0.11 255 255 127 255 78 2\n	), command, name ));


	}

}
#include "com_model.h"
//==================
// error.mdl stuff
//==================
/*
=====================
SET_MODEL wrapper

set fallback model if SetModel failed
=====================
*/

void SET_MODEL( edict_t *e, const char *model )
{
	g_engfuncs.pfnSetModel( e, model );

	if( !mp_errormdl.value )
		return;

	if( model && model[0] && e )
	{
		if( e->v.modelindex )
		{
			model_t *mod = (model_t*)g_physfuncs.pfnGetModel(e->v.modelindex);
			if( mod )
			{
				ALERT( at_console, "SET_MODEL %s %d %x %d %x\n", model, e->v.modelindex, mod->nodes, mod->type, mod->cache.data );
				if( mod->type == mod_brush &&  !mod->nodes )
					g_engfuncs.pfnSetModel( e, mp_errormdlpath.string );
			}
			else
			{
				int index = g_engfuncs.pfnPrecacheModel(model);
				model_t *mod = (model_t*)g_physfuncs.pfnGetModel(index);
				if( !mod || mod->type == mod_brush && !mod->nodes )
					g_engfuncs.pfnSetModel( e, mp_errormdlpath.string );

				ALERT( at_console, "SET_MODEL %s %d\n", model, e->v.modelindex );
			}

		}
	}
}

/*
=====================
PRECACHE_MODEL wrapper

set fallback model if PrecacheModel failed
=====================
*/
int PRECACHE_MODEL(const char *model)
{
	int index = g_engfuncs.pfnPrecacheModel( model );

	if( !index || !mp_errormdl.value )
		return index;

	model_t *mod = (model_t*)g_physfuncs.pfnGetModel( index );
	if( !mod )
	{
		ALERT( at_console, "PRECACHE_MODEL %s %d\n", model, index );
		return g_engfuncs.pfnPrecacheModel( mp_errormdlpath.string );
	}
	else
	{
		if( mod->type == mod_brush &&  !mod->nodes )
			return g_engfuncs.pfnPrecacheModel( mp_errormdlpath.string );
	}

	return index;
}

void GGM_Pause_f( void )
{
	g_fPause ^= true;
}

extern int gmsgSayText;

void GGM_SayText( const char *line )
{
	char text[254] = {};

	snprintf( text, sizeof( text ), "%s\n", line );

	MESSAGE_BEGIN( MSG_ALL, gmsgSayText, NULL );
	WRITE_BYTE( ENTINDEX(0) );
	WRITE_STRING( text );
	MESSAGE_END();
}

void GGM_SayText_f( void )
{
	GGM_SayText(CMD_ARGS());
}

int GGM_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	if( !strncmp( args, "ggm_chat ", 9 ) )
	{
		GGM_SayText( args + 9 );
		*response_buffer_size = 0;
		return 1;
	}
	return 0;
}

/*
=====================
GGM_RegisterCVars

Call on server load
=====================
*/
void GGM_RegisterCVars( void )
{
	CVAR_REGISTER( &cvar_allow_ar2 );
	CVAR_REGISTER( &cvar_allow_gravgun );
	CVAR_REGISTER( &cvar_ar2_mp5 );
	CVAR_REGISTER( &cvar_ar2_bullets );
	CVAR_REGISTER( &cvar_ar2_balls );
	CVAR_REGISTER( &cvar_allow_bigcock );
	CVAR_REGISTER( &cvar_allow_gateofbabylon );
	CVAR_REGISTER( &cvar_wresptime );
	CVAR_REGISTER( &cvar_iresptime );
	CVAR_REGISTER( &cvar_gibtime );
	CVAR_REGISTER( &cvar_hgibcount );
	CVAR_REGISTER( &cvar_agibcount );
	CVAR_REGISTER( &mp_gravgun_players );
	CVAR_REGISTER( &mp_fixhornetbug );
	CVAR_REGISTER( &mp_fixsavetime );
	CVAR_REGISTER( &mp_checkentities );
	CVAR_REGISTER( &mp_touchmenu );
	CVAR_REGISTER( &mp_touchname );
	CVAR_REGISTER( &mp_touchcommand );
	CVAR_REGISTER( &mp_serverdistclip );
	CVAR_REGISTER( &mp_maxbmodeldist );
	CVAR_REGISTER( &mp_maxtrashdist );
	CVAR_REGISTER( &mp_maxwaterdist );
	CVAR_REGISTER( &mp_maxmonsterdist );
	CVAR_REGISTER( &mp_maxotherdist );
	CVAR_REGISTER( &mp_servercliptents );
	CVAR_REGISTER( &mp_maxtentdist );
	CVAR_REGISTER( &mp_maxdecals );
	CVAR_REGISTER( &mp_enttools_checkmodels );
	CVAR_REGISTER( &mp_errormdl );
	CVAR_REGISTER( &mp_errormdlpath );
	CVAR_REGISTER( &mp_unduck );
	CVAR_REGISTER( &mp_skipdefaults );
	CVAR_REGISTER( &mp_spectator );

	g_engfuncs.pfnAddServerCommand( "ent_rungc", Ent_RunGC_f );
	g_engfuncs.pfnAddServerCommand( "mp_lightstyle", GGM_LightStyle_f );
	g_engfuncs.pfnAddServerCommand( "ent_chown", Ent_Chown_f );
	g_engfuncs.pfnAddServerCommand( "saveplayers", GGM_SavePlayers_f );
	g_engfuncs.pfnAddServerCommand( "loadplayers", GGM_LoadPlayers_f );
	g_engfuncs.pfnAddServerCommand( "ggm_save", GGM_Save_f );
	g_engfuncs.pfnAddServerCommand( "ggm_load", GGM_Load_f );
	g_engfuncs.pfnAddServerCommand( "ggm_votecommand", GGM_VoteCommand_f );
	g_engfuncs.pfnAddServerCommand( "ggm_pause", GGM_Pause_f );
	g_engfuncs.pfnAddServerCommand( "ggm_saytext", GGM_SayText_f );

	zombietime = CVAR_GET_POINTER("zombietime");


	GET_GAME_DIR(gamedir);
}
