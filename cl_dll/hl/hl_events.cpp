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
void EV_FireGlock1( struct event_args_s *args  );
void EV_FireGlock2( struct event_args_s *args  );
void EV_FireShotGunSingle( struct event_args_s *args  );
void EV_FireShotGunDouble( struct event_args_s *args  );
void EV_FireMP5( struct event_args_s *args  );
void EV_FireMP52( struct event_args_s *args  );
void EV_Crowbar( struct event_args_s *args );
void EV_Boombox( struct event_args_s *args );
void EV_FireRpg( struct event_args_s *args );
void EV_SodaDrink( struct event_args_s *args );
void EV_TripmineFire( struct event_args_s *args );
void EV_SnarkFire( struct event_args_s *args );
void EV_FireDosh( struct event_args_s *args );
void EV_BeamKatana( struct event_args_s *args );
void EV_FOTN( struct event_args_s *args );
void EV_FireAK47( struct event_args_s *args );
void EV_FireBow( struct event_args_s *args );
void EV_FireModman( struct event_args_s *args );
void EV_FireModman2( struct event_args_s *args );
void EV_FireScientist( struct event_args_s *args );
void EV_FireNStar( struct event_args_s *args );
void EV_FireMW2( struct event_args_s *args );
void EV_FireGOLDENGUN( struct event_args_s *args );
void EV_FireJackal( struct event_args_s *args );
void EV_Jihad( struct event_args_s *args );
void EV_FireZAPPER( struct event_args_s *args );

void EV_TrainPitchAdjust( struct event_args_s *args );
void EV_VehiclePitchAdjust( event_args_t *args );
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
	gEngfuncs.pfnHookEvent( "events/glock1.sc", EV_FireGlock1 );
	gEngfuncs.pfnHookEvent( "events/glock2.sc", EV_FireGlock2 );
	gEngfuncs.pfnHookEvent( "events/shotgun1.sc", EV_FireShotGunSingle );
	gEngfuncs.pfnHookEvent( "events/shotgun2.sc", EV_FireShotGunDouble );
	gEngfuncs.pfnHookEvent( "events/mp5.sc", EV_FireMP5 );
	gEngfuncs.pfnHookEvent( "events/mp52.sc", EV_FireMP52 );
	gEngfuncs.pfnHookEvent( "events/train.sc", EV_TrainPitchAdjust );
	gEngfuncs.pfnHookEvent( "events/crowbar.sc", EV_Crowbar );
	gEngfuncs.pfnHookEvent( "events/boombox.sc", EV_Boombox );
	gEngfuncs.pfnHookEvent( "events/rpg.sc", EV_FireRpg );
	gEngfuncs.pfnHookEvent( "events/soda.sc", EV_SodaDrink );
	gEngfuncs.pfnHookEvent( "events/tripfire.sc", EV_TripmineFire );
	gEngfuncs.pfnHookEvent( "events/snarkfire.sc", EV_SnarkFire );
	gEngfuncs.pfnHookEvent( "events/dosh.sc", EV_FireDosh );
	gEngfuncs.pfnHookEvent( "events/beamkatana.sc", EV_BeamKatana );
	gEngfuncs.pfnHookEvent( "events/fotns.sc", EV_FOTN );
	gEngfuncs.pfnHookEvent( "events/ak47.sc", EV_FireAK47 );
	gEngfuncs.pfnHookEvent( "events/bow.sc", EV_FireBow );
	gEngfuncs.pfnHookEvent( "events/modman.sc", EV_FireModman );
	gEngfuncs.pfnHookEvent( "events/modman2.sc", EV_FireModman2 );
	gEngfuncs.pfnHookEvent( "events/scientist.sc", EV_FireScientist );
	gEngfuncs.pfnHookEvent( "events/nstar.sc", EV_FireNStar );
	gEngfuncs.pfnHookEvent( "events/mw2.sc", EV_FireMW2 );
	gEngfuncs.pfnHookEvent( "events/goldengun.sc", EV_FireGOLDENGUN );
	gEngfuncs.pfnHookEvent( "events/jackal.sc", EV_FireJackal );
	gEngfuncs.pfnHookEvent( "events/jihad.sc", EV_Jihad );
	gEngfuncs.pfnHookEvent( "events/zapper.sc", EV_FireZAPPER );
	gEngfuncs.pfnHookEvent( "events/vehicle.sc", EV_VehiclePitchAdjust );
}
