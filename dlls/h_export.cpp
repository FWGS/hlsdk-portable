/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

// Holds engine functionality callbacks
enginefuncs_t g_engfuncs;
globalvars_t* gpGlobals;

#ifdef _WIN32
extern "C" void __stdcall GiveFnptrsToDll( enginefuncs_t* functions, globalvars_t* globals )
#else
extern "C" __attribute__(( visibility( "default" ) )) void GiveFnptrsToDll( enginefuncs_t* functions, globalvars_t* globals )
#endif
{
	memcpy( &g_engfuncs, functions, sizeof( g_engfuncs ) );
	gpGlobals = globals;
}
