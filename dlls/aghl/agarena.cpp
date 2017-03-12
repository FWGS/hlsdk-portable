//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"

#include "aggamerules.h"
#include "agglobal.h"
#include "agarena.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern int gmsgCountdown;

AgArena::AgArena()
{
  m_fMatchStart = 0.0;
  m_fNextCountdown  = 0.0;
  m_fNextSay = 0.0;
  m_Player1 = NULL;
  m_Player2 = NULL;
  m_Status = Waiting;
}

AgArena::~AgArena()
{

}


void AgArena::Think()
{
  if (!g_pGameRules)
    return;

  if (Playing == m_Status)
  {
    CBasePlayer* pPlayer1 = GetPlayer1();
    CBasePlayer* pPlayer2 = GetPlayer2();

    if (!pPlayer1 || pPlayer1 && !pPlayer1->IsAlive() || !pPlayer2 || pPlayer2 && !pPlayer2->IsAlive())
    {
      m_Status = PlayerDied;
      m_fNextCountdown = gpGlobals->time + 3.0;  //Let the effect of him dying play for 3 seconds

      if (pPlayer1)
        pPlayer1->SetIngame(false); //Cant respawn
      if (pPlayer2)
        pPlayer2->SetIngame(false); //Cant respawn
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
      //Get new arena players.
      if (!GetPlayer1())
      {
        //Get one from top of list.
        if (0 != m_lstWaitList.size())
        {
          m_Player1 = AgPlayerByIndex(m_lstWaitList.front());
          m_lstWaitList.pop_front();
        }
      }
    
      if (!GetPlayer2())
      {
        //Get one from top of list.
        if (0 != m_lstWaitList.size())
        {
          m_Player2 = AgPlayerByIndex(m_lstWaitList.front());
          m_lstWaitList.pop_front();
        }
      }

      if (GetPlayer1() && GetPlayer2())
      {
        m_Status = Countdown;
        m_fMatchStart = gpGlobals->time + 5.0;
        m_fNextCountdown = gpGlobals->time + 2.0; 
      }


      //Write waiting message
#ifdef AG_NO_CLIENT_DLL
      if (0 != m_sWinner.size())
      {
        AgString s;
        s = "Last match won by " + m_sWinner;
        AgSay(NULL,s.c_str(),NULL,10,0.4,0.1,2);
        m_sWinner = "";
      }
      AgSay(NULL,"Waiting for players to get ready!\n",&m_fNextSay,2,0.4,0.5);
#else
      MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
        WRITE_BYTE( 50 );
        WRITE_BYTE( 1 );
        WRITE_STRING( m_sWinner.c_str() );
        WRITE_STRING( "" );
      MESSAGE_END();
#endif
    }
    else if (Countdown == m_Status)
    {
      if (!GetPlayer1() || !GetPlayer2())
      {
        m_Status = Waiting; //Someone left in middle of countdown. Go back to waiting.
        return;
      }
    
      if (m_fMatchStart < gpGlobals->time)
      {
        //Clear out the map
        AgResetMap();

        GetPlayer1()->SetIngame(true);
        GetPlayer2()->SetIngame(true);
        g_pGameRules->m_ScoreCache.UpdateScore(GetPlayer1());
        g_pGameRules->m_ScoreCache.UpdateScore(GetPlayer2());    

        m_Status = Spawning;
        m_sWinner = "";

        //Time to start playing.
        if (GetPlayer1()->IsSpectator())
        {
          GetPlayer1()->Spectate_Stop();
        }
        else
          GetPlayer1()->RespawnMatch();

        if (GetPlayer2()->IsSpectator())
        {
          GetPlayer2()->Spectate_Stop();
        }
        else
          GetPlayer2()->RespawnMatch();
      
        m_Status = Playing;

#ifndef AG_NO_CLIENT_DLL
        //Stop countdown
        MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
          WRITE_BYTE( -1 );
          WRITE_BYTE( 0 );
          WRITE_STRING( "" );
          WRITE_STRING( "" );
        MESSAGE_END();
#endif
      }
      else
      {
        //Write countdown message.
#ifdef AG_NO_CLIENT_DLL
        char szMatchStart[128];
        sprintf(szMatchStart,"Match %s vs %s in %d seconds!\n",GetPlayer1()->GetName(),GetPlayer2()->GetName(),(int)(m_fMatchStart - gpGlobals->time));
        AgSay(NULL,szMatchStart,&m_fNextSay,1,0.3,0.5);
#else
        MESSAGE_BEGIN( MSG_ALL, gmsgCountdown);
          WRITE_BYTE( (int)(m_fMatchStart - gpGlobals->time) );
          WRITE_BYTE( 1 );
          WRITE_STRING( GetPlayer1()->GetName() );
          WRITE_STRING( GetPlayer2()->GetName() );
        MESSAGE_END();
#endif
      }
    }
    else if (PlayerDied == m_Status)
    {
      CBasePlayer* pPlayer1 = GetPlayer1();
      CBasePlayer* pPlayer2 = GetPlayer2();

      m_Status = Waiting;
      m_sWinner = "";

      if (pPlayer1 && !pPlayer1->IsAlive())
      {
        if(!pPlayer1->IsSpectator())
        {
          pPlayer1->Spectate_Start(false);
          pPlayer1->Spectate_Follow(m_Player2,OBS_IN_EYE);
        }
        m_lstWaitList.push_back(pPlayer1->entindex());
        pPlayer1->SetIngame(false);
        m_Player1 = NULL;

        if (pPlayer2)
        {
          pPlayer2->SetIngame(false);
          if (pPlayer2->IsAlive())
            m_sWinner = pPlayer2->GetName();
        }
      }

      if (pPlayer2 && !pPlayer2->IsAlive())
      {
        if(!pPlayer2->IsSpectator())
        {
          pPlayer2->Spectate_Start(false);
          pPlayer2->Spectate_Follow(m_Player1,OBS_IN_EYE);
        }
        m_lstWaitList.push_back(pPlayer2->entindex());
        pPlayer2->SetIngame(false);
        m_Player2 = NULL;
        if (pPlayer1)
        {
          pPlayer1->SetIngame(false);
          if (pPlayer1->IsAlive())
            m_sWinner = pPlayer1->GetName();
        }
      }

      //Stop sounds.
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop)
        {
          CLIENT_COMMAND(pPlayerLoop->edict(),"stopsound\n");
        }
      }
    }
  }
}

void AgArena::Add(CBasePlayer* pPlayer)
{
  if (GetPlayer1() == pPlayer || GetPlayer2() == pPlayer)
    return;

  if (0 == m_lstWaitList.size())
  {
    m_lstWaitList.push_back(pPlayer->entindex());
  }
  else
  {
    AgWaitList::iterator itrWaitlist = find(m_lstWaitList.begin(),m_lstWaitList.end(),pPlayer->entindex());
    if (itrWaitlist == m_lstWaitList.end())
      m_lstWaitList.push_back(pPlayer->entindex());
  }
}

void AgArena::Remove(CBasePlayer* pPlayer)
{
  if (GetPlayer1() == pPlayer)
  {
    m_Player1 = NULL;
  }
  else if (GetPlayer2() == pPlayer)
  {
    m_Player2 = NULL;
  }
  else if (0 != m_lstWaitList.size())
  {
    AgWaitList::iterator itrWaitlist = find(m_lstWaitList.begin(),m_lstWaitList.end(),pPlayer->entindex());
    if (itrWaitlist == m_lstWaitList.end())
      m_lstWaitList.erase(itrWaitlist);
    //m_lstWaitList.remove(pPlayer->entindex());
  }
}

void AgArena::Ready(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  if (Countdown != m_Status)
  {
    Add(pPlayer);
    ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Changed mode to READY.\n");
  }
  else 
  {
    ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Can not change ready state at this point.\n");
  }
}

void AgArena::NotReady(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  if (Countdown != m_Status)
  {
    pPlayer->SetIngame(false);
    if(!pPlayer->IsSpectator())
    {
      pPlayer->Spectate_Start(false);
    }
    Remove(pPlayer);
    ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Changed mode to NOT READY.\n");
  }
  else 
  {
    ClientPrint( pPlayer->pev, HUD_PRINTCONSOLE, "Can not change ready state at this point.\n");
  }
}

void AgArena::ClientConnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  //Set status
  pPlayer->SetIngame(false);

  if (!pPlayer->IsProxy())
	Add(pPlayer);
}


void AgArena::ClientDisconnected(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  //Set status
  pPlayer->SetIngame(false);

  //Just blank if active in arena. 
  //This will put the status into waiting mode within a second.
  if (!pPlayer->IsProxy())
	Remove(pPlayer);
}


//-- Martin Webrant

