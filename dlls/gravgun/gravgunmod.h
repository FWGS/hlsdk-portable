#ifndef GRAVGUNMOD_H
#define GRAVGUNMOD_H


extern cvar_t cvar_allow_gravgun;
extern cvar_t cvar_allow_ar2;
extern cvar_t cvar_ar2_mp5;
extern cvar_t cvar_ar2_bullets;
extern cvar_t cvar_ar2_balls;
extern cvar_t cvar_allow_bigcock;
extern cvar_t cvar_allow_gateofbabylon;
extern cvar_t cvar_wresptime;
extern cvar_t cvar_iresptime;
extern cvar_t mp_gravgun_players;
extern cvar_t mp_skipdefaults;
extern cvar_t mp_unduck;

extern cvar_t cvar_gibtime;
extern cvar_t cvar_hgibcount;
extern cvar_t cvar_agibcount;

extern cvar_t mp_spectator;
extern cvar_t mp_fixhornetbug;
extern cvar_t mp_checkentities;
extern cvar_t mp_touchmenu;
extern cvar_t mp_fixsavetime;

// distance clipping (client.cpp)
// useful for open world
extern cvar_t mp_serverdistclip;
extern cvar_t mp_maxbmodeldist;
extern cvar_t mp_maxtrashdist;
extern cvar_t mp_maxwaterdist;
extern cvar_t mp_maxmonsterdist;
extern cvar_t mp_maxotherdist;
extern cvar_t mp_enttools_checkmodels;

// tempentity clipping
// if enabled, ignores PVS, so use only on open world
extern cvar_t mp_servercliptents;
extern cvar_t mp_maxtentdist;

// control decals count from server
// decals is unuseful on sandbox servers
// 100 instancs of single bmodel with 100 decals
// will cause in 10000 decals in frame
// r_decals limit does not cover it because it
// only limit decal count on different models

extern cvar_t mp_maxdecals;

void GGM_RegisterCVars( void );
#define GC_COMMON (1<<0)
#define GC_ENTTOOLS (1<<1)
void Ent_RunGC( int flags, const char *userid, const char *pattern = NULL );
bool Q_stricmpext( const char *pattern, const char *text );
class CBasePlayer;
void GGM_ClientPutinServer(edict_t *pEntity , CBasePlayer *pPlayer);
void GGM_ClientFirstSpawn(CBasePlayer *pPlayer );
const char *GGM_GetPlayerID( edict_t *player );
edict_t *GGM_PlayerByID( const char *id );
void GGM_Say( edict_t *pEntity );
bool GGM_ClientCommand( CBasePlayer *player, const char *pCmd );
void GGM_InitialMenus( CBasePlayer *pPlayer );
void GGM_CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value );

enum PlayerState
{
	STATE_UNINITIALIZED = 0,
	STATE_CONNECTED,
	STATE_SPECTATOR_BEGIN,
	STATE_SPAWNED,
	STATE_SPECTATOR,
	STATE_POINT_SELECT,
	STATE_LOAD_FIX
};

enum PlayerMenuState
{
	MENUSTATE_NONE = 0,
	MENUSTATE_COOPMENU,
	MENUSTATE_COOPMENU_SPEC,
	MENUSTATE_CHECKPOINT,
	MENUSTATE_GLOBAL,
	MENUSTATE_LOCAL_CONFIRM
};


class GGM_PlayerMenu
{
	struct GGM_MenuItem
	{
		char szCommand[256];
		char szName[32];
	} m_rgItems[5];
	int m_iCount;
	char m_sTitle[32];
	bool m_fShow;

public:
	CBasePlayer *m_pPlayer;
	bool MenuSelect( int select );
	GGM_PlayerMenu &SetTitle( const char *title );
	GGM_PlayerMenu &New( const char *title, bool force = true );
	GGM_PlayerMenu &Add( const char *name, const char *command );
	GGM_PlayerMenu &Clear( void );
	void Show();
};

// full player map-independed position data
struct GGMPosition
{
	Vector vecOrigin;
	Vector vecAngles;
	char szMapName[32];
	char szTrainGlobal[32];
	Vector vecTrainOffset;
	Vector vecTrainAngles;
	bool fDuck;
};

// login record
// this maps pair of nickname and uid to registration
struct GGMLogin
{
	struct GGMLogin *pNext;
	struct {
	char szUID[33];
	char szName[32];
	} f;
	struct GGMPlayerState *pState;
};

// registration and game stats
// saved on every change to separate file
// but only for registered users
struct GGMPersist
{

	/// todo:salt/hash
	char szPassword[33];
};

// complete player state
// saved on save request, but kept in runtime
struct GGMTempState
{
	float flHealth;
	float flBattery;
	int iFrags;
	int iDeaths;
	char rgszWeapons[MAX_WEAPONS][32];// weapon names
	char rgiClip[MAX_WEAPONS];// ammo names
	int	rgszAmmo[MAX_AMMO_SLOTS];// ammo quantities
	char szWeaponName[32];
	GGMPosition pos;
	bool fIsTempBanned; // prevent some actions
};

// state which every spawned player has
struct GGMPlayerState
{
	struct GGMPlayerState *pNext;
	struct GGMPersist p;
	struct GGMTempState t;
	bool fRegistered;
	bool fNeedWrite;
	// uid or nickname
	char szUID[33];
};

// player ggm data. additional struct for CBasePlayer
struct GGMData
{
	float flSpawnTime;
	PlayerState iState;
	bool fTouchMenu;
	int iLocalConfirm;
	edict_t *pChangeLevel;
	float flEntScore;
	float flEntTime;
	GGM_PlayerMenu menu;
	bool fTouchLoading;
	struct GGMPlayerState *pState;
	char fRegisterInput[32];
};

struct GGMPlayerState *GGM_GetState(const char *uid, const char *name);
bool GGM_RestoreState( CBasePlayer *pPlayer );
bool GGM_RestorePosition( CBasePlayer *pPlayer, struct GGMPosition *pos );
void GGM_SavePosition( CBasePlayer *pPlayer, struct GGMPosition *pos );
void GGM_SaveState( CBasePlayer *pPlayer );
bool GGM_PlayerSpawn( CBasePlayer *pPlayer );
const char *GGM_GetAuthID( CBasePlayer *pPlayer );
void GGM_ServerActivate( void );
void COOP_SetupLandmarkTransition( const char *szNextMap, const char *szNextSpot, Vector vecLandmarkOffset, struct GGMPosition *pPos );
void GGM_ClearLists( void );
void GGM_Save( const char *savename );
void GGM_Load( const char *savename );
const char *GGM_PlayerName( CBaseEntity *pPlayer );
bool GGM_IsTempBanned( CBaseEntity *plr );
void GGM_TempBan( CBaseEntity *pEnt );
int GGM_ChangelevelVote( CBasePlayer *pPlayer, edict_t *pTrigger, const char *pszMapName );
void GGM_ClearVote( void );
void GGM_StartVoteCommand( CBasePlayer *pPlayer, const char *pszCommand, const char *pszMessage );
void GGM_ConnectSaveBot( void );
int GGM_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );

#endif // GRAVGUNMOD_H

