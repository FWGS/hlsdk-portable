/*
cz_music.cpp - mp3 commands that wait until the player is in the game
Copyright (C) 2026 sergiopoverony

trigger_cdaudio plays the music of the level with "mp3 play" on the client.
Maps start it as soon as they begin (cz_recoil: level_start -> musicIntro).
Xash3D FWGS ties the background track to what the player sees when it
starts: a track started under the loading screen counts as menu music and
stays paused in the game. Here the command waits until the player has been
running his commands for a moment, i.e. he sees the game.
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "czeror/cz_music.h"

#include <string.h>

// seconds the player has to be in the game before the music starts
#define CZ_MUSIC_DELAY	1.5f

static float g_flMusicPlayerSince = 0.0f;	// time of the first player think, 0 = not yet
static char g_szMusicPending[256];

void CZ_MusicReset( void )
{
	g_flMusicPlayerSince = 0.0f;
}

static bool CZ_MusicPlayerReady( void )
{
	return g_flMusicPlayerSince > 0.0f && gpGlobals->time >= g_flMusicPlayerSince + CZ_MUSIC_DELAY;
}

void CZ_MusicCommand( edict_t *pPlayer, const char *command )
{
	if( !pPlayer || !command )
		return;

	if( CZ_MusicPlayerReady())
	{
		g_szMusicPending[0] = '\0';
		CLIENT_COMMAND( pPlayer, (char *)command );
		return;
	}

	// only the last command matters: a later play or stop replaces an earlier one
	strncpy( g_szMusicPending, command, sizeof( g_szMusicPending ) - 1 );
	g_szMusicPending[sizeof( g_szMusicPending ) - 1] = '\0';
	ALERT( at_aiconsole, "music: \"%s\" waits for the player\n", command );
}

void CZ_MusicPlayerThink( edict_t *pPlayer )
{
	if( g_flMusicPlayerSince <= 0.0f || gpGlobals->time < g_flMusicPlayerSince )
		g_flMusicPlayerSince = gpGlobals->time;

	if( !g_szMusicPending[0] || !CZ_MusicPlayerReady())
		return;

	ALERT( at_aiconsole, "music: \"%s\" sent\n", g_szMusicPending );
	CLIENT_COMMAND( pPlayer, g_szMusicPending );
	g_szMusicPending[0] = '\0';
}
