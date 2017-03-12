//++ BulliT


#if !defined(_AG_GLOBAL_)
#define _AG_GLOBAL_

#pragma warning(disable:4786) 
#pragma warning(disable:4710) 
#pragma warning(disable:4511) 
#pragma warning(disable:4512)
#pragma warning(disable:4514)
#pragma warning(disable:4663) 
#pragma warning(disable:4711) 
#pragma warning(disable:4710) 
#pragma warning(disable:4100) 

#include <assert.h>
#define NOWINRES
#define NOIME
#define _WIN32_WINNT  0x0400
#include <windows.h>

#define _bool_h 1
#include <ministl/string>
#include <ministl/list>
#include <ministl/set>
#include <ministl/map>
#include <ministl/vector>
#include <ministl/algorithm>
typedef string         AgString;
typedef list<AgString> AgStringList;
typedef set<AgString, less<AgString> > AgStringSet;

void AgInitClientDll();
int AgDrawHudString(int xpos, int ypos, int iMaxX, char *szIt, int r, int g, int b );
int AgDrawHudStringCentered(int xpos, int ypos, int iMaxX, const char *szIt, int r, int g, int b );
int AgDrawConsoleString( int x, int y, const char *string, float r = 0, float g = 0, float b = 0 );
void AgSetTextColor(int r, int g, int b);
void AgStripColors(char* pszString);
void AgDirList(const AgString& sDir, AgStringSet& setFiles);

AgString AgMapname();
void AgTrim(AgString& sTrim);
void AgLog(const char* pszLog);
void AgToLower(AgString& strLower);

void AgUpdateHudColor();
void AgGetHudColor(int &r, int &g, int &b);

const char* AgGetGame();
const char* AgGetDirectory();
const char* AgGetDirectoryValve();

#endif _AG_GLOBAL_
//-- Martin Webrant
