//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "agglobal.h"
#include "player.h"
#include "gamerules.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int gmsgCountdown;
extern int gmsgCheatCheck;

AgMatch::AgMatch()
{
  m_fMatchStart = 0.0; 
  m_fNextSay = 0.0;
  m_fNextHLTV = 0.0;
  CVAR_SET_FLOAT("sv_ag_match_running",0);
  CVAR_SET_FLOAT("ag_spectalk",1);
  CVAR_SET_FLOAT("sv_ag_show_gibs",1);
}

AgMatch::~AgMatch()
{

}


void AgMatch::Think()
{
  if (m_fMatchStart > 0 )
  {
    if (m_fMatchStart < gpGlobals->time)
    {
      //Start it
      MatchStart();
    }
    else
    {
      //Countdown
      if (m_fNextSay < gpGlobals->time)
      {
#ifdef AG_NO_CLIENT_DLL
        //Play countdown beeb
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && !pPlayerLoop->IsSpectator())
            AgPlayCountdown(pPlayerLoop,(int)(m_fMatchStart - gpGlobals->time));
        }
				char szMatchStart[16];
				sprintf(szMatchStart,"%d",(int)(m_fMatchStart - gpGlobals->time));
				AgSay(NULL,szMatchStart,&m_fNextSay,1,0.5,0.45);
#else
        //Play countdown beeb
        MESSAGE_BEGIN( MSG_BROADCAST, gmsgCountdown);
          WRITE_BYTE( (int)(m_fMatchStart - gpGlobals->time) );
          WRITE_BYTE( 1 );
          WRITE_STRING( "" );
          WRITE_STRING( "" );
        MESSAGE_END();
        m_fNextSay = gpGlobals->time + 1.0;
#endif
      }
    }
  }
  /*
  else if (g_bPaused)
  {
    AgSay(NULL,"Game is paused!\n",&m_fNextSay,5,0.4);
  }
  */
  else if (0 == timelimit.value && 0 == fraglimit.value)
  {
    //Stop the match since it could go on forever.
    Abort();
  }
  
  if (m_fNextHLTV < gpGlobals->time)
  {
    //Send again in one minute.
    m_fNextHLTV = gpGlobals->time + 60.0; 
    //Spectator scoreboards
    g_pGameRules->HLTV_ResendScoreBoard();
  }
}


void AgMatch::Start(const AgString& sSpawn)
{
  if (m_fMatchStart > 0 || 0 == timelimit.value && 0 == fraglimit.value) 
    return;

  //Count players
  int iPlayers = 0;
  int i = 0;
  for ( i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && !pPlayerLoop->IsSpectator())
      iPlayers++;
  }

  if (iPlayers < (int)ag_start_minplayers.value)
  {
    UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("You need %d players to start a game on this server.",(int)ag_start_minplayers.value));
    return;
  }

  //Set match flag. (All entered after matchstart will go into specmode and they cant respawn.)
  CVAR_SET_FLOAT("sv_ag_match_running",1);
  CVAR_SET_FLOAT("ag_spectalk",0);
  CVAR_SET_FLOAT("sv_ag_show_gibs",0);

  //Set match start time.
  m_fMatchStart = gpGlobals->time + 10.0;

  m_sSpawnFlag = sSpawn;

  //Pause the game
  g_bPaused = true; 
}


void AgMatch::MatchStart()
{
  m_fMatchStart = -1;

  //Reset score cache.
  g_pGameRules->m_ScoreCache.Reset();

  //Reset available timeouts.
  g_pGameRules->m_Timeout.Reset();

  //Reset map.
  AgResetMap();

  //Reset CTF score
  g_pGameRules->m_CTF.ResetScore();
  //++ muphicks
  if (DOM == AgGametype())
    g_pGameRules->m_DOM.ResetControlPoints();
  //-- muphicks

  //Loop through all active players, reset Score and respawn.
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      if (pPlayerLoop->IsSpectator())
      {
        //Regular spectators aint spawned when match is restarted.
        pPlayerLoop->ResetScore();      //Reset the score.
        if (strstr(m_sSpawnFlag.c_str(),"nolock"))
          pPlayerLoop->SetIngame(true);  
        else
          pPlayerLoop->SetIngame(false); //Player is not allowed to enter the game if he dropped. 

        pPlayerLoop->SetDisplayGamemode(2); //Show settings.
        g_pGameRules->m_ScoreCache.UpdateScore(pPlayerLoop);
        continue;
      }
      else
      {
        if (strstr(m_sSpawnFlag.c_str(),"full"))
          pPlayerLoop->SetSpawnFull(true);
        else
          pPlayerLoop->SetSpawnFull(false);
        pPlayerLoop->SetIngame(true);   //Player is allowed to enter the game if he dropped.
        pPlayerLoop->ResetScore();      //Reset the score.
        pPlayerLoop->RespawnMatch();    //Now spawn the sucker :-)
        pPlayerLoop->SetDisplayGamemode(2); //Show settings.
        
        g_pGameRules->m_ScoreCache.UpdateScore(pPlayerLoop);
      }
    }
  }

#ifndef AG_NO_CLIENT_DLL
  //Stop countdown
  MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
    WRITE_BYTE( -1 );
    WRITE_BYTE( 0 );
    WRITE_STRING( "" );
    WRITE_STRING( "" );
  MESSAGE_END();
#endif

  //Reset spawn full variable.
  m_sSpawnFlag = "";

  //Remove pause.
  g_bPaused = false;

  //Reset timer.
  g_pGameRules->m_Timer.Reset();

  //Score log
  g_pGameRules->m_ScoreLog.Start();

  //Spectator scoreboards
  g_pGameRules->HLTV_ResendScoreBoard();
}

void AgMatch::Abort()
{
  //Turn off match
  CVAR_SET_FLOAT("sv_ag_match_running",0);
  CVAR_SET_FLOAT("sv_ag_show_gibs",1);
  CVAR_SET_FLOAT("ag_spectalk",1);

  m_fMatchStart = -1;

  //Score log off
  g_pGameRules->m_ScoreLog.End();

  //Remove pause.
  g_bPaused = false;

  //Loop through all active players.
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      if (pPlayerLoop->IsSpectator())
      {
        //Regular spectators aint spawned when match is restarted.
        pPlayerLoop->ResetScore();      //Reset the score.
        pPlayerLoop->SetIngame(true);   
        pPlayerLoop->Spectate_Stop(true);
      }
      pPlayerLoop->SetDisplayGamemode(2); //Show settings.
    }
  }
}

void AgMatch::Allow(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  pPlayer->SetIngame(true);
  g_pGameRules->m_ScoreCache.UpdateScore(pPlayer);
  if (pPlayer->IsSpectator())
  {
    pPlayer->Spectate_Stop(true);
  }

  AgString sText;
  sText = UTIL_VarArgs("\"%s\" is now allowed to enter the game",pPlayer->GetName());
  AgSay(NULL,sText);
  AgConsole(sText);
}

//-- Martin Webrant
