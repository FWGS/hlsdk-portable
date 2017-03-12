//++ BulliT


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "game.h"
#include "gamerules.h"

#include "agglobal.h"
#include "agsuddendeath.h"
#include "agscorelog.h"

extern int gmsgSuddenDeath;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgSuddenDeath::AgSuddenDeath()
{
}

AgSuddenDeath::~AgSuddenDeath()
{

}

bool AgSuddenDeath::IsSuddenDeath()
{
  bool bIsSuddenDeath = false;
  int iBestFrags = 0;
  AgScoreLogMap mapScores;
  g_pGameRules->m_ScoreLog.GetScores(mapScores);

  //Check if the top player/team is on same frags.

  if (1 < mapScores.size())
  {
    for (AgScoreLogMap::iterator itrScoreLog = mapScores.begin() ;itrScoreLog != mapScores.end(); ++itrScoreLog)
    {
	  int iFrags = (*itrScoreLog).second;
      if (iBestFrags < iFrags)
      {
        bIsSuddenDeath = false;
        iBestFrags = iFrags;
      }
      else if (iBestFrags == iFrags && 0 != iBestFrags)
      {
        bIsSuddenDeath = true;
      }
    }
  }
  
  MESSAGE_BEGIN( MSG_BROADCAST, gmsgSuddenDeath );
		WRITE_BYTE( bIsSuddenDeath ? 1 : 0 );                 
	MESSAGE_END();
  return bIsSuddenDeath;
}