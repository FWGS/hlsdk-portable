#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"game_config.h"

#define	kGameDirectory		"gamedir"		// Change!
#define	kConfigFilename		"zx.ini"	// Change!

typedef struct
{
	short			cfgID;
	const char		*groupID;
	const char		*entryID;

	char			defStr[64];
} ConfigStringStruct;

typedef struct
{
	short			cfgID;
	const char		*groupID;
	const char		*entryID;
	
	float			defValue;
} ConfigValueStruct;

//---------------------------------------
static const char *gG_Category1	= "Config Category 1";
static const char *gG_Category2	= "Config Category 2";

static ConfigStringStruct gConfigStrings[] =
{
	{	kConfig_String1,		gG_Category1,	"Entry 1",		"A_String"	},
	{	NULL,					NULL,			NULL,			0			}
};

static ConfigValueStruct gConfigValues[] =
{
	{	kConfig_Value1,			gG_Category1,	"Value 1",		1000		},
	{	kConfig_Value2,			gG_Category1,	"Value 2",		1000		},
	{	kConfig_Value3,			gG_Category2,	"Value 3",		1000		},
	{	NULL,					NULL,			NULL,			0			}
};

static char gConfigFile[256];

void CreateFullPathname(char *szName, char *szPathname)
{
	char	tempStr[256];
	DWORD	dwResult;
	
	memset(&tempStr, 0, sizeof(tempStr));
	dwResult = GetCurrentDirectory(sizeof(tempStr)-1, tempStr);

	if (dwResult != 0)
	{
		if (tempStr[strlen(tempStr)-1] != '\\')
			strcat(tempStr, "\\");

		strcat(tempStr, kGameDirectory);
		strcat(tempStr, "\\");
	}
	strcat(tempStr, szName);

	strcpy(szPathname, tempStr);
}

static void GetConfigFilename(void)
{
	CreateFullPathname( kConfigFilename, gConfigFile );
}

//------------------------------------------------------------------------
static ConfigValueStruct *FindGameValue(int cfgNum)
{
	ConfigValueStruct *cfgEntry;

	cfgEntry = gConfigValues;
	while (cfgEntry->entryID)
	{
		if (cfgEntry->cfgID == cfgNum)
			return cfgEntry;
		cfgEntry++;
	}
	return NULL;
}

float GetGameValue(int cfgNum)
{
	ConfigValueStruct *cfgEntry = FindGameValue(cfgNum);

	if (cfgEntry)
	{
		char szDefault[512];
		char szVal[512];

		memset(&szVal, 0, sizeof(szVal));
		sprintf(szDefault, "%.2f", cfgEntry->defValue);
		GetPrivateProfileString(cfgEntry->groupID, cfgEntry->entryID, szDefault, szVal,
			sizeof(szVal)-1, gConfigFile);
		
		return (atof(szVal));
	}

	return 0;
}

//------------------------------------------------------------------------
static ConfigStringStruct *FindGameString(int cfgNum)
{
	ConfigStringStruct *cfgEntry;

	cfgEntry = gConfigStrings;
	while (cfgEntry->entryID)
	{
		if (cfgEntry->cfgID == cfgNum)
			return cfgEntry;
		cfgEntry++;
	}
	return NULL;
}

void GetGameString(int cfgNum, char *szResult)
{
	ConfigStringStruct *cfgEntry = FindGameString(cfgNum);

	strcpy(szResult, "");
	if (cfgEntry)
	{
		char szVal[512];

		memset(&szVal, 0, sizeof(szVal));
		GetPrivateProfileString(cfgEntry->groupID, cfgEntry->entryID, cfgEntry->defStr, 
			szVal, sizeof(szVal)-1, gConfigFile);
		
		strcpy(szResult, szVal);
	}
}

//------------------------------------------------------------------------
static void InitializeGameValues(void)
{
	ConfigValueStruct *cfgValue;

	GetConfigFilename();

	cfgValue = gConfigValues;
	while (cfgValue->entryID)
	{
		char szVal[512];

		memset(&szVal, 0, sizeof(szVal));
		GetPrivateProfileString(cfgValue->groupID, cfgValue->entryID, "", 
			szVal, sizeof(szVal)-1, gConfigFile);
		if (strlen(szVal) == 0)
		{
			sprintf(szVal, "%.2f", cfgValue->defValue);
			WritePrivateProfileString(cfgValue->groupID, cfgValue->entryID, 
				szVal, gConfigFile);
		}
		else
			cfgValue->defValue = atof(szVal);
  
		cfgValue++;
	}

	//---------------
	ConfigStringStruct *cfgStr;

	cfgStr = gConfigStrings;
	while (cfgStr->entryID)
	{
		char szVal[512];

		memset(&szVal, 0, sizeof(szVal));
		GetPrivateProfileString(cfgStr->groupID, cfgStr->entryID, "", 
			szVal, sizeof(szVal)-1, gConfigFile);
		if (strlen(szVal) == 0)
		{
			WritePrivateProfileString(cfgStr->groupID, cfgStr->entryID, 
				cfgStr->defStr, gConfigFile);
		}
		else
			strcpy(cfgStr->defStr, szVal);
  
		cfgStr++;
	}

}

//--------------------------------------------------------------------------
void InitializeConfigutation(void)
{
	InitializeGameValues();
}
