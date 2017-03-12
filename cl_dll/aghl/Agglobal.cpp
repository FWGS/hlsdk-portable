//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include "agglobal.h"
#include <time.h>
#include "agmodelcheck.h"

cvar_t* g_phud_spectatebar = NULL;
cvar_t* g_phud_timer = NULL;
cvar_t* g_phud_playerid = NULL;
cvar_t* g_phud_settings = NULL;
cvar_t* g_phud_weapon = NULL;
cvar_t* g_phud_color = NULL;
cvar_t* g_pcl_matchreport = NULL;
cvar_t* g_pcl_playtalk = NULL;
cvar_t* g_pcl_weaponswitch = NULL;
//cvar_t* g_pcl_weaponweights = NULL;
cvar_t* g_pcl_disablespecs = NULL;
cvar_t* g_pcl_liveupdate = NULL;
cvar_t* g_pcl_scores = NULL;
cvar_t* g_pcl_scores_pos = NULL;
cvar_t* g_pcl_only_team_talk = NULL;
cvar_t* g_pcl_show_colors = NULL;
cvar_t* g_pcl_old_scoreboard = NULL;
cvar_t* g_pcl_ctf_volume = NULL;
cvar_t* g_pcl_show_local_maps = NULL;
cvar_t* g_pcl_location_keywords = NULL;
cvar_t* g_pcl_show_banner = NULL;


cvar_t* g_pirc_server = NULL;
cvar_t* g_pirc_nick = NULL;
cvar_t* g_pirc_port = NULL;
cvar_t* g_pirc_userid = NULL;
cvar_t* g_pirc_password = NULL;
cvar_t* g_pirc_fullname = NULL;
cvar_t* g_pirc_autojoin = NULL;
cvar_t* g_pirc_autocommand = NULL;
cvar_t* g_pirc_autocommand2 = NULL;
cvar_t* g_pirc_autocommand3 = NULL;

// Colors
int iNumConsoleColors = 16;
int arrConsoleColors[16][3] =
{
	{ 255, 170, 0   },	// HL console (default)
  { 255, 0,   0   },  // Red
  { 0,   255, 0   },  // Green
  { 255, 255, 0   },  // Yellow
  { 0,   0,   255 },  // Blue
  { 0,   255, 255 },  // Cyan
  { 255,   0, 255 },  // Violet
  { 136, 136, 136 },  // Q
  { 255, 255, 255 },  // White
  { 0,   0,   0   },  // Black
  { 200, 90,  70  },	// Redb
	{ 145, 215, 140 },	// Green
	{ 225, 205, 45  },	// Yellow
	{ 125, 165, 210 },	// Blue
  { 70,   70, 70  },	
	{ 200, 200, 200 },	
};

int arrHudColor[3] = 
{ 
	255, 160, 0 
};

extern int iTeamColors[5][3];
extern float g_ColorConsole[3];
//-- Martin Webrant
#ifdef _DEBUG
void AgTest();
#endif

void AgUpdateHudColor()
{
  //Hud color
	sscanf(CVAR_GET_STRING("hud_color"), "%i %i %i", &arrHudColor[0], &arrHudColor[1], &arrHudColor[2] );
  iTeamColors[0][0] = arrHudColor[0];
  iTeamColors[0][1] = arrHudColor[1];
  iTeamColors[0][2] = arrHudColor[2];

  //Console color
	sscanf(CVAR_GET_STRING("con_color"), "%i %i %i", &arrConsoleColors[0][0], &arrConsoleColors[0][1], &arrConsoleColors[0][2] );
  g_ColorConsole[0] = arrConsoleColors[0][0] / 255.0;
  g_ColorConsole[1] = arrConsoleColors[0][1] / 255.0;
  g_ColorConsole[2] = arrConsoleColors[0][2] / 255.0;
}

void AgGetHudColor(int &r, int &g, int &b)
{
	r = arrHudColor[0];
	g = arrHudColor[1];
	b = arrHudColor[2];
}

//extern void			COM_Log( char *pszFile, char *fmt, ...);

void AgInitClientDll()
{
	g_phud_spectatebar	= gEngfuncs.pfnRegisterVariable ( "hud_spectatebar",   "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_phud_timer	      = gEngfuncs.pfnRegisterVariable ( "hud_timer",         "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_phud_playerid     = gEngfuncs.pfnRegisterVariable ( "hud_playerid",      "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_phud_settings     = gEngfuncs.pfnRegisterVariable ( "hud_settings",      "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_phud_weapon       = gEngfuncs.pfnRegisterVariable ( "hud_weapon",        "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_phud_color		  = gEngfuncs.pfnRegisterVariable ( "hud_color",         "255 160 0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pcl_matchreport   = gEngfuncs.pfnRegisterVariable ( "cl_matchreport",    "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_playtalk      = gEngfuncs.pfnRegisterVariable ( "cl_playtalk",       "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pcl_weaponswitch	= gEngfuncs.pfnRegisterVariable ( "cl_autowepswitch",  "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE|FCVAR_USERINFO);
  //g_pcl_weaponweights	= gEngfuncs.pfnRegisterVariable ( "cl_weaponweights",  "", FCVAR_CLIENTDLL|FCVAR_ARCHIVE|FCVAR_USERINFO); //weapon weight factors (for auto-switching)   (-1 = noswitch)

  g_pcl_disablespecs	= gEngfuncs.pfnRegisterVariable ( "cl_disablespecs",   "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE|FCVAR_USERINFO);
  g_pcl_liveupdate    = gEngfuncs.pfnRegisterVariable ( "cl_liveupdate",     "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE); //Special for Mr. T-rex :P
  g_pcl_scores	      = gEngfuncs.pfnRegisterVariable ( "cl_scores",         "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_scores_pos    = gEngfuncs.pfnRegisterVariable ( "cl_scores_pos",     "30 50", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pcl_only_team_talk= gEngfuncs.pfnRegisterVariable ( "cl_only_team_talk", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_show_colors   = gEngfuncs.pfnRegisterVariable ( "cl_show_colors",    "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_old_scoreboard = gEngfuncs.pfnRegisterVariable ( "cl_old_scoreboard",    "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_ctf_volume     = gEngfuncs.pfnRegisterVariable ( "cl_ctf_volume",    "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pirc_server	      = gEngfuncs.pfnRegisterVariable ( "irc_server",   "irc.quakenet.eu.org", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_port	        = gEngfuncs.pfnRegisterVariable ( "irc_port",   "6667", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_nick	        = gEngfuncs.pfnRegisterVariable ( "irc_nick",   "", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_userid	      = gEngfuncs.pfnRegisterVariable ( "irc_userid",   "", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_password	    = gEngfuncs.pfnRegisterVariable ( "irc_password",   "", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_fullname	    = gEngfuncs.pfnRegisterVariable ( "irc_fullname",   "", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_autojoin	    = gEngfuncs.pfnRegisterVariable ( "irc_autojoin",   "#pmers", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_autocommand	= gEngfuncs.pfnRegisterVariable ( "irc_autocommand","", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_autocommand2	= gEngfuncs.pfnRegisterVariable ( "irc_autocommand2","", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pirc_autocommand3	= gEngfuncs.pfnRegisterVariable ( "irc_autocommand3","", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pcl_show_local_maps = gEngfuncs.pfnRegisterVariable ("cl_show_local_maps", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  g_pcl_location_keywords = gEngfuncs.pfnRegisterVariable ("cl_location_keywords", "0", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);
  g_pcl_show_banner = gEngfuncs.pfnRegisterVariable ("cl_show_banner", "1", FCVAR_CLIENTDLL|FCVAR_ARCHIVE);

  //Can use opforce stuff
  gEngfuncs.COM_AddAppDirectoryToSearchPath("opforce","opforce");

  //Setup colors
  AgUpdateHudColor();

//  COM_Log(NULL,"Modulehandle - %lx\n",GetModuleHandle("client.dll"));
}


int GetColor(char cChar)
{
  int iColor = -1;
	if (cChar >= '0' && cChar <= '9')
    iColor = cChar - '0';
  return iColor;
}

int AgDrawHudStringCentered(int xpos, int ypos, int iMaxX, const char *szIt, int r, int g, int b )
{
	// calc center
  int iSizeX = 0;
  char* pszIt = (char*)szIt;
	for ( ; *pszIt != 0 && *pszIt != '\n'; pszIt++ )
		iSizeX += gHUD.m_scrinfo.charWidths[ *pszIt ]; // variable-width fonts look cool

  //Subtract half sizex from xpos to center it.
  xpos = xpos - iSizeX / 2;

  int rx = r, gx = g, bx = b;

  pszIt = (char*)szIt;
	// draw the string until we hit the null character or a newline character
	for ( ; *pszIt != 0 && *pszIt != '\n'; pszIt++ )
	{
    if (*pszIt == '^')
    {
      pszIt++;
      int iColor = GetColor(*pszIt);
      if (iColor < iNumConsoleColors && iColor >= 0)
      {
        if (0 >= iColor || 0 == g_pcl_show_colors->value)
        {
          rx = r;
          gx = g;
          bx = b;
        }
        else
        {
          rx = arrConsoleColors[iColor][0];
          gx = arrConsoleColors[iColor][1];
          bx = arrConsoleColors[iColor][2];
        }
        pszIt++;
        if (*pszIt == 0 || *pszIt == '\n')
          break;
      }
      else
        pszIt--;
    }

		int next = xpos + gHUD.m_scrinfo.charWidths[ *pszIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;
		TextMessageDrawChar( xpos, ypos, *pszIt, rx, gx, bx );
		xpos = next;		
	}
	return xpos;
}

int AgDrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b )
{

  int rx = r, gx = g, bx = b;

	// draw the string until we hit the null character or a newline character
	for ( ; *szIt != 0 && *szIt != '\n'; szIt++ )
	{
    if (*szIt == '^')
    {
      szIt++;
      int iColor = GetColor(*szIt);
      if (iColor < iNumConsoleColors && iColor >= 0)
      {
        if (0 >= iColor || 0 == g_pcl_show_colors->value)
        {
          rx = r;
          gx = g;
          bx = b;
        }
        else
        {
          rx = arrConsoleColors[iColor][0];
          gx = arrConsoleColors[iColor][1];
          bx = arrConsoleColors[iColor][2];
        }
        szIt++;
        if (*szIt == 0 || *szIt == '\n')
          break;
      }
      else
        szIt--;
    }

		int next = xpos + gHUD.m_scrinfo.charWidths[ *szIt ]; // variable-width fonts look cool
		if ( next > iMaxX )
			return xpos;

		TextMessageDrawChar( xpos, ypos, *szIt, rx, gx, bx );
		xpos = next;		
	}

	return xpos;
}

int AgDrawConsoleString( int x, int y, const char *string, float r, float g, float b )
{
  if (0 == g_pcl_show_colors->value)
  {
    char* pszString = strdup(string);
    AgStripColors((char*)pszString);
	  int iRet = gEngfuncs.pfnDrawConsoleString( x, y, (char*) pszString );
    free(pszString);
    return iRet;
  }


  char szText[512];
  char* pText = szText;
  *pText = '\0';
  char* pszColor = (char*)string;
  while (*pszColor)
  {
    if ('^' == *pszColor)
    {
      int iColor = GetColor(*(pszColor+1));
      if (iColor < iNumConsoleColors && iColor >= 0)
        //User wants a new color
      {
        //Draw first part with previous color.
        *pText = '\0';
        *pszColor = '\0';
        int xPrev = gEngfuncs.pfnDrawConsoleString( x, y, (char*)szText);
        *pszColor = '^';
        pszColor++;
        pszColor++;

        float rx = r, gx = g, bx = b;
        //Set the color.
	      if (iColor > 0)
        {
          rx = arrConsoleColors[iColor][0] / 255.0;
          gx = arrConsoleColors[iColor][1] / 255.0;
          bx = arrConsoleColors[iColor][2] / 255.0;
        }
        if (!(rx == 0 && gx == 0 && bx == 0))
          gEngfuncs.pfnDrawSetTextColor(rx, gx, bx);

        //Draw the rest of the string (that may contain new colors)
        return AgDrawConsoleString( xPrev , y, pszColor, r, g, b);
      }
    }
    *pText = *pszColor;
    pText++;
    pszColor++;
  }
  *pText = '\0';

  if (!(r == 0 && g == 0 && b == 0))
    gEngfuncs.pfnDrawSetTextColor(r, g, b);

	return gEngfuncs.pfnDrawConsoleString( x, y, (char*) szText );
/*
  int iLength = strlen(string);
  char* pszColor = strchr(string,'^');

  if (pszColor)
  {
    //Extract color.
    ++pszColor;
    int iColor = GetColor(*pszColor);
    ++pszColor;

    //Check if we got a valid color.
    if (iColor < iNumConsoleColors && iColor >= 0)
    {
      char szTerm = *(pszColor-2);
      *(pszColor-2) = '\0';

      //Draw the first part with the previous color.
      int xPrev = gEngfuncs.pfnDrawConsoleString( x, y, (char*)string);
      *(pszColor-2) = szTerm;

      float rx = r, gx = g, bx = b;
      //Set the color.
	    if (iColor != 0)
      {
        rx = arrConsoleColors[iColor][0] / 255.0;
        gx = arrConsoleColors[iColor][1] / 255.0;
        bx = arrConsoleColors[iColor][2] / 255.0;
      }
      if (!(rx == 0 && gx == 0 && bx == 0))
        gEngfuncs.pfnDrawSetTextColor(rx, gx, bx);

      //Draw the rest of the string (that may contain new colors)
      return AgDrawConsoleString( xPrev , y, pszColor, r, g, b);
    }
  }

  if (!(r == 0 && g == 0 && b == 0))
    gEngfuncs.pfnDrawSetTextColor(r, g, b);

	return gEngfuncs.pfnDrawConsoleString( x, y, (char*) string );
  */
}

void AgStripColors(char* pszString)
{
  char* pszIt = pszString;
  while ('\0' != *pszIt)
  {
    if ('^' == *pszIt)
    {
      ++pszIt;
		  if (*pszIt >= '0' && *pszIt <= '9')
		  {
			  --pszIt;
			  memmove(pszIt,pszIt+2,strlen(pszIt+2)+1);
		  }
    }
    else
      ++pszIt;
  }
}

AgString AgMapname()
{
  return gHUD.m_Location.m_szMap;

  /*
  AgString sMap;
  sMap = gEngfuncs.pfnGetLevelName();
  if (0 == sMap.size())
    return sMap;

  sMap = sMap.substr(sMap.find("/")+1);
  sMap = sMap.substr(0,sMap.find("."));
  return sMap;
  */
}

void AgTrim(AgString& sTrim)
{
  if (0 == sTrim.length())
    return;

  int b = sTrim.find_first_not_of(" \t\r\n");
  int e = sTrim.find_last_not_of(" \t\r\n");
  if(b == -1) // No non-whitespaces
    sTrim = "";
  else
    sTrim = string(sTrim, b, e - b + 1);
}

void AgLog(const char* pszLog)
{
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/aglog.txt", AgGetDirectory());
  FILE* pFile = fopen(szFile,"a+");
  if (!pFile)
  {
    return;
  }
  
  time_t clock;
  time( &clock ); 
  fprintf(pFile,"%s : %s",pszLog,asctime(localtime(&clock)));
  fflush(pFile);
  fclose(pFile);
}


void AgDirList(const AgString& sDir, AgStringSet& setFiles)
{
#ifdef _WIN32 
  WIN32_FIND_DATA FindData;
  char szSearchDirectory[_MAX_PATH];
  sprintf(szSearchDirectory,"%s/*.*",sDir.c_str());
  HANDLE hFind = FindFirstFile(szSearchDirectory, &FindData);

  if (INVALID_HANDLE_VALUE != hFind)
  {
    do
    {
      if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      {
        AgString sFile;
        sFile = FindData.cFileName;
        setFiles.insert(sFile);
      }
    }
    while (FindNextFile(hFind, &FindData));
    FindClose(hFind);
  }
#else
  DIR* pDirectory = opendir(sDir.c_str());
  if (pDirectory)
  {
struct dirent* pFile = NULL;
    
    while (NULL != (pFile = readdir(pDirectory)))
    {
      AgString sFile;
      sFile = pFile->d_name;
      setFiles.insert(sFile);
    }
    closedir(pDirectory);
  }
#endif
}

void AgToLower(AgString& strLower)
{
  size_t i = 0;
  while (i < strLower.length())
  {
    strLower[i] = tolower(strLower[i]);
    i++;
  }
}


const char* AgGetGame()
{
	static char szGame[MAX_PATH];
	strcpy(szGame, gEngfuncs.pfnGetGameDirectory());
	char* pszGameDir = strrchr(szGame, '/');
	if (pszGameDir)
		return pszGameDir + 1;
	return szGame;
}

const char* AgGetDirectory()
{
	static char szGame[MAX_PATH];
	strcpy(szGame, gEngfuncs.pfnGetGameDirectory());
	char* pszGameDir = strrchr(szGame, '/');
	if (pszGameDir)
	{
		return szGame;
	}
	else
	{
		static char szDirectory[MAX_PATH] = "";
		if (strlen(szDirectory))
			return szDirectory;

		::GetCurrentDirectory(MAX_PATH, szDirectory);

		strcat(szDirectory, "/");
		strcat(szDirectory, szGame);
		return szDirectory;
	}
}

const char* AgGetDirectoryValve()
{
	static char szDirectory[MAX_PATH] = "";
	if (szDirectory[0] != '\0')
		return szDirectory;

	strcpy(szDirectory, AgGetDirectory());
	int iStart = strlen(szDirectory)-1;
	while ('/' != szDirectory[iStart])
	{
		szDirectory[iStart] = '\0';
		iStart--;
	}
	szDirectory[iStart] = '\0';
	return szDirectory;
}

//-- Martin Webrant
