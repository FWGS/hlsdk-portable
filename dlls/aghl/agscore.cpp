//++ BulliT

#include "extdll.h"
#include "util.h"
#include "agglobal.h"
#include "agscore.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgScore::AgScore()
{
  m_iFrags = 0;
  m_iDeaths = 0;
  m_bIngame = false;
}

AgScore::~AgScore()
{

}

//-- Martin Webrant
