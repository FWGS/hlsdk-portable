//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "agglobal.h"
#include "agvote.h"
#include "agcommand.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int gmsgVote;

FILE_GLOBAL char* s_szCommands[] =
{
  "yes - Vote yes.",
  "no  - Vote no.",
  "<vote> <value> - Call a vote.",
  "aglistvotes - List available votes."
};

FILE_GLOBAL char* s_szVotes[] =
{
  "agkick <name/#number> - Kick a player.",
  "agadmin <name/#number> - Vote a player admin.",
  "agstart <full/nolock> - Start a match. (full as value to start with all weps)",
  "agabort - Abort a match.",
  "agallow <name> - Allow a player into the match.",
  "agpause - Pause server.",
  "agmap <mapname> - Change level.",
  "agnextmap <mapname> - Change level after this is done.",
  "ag_spectalk <0/1> - Allow spectators to talk to all.",
  "agnextmode <mode> - Change mode after this level.",
};

AgVote::AgVote()
{
  m_fNextCount = 0.0;
  m_fMaxTime = 0.0;
  m_fNextVote = AgTime();
  m_bRunning = false;
}

AgVote::~AgVote()
{

}

bool AgVote::HandleCommand(CBasePlayer* pPlayer)
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

  if (1 > ag_allow_vote.value)
    return false;

  if (FStrEq(CMD_ARGV(0), "help"))
  {
    for (int i = 0; i < sizeof(s_szCommands)/sizeof(s_szCommands[0]); i++)
      AgConsole(s_szCommands[i],pPlayer);
    
    return true;
  }

  if (FStrEq(CMD_ARGV(0), "aglistvotes"))
  {
    for (int i = 0; i < sizeof(s_szVotes)/sizeof(s_szVotes[0]); i++)
      AgConsole(s_szVotes[i],pPlayer);

    GameMode.Help(pPlayer);

    return true;
  }

  //Atleast two players.
  int iPlayers = 0;
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop && !pPlayerLoop->IsProxy())
      iPlayers++;
  }

  /*
  if (IS_DEDICATED_SERVER() && 2 > iPlayers) 
    return true;
  */
  if (m_fMaxTime || m_fNextCount)
  {
    if (FStrEq("yes",CMD_ARGV(0)))
    {
      pPlayer->m_iVote = 1;
      return true;
    }
    else if (FStrEq("no",CMD_ARGV(0)))
    {
      pPlayer->m_iVote = 0;
      return true;
    }
    else
    {
      AgConsole("Vote is running, type yes or no in console.",pPlayer);
      return true;
    }
  }
  else
  {
    if (m_fNextVote > AgTime())
    {
      AgConsole(UTIL_VarArgs("Last vote was not accepted - %d seconds until next vote can be called.",(int)(m_fNextVote - AgTime())),pPlayer);

      return true;
    }

    ResetVote();

    if (FStrEq("callvote",CMD_ARGV(0)) || FStrEq("vote",CMD_ARGV(0)))
    {
      if (2 <= CMD_ARGC())
        m_sVote = CMD_ARGV(1);
      if (3 <= CMD_ARGC())
        m_sValue = CMD_ARGV(2);
    }
    else
    {
      if (1 <= CMD_ARGC())
        m_sVote = CMD_ARGV(0);
      if (2 <= CMD_ARGC())
        m_sValue = CMD_ARGS();
    }

    if (m_sVote.size() && 32 > m_sVote.size() && 32 > m_sValue.size())
    {
      //Check map
      if (FStrEq(m_sVote.c_str(),"agmap") ||
          FStrEq(m_sVote.c_str(),"changelevel") ||
          FStrEq(m_sVote.c_str(),"map"))
      {
        if (!ag_vote_map.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        char szTemp[64];
        strcpy(szTemp,m_sValue.c_str());

        //Check if it exists.
        if (IS_MAP_VALID(szTemp))
        {
          m_sVote = "agmap";
          CallVote(pPlayer);
        }
        else
          AgConsole("Map doesn't exist on server.",pPlayer);

        return true;
      }
      //Check nextmap
      else if (FStrEq(m_sVote.c_str(),"agnextmap"))
      { 
        if (!ag_vote_map.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        char szTemp[64];
        strcpy(szTemp,m_sValue.c_str());

        //Check if it exists.
        if (IS_MAP_VALID(szTemp))
        {
          m_sVote = "agnextmap";
          CallVote(pPlayer);
        }
        else
          AgConsole("Map doesn't exist on server.",pPlayer);
        return true;
      }
      //Check mode
      else if (GameMode.IsGamemode(m_sVote))
      {
        if (!ag_vote_gamemode.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        if (!GameMode.IsAllowedGamemode(m_sVote,pPlayer))
        {
          AgConsole("Gamemode not allowed by server admin.",pPlayer);
          return true;
        }
        
        CallVote(pPlayer);
        return true;
      }
     //Check nextmode
      else if (FStrEq(m_sVote.c_str(),"agnextmode"))
      { 
        if (!ag_vote_gamemode.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        if (!GameMode.IsAllowedGamemode(m_sValue,pPlayer))
        {
          AgConsole("Gamemode not allowed by server admin.",pPlayer);
          return true;
        }
        
        CallVote(pPlayer);

        return true;
      }
      //Start and pause should be there.
      else if (FStrEq(m_sVote.c_str(),"agstart") ||
               FStrEq(m_sVote.c_str(),"agabort") ||
               FStrEq(m_sVote.c_str(),"agpause"))
      {
        if (LMS == AgGametype() || ARENA == AgGametype())
        {
          AgConsole("Vote is not allowed in this gamemode.",pPlayer);
          return true;
        }
        
        if (1 > ag_vote_start.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        
        CallVote(pPlayer);
        return true;
      }
      //Check command
      else if (FStrEq(m_sVote.c_str(),"agallow") ||
               FStrEq(m_sVote.c_str(),"agkick") ||
               FStrEq(m_sVote.c_str(),"agadmin") )
      {
        if (FStrEq(m_sVote.c_str(),"agkick") && 1 > ag_vote_kick.value
          ||FStrEq(m_sVote.c_str(),"agadmin") && 1 > ag_vote_admin.value
          ||FStrEq(m_sVote.c_str(),"agallow") && 1 > ag_vote_allow.value
          )
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        CBasePlayer* pPlayerLoop = AgPlayerByName(m_sValue);
        if (pPlayerLoop)
        {
          m_sAuthID = pPlayerLoop->GetAuthID();
          CallVote(pPlayer);
        }
        else
        {
          if (!FStrEq(m_sVote.c_str(),"agkick"))
          {
            m_sValue = pPlayer->GetName();
            m_sAuthID = pPlayer->GetAuthID();
            CallVote(pPlayer);
          }
          else
           AgConsole("No such player exist on server.",pPlayer);
        }
        return true;
      }
      //Check setting
      else if ((
        0 == strncmp(m_sVote.c_str(),"ag_gauss_fix",12) || 
        0 == strncmp(m_sVote.c_str(),"ag_rpg_fix",10) ||
        0 == strncmp(m_sVote.c_str(),"ag_spectalk",11) || 
        0 == strncmp(m_sVote.c_str(),"mp_friendlyfire",15) ||
        0 == strncmp(m_sVote.c_str(),"mp_weaponstay",13))
        && m_sValue.size())
      {
        if (!ag_vote_setting.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }

        CallVote(pPlayer);
        return true;
      }
      else if (0 == strncmp(m_sVote.c_str(),"mp_timelimit",12))
      {
        if (!ag_vote_setting.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        if (atoi(m_sValue.c_str()) < ag_vote_mp_timelimit_low.value || atoi(m_sValue.c_str()) > ag_vote_mp_timelimit_high.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        CallVote(pPlayer);
        return true;
      }
      else if (0 == strncmp(m_sVote.c_str(),"mp_fraglimit",12))
      {
        if (!ag_vote_setting.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        if (atoi(m_sValue.c_str()) < ag_vote_mp_fraglimit_low.value || atoi(m_sValue.c_str()) > ag_vote_mp_fraglimit_high.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        CallVote(pPlayer);
        return true;
      }
      else if (0 == strncmp(m_sVote.c_str(),"sv_maxspeed",11))
      {
        if (!ag_vote_setting.value)
        {
          AgConsole("Vote is not allowed by server admin.",pPlayer);
          return true;
        }
        if (atoi(m_sValue.c_str()) < 270 || atoi(m_sValue.c_str()) > 350)
        {
          AgConsole("Maxpeed should be between 270 and 350.",pPlayer);
          return true;
        }
        CallVote(pPlayer);
        return true;
      }
    }
  }

  return false;
}

bool AgVote::CallVote(CBasePlayer* pPlayer)
{
  m_fMaxTime = AgTime() + 30.0;  //30 seconds is enough.
  m_fNextCount = AgTime();       //Next count directly
  pPlayer->m_iVote = 1;          //Voter voted yes
#ifdef _DEBUG
  pPlayer->m_iVote = 0;
#endif
  m_sCalled = pPlayer->GetName();
  m_bRunning = true;

  //++ muphicks
  UTIL_LogPrintf("\"%s<%d><%s><%s>\" triggered \"calledvote\" (votename \"%s\") (newsetting \"%s\")\n", 
                     pPlayer->GetName(), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID(pPlayer->edict()), pPlayer->TeamID(), 
                     m_sVote.c_str(),m_sValue.c_str()
                     );
  //-- muphicks

  return false;
}


void AgVote::Think()
{
  if (!m_bRunning)
    return;

  //Count votes.
  if (m_fNextCount != 0.0 && m_fNextCount < AgTime())
  {	
    int iFor,iAgainst,iUndecided,iPlayers;
    iFor = iAgainst = iUndecided = iPlayers = 0;

    //Count players
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop && !pPlayerLoop->IsProxy())
      {
        iPlayers++;

        if (1 == pPlayerLoop->m_iVote)
          iFor++;
        else if (0 == pPlayerLoop->m_iVote)
          iAgainst++;
        else
          iUndecided++;
      }
    }

    //Check if enough.
    if (((float)iFor / (float)iPlayers > 0.5))
    {
#ifdef AG_NO_CLIENT_DLL
			UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("Vote: %s %s\nCalled by: %s\nAccepted!",m_sVote.c_str(),m_sValue.c_str(),m_sCalled.c_str()));
#else
			MESSAGE_BEGIN( MSG_BROADCAST, gmsgVote, NULL );
				WRITE_BYTE( Accepted );
				WRITE_BYTE( iFor );
				WRITE_BYTE( iAgainst );
				WRITE_BYTE( iUndecided );
				WRITE_STRING( m_sVote.c_str() );
				WRITE_STRING( m_sValue.c_str() );
				WRITE_STRING( m_sCalled.c_str() );
			MESSAGE_END();
#endif

      //Exec vote.
      if (FStrEq(m_sVote.c_str(),"agadmin"))
      {
        for (int i = 1; i <= gpGlobals->maxClients; i++)
        {
          CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
          if (pPlayerLoop && pPlayerLoop->GetAuthID() == m_sAuthID)
          {
            pPlayerLoop->SetIsAdmin(true);
            break;
          }
        }
      }
      else if (FStrEq(m_sVote.c_str(),"agallow"))
      {
        Command.Allow(m_sValue);
      }
      else if (FStrEq(m_sVote.c_str(),"agmap"))
      {
        Command.Map(m_sValue);
      }
      else if (FStrEq(m_sVote.c_str(),"agnextmap"))
      {
        Command.NextMap(m_sValue);
      }
      else if (FStrEq(m_sVote.c_str(),"agstart"))
      {
        Command.Start(m_sValue);
      }
      else if (FStrEq(m_sVote.c_str(),"agpause"))
      {
        Command.Pause(NULL);
      }
      else if (FStrEq(m_sVote.c_str(),"agabort"))
      {
        Command.Abort(NULL);
      }
      else if (GameMode.IsAllowedGamemode(m_sVote))
      {
        GameMode.Gamemode(m_sVote);
      }
      else if (FStrEq(m_sVote.c_str(),"agnextmode"))
      {
        GameMode.NextGamemode(m_sValue);
      }
      else if (FStrEq(m_sVote.c_str(),"agkick"))
      {
        Command.Kick(m_sValue);
      }
      else
      {
        Command.Setting(m_sVote.c_str(),m_sValue);
      }

      ResetVote();
    }
    else
    {
      if (m_fMaxTime < AgTime())
      {
#ifdef AG_NO_CLIENT_DLL
				UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("Vote: %s %s\nCalled by: %s\nDenied!",m_sVote.c_str(),m_sValue.c_str(),m_sCalled.c_str()));
#else
		    MESSAGE_BEGIN( MSG_BROADCAST, gmsgVote, NULL );
			    WRITE_BYTE( Denied );
			    WRITE_BYTE( iFor );
          WRITE_BYTE( iAgainst );
          WRITE_BYTE( iUndecided );
          WRITE_STRING( m_sVote.c_str() );
          WRITE_STRING( m_sValue.c_str() );
          WRITE_STRING( m_sCalled.c_str() );
		    MESSAGE_END();
#endif

        ResetVote();
        m_fNextVote = ag_vote_failed_time.value + AgTime();
      }
      else
      {
#ifdef AG_NO_CLIENT_DLL
				UTIL_ClientPrintAll(HUD_PRINTCENTER, UTIL_VarArgs("Vote: %s %s\nCalled by: %s\nFor: %d\nAgainst: %d\nUndecided: %d",m_sVote.c_str(),m_sValue.c_str(),m_sCalled.c_str(), iFor, iAgainst, iUndecided));
#else
		    MESSAGE_BEGIN( MSG_BROADCAST, gmsgVote, NULL );
			    WRITE_BYTE( Called );
			    WRITE_BYTE( iFor );
          WRITE_BYTE( iAgainst );
          WRITE_BYTE( iUndecided );
          WRITE_STRING( m_sVote.c_str() );
          WRITE_STRING( m_sValue.c_str() );
          WRITE_STRING( m_sCalled.c_str() );
		    MESSAGE_END();
#endif
        m_fNextCount = AgTime() + 2.0; //Two more seconds.
      }
    }
  }
}


bool AgVote::ResetVote()
{
  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
      pPlayerLoop->m_iVote = -1;
  }

  m_sVote = "";
  m_sValue = "";
  m_sCalled = "";
  m_fNextCount = 0.0;
  m_fMaxTime = 0.0;
  m_sAuthID = "";
  m_fNextVote = AgTime();
  m_bRunning = false;
  return true;
}

//-- Martin Webrant
