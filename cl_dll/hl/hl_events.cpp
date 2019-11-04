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
void EV_FireRevolver1( struct event_args_s *args );
void EV_FireRevolver2( struct event_args_s *args );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireShotGunDouble( struct event_args_s *args  );
void EV_FireTommy( struct event_args_s *args );
void EV_FireRifle( struct event_args_s *args );

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
	gEngfuncs.pfnHookEvent( "events/revolver1.sc", EV_FireRevolver1 );
	gEngfuncs.pfnHookEvent( "events/revolver2.sc", EV_FireRevolver2 );
	gEngfuncs.pfnHookEvent( "events/shotgun1.sc", EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/shotgun2.sc", EV_FireShotGunDouble );
	gEngfuncs.pfnHookEvent( "events/tommygun.sc", EV_FireTommy );
	gEngfuncs.pfnHookEvent( "events/rifle.sc", EV_FireRifle );

	gEngfuncs.pfnHookEvent( "events/train.sc", EV_TrainPitchAdjust );
}
