//==

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "entity_state.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"
#include "voice_gamemgr.h"
#include "hostage_localnav.h"
#include <ctype.h>
#include "buildslave_assert.h"
#include "czeror/cz_music.h"
#include "czeror/cz_seqapi.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\client.cpp" )

extern int gmsgSayText;
extern int g_fGameOver;
extern int g_teamplay;
extern unsigned long g_ulFrameCount;
extern unsigned short g_usExplosion;
extern CVoiceGameMgr g_VoiceGameMgr;
extern void CopyToBodyQue( entvars_t* pev );

static int g_serveractive;

void set_suicide_frame( entvars_t* pev )
{
	if ( !FStrEq( STRING( pev->model ), "models/player.mdl" ) )
		return;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
}

BOOL ClientConnect( edict_t* entity, const char* name, const char* address, char rejectReason[128] )
{
	return g_pGameRules->ClientConnected( entity, name, address, rejectReason );
}

void ClientDisconnect( edict_t* entity )
{
	if ( g_fGameOver )
		return;

	char text[256];
	sprintf( text, "- %s has left the game\n", STRING( entity->v.netname ) );
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText );
		WRITE_BYTE( ENTINDEX( entity ) );
		WRITE_STRING( text );
	MESSAGE_END();

	CSound* sound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( entity ) );
	if ( sound )
		sound->Reset();
	entity->v.solid = SOLID_NOT;
	entity->v.takedamage = DAMAGE_NO;
	UTIL_SetOrigin( &entity->v, entity->v.origin );
	g_pGameRules->ClientDisconnected( entity );
}

void respawn( entvars_t* pev, BOOL copyCorpse )
{
	if ( !gpGlobals->coop && !gpGlobals->deathmatch )
	{
		SERVER_COMMAND( "reload\n" );
		return;
	}
	if ( copyCorpse )
		CopyToBodyQue( pev );
	GetClassPtr( (CBasePlayer*)pev )->Spawn();
}

void ClientKill( edict_t* entity )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( &entity->v );
	if ( !( player->m_fNextSuicideTime <= gpGlobals->time ) )
		return;
	player->m_fNextSuicideTime = gpGlobals->time + 1;
	entity->v.health = 0;
	player->Killed( &entity->v, GIB_NEVER );
}

void ClientPutInServer( edict_t* entity )
{
	CBasePlayer* player = GetClassPtr( (CBasePlayer*)&entity->v );
	player->SetCustomDecalFrames( -1 );
	player->m_iAccessoryBits = 0;
	player->m_iOldAccessoryBits = 0;
	player->Spawn();
	player->pev->effects |= EF_NOINTERP;
}

void Host_Say( edict_t* entity, int teamOnly )
{
	const char* command = CMD_ARGV( 0 );
	if ( CMD_ARGC() == 0 )
		return;
	CBasePlayer* sender = GetClassPtr( (CBasePlayer*)&entity->v );
	if ( !( sender->m_flNextChatTime <= gpGlobals->time ) )
		return;

	char temporary[256];
	char* message;
	if ( !stricmp( command, "say" ) || !stricmp( command, "say_team" ) )
	{
		if ( CMD_ARGC() <= 1 )
			return;
		message = (char*)CMD_ARGS();
	}
	else
	{
		if ( CMD_ARGC() > 1 )
			_snprintf( temporary, sizeof( temporary ), "%s %s", command, CMD_ARGS() );
		else
			_snprintf( temporary, sizeof( temporary ), "%s", command );
		temporary[sizeof( temporary ) - 1] = '\0';
		message = temporary;
	}
	if ( *message == '"' )
	{
		++message;
		message[strlen( message ) - 1] = '\0';
	}
	if ( !message || !*message )
		return;
	const unsigned char* character = (const unsigned char*)message;
	while ( *character && ( !isprint( *character ) || isspace( *character ) ) )
		++character;
	if ( !*character )
		return;

	char text[128];
	if ( teamOnly )
		sprintf( text, "%c(TEAM) %s: ", 2, STRING( entity->v.netname ) );
	else
		sprintf( text, "%c%s: ", 2, STRING( entity->v.netname ) );
	const size_t available = sizeof( text ) - 2 - strlen( text );
	if ( strlen( message ) > available )
		message[available] = '\0';
	strcat( text, message );
	strcat( text, "\n" );
	sender->m_flNextChatTime = gpGlobals->time + 1;

	CBaseEntity* recipient = NULL;
	while ( ( recipient = UTIL_FindEntityByClassname( recipient, "player" ) ) != NULL )
	{
		if ( FNullEnt( recipient->edict() ) )
			break;
		if ( !recipient->pev || ENT( recipient->pev ) == entity || !recipient->IsNetClient() )
			continue;
		if ( g_VoiceGameMgr.PlayerHasBlockedPlayer( (CBasePlayer*)recipient, sender ) )
			continue;
		if ( teamOnly && g_pGameRules->PlayerRelationship( (CBasePlayer*)recipient, CBaseEntity::Instance( entity ) ) != GR_TEAMMATE )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, recipient->pev );
			WRITE_BYTE( ENTINDEX( entity ) );
			WRITE_STRING( text );
		MESSAGE_END();
	}
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, &entity->v );
		WRITE_BYTE( ENTINDEX( entity ) );
		WRITE_STRING( text );
	MESSAGE_END();
	g_engfuncs.pfnServerPrint( text );

	const char* verb = teamOnly ? "say_team" : "say";
	if ( g_teamplay )
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", STRING( entity->v.netname ),
			GETPLAYERUSERID( entity ), GETPLAYERAUTHID( entity ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( entity ), "model" ), verb, message );
	}
	else
	{
		UTIL_LogPrintf( "\"%s<%i><%s><%i>\" %s \"%s\"\n", STRING( entity->v.netname ),
			GETPLAYERUSERID( entity ), GETPLAYERAUTHID( entity ), GETPLAYERUSERID( entity ), verb, message );
	}
}

void ClientCommand( edict_t* entity )
{
	const char* command = CMD_ARGV( 0 );
	if ( !entity->pvPrivateData )
		return;
	CBasePlayer* player = GetClassPtr( (CBasePlayer*)&entity->v );
	if ( FStrEq( command, "say" ) )
		Host_Say( entity, FALSE );
	else if ( FStrEq( command, "say_team" ) )
		Host_Say( entity, TRUE );
	else if ( FStrEq( command, "fullupdate" ) )
		player->ForceClientDllUpdate();
	else if ( FStrEq( command, "give" ) )
	{
		if ( CVAR_GET_FLOAT( "sv_cheats" ) != 0 )
			player->GiveNamedItem( STRING( ALLOC_STRING( CMD_ARGV( 1 ) ) ) );
	}
	else if ( FStrEq( command, "fov" ) )
	{
		if ( CVAR_GET_FLOAT( "sv_cheats" ) != 0 && CMD_ARGC() > 1 )
			player->m_iFOV = atoi( CMD_ARGV( 1 ) );
		else
			CLIENT_PRINTF( entity, print_console, UTIL_VarArgs( "\"fov\" is \"%d\"\n", player->m_iFOV ) );
	}
	else if ( FStrEq( command, "use" ) )
		player->SelectItem( CMD_ARGV( 1 ) );
	else if ( strstr( command, "weapon_" ) == command )
		player->SelectItem( command );
	else if ( FStrEq( command, "lastinv" ) )
		player->SelectLastItem();
	else if ( FStrEq( command, "spectate" ) && ( entity->v.flags & FL_PROXY ) )
	{
		edict_t* spot = g_pGameRules->GetPlayerSpawnSpot( player );
		player->StartObserver( entity->v.origin, spot->v.angles );
	}
	else if ( !g_pGameRules->ClientCommand( player, command ) )
	{
		char safeCommand[128];
		strncpy( safeCommand, command, sizeof( safeCommand ) - 1 );
		safeCommand[sizeof( safeCommand ) - 1] = '\0';
		for ( char* character = safeCommand; *character; ++character )
		{
			if ( *character == '%' )
				*character = ' ';
		}
		ClientPrint( &entity->v, HUD_PRINTCONSOLE, UTIL_VarArgs( "Unknown command: %s\n", safeCommand ) );
	}
}

void ClientUserInfoChanged( edict_t* entity, char* infoBuffer )
{
	if ( !entity->pvPrivateData )
		return;
	if ( entity->v.netname && *STRING( entity->v.netname ) &&
		strcmp( STRING( entity->v.netname ), g_engfuncs.pfnInfoKeyValue( infoBuffer, "name" ) ) )
	{
		char name[256];
		strncpy( name, g_engfuncs.pfnInfoKeyValue( infoBuffer, "name" ), sizeof( name ) - 1 );
		name[sizeof( name ) - 1] = '\0';
		for ( char* character = name; *character; ++character )
		{
			if ( *character == '%' )
				*character = ' ';
		}
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( entity ), infoBuffer, "name", name );
		char text[256];
		_snprintf( text, sizeof( text ), "* %s changed name to %s\n", STRING( entity->v.netname ), g_engfuncs.pfnInfoKeyValue( infoBuffer, "name" ) );
		text[sizeof( text ) - 1] = '\0';
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText );
			WRITE_BYTE( ENTINDEX( entity ) );
			WRITE_STRING( text );
		MESSAGE_END();
		if ( g_teamplay )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", STRING( entity->v.netname ),
				GETPLAYERUSERID( entity ), GETPLAYERAUTHID( entity ),
				g_engfuncs.pfnInfoKeyValue( infoBuffer, "model" ), g_engfuncs.pfnInfoKeyValue( infoBuffer, "name" ) );
		}
		else
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%i>\" changed name to \"%s\"\n", STRING( entity->v.netname ),
				GETPLAYERUSERID( entity ), GETPLAYERAUTHID( entity ), GETPLAYERUSERID( entity ), g_engfuncs.pfnInfoKeyValue( infoBuffer, "name" ) );
		}
	}
	g_pGameRules->ClientUserInfoChanged( GetClassPtr( (CBasePlayer*)&entity->v ), infoBuffer );
}

void ServerDeactivate()
{
	// the entities that pointed to the built sequence entries go away with the map
	CZ_SequenceReset();

	if ( g_serveractive == 1 )
		g_serveractive = 0;
}

void ServerActivate( edict_t* edictList, int edictCount, int clientMax )
{
	g_serveractive = 1;
	CLocalNav::Reset();
	CZ_MusicReset();
	for ( int index = 0; index < edictCount; ++index )
	{
		edict_t* entity = &edictList[index];
		if ( entity->free || index < clientMax || !entity->pvPrivateData )
			continue;
		CBaseEntity* instance = CBaseEntity::Instance( entity );
		if ( instance && !( instance->pev->flags & FL_DORMANT ) )
		{
			instance->Activate();
			instance->Restart();
		}
		else
			ALERT( at_console, "Can't instance %s\n", STRING( entity->v.classname ) );
	}
	LinkUserMessages();
}

void PlayerPreThink( edict_t* entity )
{
	if ( entity && entity->pvPrivateData )
		((CBasePlayer*)entity->pvPrivateData)->PreThink();
}

void PlayerPostThink( edict_t* entity )
{
	if ( entity && entity->pvPrivateData )
		((CBasePlayer*)entity->pvPrivateData)->PostThink();
}

void ParmsNewLevel()
{
}

void ParmsChangeLevel()
{
	if ( gpGlobals->pSaveData )
	{
		SAVERESTOREDATA* data = (SAVERESTOREDATA*)gpGlobals->pSaveData;
		data->connectionCount = BuildChangeList( data->levelList, MAX_LEVEL_CONNECTIONS );
	}
}

void StartFrame()
{
	if ( g_pGameRules )
		g_pGameRules->Think();
	CLocalNav::Think();
	if ( !g_fGameOver )
	{
		++g_ulFrameCount;
		gpGlobals->teamplay = teamplay.value;
	}
}

const char* sPlayerModelFiles[] = { "models/player.mdl" };

void ClientPrecache()
{
	PRECACHE_SOUND( "weapons/dryfire_pistol.wav" );
	PRECACHE_SOUND( "weapons/dryfire_rifle.wav" );
	PRECACHE_SOUND( "player/pl_shot1.wav" );
	PRECACHE_SOUND( "player/pl_die1.wav" );
	PRECACHE_SOUND( "player/headshot1.wav" );
	PRECACHE_SOUND( "player/headshot2.wav" );
	PRECACHE_SOUND( "player/headshot3.wav" );
	PRECACHE_SOUND( "player/bhit_flesh-1.wav" );
	PRECACHE_SOUND( "player/bhit_flesh-2.wav" );
	PRECACHE_SOUND( "player/bhit_flesh-3.wav" );
	PRECACHE_SOUND( "player/bhit_kevlar-1.wav" );
	PRECACHE_SOUND( "player/bhit_helmet-1.wav" );
	PRECACHE_SOUND( "player/die1.wav" );
	PRECACHE_SOUND( "player/die2.wav" );
	PRECACHE_SOUND( "player/die3.wav" );
	PRECACHE_SOUND( "player/death6.wav" );
	PRECACHE_SOUND( "radio/locknload.wav" );
	PRECACHE_SOUND( "radio/letsgo.wav" );
	PRECACHE_SOUND( "radio/moveout.wav" );
	PRECACHE_SOUND( "radio/com_go.wav" );
	PRECACHE_SOUND( "radio/rescued.wav" );
	PRECACHE_SOUND( "radio/rounddraw.wav" );
	PRECACHE_SOUND( "items/kevlar.wav" );
	PRECACHE_SOUND( "items/ammopickup2.wav" );
	PRECACHE_SOUND( "items/nvg_on.wav" );
	PRECACHE_SOUND( "items/nvg_off.wav" );
	PRECACHE_SOUND( "items/equip_nvg.wav" );
	PRECACHE_SOUND( "weapons/c4_beep1.wav" );
	PRECACHE_SOUND( "weapons/c4_beep2.wav" );
	PRECACHE_SOUND( "weapons/c4_beep3.wav" );
	PRECACHE_SOUND( "weapons/c4_beep4.wav" );
	PRECACHE_SOUND( "weapons/c4_beep5.wav" );
	PRECACHE_SOUND( "weapons/c4_explode1.wav" );
	PRECACHE_SOUND( "weapons/c4_plant.wav" );
	PRECACHE_SOUND( "weapons/c4_disarm.wav" );
	PRECACHE_SOUND( "weapons/c4_disarmed.wav" );
	PRECACHE_SOUND( "weapons/explode3.wav" );
	PRECACHE_SOUND( "weapons/explode4.wav" );
	PRECACHE_SOUND( "weapons/explode5.wav" );
	PRECACHE_SOUND( "player/sprayer.wav" );
	PRECACHE_SOUND( "player/pl_fallpain2.wav" );
	PRECACHE_SOUND( "player/pl_fallpain3.wav" );
	PRECACHE_SOUND( "player/pl_snow1.wav" );
	PRECACHE_SOUND( "player/pl_snow2.wav" );
	PRECACHE_SOUND( "player/pl_snow3.wav" );
	PRECACHE_SOUND( "player/pl_snow4.wav" );
	PRECACHE_SOUND( "player/pl_snow5.wav" );
	PRECACHE_SOUND( "player/pl_snow6.wav" );
	PRECACHE_SOUND( "player/pl_step1.wav" );
	PRECACHE_SOUND( "player/pl_step2.wav" );
	PRECACHE_SOUND( "player/pl_step3.wav" );
	PRECACHE_SOUND( "player/pl_step4.wav" );
	PRECACHE_SOUND( "common/npc_step1.wav" );
	PRECACHE_SOUND( "common/npc_step2.wav" );
	PRECACHE_SOUND( "common/npc_step3.wav" );
	PRECACHE_SOUND( "common/npc_step4.wav" );
	PRECACHE_SOUND( "player/pl_metal1.wav" );
	PRECACHE_SOUND( "player/pl_metal2.wav" );
	PRECACHE_SOUND( "player/pl_metal3.wav" );
	PRECACHE_SOUND( "player/pl_metal4.wav" );
	PRECACHE_SOUND( "player/pl_dirt1.wav" );
	PRECACHE_SOUND( "player/pl_dirt2.wav" );
	PRECACHE_SOUND( "player/pl_dirt3.wav" );
	PRECACHE_SOUND( "player/pl_dirt4.wav" );
	PRECACHE_SOUND( "player/pl_crete1.wav" );
	PRECACHE_SOUND( "player/pl_crete2.wav" );
	PRECACHE_SOUND( "player/pl_crete3.wav" );
	PRECACHE_SOUND( "player/pl_crete4.wav" );
	PRECACHE_SOUND( "player/pl_duct1.wav" );
	PRECACHE_SOUND( "player/pl_duct2.wav" );
	PRECACHE_SOUND( "player/pl_duct3.wav" );
	PRECACHE_SOUND( "player/pl_duct4.wav" );
	PRECACHE_SOUND( "player/pl_carpet1.wav" );
	PRECACHE_SOUND( "player/pl_carpet2.wav" );
	PRECACHE_SOUND( "player/pl_carpet3.wav" );
	PRECACHE_SOUND( "player/pl_carpet4.wav" );
	PRECACHE_SOUND( "player/pl_gravel1.wav" );
	PRECACHE_SOUND( "player/pl_gravel2.wav" );
	PRECACHE_SOUND( "player/pl_gravel3.wav" );
	PRECACHE_SOUND( "player/pl_gravel4.wav" );
	PRECACHE_SOUND( "player/pl_grate1.wav" );
	PRECACHE_SOUND( "player/pl_grate2.wav" );
	PRECACHE_SOUND( "player/pl_grate3.wav" );
	PRECACHE_SOUND( "player/pl_grate4.wav" );
	PRECACHE_SOUND( "player/pl_slosh1.wav" );
	PRECACHE_SOUND( "player/pl_slosh2.wav" );
	PRECACHE_SOUND( "player/pl_slosh3.wav" );
	PRECACHE_SOUND( "player/pl_slosh4.wav" );
	PRECACHE_SOUND( "player/pl_grass1.wav" );
	PRECACHE_SOUND( "player/pl_grass2.wav" );
	PRECACHE_SOUND( "player/pl_grass3.wav" );
	PRECACHE_SOUND( "player/pl_grass4.wav" );
	PRECACHE_SOUND( "player/pl_tile1.wav" );
	PRECACHE_SOUND( "player/pl_tile2.wav" );
	PRECACHE_SOUND( "player/pl_tile3.wav" );
	PRECACHE_SOUND( "player/pl_tile4.wav" );
	PRECACHE_SOUND( "player/pl_tile5.wav" );
	PRECACHE_SOUND( "player/pl_swim1.wav" );
	PRECACHE_SOUND( "player/pl_swim2.wav" );
	PRECACHE_SOUND( "player/pl_swim3.wav" );
	PRECACHE_SOUND( "player/pl_swim4.wav" );
	PRECACHE_SOUND( "player/pl_ladder1.wav" );
	PRECACHE_SOUND( "player/pl_ladder2.wav" );
	PRECACHE_SOUND( "player/pl_ladder3.wav" );
	PRECACHE_SOUND( "player/pl_ladder4.wav" );
	PRECACHE_SOUND( "player/pl_wade1.wav" );
	PRECACHE_SOUND( "player/pl_wade2.wav" );
	PRECACHE_SOUND( "player/pl_wade3.wav" );
	PRECACHE_SOUND( "player/pl_wade4.wav" );
	PRECACHE_SOUND( "player/pl_wood1.wav" );
	PRECACHE_SOUND( "player/pl_wood2.wav" );
	PRECACHE_SOUND( "player/pl_wood3.wav" );
	PRECACHE_SOUND( "player/pl_wood4.wav" );
	PRECACHE_SOUND( "debris/wood1.wav" );
	PRECACHE_SOUND( "debris/wood2.wav" );
	PRECACHE_SOUND( "debris/wood3.wav" );
	PRECACHE_SOUND( "player/pl_snow_light1.wav" );
	PRECACHE_SOUND( "player/pl_snow_light2.wav" );
	PRECACHE_SOUND( "player/pl_snow_light3.wav" );
	PRECACHE_SOUND( "player/pl_snow_light4.wav" );
	PRECACHE_SOUND( "plats/train_use1.wav" );
	PRECACHE_SOUND( "plats/vehicle_ignition.wav" );
	PRECACHE_SOUND( "buttons/spark5.wav" );
	PRECACHE_SOUND( "buttons/spark6.wav" );
	PRECACHE_SOUND( "debris/glass1.wav" );
	PRECACHE_SOUND( "debris/glass2.wav" );
	PRECACHE_SOUND( "debris/glass3.wav" );
	PRECACHE_SOUND( "items/flashlight1.wav" );
	PRECACHE_SOUND( "items/flashlight1.wav" );
	PRECACHE_SOUND( "common/bodysplat.wav" );
	PRECACHE_SOUND( "player/pl_pain2.wav" );
	PRECACHE_SOUND( "player/pl_pain4.wav" );
	PRECACHE_SOUND( "player/pl_pain5.wav" );
	PRECACHE_SOUND( "player/pl_pain6.wav" );
	PRECACHE_SOUND( "player/pl_pain7.wav" );
	PRECACHE_MODEL( (char*)sPlayerModelFiles[0] );
	PRECACHE_SOUND( "common/wpn_hudoff.wav" );
	PRECACHE_SOUND( "common/wpn_hudon.wav" );
	PRECACHE_SOUND( "common/wpn_moveselect.wav" );
	PRECACHE_SOUND( "common/wpn_select.wav" );
	PRECACHE_SOUND( "common/wpn_denyselect.wav" );
	PRECACHE_MODEL( "sprites/wall_puff1.spr" );
	PRECACHE_MODEL( "sprites/wall_puff2.spr" );
	PRECACHE_MODEL( "sprites/wall_puff3.spr" );
	PRECACHE_MODEL( "sprites/wall_puff4.spr" );
	PRECACHE_MODEL( "sprites/black_smoke1.spr" );
	PRECACHE_MODEL( "sprites/black_smoke2.spr" );
	PRECACHE_MODEL( "sprites/black_smoke3.spr" );
	PRECACHE_MODEL( "sprites/black_smoke4.spr" );
	PRECACHE_MODEL( "sprites/fast_wallpuff1.spr" );
	PRECACHE_MODEL( "sprites/pistol_smoke1.spr" );
	PRECACHE_MODEL( "sprites/pistol_smoke2.spr" );
	PRECACHE_MODEL( "sprites/rifle_smoke1.spr" );
	PRECACHE_MODEL( "sprites/rifle_smoke2.spr" );
	PRECACHE_MODEL( "sprites/rifle_smoke3.spr" );
	PRECACHE_MODEL( "sprites/bottom.spr" );
	PRECACHE_MODEL( "sprites/bottom2.spr" );
	PRECACHE_MODEL( "sprites/bottom3.spr" );
	PRECACHE_MODEL( "sprites/bottom_left.spr" );
	PRECACHE_MODEL( "sprites/bottom_left2.spr" );
	PRECACHE_MODEL( "sprites/bottom_left3.spr" );
	PRECACHE_MODEL( "sprites/bottom_right.spr" );
	PRECACHE_MODEL( "sprites/bottom_right2.spr" );
	PRECACHE_MODEL( "sprites/bottom_right3.spr" );
	PRECACHE_MODEL( "sprites/left.spr" );
	PRECACHE_MODEL( "sprites/left2.spr" );
	PRECACHE_MODEL( "sprites/left3.spr" );
	PRECACHE_MODEL( "sprites/right.spr" );
	PRECACHE_MODEL( "sprites/right2.spr" );
	PRECACHE_MODEL( "sprites/right3.spr" );
	PRECACHE_MODEL( "sprites/top.spr" );
	PRECACHE_MODEL( "sprites/top2.spr" );
	PRECACHE_MODEL( "sprites/top3.spr" );
	PRECACHE_MODEL( "sprites/top_left.spr" );
	PRECACHE_MODEL( "sprites/top_left2.spr" );
	PRECACHE_MODEL( "sprites/top_left3.spr" );
	PRECACHE_MODEL( "sprites/top_right.spr" );
	PRECACHE_MODEL( "sprites/top_right2.spr" );
	PRECACHE_MODEL( "sprites/top_right3.spr" );
	PRECACHE_MODEL( "sprites/vertical.spr" );
	PRECACHE_MODEL( "sprites/horizontal.spr" );
	PRECACHE_SOUND( "weapons/ric_metal-1.wav" );
	PRECACHE_SOUND( "weapons/ric_metal-2.wav" );
	PRECACHE_SOUND( "player/electro1.wav" );
	PRECACHE_SOUND( "player/electro2.wav" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "player/burn1.wav" );
	PRECACHE_SOUND( "player/burn2.wav" );
	PRECACHE_SOUND( "ambience/defuse_loop.wav" );
	PRECACHE_SOUND( "ambience/training_walla.wav" );
	PRECACHE_SOUND( "ambience/gate_pound.wav" );
	PRECACHE_SOUND( "ambience/door_metal_shake2.wav" );
	g_usExplosion = PRECACHE_EVENT( 1, "events/explosion.sc" );
}

const char* GetGameDescription()
{
	return g_pGameRules ? g_pGameRules->GetGameDescription() : "Counter-Strike: Condition Zero";
}

void Sys_Error( const char* )
{
}

void PlayerCustomization( edict_t* entity, customization_t* customization )
{
	CBasePlayer* player = entity ? (CBasePlayer*)entity->pvPrivateData : NULL;
	if ( !player )
	{
		ALERT( at_console, "PlayerCustomization:  Couldn't get player!\n" );
		return;
	}
	if ( !customization )
	{
		ALERT( at_console, "PlayerCustomization:  NULL customization!\n" );
		return;
	}
	switch ( customization->resource.type )
	{
	case t_sound:
	case t_skin:
	case t_model:
		break;
	case t_decal:
		player->SetCustomDecalFrames( customization->nUserData2 );
		break;
	default:
		ALERT( at_console, "PlayerCustomization:  Unknown customization type!\n" );
		break;
	}
}

void SpectatorConnect( edict_t* entity )
{
	if ( entity && entity->pvPrivateData )
		((CBaseSpectator*)entity->pvPrivateData)->SpectatorConnect();
}

void SpectatorDisconnect( edict_t* entity )
{
	if ( entity && entity->pvPrivateData )
		((CBaseSpectator*)entity->pvPrivateData)->SpectatorDisconnect();
}

void SpectatorThink( edict_t* entity )
{
	if ( entity && entity->pvPrivateData )
		((CBaseSpectator*)entity->pvPrivateData)->SpectatorThink();
}

void SetupVisibility( edict_t* viewEntity, edict_t* client, unsigned char** pvs, unsigned char** pas )
{
	edict_t* view = viewEntity ? viewEntity : client;
	if ( client->v.flags & FL_PROXY )
	{
		*pvs = NULL;
		*pas = NULL;
		return;
	}
	Vector origin = view->v.origin + view->v.view_ofs;
	if ( view->v.flags & FL_DUCKING )
		origin = origin + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	*pvs = ENGINE_SET_PVS( origin );
	*pas = ENGINE_SET_PAS( origin );
}

#define MAX_ENTITIES	1380
#ifndef MAX_CLIENTS
#define MAX_CLIENTS		32
#endif

typedef struct
{
	float m_fTimeEnteredPVS;
} ENTITYPVSSTATUS;

typedef struct
{
	ENTITYPVSSTATUS m_Status[MAX_ENTITIES];
	int headnode;
	int num_leafs;
	short leafnums[MAX_ENT_LEAFS];
} PLAYERPVSSTATUS;

static PLAYERPVSSTATUS g_PVSStatus[MAX_CLIENTS];

void ResetPlayerPVS( edict_t* client, int clientnum )
{
	ASSERT_AT( clientnum >=0 && clientnum < MAX_CLIENTS, 1182 );
	PLAYERPVSSTATUS& status = g_PVSStatus[clientnum];
	memset( &status, 0, sizeof( status ) );
	status.headnode = client->headnode;
	status.num_leafs = client->num_leafs;
	memcpy( status.leafnums, client->leafnums, sizeof( status.leafnums ) );
}

bool CheckPlayerPVSLeafChanged( edict_t* client, int clientnum )
{
	ASSERT_AT( clientnum >=0 && clientnum < MAX_CLIENTS, 1196 );
	const PLAYERPVSSTATUS& status = g_PVSStatus[clientnum];
	if ( status.headnode != client->headnode || status.num_leafs != client->num_leafs )
		return true;
	for ( int leaf = 0; leaf < client->num_leafs; ++leaf )
	{
		if ( status.leafnums[leaf] != client->leafnums[leaf] )
			return true;
	}
	return false;
}

void MarkEntityInPVS( int clientnum, int entitynum, float time, bool inPVS )
{
	ASSERT_AT( clientnum >=0 && clientnum < MAX_CLIENTS, 1220 );
	ASSERT_AT( entitynum >= 0 && entitynum < MAX_ENTITIES, 1221 );
	g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS = inPVS ? time : 0;
}

bool CheckEntityRecentlyInPVS( int clientnum, int entitynum, float time )
{
	ASSERT_AT( clientnum >=0 && clientnum < MAX_CLIENTS, 1232 );
	ASSERT_AT( entitynum >= 0 && entitynum < MAX_ENTITIES, 1233 );
	float entered = g_PVSStatus[clientnum].m_Status[entitynum].m_fTimeEnteredPVS;
	return entered != 0 && time <= entered + 1;
}

int AddToFullPack( entity_state_s* state, int index, edict_t* entity, edict_t* host,
	int hostFlags, int player, unsigned char* set )
{
	const int visibilityController = 1 << 18;
	const int visibilityTarget = 1 << 19;
	const bool controlsVisibility = ( entity->v.flags & visibilityController ) != 0;
	if ( !controlsVisibility &&
		( ( ( entity->v.effects & EF_NODRAW ) && entity != host ) ||
		!entity->v.modelindex || !STRING( entity->v.model ) ) )
		return 0;
	if ( ( entity->v.flags & FL_SPECTATOR ) && entity != host )
		return 0;
	const int hostIndex = ENTINDEX( host ) - 1;
	if ( CheckPlayerPVSLeafChanged( host, hostIndex ) )
		ResetPlayerPVS( host, hostIndex );
	if ( entity != host && !CheckEntityRecentlyInPVS( hostIndex, index, gpGlobals->time ) )
	{
		bool visible;
		if ( ( entity->v.flags & visibilityTarget ) && entity->v.iuser2 )
			visible = entity->v.iuser1 != 0;
		else
			visible = ENGINE_CHECK_VISIBILITY( entity, set ) != 0;
		if ( controlsVisibility )
		{
			edict_t* target = FIND_ENTITY_BY_STRING( NULL, "targetname", STRING( entity->v.target ) );
			while ( !FNullEnt( target ) )
			{
				if ( target->v.iuser2 )
					target->v.iuser1 = visible ? 1 : 0;
				target = FIND_ENTITY_BY_STRING( target, "targetname", STRING( entity->v.target ) );
			}
		}
		MarkEntityInPVS( hostIndex, index, gpGlobals->time, visible );
		if ( !visible )
			return 0;
	}
	if ( ( entity->v.flags & FL_SKIPLOCALHOST ) && ( hostFlags & 1 ) && host == entity->v.owner )
		return 0;
	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );
		if ( entity->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND && !( entity->v.groupinfo & host->v.groupinfo ) )
				return 0;
			if ( g_groupop == GROUP_OP_NAND && ( entity->v.groupinfo & host->v.groupinfo ) )
				return 0;
		}
		UTIL_UnsetGroupTrace();
	}
	memset( state, 0, sizeof( *state ) );
	state->entityType = ( entity->v.flags & FL_CUSTOMENTITY ) ? ENTITY_BEAM : ENTITY_NORMAL;
	state->number = index;
	state->animtime = (float)( (int)( 1000.0 * entity->v.animtime ) / 1000.0 );
	state->origin = entity->v.origin;
	state->angles = entity->v.angles;
	state->mins = entity->v.mins;
	state->maxs = entity->v.maxs;
	state->startpos = entity->v.startpos;
	state->endpos = entity->v.endpos;
	state->impacttime = entity->v.impacttime;
	state->starttime = entity->v.starttime;
	state->modelindex = entity->v.modelindex;
	state->frame = entity->v.frame;
	state->skin = (short)entity->v.skin;
	state->effects = entity->v.effects;
	if ( !player && entity->v.animtime != 0 && entity->v.velocity == g_vecZero )
		state->eflags |= EFLAG_SLERP;
	state->scale = entity->v.scale;
	state->solid = (short)entity->v.solid;
	state->colormap = entity->v.colormap;
	state->movetype = entity->v.movetype;
	state->sequence = entity->v.sequence;
	state->framerate = entity->v.framerate;
	state->body = entity->v.body;
	memcpy( state->controller, entity->v.controller, sizeof( state->controller ) );
	memcpy( state->blending, entity->v.blending, sizeof( entity->v.blending ) );
	state->rendermode = entity->v.rendermode;
	state->renderamt = (int)entity->v.renderamt;
	state->renderfx = entity->v.renderfx;
	state->rendercolor.r = (byte)(int)entity->v.rendercolor.x;
	state->rendercolor.g = (byte)(int)entity->v.rendercolor.y;
	state->rendercolor.b = (byte)(int)entity->v.rendercolor.z;
	state->aiment = entity->v.aiment ? ENTINDEX( entity->v.aiment ) : 0;
	state->owner = 0;
	if ( entity->v.owner )
	{
		int owner = ENTINDEX( entity->v.owner );
		if ( owner > 0 && owner <= gpGlobals->maxClients )
			state->owner = owner;
	}
	if ( player )
	{
		state->basevelocity = entity->v.basevelocity;
		state->weaponmodel = entity->v.weaponmodel ? MODEL_INDEX( STRING( entity->v.weaponmodel ) ) : 0;
		state->gaitsequence = entity->v.gaitsequence;
		state->spectator = entity->v.flags & FL_SPECTATOR;
		state->friction = entity->v.friction;
		state->gravity = entity->v.gravity;
		state->usehull = ( entity->v.flags & FL_DUCKING ) != 0;
		state->health = (int)entity->v.health;
	}
	else
		state->playerclass = entity->v.playerclass;
	state->iuser4 = entity->v.iuser4;
	state->iuser3 = entity->v.iuser3;
	state->iuser1 = entity->v.iuser1;
	state->iuser2 = entity->v.iuser2;
	CBaseEntity* instance = entity ? (CBaseEntity*)entity->pvPrivateData : NULL;
	if ( instance && instance->Classify() != CLASS_NONE && instance->Classify() != CLASS_MACHINE )
		state->eflags |= EFLAG_FLESH_SOUND;
	else
		state->eflags &= ~EFLAG_FLESH_SOUND;
	return 1;
}

void CreateBaseline( int player, int index, entity_state_s* baseline, edict_t* entity,
	int playerModelIndex, Vector playerMins, Vector playerMaxs )
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)(int)entity->v.renderamt;
	baseline->rendercolor.r = (byte)(int)entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte)(int)entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte)(int)entity->v.rendercolor.z;
	baseline->renderfx = (byte)entity->v.renderfx;
	if ( player )
	{
		baseline->mins = playerMins;
		baseline->maxs = playerMaxs;
		baseline->colormap = index;
		baseline->modelindex = playerModelIndex;
		baseline->friction = 1;
		baseline->movetype = MOVETYPE_WALK;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1;
		baseline->gravity = 1;
		baseline->scale = entity->v.scale;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;
		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex;
		baseline->movetype = entity->v.movetype;
		baseline->scale = entity->v.scale;
		baseline->solid = (short)entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char name[32];
	int field;
} entity_field_alias_t;

enum
{
	FIELD_ORIGIN0,
	FIELD_ORIGIN1,
	FIELD_ORIGIN2,
	FIELD_ANGLES0,
	FIELD_ANGLES1,
	FIELD_ANGLES2,
	CUSTOMFIELD_SKIN,
	CUSTOMFIELD_SEQUENCE,
	CUSTOMFIELD_ANIMTIME
};

static entity_field_alias_t entity_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
	{ "angles[0]", 0 },
	{ "angles[1]", 0 },
	{ "angles[2]", 0 }
};

static entity_field_alias_t player_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 }
};

entity_field_alias_t custom_entity_field_alias[] =
{
	{ "origin[0]", 0 },
	{ "origin[1]", 0 },
	{ "origin[2]", 0 },
	{ "angles[0]", 0 },
	{ "angles[1]", 0 },
	{ "angles[2]", 0 },
	{ "skin", 0 },
	{ "sequence", 0 },
	{ "animtime", 0 }
};

void Entity_FieldInit( delta_s* fields )
{
	for ( int index = 0; index < ARRAYSIZE( entity_field_alias ); ++index )
		entity_field_alias[index].field = DELTA_FINDFIELD( fields, entity_field_alias[index].name );
}

void Entity_Encode( delta_s* fields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Entity_FieldInit( fields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	if ( current->number - 1 == ENGINE_CURRENT_PLAYER() )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_UNSETBYINDEX( fields, entity_field_alias[index].field );
	}
	if ( current->impacttime != 0 && current->starttime != 0 )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ANGLES2; ++index )
			DELTA_UNSETBYINDEX( fields, entity_field_alias[index].field );
	}
	if ( current->movetype == MOVETYPE_FOLLOW && current->aiment != 0 )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_UNSETBYINDEX( fields, entity_field_alias[index].field );
	}
	else if ( current->aiment != previous->aiment )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_SETBYINDEX( fields, entity_field_alias[index].field );
	}
}

void Player_FieldInit( delta_s* fields )
{
	for ( int index = 0; index < ARRAYSIZE( player_field_alias ); ++index )
		player_field_alias[index].field = DELTA_FINDFIELD( fields, player_field_alias[index].name );
}

void Player_Encode( delta_s* fields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Player_FieldInit( fields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	if ( current->number - 1 == ENGINE_CURRENT_PLAYER() )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_UNSETBYINDEX( fields, entity_field_alias[index].field );
	}
	if ( current->movetype == MOVETYPE_FOLLOW && current->aiment != 0 )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_UNSETBYINDEX( fields, entity_field_alias[index].field );
	}
	else if ( current->aiment != previous->aiment )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_SETBYINDEX( fields, entity_field_alias[index].field );
	}
}

void Custom_Entity_FieldInit( delta_s* fields )
{
	for ( int index = 0; index < ARRAYSIZE( custom_entity_field_alias ); ++index )
		custom_entity_field_alias[index].field = DELTA_FINDFIELD( fields, custom_entity_field_alias[index].name );
}

void Custom_Encode( delta_s* fields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Custom_Entity_FieldInit( fields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	int beamType = current->rendermode & 0x0f;
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		for ( int index = FIELD_ORIGIN0; index <= FIELD_ORIGIN2; ++index )
			DELTA_UNSETBYINDEX( fields, custom_entity_field_alias[index].field );
	}
	if ( beamType != BEAM_POINTS )
	{
		for ( int index = FIELD_ANGLES0; index <= FIELD_ANGLES2; ++index )
			DELTA_UNSETBYINDEX( fields, custom_entity_field_alias[index].field );
	}
	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( fields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field );
		DELTA_UNSETBYINDEX( fields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field );
	}
	if ( (int)previous->animtime == (int)current->animtime )
		DELTA_UNSETBYINDEX( fields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field );
}

void RegisterEncoders()
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( edict_t* entity, weapon_data_s* data )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( &entity->v );
	memset( data, 0, MAX_WEAPONS * sizeof( *data ) );
	if ( !player )
		return 1;
	for ( int slot = 0; slot < MAX_ITEM_TYPES; ++slot )
	{
		for ( CBasePlayerItem* item = player->m_rgpPlayerItems[slot]; item; item = item->m_pNext )
		{
			CBasePlayerWeapon* weapon = (CBasePlayerWeapon*)item->GetWeaponPtr();
			if ( !weapon || !weapon->UseDecrement() )
				continue;
			ItemInfo info;
			memset( &info, 0, sizeof( info ) );
			weapon->GetItemInfo( &info );
			weapon_data_t& state = data[info.iId];
			state.m_iId = info.iId;
			state.m_iClip = weapon->m_iClip;
			state.m_flTimeWeaponIdle = weapon->m_flTimeWeaponIdle < -0.001f ? -0.001f : weapon->m_flTimeWeaponIdle;
			state.m_flNextPrimaryAttack = weapon->m_flNextPrimaryAttack < -0.001f ? -0.001f : weapon->m_flNextPrimaryAttack;
			state.m_flNextSecondaryAttack = weapon->m_flNextSecondaryAttack < -0.001f ? -0.001f : weapon->m_flNextSecondaryAttack;
			state.m_flNextReload = weapon->m_flNextReload >= -0.001f ? weapon->m_flNextReload : -0.001f;
			state.m_fInReload = weapon->m_fInReload;
			state.m_fInSpecialReload = weapon->m_fInSpecialReload;
			state.m_fInZoom = weapon->m_iShotsFired;
			state.m_fAimedDamage = weapon->m_flLastFire;
			state.m_iWeaponState = weapon->m_iWeaponState;
		}
	}
	return 1;
}

void UpdateClientData( const edict_t* entity, int sendWeapons, clientdata_s* data )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( const_cast<entvars_t*>( &entity->v ) );
	data->flags = entity->v.flags;
	data->health = entity->v.health;
	data->viewmodel = MODEL_INDEX( STRING( entity->v.viewmodel ) );
	data->waterlevel = entity->v.waterlevel;
	data->watertype = entity->v.watertype;
	data->weapons = entity->v.weapons;
	data->origin = entity->v.origin;
	data->velocity = entity->v.velocity;
	data->view_ofs = entity->v.view_ofs;
	data->punchangle = entity->v.punchangle;
	data->bInDuck = entity->v.bInDuck;
	data->flTimeStepSound = entity->v.flTimeStepSound;
	data->flDuckTime = entity->v.flDuckTime;
	data->flSwimTime = entity->v.flSwimTime;
	data->waterjumptime = (int)entity->v.teleport_time;
	strcpy( data->physinfo, ENGINE_GETPHYSINFO( entity ) );
	data->maxspeed = entity->v.maxspeed;
	data->fov = entity->v.fov;
	data->weaponanim = entity->v.weaponanim;
	data->pushmsec = entity->v.pushmsec;
	data->iuser1 = entity->v.iuser1;
	data->iuser2 = entity->v.iuser2;
	if ( sendWeapons && player )
	{
		player->TabulateAmmo();
		data->ammo_shells = player->ammo_buckshot;
		data->ammo_nails = player->ammo_9mm;
		data->ammo_cells = player->ammo_556nato;
		data->ammo_rockets = player->ammo_556natobox;
		data->vuser2.x = (float)player->ammo_762nato;
		data->vuser2.y = (float)player->ammo_45acp;
		data->vuser2.z = (float)player->ammo_50ae;
		data->vuser3.x = (float)player->ammo_338mag;
		data->vuser3.y = (float)player->ammo_57mm;
		data->vuser3.z = (float)player->ammo_357sig;
		data->vuser4.y = (float)player->ammo_66mm;
		data->vuser4.z = (float)player->ammo_762natobox;
		data->m_flNextAttack = player->m_flNextAttack;
		data->iuser3 = player->m_bCanShoot;
		if ( player->m_pActiveItem )
		{
			CBasePlayerWeapon* weapon = (CBasePlayerWeapon*)player->m_pActiveItem->GetWeaponPtr();
			if ( weapon && weapon->UseDecrement() )
			{
				ItemInfo info;
				memset( &info, 0, sizeof( info ) );
				weapon->GetItemInfo( &info );
				data->m_iId = info.iId;
				data->tfstate = info.bIsAccessory;
				data->vuser4.x = (float)weapon->m_iPrimaryAmmoType;
				data->vuser4.y = (float)player->m_rgAmmo[weapon->m_iPrimaryAmmoType];
				data->vuser4.z = (float)player->ammo_762natobox;
			}
		}
	}
}

void CmdStart( const edict_t* entity, const usercmd_s*, unsigned int seed )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( const_cast<entvars_t*>( &entity->v ) );
	if ( player )
	{
		if ( player->pev->groupinfo )
			UTIL_SetGroupTrace( player->pev->groupinfo, GROUP_OP_AND );
		player->random_seed = seed;
	}
}

void CmdEnd( const edict_t* entity )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( const_cast<entvars_t*>( &entity->v ) );
	if ( player && player->pev->groupinfo )
		UTIL_UnsetGroupTrace();
}

int ConnectionlessPacket( const netadr_s*, const char*, char*, int* responseSize )
{
	*responseSize = 0;
	return 0;
}

int GetHullBounds( int hull, float*, float* )
{
	return (unsigned int)hull <= 2;
}

void CreateInstancedBaselines()
{
}

int InconsistentFile( const edict_t*, const char* filename, char* disconnectMessage )
{
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;
	sprintf( disconnectMessage, "Server is enforcing file consistency for %s\n", filename );
	return 1;
}

int AllowLagCompensation()
{
	return 1;
}

typedef struct
{
	const char* pszBSPName;
	const char* pszTitleName;
} TITLECOMMENT;

TITLECOMMENT gTitleComments[] =
{
	{ "cz_alamo", "#ALAMO_TITLE" },
	{ "cz_alamo2", "#ALAMO_TITLE" },
	{ "cz_brecon01", "#BRECON_TITLE" },
	{ "cz_brecon02", "#BRECON_TITLE" },
	{ "cz_brecon03", "#BRECON_TITLE" },
	{ "cz_brecon04", "#BRECON_TITLE" },
	{ "cz_brecon05", "#BRECON_TITLE" },
	{ "cz_brecon06", "#BRECON_TITLE" },
	{ "cz_downed1", "#DOWNED_TITLE" },
	{ "cz_downed2", "#DOWNED_TITLE" },
	{ "cz_downed3", "#DOWNED_TITLE" },
	{ "cz_downed4", "#DOWNED_TITLE" },
	{ "cz_druglab1", "#DRUGLAB_TITLE" },
	{ "cz_druglab2", "#DRUGLAB_TITLE" },
	{ "cz_druglab3", "#DRUGLAB_TITLE" },
	{ "cz_druglab4", "#DRUGLAB_TITLE" },
	{ "cz_fastline1", "#FASTLINE_TITLE" },
	{ "cz_fastline2", "#FASTLINE_TITLE" },
	{ "cz_fastline3", "#FASTLINE_TITLE" },
	{ "cz_fastline4", "#FASTLINE_TITLE" },
	{ "cz_end", "#END_TITLE" },
	{ "cz_hankagai1", "#HANKAGAI_TITLE" },
	{ "cz_hankagai2", "#HANKAGAI_TITLE" },
	{ "cz_hankagai3", "#HANKAGAI_TITLE" },
	{ "cz_hankagai4", "#HANKAGAI_TITLE" },
	{ "cz_hankagai5", "#HANKAGAI_TITLE" },
	{ "cz_HR01", "#HIGHRISE_TITLE" },
	{ "cz_HR02", "#HIGHRISE_TITLE" },
	{ "cz_HR02b", "#HIGHRISE_TITLE" },
	{ "cz_HR03", "#HIGHRISE_TITLE" },
	{ "cz_HR04", "#HIGHRISE_TITLE" },
	{ "cz_HR05", "#HIGHRISE_TITLE" },
	{ "cz_HR06", "#HIGHRISE_TITLE" },
	{ "cz_HR07", "#HIGHRISE_TITLE" },
	{ "cz_lostcause", "#LOSTCAUSE_TITLE" },
	{ "cz_lostcause2", "#LOSTCAUSE_TITLE" },
	{ "cz_miami1", "#MIAMI_TITLE" },
	{ "cz_miami2", "#MIAMI_TITLE" },
	{ "cz_motor1", "#MOTORCADE_TITLE" },
	{ "cz_motor2", "#MOTORCADE_TITLE" },
	{ "cz_motor3", "#MOTORCADE_TITLE" },
	{ "cz_pipedream1", "#PIPEDREAM_TITLE" },
	{ "cz_pipedream2", "#PIPEDREAM_TITLE" },
	{ "cz_pipedream3", "#PIPEDREAM_TITLE" },
	{ "cz_recoil-intro", "#RECOIL_TITLE" },
	{ "cz_recoil", "#RECOIL_TITLE" },
	{ "cz_recoil2", "#RECOIL_TITLE" },
	{ "cz_run1", "#RUN_TITLE" },
	{ "cz_run2", "#RUN_TITLE" },
	{ "cz_run3", "#RUN_TITLE" },
	{ "cz_sandstorm", "#SANDSTORM_TITLE" },
	{ "cz_sandstorm2", "#SANDSTORM_TITLE" },
	{ "cz_sandstorm3", "#SANDSTORM_TITLE" },
	{ "cz_sandstorm4", "#SANDSTORM_TITLE" },
	{ "cz_Silo01", "#SILO_TITLE" },
	{ "cz_Silo02", "#SILO_TITLE" },
	{ "cz_Silo03", "#SILO_TITLE" },
	{ "cz_Silo04", "#SILO_TITLE" },
	{ "cz_Silo05", "#SILO_TITLE" },
	{ "cz_sniper1", "#SNIPER_TITLE" },
	{ "cz_sniper2", "#SNIPER_TITLE" },
	{ "cz_thinice01", "#THINICE_TITLE" },
	{ "cz_thinice02", "#THINICE_TITLE" },
	{ "cz_thinice03", "#THINICE_TITLE" },
	{ "cz_train01", "#TRAINING_TITLE" },
	{ "cz_train02", "#TRAINING_TITLE" },
	{ "cz_train03", "#TRAINING_TITLE" },
	{ "cz_truth1", "#TRUTH_TITLE" },
	{ "cz_truth2", "#TRUTH_TITLE" },
	{ "cz_truth3", "#TRUTH_TITLE" },
	{ "cz_truth4", "#TRUTH_TITLE" },
	{ "cz_turncrank", "#TURNCRANK_TITLE" },
	{ "cz_turncrank2", "#TURNCRANK_TITLE" },
	{ "cz_turncrank3", "#TURNCRANK_TITLE" }
};

extern "C" EXPORT void SV_SaveGameComment( char* buffer, int bufferSize )
{
	const char* mapName = STRING( gpGlobals->mapname );
	const char* title = NULL;
	for ( int index = 0; index < ARRAYSIZE( gTitleComments ); ++index )
	{
		if ( !strnicmp( mapName, gTitleComments[index].pszBSPName, strlen( gTitleComments[index].pszBSPName ) ) )
		{
			title = gTitleComments[index].pszTitleName;
			break;
		}
	}
	if ( !title )
		title = mapName && *mapName ? mapName : "unknown";
	strncpy( buffer, title, bufferSize - 1 );
	buffer[bufferSize - 1] = '\0';
}
