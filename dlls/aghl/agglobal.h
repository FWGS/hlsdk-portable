//++ BulliT

#if !defined(_AG_GLOBAL_H_)
#define _AG_GLOBAL_H_

#pragma warning(disable:4786) 
#pragma warning(disable:4530) 

#include "hltv.h"


#define _bool_h 1
#include <ctype.h>
#include <ministl/string>
#include <ministl/list>
#include <ministl/set>
#include <ministl/map>
#include <ministl/vector>
#include <ministl/algorithm>

typedef string         AgString;

typedef string AgString;
typedef list<AgString> AgStringList;
typedef set<AgString, less<AgString> > AgStringSet;

#define ADD_SERVER_COMMAND	( *g_engfuncs.pfnAddServerCommand )
extern DLL_GLOBAL BOOL		g_fGameOver;
extern cvar_t	timelimit;



extern cvar_t	ag_gamemode;

extern cvar_t	ag_spectalk;
extern cvar_t	ag_max_spectators;
extern cvar_t	ag_spec_enable_disable;

extern cvar_t	ag_pure;
extern cvar_t	ag_allow_vote;
extern cvar_t	ag_match_running;
extern cvar_t	ag_player_id;
extern cvar_t	ag_lj_timer;
extern cvar_t	ag_auto_admin;

extern cvar_t	ag_wallgauss;
extern cvar_t	ag_headshot;
extern cvar_t	ag_blastradius;

extern cvar_t	ag_ban_crowbar;
extern cvar_t	ag_ban_glock;
extern cvar_t	ag_ban_357;
extern cvar_t	ag_ban_mp5;
extern cvar_t	ag_ban_shotgun;
extern cvar_t	ag_ban_crossbow;
extern cvar_t	ag_ban_rpg;
extern cvar_t	ag_ban_gauss;
extern cvar_t	ag_ban_egon;
extern cvar_t	ag_ban_hornet;

extern cvar_t	ag_ban_hgrenade;
extern cvar_t	ag_ban_satchel;
extern cvar_t	ag_ban_tripmine;
extern cvar_t	ag_ban_snark;
extern cvar_t	ag_ban_m203;
extern cvar_t	ag_ban_longjump;
extern cvar_t	ag_ban_9mmar;
extern cvar_t	ag_ban_bockshot;
extern cvar_t	ag_ban_uranium;
extern cvar_t	ag_ban_bolts;
extern cvar_t	ag_ban_rockets;
extern cvar_t	ag_ban_357ammo;

extern cvar_t	ag_ban_armour;
extern cvar_t	ag_ban_health;

extern cvar_t ag_ban_recharg;

extern cvar_t	ag_start_crowbar;
extern cvar_t	ag_start_glock;
extern cvar_t	ag_start_357;
extern cvar_t	ag_start_mp5;
extern cvar_t	ag_start_shotgun;
extern cvar_t	ag_start_crossbow;
extern cvar_t	ag_start_rpg;
extern cvar_t	ag_start_gauss;
extern cvar_t	ag_start_egon;
extern cvar_t	ag_start_hornet;

extern cvar_t	ag_start_hgrenade;
extern cvar_t	ag_start_satchel;
extern cvar_t	ag_start_tripmine;
extern cvar_t	ag_start_snark;
extern cvar_t	ag_start_m203;
extern cvar_t	ag_start_longjump;
extern cvar_t	ag_start_9mmar;
extern cvar_t	ag_start_bockshot;
extern cvar_t	ag_start_uranium;
extern cvar_t	ag_start_bolts;
extern cvar_t	ag_start_rockets;
extern cvar_t	ag_start_357ammo;

extern cvar_t	ag_start_armour;
extern cvar_t	ag_start_health;

extern cvar_t ag_dmg_crowbar;	  
extern cvar_t ag_dmg_glock;	  
extern cvar_t ag_dmg_357;		  
extern cvar_t ag_dmg_mp5;	      
extern cvar_t ag_dmg_shotgun;	  
extern cvar_t ag_dmg_crossbow;
extern cvar_t ag_dmg_bolts;     
extern cvar_t ag_dmg_rpg;		  
extern cvar_t ag_dmg_gauss;	  
extern cvar_t ag_dmg_egon_wide;
extern cvar_t ag_dmg_egon_narrow;
extern cvar_t ag_dmg_hornet;	  
extern cvar_t ag_dmg_hgrenade;
extern cvar_t ag_dmg_satchel;	  
extern cvar_t ag_dmg_tripmine;
extern cvar_t ag_dmg_m203;	

extern cvar_t ag_spawn_volume;
extern cvar_t ag_show_gibs;

extern cvar_t	ag_allow_timeout;

extern cvar_t	ag_vote_start;
extern cvar_t	ag_vote_setting;
extern cvar_t	ag_vote_gamemode;
extern cvar_t	ag_vote_kick;
extern cvar_t	ag_vote_admin;
extern cvar_t	ag_vote_allow;
extern cvar_t	ag_vote_map;
extern cvar_t	ag_vote_failed_time;

extern cvar_t	ag_start_minplayers;

extern cvar_t	ag_vote_mp_timelimit_low;
extern cvar_t	ag_vote_mp_timelimit_high;
extern cvar_t	ag_vote_mp_fraglimit_low;
extern cvar_t	ag_vote_mp_fraglimit_high;

extern cvar_t	ag_floodmsgs;
extern cvar_t	ag_floodpersecond;
extern cvar_t	ag_floodwaitdelay;

extern cvar_t	ag_ctf_flag_resettime;
extern cvar_t	ag_ctf_capturepoints;
extern cvar_t	ag_ctf_teamcapturepoints;
extern cvar_t	ag_ctf_capture_limit;
extern cvar_t	ag_ctf_returnpoints;
extern cvar_t	ag_ctf_carrierkillpoints;
extern cvar_t	ag_ctf_stealpoints;
extern cvar_t	ag_ctf_defendpoints;
extern cvar_t ag_ctf_roundbased;

//++ muphicks
extern cvar_t ag_dom_mincontroltime;
extern cvar_t ag_dom_controlpoints;
extern cvar_t ag_dom_resetscorelimit;
extern cvar_t ag_dom_scorelimit;
//-- muphicks

extern cvar_t ag_gauss_fix;
extern cvar_t ag_rpg_fix;

extern bool g_bLangame;
extern bool g_bUseTeamColors;

void AgInitGame();
CBasePlayer* AgPlayerByIndex(int iPlayerIndex);
CBasePlayer* AgPlayerByName(const AgString& sNameOrPlayerNumber);

void AgChangelevel(const AgString& sLevelname);

void AgSay(CBasePlayer* pPlayer, const AgString& sText, float* pfFloodProtected = NULL, float fHoldTime = 3.5, float x = -1, float y = -1, int iChannel = 5);
void AgConsole(const AgString& sText, CBasePlayer* pPlayer = NULL);

void AgResetMap();

char* AgStringToLower(char* pszString);
void AgToLower(AgString& strLower);
void AgTrim(AgString& sTrim);


void AgLog(const char* pszLog);

double AgTime();

void AgDirList(const AgString& sDir, AgStringSet& setFiles);

void AgSendDirectorMessage( CBaseEntity *ent1, CBaseEntity *ent2, int priority ); 

void AgStripColors(char* pszString);


void AgGetDetails(char* pszDetails, int iMaxSize, int* piSize);
void AgGetPlayerInfo(char* pszPlayerInfo, int iMaxSize, int* piSize);

char* AgOSVersion();

AgString AgReadFile(const char* pszFile);

void AgDisplayGreetingMessage(const char* pszAuthID);

bool AgIsCTFMap(const char* pszMap);
//++ muphicks
bool AgIsDOMMap(const char* pszMap);
//-- muphicks

void AgSound(CBasePlayer* pPlayer, const char* pszWave);
void AgPlayCountdown(CBasePlayer* pPlayer, int iSeconds);

bool AgIsLocalServer();

const char* AgGetGame();
const char* AgGetDirectory();
const char* AgGetDirectoryValve();

#endif // !defined(_AG_GLOBAL_H_)

//-- Martin Webrant
