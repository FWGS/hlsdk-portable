//++ BulliT

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"gamerules.h"
#include  "aggamemode.h"
#include  "agglobal.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
DLL_GLOBAL AgGameMode GameMode;

DLL_GLOBAL AgGame* g_pGame = NULL;
DLL_GLOBAL AgString g_sGamemode;
DLL_GLOBAL AgString g_sNextmode;
DLL_GLOBAL BYTE g_GameType = STANDARD;

void SetupGametype()
{
  AgString sGametype = CVAR_GET_STRING("sv_ag_gametype");
  g_GameType = STANDARD;
  if (sGametype == "arena")
    g_GameType = ARENA;
  else if (sGametype == "arcade")
    g_GameType = ARCADE;
#ifndef AG_NO_CLIENT_DLL
  else if (sGametype == "ctf")
    g_GameType = CTF;
  //++ muphicks
  else if (sGametype == "dom")
    g_GameType = DOM;
  //--muphicks
#endif
  else if (sGametype == "lms")
    g_GameType = LMS;
  else if (sGametype == "sgbow")
    g_GameType = SGBOW;
  else if (sGametype == "instagib")
    g_GameType = INSTAGIB;
}

AgString AgGamename()
{
  if (g_pGame)
    return g_pGame->m_sName;

  return "Half-Life";
}

AgString AgGamedescription()
{
  if (g_pGame)
    return g_pGame->m_sDescription;

  return "Half-Life";
}

void  gamemode(void)
{
  GameMode.Gamemode(CMD_ARGV(0));
}

void  nextmode(void)
{
  if (2 == CMD_ARGC())
    GameMode.NextGamemode(CMD_ARGV(1));
  else
    AgConsole(g_sNextmode.size() ? g_sNextmode : g_sGamemode, NULL);
}


AgGameMode::AgGameMode()
{
  m_fNextCheck = 0;
}

AgGameMode::~AgGameMode()
{
  for (AgGameMap::iterator itrGames = m_mapGames.begin() ;itrGames != m_mapGames.end(); ++itrGames)
    delete (*itrGames).second;
  m_mapGames.clear();
}

bool AgGameMode::HandleCommand(CBasePlayer* pPlayer)
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
    if (1 == CMD_ARGC())
    {
      if (FStrEq(CMD_ARGV(0), "help"))
      {
        Help(pPlayer);
        return true;
      }

      if (!IsAllowedGamemode(CMD_ARGV(0)),pPlayer)
        return false;

      Gamemode(CMD_ARGV(0),pPlayer);
      return true;
    }
    else if (2 == CMD_ARGC())
    {
      if (FStrEq(CMD_ARGV(0), "agnextmode"))
      {
        if (!IsAllowedGamemode(CMD_ARGV(1)),pPlayer)
          return false;

        NextGamemode(CMD_ARGV(1),pPlayer);
        return true;
      }
    }
  }

  if (1 == CMD_ARGC() && FStrEq(CMD_ARGV(0), "agnextmode"))
  {
    AgConsole(g_sNextmode.size() ? g_sNextmode : g_sGamemode, pPlayer);
    return true;
  }

  return false;
}

void AgGameMode::Help(CBasePlayer* pPlayer)
{
  for (AgGameMap::iterator itrGames = m_mapGames.begin() ;itrGames != m_mapGames.end(); ++itrGames)
  {
    AgConsole(UTIL_VarArgs("%s - %s",(*itrGames).second->m_sShortname.c_str(),(*itrGames).second->m_sDescription.c_str()),pPlayer);
  }
}

void AgGameMode::Think()
{
  if (m_fNextCheck > gpGlobals->time) 
    return;
  m_fNextCheck = gpGlobals->time + 1; //Check every second.

  if (g_sGamemode != CVAR_GET_STRING("sv_ag_gamemode"))
  {
    //Gamemode has changed. Save the new one and changelevel. The new settings will be set just before allocating the new gamerules.
    g_sGamemode = CVAR_GET_STRING("sv_ag_gamemode");
    CVAR_SET_FLOAT("sv_ag_match_running",0);
    CVAR_SET_FLOAT("ag_spectalk",1);
    CVAR_SET_FLOAT("sv_ag_show_gibs",1);
    g_pGameRules->m_Settings.Changelevel(STRING(gpGlobals->mapname));
  }
}

void AgGameMode::Gamemode(const AgString& sGamemode,CBasePlayer* pPlayer)
{
  if ((IsGamemode(sGamemode) && !pPlayer) || IsAllowedGamemode(sGamemode,pPlayer))
  {
    CVAR_SET_STRING("sv_ag_gamemode",sGamemode.c_str());
    AgConsole("Gamemode changed.", pPlayer);
    g_sNextmode = "";
  }
  else
  {
    AgConsole("Gamemode not allowed by server admin.",pPlayer);
  }
}

void AgGameMode::NextGamemode(const AgString& sGamemode,CBasePlayer* pPlayer)
{
  if ((IsGamemode(sGamemode) && !pPlayer) || IsAllowedGamemode(sGamemode,pPlayer))
  {
    g_sNextmode = sGamemode;
    AgConsole("Next Gamemode changed.", pPlayer);
  }
  else
  {
    AgConsole("Gamemode not allowed by server admin.",pPlayer);
  }
}

void AgGameMode::ExecConfig()
{
  m_fNextCheck = 0;

  if (g_sNextmode.size())
  {
    CVAR_SET_STRING("sv_ag_gamemode",g_sNextmode.c_str());
    g_sNextmode = "";
  }

  AgGameMap::iterator itrGames =   m_mapGames.find(CVAR_GET_STRING("sv_ag_gamemode"));
  if (itrGames == m_mapGames.end())
  {
    //eh? - error in config.
    g_pGame = NULL;
    AgConsole("Error in server gamemode configuration.\n");
  }
  else
  {
    g_pGame = (*itrGames).second;
 	  SERVER_COMMAND( UTIL_VarArgs("exec gamemodes/%s\n",g_pGame->m_sCfg.c_str() ));
	  SERVER_EXECUTE( );  
    
    //So that map does not restart directly.
    g_sGamemode = CVAR_GET_STRING("sv_ag_gamemode");
    
    //Setup the gametype.
    SetupGametype();
  }
}

bool AgGameMode::IsGamemode(const AgString& sGamemode)
{
  if (0 == sGamemode.size())
    return false;

  AgGameMap::iterator itrGames = m_mapGames.find(sGamemode);
  if (itrGames == m_mapGames.end())
    return false;

  return true;
}

bool AgGameMode::IsAllowedGamemode(const AgString& sGamemode,CBasePlayer* pPlayer)
{
  if (!IsGamemode(sGamemode))
    return false;

  //If empty we allow all.
  if (0 == strlen(CVAR_GET_STRING("sv_ag_allowed_gamemodes")))
    return true;

  //Check what gamemodes that are allowed.
  return (NULL != strstr(CVAR_GET_STRING("sv_ag_allowed_gamemodes"),sGamemode.c_str()));
}


void AgGameMode::Init()
{
  //Set this initially, so server dont restart map right away.
  g_sGamemode = CVAR_GET_STRING("sv_ag_gamemode");

  LoadGames();

  for (AgGameMap::iterator itrGames = m_mapGames.begin() ;itrGames != m_mapGames.end(); ++itrGames)
  {
    ADD_SERVER_COMMAND((char*)(*itrGames).second->m_sShortname.c_str(),gamemode);
  }
  ADD_SERVER_COMMAND("agnextmode",nextmode);
}

void AgGameMode::LoadGames()
{
  if (0 != m_mapGames.size())
    return; //Already loaded.
  
  char	szDir[MAX_PATH];
  sprintf(szDir, "%s/gamemodes", AgGetDirectory());

  AgStringSet setFiles;
  AgDirList(szDir,setFiles);

  for (AgStringSet::iterator itrFiles = setFiles.begin() ;itrFiles != setFiles.end(); ++itrFiles)
  {
    AgString sFile = AgString(szDir) + "/" + *itrFiles;

    //AgConsole(UTIL_VarArgs("Found gamemode file %s",sFile.c_str()));

    if (!strstr(sFile.c_str(),".cfg"))
      continue;

    //Read the description lines.
    FILE* pFile = fopen(sFile.c_str(),"r");
	  if (!pFile)
      continue;

    char	szData[4096];
    int iRead = fread(szData,sizeof(char),sizeof(szData)-2,pFile);
    fclose(pFile);
    if (0 >= iRead)
      continue;
    szData[iRead] = '\0';

    AgGame* pGame = new AgGame;
    char* pszParse = NULL;
    pszParse = strtok(szData, "\n");
    if (pszParse)
    {
      pGame->m_sCfg = *itrFiles;
      pGame->m_sShortname = pGame->m_sCfg.substr(0,pGame->m_sCfg.size() - 4);
      pGame->m_sName = pszParse;
      pGame->m_sName = pGame->m_sName.substr(2);

      pszParse = strtok(NULL, "\n");
      if (pszParse)
      {
        pGame->m_sDescription = pszParse;
        pGame->m_sDescription = pGame->m_sDescription.substr(2);
      }

      if (pGame->IsValid())
      {
        AgTrim(pGame->m_sShortname);
        AgTrim(pGame->m_sDescription);
        AgTrim(pGame->m_sName);
        AgTrim(pGame->m_sCfg);
        m_mapGames.insert(AgGameMap::value_type(pGame->m_sShortname,pGame));
        AgConsole(UTIL_VarArgs("Added gamemode %s",pGame->m_sShortname.c_str()));
      }
      else
        delete pGame;
    }
  }
}

//-- Martin Webrant
