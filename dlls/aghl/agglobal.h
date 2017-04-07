//++ BulliT

#if !defined(_AG_GLOBAL_H_)
#define _AG_GLOBAL_H_

#pragma warning(disable:4786) 
#pragma warning(disable:4530) 

#include "hltv.h"

#define _bool_h 1
#include <ministl/string>
#include <ministl/list>
#include <ministl/set>
#include <ministl/map>
#include <ministl/vector>
#include <ministl/algorithm>
typedef string                         AgString;
typedef set<AgString, less<AgString> > AgStringSet;

CBasePlayer* AgPlayerByIndex(int iPlayerIndex);
void         AgConsole(const AgString& sText, CBasePlayer* pPlayer = NULL);
void         AgSay(CBasePlayer* pPlayer, const AgString& sText, float* pfFloodProtected = NULL, float fHoldTime = 3.5, float x = -1, float y = -1, int iChannel = 5);
void         AgResetMap();
void         AgInitGame();

#endif // !defined(_AG_GLOBAL_H_)

//-- Martin Webrant
