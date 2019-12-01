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
#ifdef MOBILE_HACKS 
int g_iModType;
#endif // MOBILE_HACKS

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
#ifdef MOBILE_HACKS
	char szGameFolder[64];

	(*g_engfuncs.pfnGetGameDir)( szGameFolder );

	if( FStrEq( szGameFolder, "aom" ) )
	{
		g_iModType = MOD_AOM;
	}
	else if( FStrEq( szGameFolder, "biglolly" ) )
	{
		g_iModType = MOD_BIGLOLLY;
	}
	else if( FStrEq( szGameFolder, "bshift" ) )
	{
		g_iModType = MOD_BSHIFT;
	}
	else if( FStrEq( szGameFolder, "halfsecret" ) )
	{
		g_iModType = MOD_HALFSECRET;
	}
	else if( FStrEq( szGameFolder, "borderlands" )
	    || FStrEq( szGameFolder, "caseclosed" )
	    || FStrEq( szGameFolder, "vendetta" ) )
	{
		g_iModType = MOD_HEVSUIT;
	}
	else if( FStrEq( szGameFolder, "induction" ) )
	{
		g_iModType = MOD_INDUCTION;
	}
	else if( FStrEq( szGameFolder, "redempt" ) )
	{
		g_iModType = MOD_REDEMPT;
	}
	else if( FStrEq( szGameFolder, "sewerbeta" ) )
	{
		g_iModType = MOD_SEWER_BETA;
	}
	else if( FStrEq( szGameFolder, "tot" ) )
	{
		g_iModType = MOD_TOT;
	}
	else
	{
		g_iModType = MOD_VALVE;
	}
#endif // MOBILE_HACKS
}
