#include "agcrc32enforcer.h"
#include "hud.h"
#include "cl_util.h"

static const char *szDisconnect = "disconnect\n";

struct FILES
{
  const char*		pszFile;
  WORD32	w32CheckSum;
};

static FILES s_Files[] = 
{
		"gfx.wad", 1240178454U,
//		"halflife.wad", 1657905259U,
		"liquids.wad", 1067140096U, 
		"models/player/hgrunt/hgrunt.mdl", 4178952236U,
		"models/player/scientist/scientist.mdl", 801952511U,
		"models/player/gordon/gordon.mdl", 1899521925U,
		"models/player/helmet/helmet.mdl", 413544432U,
		"models/player/robo/robo.mdl", 1066728661U,
		"models/player/barney/barney.mdl", 2342238586U,
		"models/player/recon/recon.mdl", 196824764U,
		"models/player/zombie/zombie.mdl", 2613106147U,
		"models/player/gman/gman.mdl", 363240166U,
		"models/player.mdl", 348061911U,
		"models/flag.mdl", 1312518787U,
		"models/p_crowbar.mdl", 2596481415U,
		"models/p_9mmhandgun.mdl", 459325257U,
		"models/p_9mmAR.mdl", 786579345U,
		"models/p_357.mdl", 96835772U,
		"models/p_gauss.mdl", 2413538144U,
		"models/p_rpg.mdl", 3201388383U,
		"models/p_crossbow.mdl", 1075131750U,
		"models/p_egon.mdl", 1795269724U,
		"models/p_tripmine.mdl", 2904825111U,
		"models/p_satchel.mdl", 1240685151U,
		"models/p_satchel_radio.mdl", 3737744643U,
		"models/p_shotgun.mdl", 2602382707U,
		"models/p_grenade.mdl", 3427694132U,
		"models/p_squeak.mdl", 472781321U,
		"models/p_hgun.mdl", 2327206545U,
//		"models/player/blue/blue.mdl", 3578029767U,
//		"models/player/red/red.mdl", 834545538U,
};

WORD32 AgCRC32EnforceFileInternal(const char* pszFile)
{
	int iLength = 0;
	void* pFile = gEngfuncs.COM_LoadFile(pszFile, 5, &iLength);
	if (pFile)
	{
		WORD32 w32CheckSumFile = AgCRC32(pFile, iLength);
		gEngfuncs.COM_FreeFile(pFile);
    return w32CheckSumFile;
	}
	return -1;
}

bool AgCRC32EnforceFile(const char* pszFile, WORD32 w32CheckSum)
{
	if (w32CheckSum != AgCRC32EnforceFileInternal(pszFile))
  {
		char szMessage[256];
		sprintf(szMessage,"File check enforced and %s is either damaged or changed. Run scandisk and reinstall file.\n", pszFile);
		AgLog(szMessage);
		ConsolePrint(szMessage);

    ServerCmd( "say <AG Mod> Disconnected for using invalid file.\n" );
	  ClientCmd( szDisconnect );
    return false;
  }
	return true;
}

bool AgCRC32EnforceFiles()
{
	bool bPassed = true;
  for (int i = 0; i < sizeof(s_Files)/sizeof(s_Files[0]); i++)
	{
		if (s_Files[i].w32CheckSum != AgCRC32EnforceFileInternal(s_Files[i].pszFile))
    {
		  char szMessage[256];
		  sprintf(szMessage,"File check enforced and %s is either damaged or changed. Run scandisk and reinstall file.\n", s_Files[i].pszFile);
		  AgLog(szMessage);
		  ConsolePrint(szMessage);
#if !_DEBUG
		  bPassed = false;
#endif
    }
	}

/*
  if (bPassed)
  {
    //Need special check for the 2 blue models...
#define OLD_BLUE 3578029767
#define NEW_BLUE 945015980
#define PLAYER 348061911

#define OLD_RED 834545538
#define NEW_RED 2809992869
#define PLAYER 348061911
    WORD32 w32CheckSumBlue = AgCRC32EnforceFileInternal("models/player/blue/blue.mdl");
    WORD32 w32CheckSumRed  = AgCRC32EnforceFileInternal("models/player/red/red.mdl");

    if (!(w32CheckSumBlue == OLD_BLUE || w32CheckSumBlue == NEW_BLUE || w32CheckSumBlue == PLAYER))
    {
			  char szMessage[256];
			  sprintf(szMessage,"File check enforced and %s is either damaged or changed. Run scandisk and reinstall file.\n", "models/player/blue/blue.mdl");
			  AgLog(szMessage);
			  ConsolePrint(szMessage);
	  #if !_DEBUG
			  bPassed = false;
	  #endif
    }

    if (!(w32CheckSumRed == OLD_RED || w32CheckSumRed == NEW_RED || w32CheckSumRed == PLAYER))
    {
			  char szMessage[256];
			  sprintf(szMessage,"File check enforced and %s is either damaged or changed. Run scandisk and reinstall file.\n", "models/player/red/red.mdl");
			  AgLog(szMessage);
			  ConsolePrint(szMessage);
	  #if !_DEBUG
			  bPassed = false;
	  #endif
    }

  }
*/
  if (!bPassed)
  {
    ServerCmd( "say <AG Mod> Disconnected for using invalid file.\n" );
	  ClientCmd( szDisconnect );
    return false;
  }

	return true;
}
