//++ BulliT

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"gamerules.h"
#include "agcommand.h"
#include "agadmincache.h"
#include "agglobal.h"

DLL_GLOBAL AgCommand Command; //The one and only
FILE_GLOBAL CBasePlayer* s_pAdmin = NULL;



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

struct COMMANDS
{
  char szCommand[18];
  void (*pServer)(void);
  void (*pClient)(CBasePlayer* pPlayer);
  char szDescription[64];
};


void  addadmin_client(CBasePlayer* pPlayer)
{
  if (3 == CMD_ARGC())
    Command.AddAdmin(CMD_ARGV(1),CMD_ARGV(2),pPlayer);
}

void  addadmin(void)
{
  addadmin_client(NULL);
}

void  listadmins_client(CBasePlayer* pPlayer)
{
  if (1 == CMD_ARGC())
    Command.ListAdmins(pPlayer);
}

void  listadmins(void)
{
  listadmins_client(NULL);
}

void  deladmin_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.DelAdmin(CMD_ARGV(1),pPlayer);
}

void  deladmin(void)
{
  deladmin_client(NULL);
}

void  allow_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.Allow(CMD_ARGV(1),pPlayer);
  else if (1 == CMD_ARGC())
    Command.Allow("",pPlayer);
}

void  allow(void)
{
  allow_client(NULL);
}

void  agabort_client(CBasePlayer* pPlayer)
{
  Command.Abort(pPlayer);
}

void  agabort(void)
{
  agabort_client(NULL);
}


void  start_client(CBasePlayer* pPlayer)
{
  if (1 == CMD_ARGC())
    Command.Start("");
  if (2 <= CMD_ARGC())
    Command.Start(CMD_ARGS());
}

void  start(void)
{
  start_client(NULL);
}

void  pause_client(CBasePlayer* pPlayer)
{
  Command.Pause(pPlayer);
}

void  agpause(void)
{
  pause_client(NULL);
}

void  kick_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.Kick(CMD_ARGV(1));
}

void  map_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.Map(CMD_ARGV(1));
}

void  help_client(CBasePlayer* pPlayer)
{
  if (1 == CMD_ARGC())
    Command.Help(pPlayer);
}

void  help(void)
{
  help_client(NULL);
}

void  nextmap_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.NextMap(CMD_ARGV(1),pPlayer);
  else if (1 == CMD_ARGC())
    Command.NextMap("",pPlayer);
}

void  nextmap(void)
{
  nextmap_client(NULL);

}

void  spectator_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.Spectator(pPlayer,CMD_ARGV(1));
}

void  spectator(void)
{
  spectator_client(NULL);
}


void  teamup_client(CBasePlayer* pPlayer)
{
  if (3 == CMD_ARGC())
    Command.TeamUp(pPlayer,CMD_ARGV(1),CMD_ARGV(2));
}

void  teamup(void)
{
  teamup_client(NULL);
}

void  variables_client(CBasePlayer* pPlayer)
{
  if (1 == CMD_ARGC())
    Command.Variables(pPlayer);
}

void  variables(void)
{
  variables_client(NULL);
}


void  exec_client(CBasePlayer* pPlayer)
{
  if (2 == CMD_ARGC())
    Command.Exec(CMD_ARGV(1),pPlayer);
}

void  exec(void)
{
  exec_client(NULL);
}

FILE_GLOBAL COMMANDS s_Commands[]=
{
  "agaddadmin",addadmin,NULL,"agaddadmin <admin> <password> - Add new admin.",
  "aglistadmins",listadmins,NULL,"aglistadmins - List all admins.",
  "agdeladmin",deladmin,NULL,"agdeladmin <admin> - Delete existing admin.",

  "agforcespectator",spectator,spectator_client,"agforcespectator <name/#number> Force a player into specmode",
  "agforceteamup",teamup,teamup_client,"agforceteamup <name/#number> team - Force a player into a team",
  "agkick",NULL,kick_client,"agkick <name/#number> - Kick a player.",
  "agmap",NULL,map_client,"agmap <mapname> - Change level.",
  "agnextmap",nextmap,nextmap_client,"agnextmap <mapname> - Set next level.",
  "agexec",exec,exec_client,"agexec <config> - Executes a server configuration.",

  "agstart",start,start_client,"agstart <full/nolock> - Start a match.",
  "agabort",agabort,agabort_client,"agabort - Abort a match.",
  "agallow",allow,allow_client,"agallow <name> - Allow a player into the match.",
  "agpause",agpause,pause_client,"agpause - Pause server.",
  "help",help,help_client,"help - List commands.",
  "variables",variables,variables_client,"variables - Server variable list.",
};


FILE_GLOBAL char* s_szVars[] =
{
  "sv_ag_max_spectators <0-32> - Max spectators allowed.",
  "sv_ag_spec_enable_disable<0/1> - Allow players to disable tracking in spectator.",
  "sv_ag_pure <0/1> - 0 spike check and simple variable checks, 1 harder variable check.",
  "sv_ag_match_running <0/1> - Tells if a match is running.",
  "sv_ag_allow_vote <0/1> - Allow any vote.",
  "sv_ag_vote_setting <0/1> - Vote ag_xxx and mp_xxx settings.",
  "sv_ag_vote_gamemode <0/1> - Allow gamemode switching.",
  "sv_ag_vote_kick <0/1> - Allow voting a kick.",
  "sv_ag_vote_admin <0/1> - Allow voting an admin.",
  "sv_ag_vote_map <0/1> - Allow map voting.",
  "sv_ag_vote_mp_timelimit_low <0-999> - Lowest timelimit to vote on.",
  "sv_ag_vote_mp_timelimit_high <0-999> - Highest timelimit to vote on.",
  "sv_ag_vote_mp_fraglimit_low <0-999> - Lowest fraglimit to vote on.",
  "sv_ag_vote_mp_fraglimit_high <0-999> - Highest fraglimit to vote on.",
  "sv_ag_floodmsgs <4> - Flood messages to tolerate. 0 will deactive it.",
  "sv_ag_floodpersecond <4> - Flood messages per second.",
  "sv_ag_floodwaitdelay <10> - Flood penalty timer.",
  "sv_ag_show_gibs <0/1> - Show dead bodies.",
  "sv_ag_spawn_volume <0-1> - The spawn sound volume.",
  "sv_ag_player_id <5> - Player id show to other players. In seconds.",
  "sv_ag_auto_admin <1> - Give auto admin to admins in admin list.",
  "sv_ag_lj_timer <0-999> - Countdown seconds for long jump. 0 turns it off.",
  "sv_ag_wallgauss <0/1> - Wallgauss on/off. On is for the weak :)",
  "sv_ag_headshot <1-3> - Set power of headshot. Normally 3.",
  "sv_ag_blastradius <1> - Blast radius for explosions. Normally 1",
  "sv_ag_allowed_gamemodes <ffa;tdm> - Allowed gamemodes, could be any off ffa;tdm;arena;arcade;sgbow;instagib",
};

AgCommand::AgCommand()
{

}

AgCommand::~AgCommand()
{

}

void AgCommand::Init()
{
  for (int i = 0; i < sizeof(s_Commands)/sizeof(s_Commands[0]); i++)
  {
    if (s_Commands[i].pServer)
      ADD_SERVER_COMMAND(s_Commands[i].szCommand,s_Commands[i].pServer);
  }
}

bool AgCommand::HandleCommand(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return false;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return false;

  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules || 0 == CMD_ARGC())
    return false;

  if (pPlayer->IsAdmin())
  {
    //Server command
    for (int i = 0; i < sizeof(s_Commands)/sizeof(s_Commands[0]); i++)
    {
      if (s_Commands[i].pClient)
      {
        if (FStrEq(s_Commands[i].szCommand,CMD_ARGV(0))) 
        {
          s_Commands[i].pClient(pPlayer);
          if (FStrEq(s_Commands[i].szCommand,"help"))
            return false;
          else
            return true;
        }
      }
      else if (s_Commands[i].pServer)
      {
        if (FStrEq(s_Commands[i].szCommand,CMD_ARGV(0)))
        {
          s_Commands[i].pServer();
          return true;
        }
      }
    }

    if (1 == CMD_ARGC())
    {
      if (GameMode.IsGamemode(CMD_ARGV(0)))
      {
        if (!GameMode.IsAllowedGamemode(CMD_ARGV(0),pPlayer))
        {
          AgConsole("Gamemode not allowed by server admin.",pPlayer);
          return true;
        }

        GameMode.Gamemode(CMD_ARGV(0));
        return true;
      }
    }


    if (0 < CMD_ARGC() && 
        (0 == strnicmp(CMD_ARGV(0),"ag_spectalk",11) 
       ||0 == strnicmp(CMD_ARGV(0),"mp_timelimit",12)
       ||0 == strnicmp(CMD_ARGV(0),"ag_gauss_fix",12)  
       ||0 == strnicmp(CMD_ARGV(0),"ag_rpg_fix",10) 
       ||0 == strnicmp(CMD_ARGV(0),"mp_fraglimit",12)
       ||0 == strnicmp(CMD_ARGV(0),"mp_friendlyfire",15)
       ||0 == strnicmp(CMD_ARGV(0),"mp_weaponstay",13)
       ))
    {
      if (1 == CMD_ARGC())
        Setting(CMD_ARGV(0), "", pPlayer);
      else if (2 == CMD_ARGC())
        Setting(CMD_ARGV(0), CMD_ARGV(1), pPlayer);
      return true;
    }
  }
  return false;
}

void AgCommand::AddAdmin(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer)
{
  AdminCache.AddAdmin(sAdmin,sPassword,pPlayer);
}

void AgCommand::ListAdmins(CBasePlayer* pPlayer)
{
  AdminCache.ListAdmins(pPlayer);
}

void AgCommand::DelAdmin(const AgString& sAdmin,CBasePlayer* pPlayer)
{
  AdminCache.DelAdmin(sAdmin,pPlayer);
}



void AgCommand::Allow(const AgString& sPlayerIdOrName,CBasePlayer* pPlayer)
{
  if (!g_pGameRules)
    return;

  if (ARENA == AgGametype() || LMS == AgGametype())
  {
    AgConsole("Not allowed.",pPlayer);
    return;
  }

  //Get player.
  CBasePlayer* pPlayerLoop = AgPlayerByName(sPlayerIdOrName);
  if (!pPlayerLoop && pPlayer && 0 == sPlayerIdOrName.size())
    pPlayerLoop = pPlayer;

  if (pPlayerLoop)
  {
    g_pGameRules->m_Match.Allow(pPlayerLoop);
  }
  else
  {
    AgConsole("No such player exist on server.",pPlayer);
  }
}

void AgCommand::Abort(CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  if (LMS == AgGametype() || ARENA == AgGametype())
  {
    AgConsole("Abort is not allowed in this gamemode.",pPlayer);
    return;
  }

  g_pGameRules->m_Match.Abort();
}

void AgCommand::Start(const AgString& sSpawn)
{
  if (LMS == AgGametype() || ARENA == AgGametype())
  {
    AgConsole("Start is not allowed in this gamemode.");
    return;
  }

  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;


  g_pGameRules->Start(sSpawn);
}

void AgCommand::Pause(CBasePlayer* pPlayer)
{
  if (LMS == AgGametype() || ARENA == AgGametype())
  {
    AgConsole("Pause is not allowed in this gamemode.");
    return;
  }

  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  g_pGameRules->m_Timeout.TogglePause();
}

void AgCommand::Kick(const AgString& sPlayerIdOrName)
{
  if (32 < sPlayerIdOrName.size())
    return;

  char szCommand[128];
  sprintf(szCommand,"kick %s\n",sPlayerIdOrName.c_str());
  SERVER_COMMAND( szCommand );
}

void AgCommand::Map(const AgString& sMap)
{
  if (32 < sMap.size() || !g_pGameRules)
    return;

  g_pGameRules->m_Settings.Changelevel(sMap);
}

void AgCommand::NextMap(const AgString& sMap, CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  if (sMap.size() && 32 > sMap.size())
  {
    g_pGameRules->m_Settings.SetNextLevel(sMap);
  }
  else
  {
    char szSetting[64];
    sprintf(szSetting,"ag_nextmap is \"%s\"",g_pGameRules->m_Settings.GetNextLevel().c_str());
    AgConsole(szSetting,pPlayer);
  }
}


void AgCommand::Setting(const AgString& sSetting, const AgString& sValue, CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  if (sValue.size())
  {
    g_pGameRules->m_Settings.AdminSetting(sSetting,sValue);
  }
  else
  {
    char szSetting[64];
    sprintf(szSetting,"%s is \"%s\"",sSetting.c_str(),CVAR_GET_STRING(sSetting.c_str()));
    AgConsole(szSetting,pPlayer);
  }
}

void AgCommand::Help(CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  for (int i = 0; i < sizeof(s_Commands)/sizeof(s_Commands[0]); i++)
  {
    if (!pPlayer && s_Commands[i].pServer)
      AgConsole(s_Commands[i].szDescription,pPlayer);
    else if (pPlayer
      && 0 == strcmp(s_Commands[i].szCommand,"agaddadmin") 
      && 0 == strcmp(s_Commands[i].szCommand,"aglistadmins")
      && 0 == strcmp(s_Commands[i].szCommand,"agdeladmin"))
      AgConsole(s_Commands[i].szDescription,pPlayer);
  }

  GameMode.Help(pPlayer);
}


void AgCommand::Variables(CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  for (int i = 0; i < sizeof(s_szVars)/sizeof(s_szVars[0]); i++)
    AgConsole(s_szVars[i],pPlayer);
}

void AgCommand::TeamUp(CBasePlayer* pPlayer, const AgString& sPlayerIdOrName, const AgString& sTeam)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;
  if (!g_pGameRules->IsTeamplay())
    return;

  CBasePlayer* pTeamUpPlayer = AgPlayerByName(sPlayerIdOrName);
  if (pTeamUpPlayer)
    pTeamUpPlayer->ChangeTeam(sTeam.c_str(),true);
}

void AgCommand::Spectator(CBasePlayer* pPlayer, const AgString& sPlayerIdOrName)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;
  if (!g_pGameRules->IsTeamplay())
    return;

  CBasePlayer* pSpectatorPlayer = AgPlayerByName(sPlayerIdOrName);
  if (pSpectatorPlayer)
    pSpectatorPlayer->Spectate_Start();
}


void AgCommand::Exec(const AgString& sExec, CBasePlayer* pPlayer)
{
  ASSERT(NULL != g_pGameRules);
  if (!g_pGameRules)
    return;

  char szCommand[128];
  sprintf(szCommand,"exec %s\n",sExec.c_str());
  SERVER_COMMAND( szCommand );
  SERVER_EXECUTE();
}

//-- Martin Webrant
