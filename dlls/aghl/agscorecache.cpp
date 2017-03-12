//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "agscore.h"
#include "agscorecache.h"
#include "agglobal.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgScoreCache::AgScoreCache()
{
}

AgScoreCache::~AgScoreCache()
{
  Reset();
}


AgScore* AgScoreCache::FindScore(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return NULL;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return NULL;

  if (g_bLangame)
  {
	AgScoreMap::iterator itrScores = m_mapScores.find(g_pGameRules->GetIPAddress(pPlayer->edict()));
	if (itrScores != m_mapScores.end())
	{
	  return (*itrScores).second;
	}
    return NULL;
  }



  //Search for auth id.

  AgScoreMap::iterator itrScores = m_mapScores.find(pPlayer->GetAuthID());
  if (itrScores != m_mapScores.end())
  {
    return (*itrScores).second;
  }

  return NULL;
}

void AgScoreCache::RestoreScore(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  AgScore* pScore = FindScore(pPlayer);

  if (pScore)
  {
    pPlayer->pev->frags = pScore->m_iFrags;
    pPlayer->m_iDeaths = pScore->m_iDeaths;

    ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Your score was autorestored.\n");
  }
}

void AgScoreCache::RestoreInGame(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  AgScore* pScore = FindScore(pPlayer);
  if (pScore)
    pPlayer->SetIngame(pScore->m_bIngame);
}


void AgScoreCache::UpdateScore(CBasePlayer* pPlayer)
{
  ASSERT(NULL != pPlayer);
  if (!pPlayer)
    return;
  ASSERT(NULL != pPlayer->pev);
  if (!pPlayer->pev)
    return;

  if (1 > ag_match_running.value)
    return;

  if (pPlayer->IsProxy() || pPlayer->pev->flags & FL_FAKECLIENT)
    return;

  AgScore* pScore = FindScore(pPlayer);

  if (!pScore)
  {

    pScore = new AgScore;

    if (!g_bLangame)
      m_mapScores.insert(AgScoreMap::value_type(pPlayer->GetAuthID(),pScore));
    else
      m_mapScores.insert(AgScoreMap::value_type(g_pGameRules->GetIPAddress(pPlayer->edict()),pScore));
  }

  
  pScore->m_iFrags = (int)pPlayer->pev->frags;
  pScore->m_iDeaths = pPlayer->m_iDeaths;
  pScore->m_bIngame = pPlayer->IsIngame();
}

void AgScoreCache::Reset()
{
  for (AgScoreMap::iterator itrScores = m_mapScores.begin() ;itrScores != m_mapScores.end(); ++itrScores)
    delete (*itrScores).second;

  m_mapScores.clear();
}


//-- Martin Webrant
