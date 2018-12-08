#ifndef COOP_UTIL_H
#define COOP_UTIL_H
#include "gravgunmod.h"
extern cvar_t mp_coop;
extern cvar_t mp_coop_nofriendlyfire;
extern cvar_t mp_coop_checkpoints;
extern cvar_t mp_coop_strongcheckpoints;


extern cvar_t mp_semclip;
extern cvar_t mp_coop_reconnect_hack;
extern cvar_t mp_coop_noangry;


extern cvar_t sentences_txt;
extern cvar_t materials_txt;

extern bool g_fSavedDuck;
extern bool g_fPause;

// triggers.cpp, CChangeLevel
struct COOPChangelevelData
{
	bool fIsBack;
	bool fSkipSpawnCheck;
	struct GGMPosition savedPosition;
	bool fSpawnSaved;
	bool fUsed;
	unsigned int bitsTouchCount;
	float flRepeatTimer;
	const char *pszMapName;
	const char *pszLandmarkName;
	bool fValid;
};

void UTIL_CleanSpawnPoint( Vector origin, float radius );

bool COOP_SetDefaultSpawnPosition( CBasePlayer *pPlayer );
void UTIL_CoopPrintMessage( const char *format, ... );
void UTIL_CoopHudMessage( int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... );
void UTIL_CoopPlayerMessage( CBaseEntity *pPlayer, int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... );
bool COOP_PlayerDeath( CBasePlayer *pPlayer );
void COOP_RegisterCVars( void );
bool COOP_ClientCommand( edict_t *pEntity );
bool COOP_ConfirmMenu(CBaseEntity *pTrigger, CBaseEntity *pActivator, int count2, char *mapname );
void COOP_ResetVote( void );

void COOP_ServerActivate( void );
bool COOP_GetOrigin( Vector *pvecNewOrigin, const Vector &vecOrigin, const char *pszMapName );
class CBasePlayer;
CBaseEntity *UTIL_CoopGetPlayerTrain( CBaseEntity *pPlayer);
void UTIL_SpawnPlayer( CBasePlayer *pPlayer );
void UTIL_BecomeSpectator( CBasePlayer *pPlayer );
void COOP_GiveDefaultWeapons( CBasePlayer *pPlayer );
void COOP_AddDefaultWeapon( const char *pszClassName );
void COOP_WriteState( const char *path );
bool COOP_ReadState( const char *path );
void COOP_AutoSave( CBaseEntity *pPlayer );
bool COOP_PlayerSpawn( CBasePlayer *pPlayer );
void COOP_ClearSaves( void );
struct COOPChangelevelData *COOP_GetTriggerData( CBaseEntity *pTrigger );
#endif // COOP_UTIL_H

