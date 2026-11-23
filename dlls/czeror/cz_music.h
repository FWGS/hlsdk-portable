/*
cz_music.h - mp3 commands that wait until the player is in the game
Copyright (C) 2026 sergiopoverony
*/
#pragma once
#if !defined( CZ_MUSIC_H )
#define CZ_MUSIC_H

// sends an "mp3 ..." command to the player now, or once he's in the game
void CZ_MusicCommand( edict_t *pPlayer, const char *command );

// the player runs his commands: he's in the game
void CZ_MusicPlayerThink( edict_t *pPlayer );

// a map was started or a game loaded
void CZ_MusicReset( void );

#endif // CZ_MUSIC_H
