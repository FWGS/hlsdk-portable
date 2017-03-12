//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"
#include "studio_util.h"
#include "r_studioint.h"
#include "parsemsg.h"
#include "AgModel.h"
#include "AgModelCheck.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef AG_USE_CHEATPROTECTION


AgModelCheck g_ModelCheck;
extern int g_iPure;
extern engine_studio_api_t IEngineStudio;

static char* s_szPure1[] =
{
  "gordon",
  "helmet",
  "zombie",
  "robo",
  "recon",
  "hgrunt",
  "gman",
  "barney",
  "gina",
  "scientist",
  "robo",
  "pmers",
	"player",		
  "blue",
  "red",
};

static char szDisconnect[] = "disconnect\n";

AgModelCheck::AgModelCheck()
{
  m_bScannedStandard = false;
}

AgModelCheck::~AgModelCheck()
{
//  m_setChecked.erase();
}

bool AgModelCheck::Check()
{
  if (m_bScannedStandard)
    return CheckCurrent();

  for (int i = 0; i < (sizeof(s_szPure1) / sizeof(s_szPure1[0])); i++)
  {
    if (!CheckOne(s_szPure1[i]))
      return false;
  }
  m_bScannedStandard = true;
  return CheckCurrent();
}

bool AgModelCheck::CheckCurrent()
{
  bool bPassed = true;

	for ( int i = 1; i <= MAX_PLAYERS; i++ )
	{
		GetPlayerInfo( i, &g_PlayerInfoList[i] );
		if (NULL == g_PlayerInfoList[i].name)
      continue;

    bPassed = CheckOne(gEngfuncs.PlayerInfo_ValueForKey(i,"model"));
    if (!bPassed)
      break;
  }

  return bPassed;
}

bool AgModelCheck::CheckOne(const char* pszModel)
{
  bool bPassed = true;
  char szModel[MAX_PATH];
  sprintf(szModel,"/models/player/%s/%s.mdl",pszModel,pszModel);
  if (0 != m_setChecked.size() && m_setChecked.end() != m_setChecked.find(szModel))
    return true;

#ifdef _DEBUG
	char szMessage[256];
    sprintf(szMessage,"Checking %s\n",szModel);
    ConsolePrint(szMessage);
#endif

  AgModel Model;
  bPassed = Model.CheckModel(szModel);
  bool bChecked = Model.IsChecked();

  if (!bPassed)
  {
    ServerCmd( "say <AG Mod> Disconnected for using invalid model.\n" );
	  ClientCmd( szDisconnect );
    return false;
  }
  if (bChecked)
  {
    //No need to try to load it again.
    m_setChecked.insert(szModel);
  }

  return true;
}

#endif //AG_USE_CHEATPROTECTION


//-- Martin Webrant
