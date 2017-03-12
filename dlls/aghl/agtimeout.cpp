//++ BulliT


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "game.h"
#include "player.h"

#include "agtimeout.h"
#include "agglobal.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern int gmsgTimeout;

AgTimeout::AgTimeout()
{
  m_fDisablePause = 0.0;
  m_fTimeout = 0.0;
  m_State = Inactive;
}

AgTimeout::~AgTimeout()
{

}


void AgTimeout::Think()
{
  if (Inactive == m_State)
    return;

  if (Called == m_State)
  {
    if (m_fTimeout < AgTime())
    {
      m_State = Pause;
      //Stop sounds.
      for ( int i = 1; i <= gpGlobals->maxClients; i++ )
      {
        CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
        if (pPlayerLoop)
          CLIENT_COMMAND(pPlayerLoop->edict(),"stopsound\n");
      }
    }
    else
    {
#ifndef AG_NO_CLIENT_DLL
      MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimeout );
        WRITE_BYTE( m_State );
		    WRITE_BYTE( (int)(m_fTimeout - AgTime()) ); 
	    MESSAGE_END();
#endif
    }
  }
  else if (Countdown == m_State)
  {
    if (m_fTimeout < AgTime())
    {
      TogglePause(DisablePause);

#ifndef AG_NO_CLIENT_DLL
      MESSAGE_BEGIN( MSG_ALL, gmsgTimeout );
        WRITE_BYTE( 0 );
		    WRITE_BYTE( 0 ); 
	    MESSAGE_END();
#endif
    }
    else
    {
#ifndef AG_NO_CLIENT_DLL
      MESSAGE_BEGIN( MSG_BROADCAST, gmsgTimeout );
        WRITE_BYTE( m_State );
		    WRITE_BYTE( (int)(m_fTimeout - AgTime()) ); 
	    MESSAGE_END();
#endif
    }
  }
  else if (Pause == m_State)
  {
    m_fTimeout = AgTime() + 60; //60 sec timeout.
    TogglePause(Countdown);
    return;
  }

  //Check pause
  if (m_fDisablePause != 0.0 && m_fDisablePause < AgTime())
  {
	  if (0 < CVAR_GET_FLOAT("pausable"))
		  CVAR_SET_STRING("pausable", "0");
	  m_fDisablePause = 0.0;

	  if (DisablePause == m_State)
	  {
			m_State = Inactive;
#ifndef AG_NO_CLIENT_DLL
		  MESSAGE_BEGIN( MSG_ALL, gmsgTimeout );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 0 ); 
		  MESSAGE_END();
#endif
	  }
	  
  }
}

void AgTimeout::TogglePause(enumState State)
{
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && pPlayerLoop->IsPlayer())
    {
      CVAR_SET_STRING("pausable", "1");
      CLIENT_COMMAND(pPlayerLoop->edict(), "pause\n" ); //This will not trigg directly so we have to wait a second to release the pauseable.
      m_fDisablePause = AgTime() + 3;        //Let client have 2 second of pauseable 1 state.
      m_State = State;
      break;
    }
  }
}

bool AgTimeout::CanTimeout(CBasePlayer* pPlayer)
{
  AgString sSearch;

  if (g_pGameRules->IsTeamplay())
    sSearch = pPlayer->TeamID();
  else 
    sSearch = pPlayer->GetName();

  for (AgStringList::iterator itrStrings = m_lstStrings.begin() ;itrStrings != m_lstStrings.end(); ++itrStrings)
  {
    if (0 == stricmp((*itrStrings).c_str(),sSearch.c_str()))
      return false;
  }
  return true;
}

void AgTimeout::AddTimeout(CBasePlayer* pPlayer)
{
  if (g_pGameRules->IsTeamplay())
    m_lstStrings.push_back(pPlayer->TeamID());
  else 
    m_lstStrings.push_back(pPlayer->GetName());

  UTIL_LogPrintf("\"%s<%d><%s><%s>\" triggered \"Timeout\"\n", 
                     pPlayer->GetName(), GETPLAYERUSERID(pPlayer->edict()), GETPLAYERAUTHID(pPlayer->edict()), pPlayer->TeamID());

}

void AgTimeout::Reset()
{
  if (m_lstStrings.size())
    m_lstStrings.erase(m_lstStrings.begin(),m_lstStrings.end());
}

void AgTimeout::Timeout(CBasePlayer* pPlayer)
{
#ifndef AG_NO_CLIENT_DLL
  if (0 < ag_match_running.value && NULL != pPlayer && Inactive == m_State && pPlayer->IsIngame() && !pPlayer->IsSpectator() && ag_allow_timeout.value > 0)
  {
    //Check if his team got more timeouts.
    if (!CanTimeout(pPlayer))
    {
      AgConsole("You already used your timeout\n",pPlayer);
      return;
    }
    AddTimeout(pPlayer);

    AgString sScore;
    sScore = "Timeout called by ";
    sScore += pPlayer->GetName();
    sScore += "\n";

    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop)
      {
        AgConsole(sScore.c_str(),pPlayerLoop);
      }
    }  


    m_State = Called;
    m_fTimeout = AgTime() + 10; //10 secs countdown
  }
  else
#endif
  {
      AgConsole("Timeout is not allowed\n",pPlayer);
      return;
  }
}
//-- Martin Webrant
