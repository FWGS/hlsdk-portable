//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "com_weapons.h"
#include "parsemsg.h"
#include "demo.h"
#include "demo_api.h"
#include "agglobal.h"
#include "aghudglobal.h"
#include "agmodelcheck.h"

#ifdef AG_USE_CHEATPROTECTION
#include "agwallhack.h"
#include "agcrc32enforcer.h"
#endif

#include "AgVariableChecker.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"
#include "AgVGuiMapBrowser.h"
#include "AgDownload.h"

DECLARE_MESSAGE(m_Global, PlaySound )
DECLARE_MESSAGE(m_Global, CheatCheck )
DECLARE_MESSAGE(m_Global, WhString )
DECLARE_MESSAGE(m_Global, SpikeCheck )
DECLARE_MESSAGE(m_Global, Gametype )
DECLARE_MESSAGE(m_Global, AuthID )
DECLARE_MESSAGE(m_Global, MapList )
DECLARE_MESSAGE(m_Global, CRC32 )
DECLARE_COMMAND(m_Global, Winamp);
DECLARE_COMMAND(m_Global, ToggleWinamp);
DECLARE_COMMAND(m_Global, ToggleMapBrowser);
DECLARE_COMMAND(m_Global, LoadAuthID);
DECLARE_COMMAND(m_Global, UnloadAuthID);
DECLARE_COMMAND(m_Global, AgRecord);

int       g_iPure = 1;
BYTE g_GameType = STANDARD;

typedef map<int, AgString, less<int> > AgPlayerToAuthID;
typedef map<AgString, AgString, less<AgString> > AgAuthIDToRealName;
static AgPlayerToAuthID s_mapAuthID;
static AgAuthIDToRealName s_mapRealName;


static int s_iCheckWallhack = 0;
extern cvar_t* g_pcl_scores;
int AgHudGlobal::Init(void)
{
  m_iFlags = 0;
	
  gHUD.AddHudElem(this);

	HOOK_MESSAGE( PlaySound );
	HOOK_MESSAGE( CheatCheck );
	HOOK_MESSAGE( WhString );
  HOOK_MESSAGE( SpikeCheck );
  HOOK_MESSAGE( Gametype );
  HOOK_MESSAGE( AuthID );
  HOOK_MESSAGE( MapList );
	HOOK_MESSAGE( CRC32 );
  HOOK_COMMAND("winamp",Winamp);
  HOOK_COMMAND("togglewinamp",ToggleWinamp);
  HOOK_COMMAND("togglemapbrowser",ToggleMapBrowser);
  HOOK_COMMAND("loadauthid",LoadAuthID);
  HOOK_COMMAND("unloadauthid",UnloadAuthID);
  HOOK_COMMAND("agrecord",AgRecord);
	return 1;
};

int AgHudGlobal::VidInit(void)
{
	if (!gEngfuncs.pDemoAPI->IsRecording())
    s_mapAuthID.clear();
	return 1;
};

void AgHudGlobal::Reset(void)
{
  m_iFlags |= HUD_ACTIVE;
  m_fCheckColor = 0;
}

int iOverLay = 0;

int AgHudGlobal::Draw(float fTime)
{
	if (m_fCheckColor < gHUD.m_flTime)
	{
		AgUpdateHudColor();
		m_fCheckColor = gHUD.m_flTime + 1; //every second
	}
  if (g_pcl_scores->value < 1)
    return 1;

  int xpos, ypos;
  xpos = 30;
  ypos = 50;
	sscanf(CVAR_GET_STRING("cl_scores_pos"), "%i %i", &xpos, &ypos);

  int r,g,b;

  if (gViewPort && gViewPort->m_pScoreBoard)
  {
    for (int iRow = 0, iLines = 0; iRow < gViewPort->m_pScoreBoard->m_iRows && iLines < g_pcl_scores->value; iRow++)
    {
      if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] == 1 && gHUD.m_Teamplay)
      {
        char szScore[64];
        team_info_t* team_info = &g_TeamInfo[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];
        sprintf(szScore,"%-5i %s",team_info->frags,team_info->name);

        r = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0];
        g = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1];
        b = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2];

        FillRGBA( xpos - 10, ypos + 2 , iOverLay, gHUD.m_scrinfo.iCharHeight * 0.9, r, g, b, 20 );

        ScaleColors(r,g,b,135);

        int ixposnew = gHUD.DrawHudString( xpos, ypos, ScreenWidth, szScore, r, g, b );
        iOverLay = max(ixposnew - xpos + 20,iOverLay);
        ypos += gHUD.m_scrinfo.iCharHeight * 0.9;
        iLines++;
      }
      else if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] == 0 && !gHUD.m_Teamplay)
      {
        char szScore[64];
				hud_player_info_t* pl_info = &g_PlayerInfoList[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];
				extra_player_info_t* pl_info_extra = &g_PlayerExtraInfo[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];
        sprintf(szScore,"%-5i %s",pl_info_extra->frags,pl_info->name);

        r = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][0];
        g = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][1];
        b = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][2];

			  FillRGBA( xpos - 10, ypos + 2, iOverLay, gHUD.m_scrinfo.iCharHeight * 0.9, r, g, b, 10 );

        ScaleColors(r,g,b,135);

        int ixposnew = gHUD.DrawHudString( xpos, ypos, ScreenWidth, szScore, r, g, b );
        iOverLay = max(ixposnew - xpos + 20,iOverLay);
        ypos += gHUD.m_scrinfo.iCharHeight * 0.9;
        iLines++;
      }
    }
  }
  return 1;
}


int AgHudGlobal::MsgFunc_PlaySound(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
  
	vec3_t origin;
  /*int iPlayer = */READ_BYTE();
	for ( int i = 0 ; i < 3 ; i++)
		origin[i] = READ_COORD();
	char* pszSound = READ_STRING();

  gEngfuncs.pfnPlaySoundByName( pszSound, 1); 
  //this does not work - gEngfuncs.pfnPlaySoundByNameAtLocation( pszSound, 1, origin);

  //gEngfuncs.pEventAPI->EV_PlaySound( -1, origin, 0, pszName, 1.0, ATTN_NORM, 0, PITCH_NORM );

	return 1;
}

extern bool AgCRC32EnforceFiles();

int AgHudGlobal::MsgFunc_CheatCheck(const char *pszName, int iSize, void *pbuf)
{
 	BEGIN_READ( pbuf, iSize );
  int iPure = READ_BYTE(); 
  
	g_iPure = iPure;

#ifdef AG_USE_CHEATPROTECTION
  if (0 < g_iPure)
	  AgCRC32EnforceFiles();

  g_VariableChecker.Activate();

  DWORD dwTime = GetTickCount();
  
#ifdef _DEBUG
	AgLog( "Checking for spikes\n" );
#endif //_DEBUG
  if (!g_ModelCheck.Check())
    return 1;

  if (s_iCheckWallhack)
  {
#ifdef AG_USE_CHEATPROTECTION
#ifdef _DEBUG
	AgLog( "Checking for wallhack\n" );
#endif //_DEBUG
    if (!g_Wallhack.Check())
      return 1;
#endif
  }

#ifdef _DEBUG
  char szTime[64];
  sprintf(szTime,"Cheat check took %dms\n",int((GetTickCount() - dwTime)));
  ConsolePrint(szTime);
  AgLog(szTime);
#endif  

#endif //AG_USE_CHEATPROTECTION

  return 1;
}


int AgHudGlobal::MsgFunc_WhString(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
#ifdef AG_USE_CHEATPROTECTION
  g_Wallhack.AddBadStrings(READ_STRING());
#else
  READ_STRING();
#endif
  s_iCheckWallhack = 1;
  return 1;
}

int AgHudGlobal::MsgFunc_SpikeCheck(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
#ifdef AG_USE_CHEATPROTECTION
  g_ModelCheck.CheckOne(READ_STRING());
#else
  READ_STRING();
#endif
  return 1;
}

int AgHudGlobal::MsgFunc_Gametype(const char *pszName, int iSize, void *pbuf)
{
  BEGIN_READ( pbuf, iSize );
  g_GameType = READ_BYTE();
  return 1;
}

int AgHudGlobal::MsgFunc_AuthID(const char *pszName, int iSize, void *pbuf)
{
  BEGIN_READ( pbuf, iSize );
  int iPlayer = READ_BYTE();
  AgString sAuthID = READ_STRING();

  int iCutId = sAuthID.find("_");
  if (-1 != iCutId)
	  sAuthID = sAuthID.substr(iCutId+1);

  AgPlayerToAuthID::iterator itrAuthID = s_mapAuthID.find(iPlayer);
  if (itrAuthID == s_mapAuthID.end())
    s_mapAuthID.insert(AgPlayerToAuthID::value_type(iPlayer,sAuthID));
  else
    (*itrAuthID).second = sAuthID;

  return 1;
}

int AgHudGlobal::MsgFunc_MapList( const char *pszName, int iSize, void *pbuf )
{
	if (gViewPort && gViewPort->m_pMapBrowser)
		return gViewPort->m_pMapBrowser->MsgFunc_MapList( pszName, iSize, pbuf );
	return 1;
}

int AgHudGlobal::MsgFunc_CRC32( const char *pszName, int iSize, void *pbuf )
{
  BEGIN_READ( pbuf, iSize );
	int iCheckSum = READ_LONG();
#ifdef AG_USE_CHEATPROTECTION
	AgCRC32EnforceFile(READ_STRING(), iCheckSum);
#else
	READ_STRING();
#endif
	return 1;
}

void AgHudGlobal::UserCmd_Winamp()
{
  gViewPort->UserCmd_Winamp();
}

void AgHudGlobal::UserCmd_ToggleWinamp()
{
	gViewPort->ToggleWinamp();
}

void AgHudGlobal::UserCmd_ToggleMapBrowser()
{
	gViewPort->ToggleMapBrowser();
}

void AgHudGlobal::UserCmd_LoadAuthID()
{
  char* pszFileName = "realnames.txt";
	if (gEngfuncs.Cmd_Argc() == 2)
	{
		char szSaveAs[MAX_PATH];
    AgString sUrl = gEngfuncs.Cmd_Argv(1);
		sprintf(szSaveAs,"%s/%s",AgGetDirectory(),pszFileName);
    sUrl = "http://" + sUrl;
    AgDownload download;
    download.DownloadFile(sUrl.c_str(), szSaveAs);
	}

	int iFilePos = 0, iFileSize = 0;
	char* pFile = (char*)gEngfuncs.COM_LoadFile(pszFileName, 5, NULL);
	if (!pFile)
  {
		char szMessage[256];
		sprintf(szMessage, "Could not load file %s\n", pszFileName);
		ConsolePrint(szMessage);
		return;
  }
  AgString sRealNames(pFile);
	gEngfuncs.COM_FreeFile(pFile);

  int iPosNewLine = sRealNames.find_first_of("\n");
  while (-1 != iPosNewLine)
  {
    AgString sAuthID, sRealName;
    int iPosRealName = sRealNames.find_first_of(" \t");
    sAuthID = sRealNames.substr(0,iPosRealName);
    sRealName = sRealNames.substr(iPosRealName+1,min(32,iPosNewLine - iPosRealName));

    AgTrim(sAuthID);
    AgTrim(sRealName);
    
    if ("//" != sAuthID.substr(0,2))
    {
      if (sAuthID.size() && sRealName.size())
        s_mapRealName.insert(AgAuthIDToRealName::value_type(sAuthID, sRealName));
    }
    else
    {
      AgString sComment = sRealNames.substr(2,iPosNewLine-2);
      AgTrim(sComment);
      sComment += "\n";
      ConsolePrint(sComment.c_str());
    }
    
    sRealNames = sRealNames.substr(iPosNewLine+1);
    iPosNewLine = sRealNames.find_first_of("\n");
  }

  char szCount[64];
  sprintf(szCount,"Loaded auth id's - %d\n",(int)s_mapRealName.size());
  ConsolePrint(szCount);
}

void AgHudGlobal::UserCmd_UnloadAuthID()
{
  ConsolePrint("Unloaded all auth id's\n");
  s_mapRealName.clear();
}

AgString AgGetAuthID(int iPlayer)
{
  AgPlayerToAuthID::iterator itrAuthID = s_mapAuthID.find(iPlayer);
  if (itrAuthID != s_mapAuthID.end())
  {
    return (*itrAuthID).second;
  }
  return "";
}

AgString AgGetRealName(int iPlayer)
{
	if (s_mapRealName.size())
	{
 		AgString sRealName;
		AgPlayerToAuthID::iterator itrAuthID = s_mapAuthID.find(iPlayer);
		if (itrAuthID != s_mapAuthID.end())
		{
			AgAuthIDToRealName::iterator itrRealName = s_mapRealName.find((*itrAuthID).second);
			if (itrRealName != s_mapRealName.end())
				return (*itrRealName).second;
		}
	}
	return g_PlayerInfoList[iPlayer].name;
}

void AgHudGlobal::UserCmd_AgRecord()
{
  time_t t_now;
  time(&t_now);            
  struct tm* now = localtime(&t_now);
  now->tm_year += 1900;

  char szExtra[128];
  if (gEngfuncs.Cmd_Argc() == 2)
    sprintf(szExtra, "_%s",gEngfuncs.Cmd_Argv(1));
  else
    szExtra[0] = '\0';

  char szCMD[128];
  sprintf(szCMD, "record %04d%02d%02d_%02d%02d%02d_%s%s\n",now->tm_year, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, AgMapname().c_str(), szExtra);
	ClientCmd(szCMD);
}

//-- Martin Webrant
