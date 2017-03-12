//++ BulliT


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

#include "agglobal.h"
#include "agscorelog.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgScoreLog::AgScoreLog()
{
  m_fNextLogUpdate = 0;
}

AgScoreLog::~AgScoreLog()
{
}

void AgScoreLog::Think()
{
  if (0 < ag_match_running.value && m_fNextLogUpdate < gpGlobals->time && g_pGameRules)
  {	
    if (0 < timelimit.value)
    {
      long lTime = (long)timelimit.value * 60 - (long)g_pGameRules->m_Timer.GetEffectiveTime();
      ldiv_t d = ldiv(lTime, 60L);
      UTIL_LogPrintf("World triggered \"Time left\" (Minutes \"%ld\") (Seconds \"%02ld\")\n", d.quot,d.rem);
    }
    
    Score();
    
    m_fNextLogUpdate = gpGlobals->time + 30.0;
  }
}

void AgScoreLog::Start()
{
  if (0 < ag_match_running.value)
  {
    UTIL_LogPrintf("World triggered \"Match started\"\n");
  }
}

void AgScoreLog::End()
{
  if (0 < ag_match_running.value)
  {
    UTIL_LogPrintf("World triggered \"Match over\"\n");
    EndScore();
    m_fNextLogUpdate = 0;
  }
}

void AgScoreLog::GetScores(AgScoreLogMap& mapScores)
{
  if (g_pGameRules->IsTeamplay())
  {
    
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop)
      {
        if (0 != strlen(pPlayerLoop->TeamID()) && !pPlayerLoop->IsSpectator())
        {
          AgScoreLogMap::iterator itrScoreLog = mapScores.find(pPlayerLoop->TeamID());
          if (itrScoreLog != mapScores.end())
          {
            (*itrScoreLog).second += (int)pPlayerLoop->pev->frags;
          }
          else
          {
            mapScores.insert(AgScoreLogMap::value_type(pPlayerLoop->TeamID(),pPlayerLoop->pev->frags));
          }
        }
      }
    }
  }
  else
  {
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
      CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
      if (pPlayerLoop)
      {
        if (0 != strlen(pPlayerLoop->GetName()) && !pPlayerLoop->IsSpectator())
        {
          mapScores.insert(AgScoreLogMap::value_type(pPlayerLoop->GetName(),pPlayerLoop->pev->frags));
        }
      }
    }
  }
}

void AgScoreLog::Score()
{
  AgScoreLogMap mapScores;
  GetScores(mapScores);
  
  char szScore[1200]; //32 players/teams with 32 chars names + score for each.
  szScore[0] = '\0';
  int iSize = 0;

  for (AgScoreLogMap::iterator itrScoreLog = mapScores.begin() ;itrScoreLog != mapScores.end(); ++itrScoreLog)
    iSize += sprintf(&szScore[iSize],"%s:%d ",(*itrScoreLog).first.c_str(),(*itrScoreLog).second);

  szScore[iSize] = '\0';

  UTIL_LogPrintf( "World triggered \"Score Update\" (scores \"%s\")\n", szScore );
  mapScores.clear();
}

void AgScoreLog::EndScore()
{
  AgScoreLogMap mapScores;
  GetScores(mapScores);
  
  char szScore[1200]; //32 players/teams with 32 chars names + score for each.
  szScore[0] = '\0';
  int iSize = 0;

  for (AgScoreLogMap::iterator itrScoreLog = mapScores.begin() ;itrScoreLog != mapScores.end(); ++itrScoreLog)
    iSize += sprintf(&szScore[iSize],"%s:%d ",(*itrScoreLog).first.c_str(),(*itrScoreLog).second);

  szScore[iSize] = '\0';

  UTIL_LogPrintf( "World triggered \"Score Update\" (scores \"%s\")\n", szScore );

  for ( int i = 1; i <= gpGlobals->maxClients; i++ )
  {
    CBasePlayer* pPlayerLoop = AgPlayerByIndex(i);
    if (pPlayerLoop)
    {
      AgString sScore;
      sScore = szScore;
      sScore += "\n";
      AgConsole(sScore.c_str(),pPlayerLoop);
    }
  }  

  mapScores.clear();
}

//-- Martin Webrant

