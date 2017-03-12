//++ BulliT

#if !defined(AFX_AGSCORECACHE_H__92DDA4B4_AB28_483B_8028_FAAB0667ECD4__INCLUDED_)
#define AFX_AGSCORECACHE_H__92DDA4B4_AB28_483B_8028_FAAB0667ECD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "agscore.h"

class CBasePlayer;
class AgScoreCache  
{
  typedef map<AgString,AgScore*, less<AgString> > AgScoreMap;
  AgScoreMap m_mapScores; //Holds the map score for each player.

public:
	AgScoreCache();
	virtual ~AgScoreCache();

  //Score cache.
  AgScore* FindScore(CBasePlayer* pPlayer);     //Find a score for a player.
  void     RestoreScore(CBasePlayer* pPlayer);  //Restore his score from scorecache.
  void     UpdateScore(CBasePlayer* pPlayer);   //Update the cache with current score.
  void     RestoreInGame(CBasePlayer* pPlayer); //Restore ingame flag from the scorecache.
  void     Reset();                             //Reset it.
};

#endif // !defined(AFX_AGSCORECACHE_H__92DDA4B4_AB28_483B_8028_FAAB0667ECD4__INCLUDED_)

//-- Martin Webrant
