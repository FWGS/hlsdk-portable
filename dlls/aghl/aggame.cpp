//++ BulliT

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"gamerules.h"
#include "aggame.h"
#include "agglobal.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgGame::AgGame()
{

}

AgGame::~AgGame()
{

}

bool AgGame::IsValid()
{
  return (0 != m_sShortname.size() &&
          10 > m_sShortname.size() &&
          0 != m_sName.size() &&
          0 != m_sCfg.size() &&
          0 != m_sDescription.size());
}

//-- Martin Webrant
