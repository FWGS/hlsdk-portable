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

#include "../hud.h"
#include "../cl_util.h"
#include "event_api.h"

extern "C"
{
// HLDM
void EV_Knife( struct event_args_s *args );
void EV_Axe( struct event_args_s *args );
void EV_Hammer( struct event_args_s *args );
void EV_Spear( struct event_args_s *args );
void EV_FireGlock1( struct event_args_s *args );
void EV_FireGlock2( struct event_args_s *args );
void EV_FireBeretta( struct event_args_s *args );
void EV_FireP228( struct event_args_s *args );
void EV_FireDeagle( struct event_args_s *args );
void EV_FireRevolver( struct event_args_s *args );
void EV_FireShotGunSingle( struct event_args_s *args );
void EV_FireMP5K( struct event_args_s *args );
void EV_FireUzi( struct event_args_s *args );
void EV_FireGMGeneral( struct event_args_s *args );

void EV_TrainPitchAdjust( struct event_args_s *args );
}

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
	gEngfuncs.pfnHookEvent( "events/knife.sc",	EV_Knife );
	gEngfuncs.pfnHookEvent( "events/axe.sc",	EV_Axe );
	gEngfuncs.pfnHookEvent( "events/hammer.sc",	EV_Hammer );
	gEngfuncs.pfnHookEvent( "events/null.sc",	EV_Spear );
	gEngfuncs.pfnHookEvent( "events/glock21.sc",	EV_FireGlock1 );
	gEngfuncs.pfnHookEvent( "events/glock22.sc",	EV_FireGlock2 );
	gEngfuncs.pfnHookEvent( "events/beretta.sc",	EV_FireBeretta );
	gEngfuncs.pfnHookEvent( "events/p228.sc",	EV_FireP228 );
	gEngfuncs.pfnHookEvent( "events/deagle.sc",	EV_FireDeagle );
	gEngfuncs.pfnHookEvent( "events/deagle1.sc",	EV_FireRevolver );
	gEngfuncs.pfnHookEvent( "events/shotgun.sc",	EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/mp5k.sc",	EV_FireMP5K );
	gEngfuncs.pfnHookEvent( "events/mp5.sc",	EV_FireUzi );
	gEngfuncs.pfnHookEvent( "events/mp5k2.sc",	EV_FireGMGeneral );
	gEngfuncs.pfnHookEvent( "events/train.sc",	EV_TrainPitchAdjust );
}
