#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gravgunmod.h"
#include "player.h"
#include "coop_util.h"


cvar_t cvar_allow_gravgun = { "mp_allow_gravgun","1", FCVAR_SERVER };
cvar_t cvar_allow_ar2 = { "mp_allow_ar2","0", FCVAR_SERVER };
cvar_t cvar_ar2_mp5 = { "mp_ar2_mp5","0", FCVAR_SERVER };
cvar_t cvar_ar2_balls = { "mp_ar2_balls","0", FCVAR_SERVER };
cvar_t cvar_ar2_bullets = { "mp_ar2_bullets","0", FCVAR_SERVER };
cvar_t cvar_wresptime = { "mp_wresptime","20", FCVAR_SERVER };
cvar_t cvar_iresptime = { "mp_iresptime","30", FCVAR_SERVER };
cvar_t cvar_gibtime = { "mp_gibtime","250", FCVAR_SERVER };
cvar_t cvar_hgibcount = { "mp_hgibcount","12", FCVAR_SERVER };
cvar_t cvar_agibcount = { "mp_agibcount","8", FCVAR_SERVER };
cvar_t mp_gravgun_players = { "mp_gravgun_players", "0", FCVAR_SERVER };

cvar_t mp_fixhornetbug = { "mp_fixhornetbug", "0", FCVAR_SERVER };
cvar_t mp_checkentities = { "mp_checkentities", "0", FCVAR_SERVER };
cvar_t mp_touchmenu = { "mp_touchmenu", "1", FCVAR_SERVER };

void Ent_RunGC_f( void );

void GGM_RegisterCVars( void )
{
	CVAR_REGISTER( &cvar_allow_ar2 );
	CVAR_REGISTER( &cvar_allow_gravgun );
	CVAR_REGISTER( &cvar_ar2_mp5 );
	CVAR_REGISTER( &cvar_ar2_bullets );
	CVAR_REGISTER( &cvar_ar2_balls );
	CVAR_REGISTER( &cvar_wresptime );
	CVAR_REGISTER( &cvar_iresptime );
	CVAR_REGISTER( &cvar_gibtime );
	CVAR_REGISTER( &cvar_hgibcount );
	CVAR_REGISTER( &cvar_agibcount );
	CVAR_REGISTER( &mp_gravgun_players );
	CVAR_REGISTER( &mp_fixhornetbug );
	CVAR_REGISTER( &mp_checkentities );
	CVAR_REGISTER( &mp_touchmenu );
	g_engfuncs.pfnAddServerCommand( "ent_rungc", Ent_RunGC_f );
}

void Ent_RunGC( bool common, bool enttools, const char *userid )
{
	int i, count = 0, removed = 0;
	edict_t *ent = g_engfuncs.pfnPEntityOfEntIndex( gpGlobals->maxClients + 5 );

	ALERT( at_warning, "Running garbage collector\n" );

	for( i = gpGlobals->maxClients; i < gpGlobals->maxEntities; i++, ent++ )
	{
		const char *classname = STRING( ent->v.classname );

		if( ent->free )
			continue;

		if( !classname || !ent->v.classname || !classname[0] )
			continue;

		count++;

		if( ent->v.flags & FL_KILLME )
			continue;

		if( common )
		{
			if( !strcmp( classname, "gib" ) )
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
		if( !enttools )
		{
			if( strncmp( classname, "monster_", 8 ) || strncmp( classname, "weapon_", 7 ) || strncmp( classname, "ammo_", 5 ) )
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

		if( enttools && entity->enttools_data.enttools )
		{
			if( !userid || !strcmp( userid, entity->enttools_data.ownerid ) )
			{
				ent->v.flags |= FL_KILLME;
				removed++;
				continue;
			}
		}

		else if( common && !entity->IsInWorld() )
		{
			ent->v.flags |= FL_KILLME;
			removed++;
			continue;
		}
	}

	ALERT( at_notice, "Total %d entities, %d cleaned\n", count, removed );

}

void Ent_RunGC_f()
{
	int enttools = atoi(CMD_ARGV(1));
	Ent_RunGC( !enttools, enttools, NULL );
}

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
			Ent_RunGC( true, true, NULL );
			return 1;
		}

		if( gpGlobals->maxEntities - index < 100 )
		{
			if( !strncmp( STRING(pent->v.classname), "env_", 4) )
				return 1;

			if( !strcmp( STRING(pent->v.classname), "gib" ) )
				return 1;


			Ent_RunGC( true, false, NULL );

			return 0;
		}

		if( index > gpGlobals->maxEntities / 2 && counter - lastgc > 64 )
		{
			lastgc = counter;
			Ent_RunGC( true, false, NULL );
			return 0;
		}
	}

	return 0;
}


void GGM_ClientPutinServer(edict_t *pEntity, CBasePlayer *pPlayer)
{
	if( mp_touchmenu.value && pPlayer->gravgunmod_data.m_state == STATE_UNINITIALIZED )
		g_engfuncs.pfnQueryClientCvarValue2( pEntity, "touch_enable", 111 );

	pPlayer->gravgunmod_data.m_state = STATE_CONNECTED;

	const char *uid = GETPLAYERAUTHID( pPlayer->edict() );
	if( !uid || strstr(uid, "PENDING") )
		uid = g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "ip" );

	strncpy( pPlayer->gravgunmod_data.uid, uid, 32 );
	pPlayer->gravgunmod_data.uid[32] = 0;
	pPlayer->gravgunmod_data.m_flEntTime = 0;
	pPlayer->gravgunmod_data.m_flEntScope = 0;
	pPlayer->gravgunmod_data.menu.pPlayer = pPlayer;
	pPlayer->gravgunmod_data.menu.Clear();
}

void GGM_ClientFirstSpawn(CBasePlayer *pPlayer)
{
	// AGHL-like spectator
	if( mp_spectator.value )
	{
		pPlayer->RemoveAllItems( TRUE );
		UTIL_BecomeSpectator( pPlayer );
	}

}

edict_t *GGM_PlayerByID( const char *id )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );

		if( plr && plr->IsPlayer() )
		{
			CBasePlayer *player = (CBasePlayer *) plr;

			if( !strcmp( player->gravgunmod_data.uid, id ) )
				return player->edict();
		}
	}

	return NULL;
}

const char *GGM_GetPlayerID( edict_t *player )
{
	CBasePlayer *plr = (CBasePlayer*)CBaseEntity::Instance( player );

	if( !plr->IsPlayer() )
		return NULL;

	return plr->gravgunmod_data.uid;
}

/*
===============================

CMD_ARGV replacement

We do not have access to engine's COM_TokenizeString,
so perform all parsing here, allowing to change CMD_ARGV result

===============================
*/

bool cmd_used;
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

	cmd_used = true;

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
	cmd_used = false;
}

}

extern "C" int CMD_ARGC()
{
	if( cmd_used )
	{
		return GGM::cmd_argc;
	}
	return g_engfuncs.pfnCmd_Argc();
}

extern "C" const char *CMD_ARGS()
{
	if( cmd_used )
	{
		if(!GGM::cmd_args)
			return "";
		return GGM::cmd_args;
	}
	return g_engfuncs.pfnCmd_Args();
}
extern "C" const char *CMD_ARGV( int i )
{
	if( cmd_used )
	{
		if( i < 0 || i >= GGM::cmd_argc|| !GGM::cmd_argv[i] )
			return "";
		return GGM::cmd_argv[i];
	}
	return g_engfuncs.pfnCmd_Argv( i );
}



GGM_PlayerMenu &GGM_PlayerMenu::Add(const char *name, const char *command)
{
	if( m_iCount > 4 )
	{
		ALERT( at_error, "GGM_PlayerMenu::Add: Only 5 menu items supported" );
		return *this;
	}

	strncpy( m_items[m_iCount].name, name, sizeof(m_items[m_iCount].name) - 1 );
	strncpy( m_items[m_iCount].command, command, sizeof(m_items[m_iCount].command) - 1 );
	m_iCount++;
	return *this;
}
GGM_PlayerMenu &GGM_PlayerMenu::Clear()
{
	m_iCount = 0;
	return *this;
}

GGM_PlayerMenu &GGM_PlayerMenu::SetTitle(const char *title)
{
	strncpy( m_sTitle, title, sizeof(m_sTitle) - 1);
	return *this;
}
GGM_PlayerMenu &GGM_PlayerMenu::New(const char *title)
{
	SetTitle(title);
	return Clear();
}
extern int gmsgShowMenu;
void GGM_PlayerMenu::Show()
{
	if( pPlayer->gravgunmod_data.m_fTouchMenu)
	{
		char buf[256];
		#define MENU_STR(VAR) (#VAR)
		sprintf( buf, MENU_STR(slot10\ntouch_hide _coops*\ntouch_show _coops\ntouch_addbutton "_coopst" "#%s" "" 0.16 0.11 0.41 0.3 0 255 0 255 78 1.5\n), m_sTitle);
		CLIENT_COMMAND( pPlayer->edict(), buf);
		for( int i = 0; i < m_iCount; i++ )
		{
			sprintf( buf, MENU_STR(touch_settexture _coops%d "#%d. %s"\ntouch_show _coops%d\n), i+1, i+1, m_items[i].name, i + 1 );
			CLIENT_COMMAND( pPlayer->edict(), buf);
		}
	}
	else
	{
		char buf[128], *pbuf = buf;
		short int flags = 1<<9;
		pbuf += sprintf( pbuf, "^2%s:\n", m_sTitle );
		for( int i = 0; i < m_iCount; i++ )
		{
			pbuf += sprintf( pbuf, "^3%d.^7 %s\n", i+1, m_items[i].name);
			flags |= 1<<i;
		}
		MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pPlayer->pev);
		WRITE_SHORT( flags ); // slots
		WRITE_CHAR( 255 ); // show time
		WRITE_BYTE( 0 ); // need more
		WRITE_STRING( buf );
		MESSAGE_END();
	}
}

// client.cpp
void ClientCommand( edict_t *pEntity );

bool GGM_PlayerMenu::MenuSelect( int select )
{
	if( select > m_iCount || select < 1 )
		return false;

	GGM::Cmd_TokenizeString( m_items[select-1].command );
	ClientCommand( pPlayer->edict() );
	GGM::Cmd_Reset();
}
