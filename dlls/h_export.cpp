/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/*

===== h_export.cpp ========================================================

  Entity classes exported by Halflife.

*/

#include "extdll.h"
#include "util.h"

#include "cbase.h"

// Holds engine functionality callbacks
enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;
server_physics_api_t g_physfuncs;

#ifdef _WIN32

// Required DLL entry point
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	if( fdwReason == DLL_PROCESS_ATTACH )
	{
	}
	else if( fdwReason == DLL_PROCESS_DETACH )
	{
	}
	return TRUE;
}

// stdcall for win32
#define EXPORT2 WINAPI
#else
#define EXPORT2
#endif

extern "C" void DLLEXPORT EXPORT2 GiveFnptrsToDll( enginefuncs_t *pengfuncsFromEngine, globalvars_t *pGlobals )
{
	memcpy( &g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t) );
	gpGlobals = pGlobals;
}

int ShouldCollide( edict_t *pentTouched, edict_t *pentOther )
{
	return 1;
}

void OnFreeEntPrivateData(edict_s *pEdict)
{
#if 0
	if(pEdict && pEdict->pvPrivateData)
	{
		((CBaseEntity*)pEdict->pvPrivateData)->~CBaseEntity();
	}
#endif
}

void GameShutdown(void)
{

}

void CvarValue( const edict_t *pEnt, const char *value )
{
}

void CvarValue2( const edict_t *pEnt, int requestID, const char *cvarName, const char *value );



NEW_DLL_FUNCTIONS gNewDLLFunctions =
{
	OnFreeEntPrivateData, //pfnOnFreeEntPrivateData
	GameShutdown, //pfnGameShutdown
	ShouldCollide, //pfnShouldCollide
	CvarValue,
	CvarValue2
};


extern "C" int DLLEXPORT EXPORT2 GetNewDLLFunctions(NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion)
{
	if(!pFunctionTable || *interfaceVersion != NEW_DLL_FUNCTIONS_VERSION)
	{
		*interfaceVersion = NEW_DLL_FUNCTIONS_VERSION;
		return FALSE;
	}
	memcpy(pFunctionTable, &gNewDLLFunctions, sizeof(gNewDLLFunctions));
	return TRUE;
}


