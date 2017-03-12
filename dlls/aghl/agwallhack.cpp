//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "agglobal.h"
#include "agwallhack.h"

extern int gmsgWallhack;
DLL_GLOBAL AgWallhack Wallhack;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
AgWallhack::AgWallhack()
{
}

AgWallhack::~AgWallhack()
{

}

void AgWallhack::Init()
{
  m_sData = AgReadFile("cheats.dat");
}

void AgWallhack::SendToPlayer(CBasePlayer* pPlayer)
{
  if (m_sData.size() > 190 || 0 == m_sData.size())
    return;

#ifdef AG_DISABLE_WALLHACK
  #pragma message("ENABLE WALLHACK CHECK")
  return;
#endif

#ifdef AG_USE_CHEATPROTECTION
  MESSAGE_BEGIN(MSG_ONE, gmsgWallhack, NULL, pPlayer->pev );
    WRITE_STRING(m_sData.c_str());
  MESSAGE_END();
#endif //AG_USE_CHEATPROTECTION
}

//-- Martin Webrant
