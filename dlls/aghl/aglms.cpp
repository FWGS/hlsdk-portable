//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "agglobal.h"
#include "aglms.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//extern int gmsgCountdown;

AgLMS::AgLMS()
{
  m_fMatchStart = 0.0;
  m_fNextCountdown  = 0.0;
  m_fNextSay = 0.0;
  m_Status = Waiting;
}

AgLMS::~AgLMS()
{

}


void AgLMS::Think()
{
  if (!g_pGameRules)
    return;

  if (Playing == m_Status)
  {
    if (m_fNextCountdown > gpGlobals->time)
      return;
    m_fNextCountdown = gpGlobals->time + 0.5; 
    
    if (g_pGameRules->IsTeamplay())
    {
      //Teams.
      AgStringSet setTeams;
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop)
        {
          if (!pPlayerLoop->IsAlive())
          {
            pPlayerLoop->SetIngame(false); //Cant respawn
            if(!pPlayerLoop->IsSpectator())
            {
              //Quake1 teleport splash around him.
              MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_TELEPORT ); 
                WRITE_COORD(pPlayerLoop->pev->origin.x);
                WRITE_COORD(pPlayerLoop->pev->origin.y);
                WRITE_COORD(pPlayerLoop->pev->origin.z);
              MESSAGE_END();

              pPlayerLoop->Spectate_Start();
              pPlayerLoop->Observer_SetMode(OBS_IN_EYE);
            }
          }
          else 
          {
            setTeams.insert(pPlayerLoop->m_szTeamName);
          }
        }
      }
      
      if (!(setTeams.size() > 1))
      {
          m_sWinner = "";
          AgStringSet::iterator itrTeams = setTeams.begin();
          if (itrTeams != setTeams.end())
          {
            m_sWinner = *itrTeams;
          }
          m_Status = Waiting;
      }
    }
    else
    {
      int iPlayersAlive = 0;
      CBasePlayer* pPlayer = NULL;
    
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop)
        {
          if (!pPlayerLoop->IsAlive())
          {
            pPlayerLoop->SetIngame(false); //Cant respawn
            if(!pPlayerLoop->IsSpectator())
            {
              //Quake1 teleport splash around him.
              MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_TELEPORT ); 
                WRITE_COORD(pPlayerLoop->pev->origin.x);
                WRITE_COORD(pPlayerLoop->pev->origin.y);
                WRITE_COORD(pPlayerLoop->pev->origin.z);
              MESSAGE_END();

              pPlayerLoop->Spectate_Start();
              pPlayerLoop->Observer_SetMode(OBS_IN_EYE);
            }
          }
          else 
          {
            iPlayersAlive++;
            pPlayer = pPlayerLoop;
          }
        }
      }

      if (!(iPlayersAlive > 1))
      {
          m_sWinner = "";
          if (pPlayer)
            m_sWinner = pPlayer->GetName();
          m_Status = Waiting;
      }
    }
  }
  else
  {
    //We only update status once every second.
    if (m_fNextCountdown > gpGlobals->time)
      return;
    m_fNextCountdown = gpGlobals->time + 1.0; 

    //Handle the status
    if (Waiting == m_Status)
    {
      if (g_pGameRules->IsTeamplay())
      {
        //Teams.
        AgStringSet setTeams;
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && pPlayerLoop->m_bReady)
          {
            setTeams.insert(pPlayerLoop->m_szTeamName);
          }
        }
      
        if (setTeams.size() > 1)
        {
          m_Status = Countdown;
          m_fMatchStart = gpGlobals->time + 8.0;
          m_fNextCountdown = gpGlobals->time + 3.0; 
        }
      }
      else
      {
        int iPlayers = 0;
        for ( int i = 1; i <= gpGlobals->maxClients; i++ )
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && pPlayerLoop->m_bReady)
          {
            iPlayers++; 
          }
        }
        if (iPlayers > 1)
        {
          m_Status = Countdown;
          m_fMatchStart = gpGlobals->time + 8.0;
          m_fNextCountdown = gpGlobals->time + 3.0; 
        }
     }

      //Print winner
      if (0 != m_sWinner.size())
      {
        AgString s;
        s = "Last match won by " + m_sWinner;
        AgSay(NULL,s.c_str(),NULL,10,0.4,0.1,2);
        m_sWinner = "";
      }

      //Write waiting message
      AgSay(NULL,"Waiting for players to get ready!\n",&m_fNextSay,2,0.4,0.5);
    }
    else if (Countdown == m_Status)
    {
      if (m_fMatchStart < gpGlobals->time)
      {
        //Clear out the map
        AgResetMap();

        int i = 1;

        for ( i = 1; i <= gpGlobals->maxClients; i++ )
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && pPlayerLoop->m_bReady)
          {
            pPlayerLoop->SetIngame(true);
          }
        }

        m_Status = Spawning;
        m_sWinner = "";

        //Time to start playing.
        for ( i = 1; i <= gpGlobals->maxClients; i++ )
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && pPlayerLoop->m_bReady)
          {
            if (pPlayerLoop->IsSpectator())
            {
              pPlayerLoop->Spectate_Stop();
            }
            else
              pPlayerLoop->RespawnMatch();
          }
        }
     
        m_Status = Playing;
      }
      else
      {
        //Write countdown message.

        char szMatchStart[128];
        sprintf(szMatchStart,"Match will begin in %d seconds!\n",(int)(m_fMatchStart - gpGlobals->time));
        AgSay(NULL,szMatchStart,&m_fNextSay,1,0.3,0.5);
      }
    }
  }
}

void AgLMS::ClientConnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  //Set status
  pPlayer->SetIngame(false);
}


void AgLMS::ClientDisconnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  //Set status
  pPlayer->SetIngame(false);
}

//-- Martin Webrant
