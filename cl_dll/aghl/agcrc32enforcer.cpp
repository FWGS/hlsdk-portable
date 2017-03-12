#include "agcrc32enforcer.h"
#include "hud.h"
#include "cl_util.h"

static char szDisconnect[] = "disconnect\n";

struct FILES
{
  char*		pszFile;
	WORD32	w32CheckSum;
};

static FILES s_Files[] = 
{
		"gfx.wad", 1240178454,
//		"halflife.wad", 1657905259,
		"liquids.wad", 1067140096, 
		"models/player/hgrunt/hgrunt.mdl", 4178952236,
		"models/player/scientist/scientist.mdl", 801952511,
		"models/player/gordon/gordon.mdl", 1899521925,
		"models/player/helmet/helmet.mdl", 413544432,
		"models/player/robo/robo.mdl", 1066728661,
		"models/player/barney/barney.mdl", 2342238586,
		"models/player/recon/recon.mdl", 196824764,
		"models/player/zombie/zombie.mdl", 2613106147,
		"models/player/gman/gman.mdl", 363240166,
		"models/player.mdl", 348061911,
		"models/flag.mdl", 1312518787,
		"models/p_crowbar.mdl", 2596481415,
		"models/p_9mmhandgun.mdl", 459325257,
		"models/p_9mmAR.mdl", 786579345,
		"models/p_357.mdl", 96835772,
		"models/p_gauss.mdl", 2413538144,
		"models/p_rpg.mdl", 3201388383,
		"models/p_crossbow.mdl", 1075131750,
		"models/p_egon.mdl", 1795269724,
		"models/p_tripmine.mdl", 2904825111,
		"models/p_satchel.mdl", 1240685151,
		"models/p_satchel_radio.mdl", 3737744643,
		"models/p_shotgun.mdl", 2602382707,
		"models/p_grenade.mdl", 3427694132,
		"models/p_squeak.mdl", 472781321,
		"models/p_hgun.mdl", 2327206545,
//		"models/player/blue/blue.mdl", 3578029767,
//		"models/player/red/red.mdl", 834545538,
};

WORD32 AgCRC32EnforceFileInternal(char* pszFile)
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

bool AgCRC32EnforceFile(char* pszFile, WORD32 w32CheckSum)
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
#ifndef _DEBUG
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
	  #ifndef _DEBUG
			  bPassed = false;
	  #endif
    }

    if (!(w32CheckSumRed == OLD_RED || w32CheckSumRed == NEW_RED || w32CheckSumRed == PLAYER))
    {
			  char szMessage[256];
			  sprintf(szMessage,"File check enforced and %s is either damaged or changed. Run scandisk and reinstall file.\n", "models/player/red/red.mdl");
			  AgLog(szMessage);
			  ConsolePrint(szMessage);
	  #ifndef _DEBUG
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