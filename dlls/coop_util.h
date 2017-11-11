#ifndef COOP_UTIL_H
#define COOP_UTIL_H
extern cvar_t mp_gravgun_players;
extern cvar_t mp_coop;
extern cvar_t mp_coop_changelevel;
extern cvar_t mp_coop_nofriendlyfire;
extern cvar_t mp_coop_disabledmap;
extern cvar_t mp_coop_checkpoints;
extern cvar_t mp_skipdefaults;
extern cvar_t mp_coop_strongcheckpoints;


extern cvar_t mp_unduck;
extern cvar_t mp_semclip;
extern cvar_t mp_coop_reconnect_hack;
extern cvar_t mp_coop_noangry;


extern cvar_t sentences_txt;
extern cvar_t materials_txt;

extern bool g_fSavedDuck;
extern bool g_fPause;

struct SavedCoords
{
	char ip[32][32];
	Vector origin[32];
	Vector angles[32];
	char landmark[32];
	Vector triggerorigin;
	Vector triggerangles;
	Vector offset;
	int iCount;
	bool valid;
	bool validoffset;
	bool validspawnpoint;
	int changeback;
	bool trainsaved;
	Vector trainoffset;
	char trainglobal[256];
	int trainuser1;
	bool fUsed;
	bool fDuck;
};



void UTIL_CoopValidateOffset( void );
void UTIL_CleanSpawnPoint( Vector origin, float radius );
char *UTIL_CoopPlayerName( CBaseEntity *pPlayer );

bool UTIL_CoopGetSpawnPoint( Vector *point, Vector *angles);

bool UTIL_CoopRestorePlayerCoords(CBaseEntity *player, Vector *origin, Vector *angles );
void UTIL_CoopSaveTrain( CBaseEntity *pPlayer, SavedCoords *coords);
Vector UTIL_FixupSpawnPoint(Vector spawn);
void UTIL_CoopActivateChangeLevel( CBaseEntity *pTrigger );
void UTIL_CoopClearData( void );
void UTIL_CoopApplyData( void );
void UTIL_CoopPrintMessage( const char *format, ... );
void UTIL_CoopHudMessage( int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... );
void UTIL_CoopPlayerMessage( CBaseEntity *pPlayer, int channel, float time, unsigned int color1, unsigned int color2, float x, float y,  const char *format, ... );

void COOP_RegisterCVars( void );

#ifdef PLAYER_H
void UTIL_CoopKickPlayer(CBaseEntity *pPlayer);
bool UTIL_CoopIsBadPlayer( CBaseEntity *plr );
void UTIL_CoopNewCheckpoint( entvars_t *pevPlayer );
CBaseEntity *UTIL_CoopGetPlayerTrain( CBaseEntity *pPlayer);
void UTIL_CoopMenu( CBasePlayer *pPlayer );
void UTIL_SpawnPlayer( CBasePlayer *pPlayer );
void UTIL_BecomeSpectator( CBasePlayer *pPlayer );
void UTIL_CoopCheckpointMenu( CBasePlayer *pPlayer );
void UTIL_CoopVoteMenu( CBasePlayer *pPlayer );
void UTIL_CoopShowMenu( CBasePlayer *pPlayer, const char *title, int count, const char **slot, signed char time = -1 );
bool UTIL_CoopConfirmMenu( CBaseEntity *pTrigger, CBaseEntity *pActivator, int count2, char *mapname );
extern int g_iMenu;

// Show to all spawned players: voting, etc..
class GlobalMenu
{
public:

	int m_iConfirm;
	int m_iVoteCount;
	int m_iMaxCount;
	int m_iBanCount;
	float m_flTime;
	const char *maps[5];
	int votes[5];
	CBaseEntity *triggers[5];
	EHANDLE m_pTrigger;
	EHANDLE m_pPlayer;
	void VoteMenu( CBasePlayer *pPlayer );
	void ConfirmMenu( CBasePlayer *pPlayer, CBaseEntity *trigger, const char *mapname );
	void ShowGlobalMenu( const char *title, int count, const char **menu );
	void Process( CBasePlayer *pPlayer, int imenu );
};

extern GlobalMenu g_GlobalMenu;

class CWeaponList
{
	char weapons[64][256];
	int m_iWeapons;
public:
	void AddWeapon( const char *classname );
	void GiveToPlayer(CBasePlayer *pPlayer);
	void Clear();
};

extern CWeaponList g_WeaponList;

#endif
extern struct SavedCoords g_SavedCoords, s_SavedCoords;

#endif // COOP_UTIL_H

