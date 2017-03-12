//++ BulliT
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "game.h"
#include "agglobal.h"
#include "agsettings.h"
#ifdef AGSTATS
#include "agstats.h"
#endif
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DLL_GLOBAL bool g_bMapchange = false;
DLL_GLOBAL AgString g_sNextMap;
DLL_GLOBAL AgString g_sNextRules;

extern cvar_t timeleft, fragsleft;
extern int gmsgNextmap;


AgSettings::AgSettings()
{
  m_bChangeNextLevel = false;
  g_bMapchange = false;

  m_bCheckNextMap = true;
  m_bCalcNextMap = true;

  m_fNextCheck = gpGlobals->time + 10.0;
}

AgSettings::~AgSettings()
{

}

bool AgSettings::Think()
{
  if (!g_pGameRules)
    return false;

  if (g_bMapchange)
    return false;

  if (m_bChangeNextLevel)
  {
    m_bChangeNextLevel = false;
    g_bMapchange = true;
    //Change the map.
    AgChangelevel(g_sNextMap);

    if (g_sNextRules.size())
    {
	    SERVER_COMMAND((char*)g_sNextRules.c_str());
      g_sNextRules = "";
    }
    return false;
  }

  if (g_fGameOver)
    return true;
  
  //No need to do rest of this every frame.
  if (m_fNextCheck > gpGlobals->time) 
    return true;

  if (m_bCalcNextMap)
    CalcNextMap();

  m_fNextCheck = gpGlobals->time + 5.0; //Every 5 seconds.

  //Check if to display next map.
  if (m_bCheckNextMap && timelimit.value || m_bCheckNextMap && fraglimit.value)
  {
    if (timeleft.value && 60 > timeleft.value || fraglimit.value && 2 > fragsleft.value)
    {
#ifdef AG_NO_CLIENT_DLL
      AgSay(NULL,UTIL_VarArgs("Next map is %s\n",GetNextLevel().c_str()),NULL,30,0.03,0.05,2);
#else
      MESSAGE_BEGIN(MSG_BROADCAST,gmsgNextmap);
        WRITE_STRING( GetNextLevel().c_str() );
      MESSAGE_END();
#endif

      m_bCheckNextMap = false;
    }
  }

  return true;
}

bool AgSettings::AdminSetting(const AgString& sSetting, const AgString& sValue)
{
  if (0 == strnicmp(sSetting.c_str(),"ag_",3)
    ||0 == strnicmp(sSetting.c_str(),"mp_timelimit",12)
    ||0 == strnicmp(sSetting.c_str(),"mp_fraglimit",12)
    )
  {
    CVAR_SET_STRING(sSetting.c_str(),sValue.c_str());
    return true;
  }
  return false;
}


void AgSettings::Changelevel(const AgString& sMap)
{
  if (32 < sMap.size() || 0 == sMap.size())
    return;

  char szTemp[64];
  strcpy(szTemp,sMap.c_str());

  //Check if it exists.
  if (IS_MAP_VALID(szTemp))
  {
    g_sNextMap = sMap;
    g_sNextRules = "";
    g_pGameRules->GoToIntermission();

#ifdef AGSTATS
    Stats.OnChangeLevel();
#endif
  }
}


void AgSettings::SetNextLevel(const AgString& sMap)
{
  if (32 < sMap.size() || 0 == sMap.size())
    return;
  
  char szTemp[64];
  strcpy(szTemp,sMap.c_str());
  
  //Check if it exists.
  if (IS_MAP_VALID(szTemp))
    g_sNextMap = sMap;
  
  if (g_sNextMap.size())
  {
#ifdef AG_NO_CLIENT_DLL
			char szNextMap[128];
			sprintf(szNextMap, "Next map is %s", g_sNextMap.c_str());
			AgSay(NULL,szNextMap,NULL,5,0.5,0.2);
#else
      MESSAGE_BEGIN(MSG_BROADCAST,gmsgNextmap);
        WRITE_STRING( g_sNextMap.c_str() );
      MESSAGE_END();
#endif
  }
}

AgString AgSettings::GetNextLevel()
{
  return g_sNextMap;
}

void AgSettings::ChangeNextLevel()
{
  if (32 < g_sNextMap.size() || 0 == g_sNextMap.size())
    return;
  
  m_bChangeNextLevel = true;
}

/*
void AgSettings::CalcNextMap()
{
  //Calc next map, wont work with maps that are in more than one place in mapcycle file.
  typedef list<AgString> AgMapList;
  AgMapList lstMaps;
  
  char *pszMapFile = (char*) CVAR_GET_STRING( "mapcyclefile" );
  ASSERT( pszMapFile != NULL );
  
  // Load the file
  int nLength = 0;
  char* pFileList = (char *)LOAD_FILE_FOR_ME(pszMapFile,&nLength);
  
  if (pFileList && nLength)
  {
    // Loop while there are lines
    char *pFileCur = pFileList;
    while (nLength > 0)
    {
      // Get the next line
      char szLine [256];
      char *pszLine = szLine;
      while (nLength > 0 && *pFileCur != '\n')
      {
        char c = *pFileCur++;
        if (c > ' ' && c < 127) *pszLine++ = c;
        nLength--;
      }
      
      // Remove the LF
      if (nLength > 0)
      {
        nLength--;
        pFileCur++;
      }
      
      // Terminate the line
      *pszLine++ = 0;
      
      // If there is anything in the line, add to map list
      if (szLine[0] && IS_MAP_VALID(szLine))
        lstMaps.push_back(szLine);
    }
    
    // Free the file
    FREE_FILE(pFileList);
  }
  
  //Find the next map. Ain't there a find function in stl? weird..
  AgMapList::iterator itrMaps = lstMaps.begin();
  for ( ;itrMaps != lstMaps.end() && *itrMaps != g_sNextMap; ++itrMaps)
  {
  }
  
  if (itrMaps == lstMaps.end())
  {
    if (0 == lstMaps.size())
    {
      //No maps in list. Set the current.
      g_sNextMap = STRING(gpGlobals->mapname); 
    }
    else
    {
      //Map aint in list. Do default to first map.
      g_sNextMap = *lstMaps.begin();
    }
  }
  else 
  {
    ++itrMaps;
    if (itrMaps == lstMaps.end())
    {
      //End of list, use first map in list.
      g_sNextMap = *lstMaps.begin();
    }
    else
    {
      //Set next map.
      g_sNextMap = *itrMaps;
    }

  }
  
  //Still empty? Should not be so... - this is VERY defenisive programming :)
  if (0 == g_sNextMap.size())
    g_sNextMap = STRING(gpGlobals->mapname); 
  
  //No need to calc more.
  m_bCalcNextMap = false;
  lstMaps.clear();
}
*/


#define MAX_RULE_BUFFER 1024

typedef struct mapcycle_item_s
{
	struct mapcycle_item_s *next;

	char mapname[ 32 ];
	int  minplayers, maxplayers;
	char rulebuffer[ MAX_RULE_BUFFER ];
} mapcycle_item_t;

typedef struct mapcycle_s
{
	struct mapcycle_item_s *items;
	struct mapcycle_item_s *next_item;
} mapcycle_t;

/*
==============
DestroyMapCycle

Clean up memory used by mapcycle when switching it
==============
*/
void AgDestroyMapCycle( mapcycle_t *cycle )
{
	mapcycle_item_t *p, *n, *start;
	p = cycle->items;
	if ( p )
	{
		start = p;
		p = p->next;
		while ( p != start )
		{
			n = p->next;
			delete p;
			p = n;
		}
		
		delete cycle->items;
	}
	cycle->items = NULL;
	cycle->next_item = NULL;
}

static char com_token[ 1500 ];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *AgCOM_Parse (char *data)
{
	int             c;
	int             len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;                    // end of file;
		data++;
	}
	
// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

// parse single characters
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
	if (c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c == ',' )
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}

/*
==============
COM_TokenWaiting

Returns 1 if additional data is waiting to be processed on this line
==============
*/
int AgCOM_TokenWaiting( char *buffer )
{
	char *p;

	p = buffer;
	while ( *p && *p!='\n')
	{
		if ( !isspace( *p ) || isalnum( *p ) )
			return 1;

		p++;
	}

	return 0;
}



/*
==============
ReloadMapCycleFile


Parses mapcycle.txt file into mapcycle_t structure
==============
*/
int AgReloadMapCycleFile( char *filename, mapcycle_t *cycle )
{
	char szBuffer[ MAX_RULE_BUFFER ];
	char szMap[ 32 ];
	int length;
	char *pFileList;
	char *aFileList = pFileList = (char*)LOAD_FILE_FOR_ME( filename, &length );
	int hasbuffer;
	mapcycle_item_s *item, *newlist = NULL, *next;

	if ( pFileList && length )
	{
		// the first map name in the file becomes the default
		while ( 1 )
		{
			hasbuffer = 0;
			memset( szBuffer, 0, MAX_RULE_BUFFER );

			pFileList = AgCOM_Parse( pFileList );
			if ( strlen( com_token ) <= 0 )
				break;

			strcpy( szMap, com_token );

			// Any more tokens on this line?
			if ( AgCOM_TokenWaiting( pFileList ) )
			{
				pFileList = AgCOM_Parse( pFileList );
				if ( strlen( com_token ) > 0 )
				{
					hasbuffer = 1;
					strcpy( szBuffer, com_token );
				}
			}

			// Check map
			if ( IS_MAP_VALID( szMap ) )
			{
				// Create entry
				char *s;

				item = new mapcycle_item_s;

				strcpy( item->mapname, szMap );

				item->minplayers = 0;
				item->maxplayers = 0;

				memset( item->rulebuffer, 0, MAX_RULE_BUFFER );

				if ( hasbuffer )
				{
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "minplayers" );
					if ( s && s[0] )
					{
						item->minplayers = atoi( s );
						item->minplayers = max( item->minplayers, 0 );
						item->minplayers = min( item->minplayers, gpGlobals->maxClients );
					}
					s = g_engfuncs.pfnInfoKeyValue( szBuffer, "maxplayers" );
					if ( s && s[0] )
					{
						item->maxplayers = atoi( s );
						item->maxplayers = max( item->maxplayers, 0 );
						item->maxplayers = min( item->maxplayers, gpGlobals->maxClients );
					}

					// Remove keys
					//
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "minplayers" );
					g_engfuncs.pfnInfo_RemoveKey( szBuffer, "maxplayers" );

					strcpy( item->rulebuffer, szBuffer );
				}

				item->next = cycle->items;
				cycle->items = item;
			}
			else
			{
				ALERT( at_console, "Skipping %s from mapcycle, not a valid map\n", szMap );
			}

		}

		FREE_FILE( aFileList );
	}

	// Fixup circular list pointer
	item = cycle->items;

	// Reverse it to get original order
	while ( item )
	{
		next = item->next;
		item->next = newlist;
		newlist = item;
		item = next;
	}
	cycle->items = newlist;
	item = cycle->items;

	// Didn't parse anything
	if ( !item )
	{
		return 0;
	}

	while ( item->next )
	{
		item = item->next;
	}
	item->next = cycle->items;
	
	cycle->next_item = item->next;

	return 1;
}

/*
==============
CountPlayers

Determine the current # of active players on the server for map cycling logic
==============
*/
int AgCountPlayers( void )
{
	int	num = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( i );

		if ( pEnt )
		{
			num = num + 1;
		}
	}

	return num;
}

/*
==============
ExtractCommandString

Parse commands/key value pairs to issue right after map xxx command is issued on server
 level transition
==============
*/
void AgExtractCommandString( char *s, char *szCommand )
{
	// Now make rules happen
	char	pkey[512];
	char	value[512];	// use two buffers so compares
								// work without stomping on each other
	char	*o;
	
	if ( *s == '\\' )
		s++;

	while (1)
	{
		o = pkey;
		while ( *s != '\\' )
		{
			if ( !*s )
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;

		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		strcat( szCommand, pkey );
		if ( strlen( value ) > 0 )
		{
			strcat( szCommand, " " );
			strcat( szCommand, value );
		}
		strcat( szCommand, "\n" );

		if (!*s)
			return;
		s++;
	}
}

/*
==============
ChangeLevel

Server is changing to a new level, check mapcycle.txt for map name and setup info
==============
*/
void AgSettings::CalcNextMap()
{
	static char szPreviousMapCycleFile[ 256 ];
	static mapcycle_t mapcycle;

	char szNextMap[32];
	char szFirstMapInList[32];
	char szCommands[ 1500 ];
	char szRules[ 1500 ];
	int minplayers = 0, maxplayers = 0;
	strcpy( szFirstMapInList, "boot_camp" );  // the absolute default level is hldm1

	int	curplayers;
	BOOL do_cycle = TRUE;

	// find the map to change to
	char *mapcfile = (char*)CVAR_GET_STRING( "mapcyclefile" );
	ASSERT( mapcfile != NULL );

	szCommands[ 0 ] = '\0';
	szRules[ 0 ] = '\0';

	curplayers = AgCountPlayers();

	// Has the map cycle filename changed?
	if ( stricmp( mapcfile, szPreviousMapCycleFile ) )
	{
		strcpy( szPreviousMapCycleFile, mapcfile );

		AgDestroyMapCycle( &mapcycle );

		if ( !AgReloadMapCycleFile( mapcfile, &mapcycle ) || ( !mapcycle.items ) )
		{
			ALERT( at_console, "Unable to load map cycle file %s\n", mapcfile );
			do_cycle = FALSE;
		}
	}

	if ( do_cycle && mapcycle.items )
	{
		BOOL keeplooking = FALSE;
		BOOL found = FALSE;
		mapcycle_item_s *item;

		// Assume current map
		strcpy( szNextMap, STRING(gpGlobals->mapname) );
		strcpy( szFirstMapInList, STRING(gpGlobals->mapname) );

		// Traverse list
		for ( item = mapcycle.next_item; item->next != mapcycle.next_item; item = item->next )
		{
			keeplooking = FALSE;

			ASSERT( item != NULL );

			if ( item->minplayers != 0 )
			{
				if ( curplayers >= item->minplayers )
				{
					found = TRUE;
					minplayers = item->minplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( item->maxplayers != 0 )
			{
				if ( curplayers <= item->maxplayers )
				{
					found = TRUE;
					maxplayers = item->maxplayers;
				}
				else
				{
					keeplooking = TRUE;
				}
			}

			if ( keeplooking )
				continue;

			found = TRUE;
			break;
		}

		if ( !found )
		{
			item = mapcycle.next_item;
		}			
		
		// Increment next item pointer
		mapcycle.next_item = item->next;

		// Perform logic on current item
		strcpy( szNextMap, item->mapname );

		AgExtractCommandString( item->rulebuffer, szCommands );
		strcpy( szRules, item->rulebuffer );
	}

	if ( !IS_MAP_VALID(szNextMap) )
	{
		strcpy( szNextMap, szFirstMapInList );
	}

	ALERT( at_console, "CHANGE LEVEL: %s\n", szNextMap );
	if ( minplayers || maxplayers )
	{
		ALERT( at_console, "PLAYER COUNT:  min %i max %i current %i\n", minplayers, maxplayers, curplayers );
	}
	if ( strlen( szRules ) > 0 )
	{
		ALERT( at_console, "RULES:  %s\n", szRules );
	}

  g_sNextMap = szNextMap;
  g_sNextRules = szCommands;
  //No need to calc more.
  m_bCalcNextMap = false;
}


//-- Martin Webrant
