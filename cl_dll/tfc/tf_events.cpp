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
#include "pm_defs.h"
#include "ev_tfc.h"

cvar_t *cl_gibcount;
cvar_t *cl_giblife;
cvar_t *cl_gibvelscale;
cvar_t *cl_localblood;

extern "C"
{
void EV_TFC_Axe( struct event_args_s *args );
void EV_FireTFCShotgun( struct event_args_s *args );
void EV_FireTFCSuperShotgun( struct event_args_s *args );
void EV_ReloadTFCShotgun( struct event_args_s *args );
void EV_PumpTFCShotgun( struct event_args_s *args );
void EV_FireTFCNailgun( struct event_args_s *args );
void EV_FireTFCSuperNailgun( struct event_args_s *args );
void EV_FireTFCAutoRifle( struct event_args_s *args );
void EV_TFC_Gas( struct event_args_s *args );
void EV_TFC_DoorGoUp( struct event_args_s *args );
void EV_TFC_DoorGoDown( struct event_args_s *args );
void EV_TFC_DoorHitTop( struct event_args_s *args );
void EV_TFC_DoorHitBottom( struct event_args_s *args );
void EV_TFC_Explosion( struct event_args_s *args );
void EV_TFC_Grenade( struct event_args_s *args );
void EV_TFC_NormalGrenade( struct event_args_s *args );
void EV_TFC_FireRPG( struct event_args_s *args );
void EV_FireTFCSniper( struct event_args_s *args );
void EV_TFC_SniperHit( struct event_args_s *args );
void EV_TFC_FireIC( struct event_args_s *args );
void EV_TFC_NailgrenadeNail( struct event_args_s *args );
void EV_TFC_GrenadeLauncher( struct event_args_s *args );
void EV_TFC_PipeLauncher( struct event_args_s *args );
void EV_TFC_NormalShot( struct event_args_s *args );
void EV_TFC_SuperShot( struct event_args_s *args );
void EV_TFC_SteamShot( struct event_args_s *args );
void EV_TFC_EngineerGrenade( struct event_args_s *args );
void EV_TFC_Concussion( struct event_args_s *args );
void EV_TFC_Assault_WindUp( struct event_args_s *args );
void EV_TFC_Assault_WindDown( struct event_args_s *args );
void EV_TFC_Assault_Start( struct event_args_s *args );
void EV_TFC_Assault_Fire( struct event_args_s *args );
void EV_TFC_Assault_Spin( struct event_args_s *args );
void EV_TFC_Assault_StartSpin( struct event_args_s *args );
void EV_TFC_AxeDecal( struct event_args_s *args );
void EV_TFC_NapalmFire( struct event_args_s *args );
void EV_TFC_MirvGrenadeMain( struct event_args_s *args );
void EV_TFC_MirvGrenade( struct event_args_s *args );
void EV_TFC_NapalmBurn( struct event_args_s *args );
void EV_TFC_EMP( struct event_args_s *args );
void EV_TFC_Flame_Fire( struct event_args_s *args );
void EV_TFC_Railgun( struct event_args_s *args );
void EV_TFC_Tranquilizer( struct event_args_s *args );
void EV_TFC_NailGrenade( struct event_args_s *args );

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
    gEngfuncs.pfnHookEvent("events/wpn/tf_axe.sc", EV_TFC_Axe);
    gEngfuncs.pfnHookEvent("events/wpn/tf_sg.sc", EV_FireTFCShotgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_sgreload.sc", EV_ReloadTFCShotgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_sgpump.sc", EV_PumpTFCShotgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_nail.sc", EV_FireTFCNailgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_snail.sc", EV_FireTFCSuperNailgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_ssg.sc", EV_FireTFCSuperShotgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_ar.sc", EV_FireTFCAutoRifle);
    gEngfuncs.pfnHookEvent("events/explode/tf_gas.sc", EV_TFC_Gas);
    gEngfuncs.pfnHookEvent("events/door/doorgoup.sc", EV_TFC_DoorGoUp);
    gEngfuncs.pfnHookEvent("events/door/doorgodown.sc", EV_TFC_DoorGoDown);
    gEngfuncs.pfnHookEvent("events/door/doorhittop.sc", EV_TFC_DoorHitTop);
    gEngfuncs.pfnHookEvent("events/door/doorhitbottom.sc", EV_TFC_DoorHitBottom);
    gEngfuncs.pfnHookEvent("events/explode/tf_pipe.sc", EV_TFC_Explosion);
    gEngfuncs.pfnHookEvent("events/explode/tf_gren.sc", EV_TFC_Grenade);
    gEngfuncs.pfnHookEvent("events/explode/tf_normalgren.sc", EV_TFC_NormalGrenade);
    gEngfuncs.pfnHookEvent("events/wpn/tf_rpg.sc", EV_TFC_FireRPG);
    gEngfuncs.pfnHookEvent("events/wpn/tf_sniper.sc", EV_FireTFCSniper);
    gEngfuncs.pfnHookEvent("events/wpn/tf_sniperhit.sc", EV_TFC_SniperHit);
    gEngfuncs.pfnHookEvent("events/wpn/tf_ic.sc", EV_TFC_FireIC);
    gEngfuncs.pfnHookEvent("events/explode/tf_nailgren.sc", EV_TFC_NailgrenadeNail);
    gEngfuncs.pfnHookEvent("events/wpn/tf_gl.sc", EV_TFC_GrenadeLauncher);
    gEngfuncs.pfnHookEvent("events/wpn/tf_pipel.sc", EV_TFC_PipeLauncher);
    gEngfuncs.pfnHookEvent("events/wpn/tf_mednormal.sc", EV_TFC_NormalShot);
    gEngfuncs.pfnHookEvent("events/wpn/tf_medsuper.sc", EV_TFC_SuperShot);
    gEngfuncs.pfnHookEvent("events/wpn/tf_medsteam.sc", EV_TFC_SteamShot);
    gEngfuncs.pfnHookEvent("events/explode/tf_engrgren.sc", EV_TFC_EngineerGrenade);
    gEngfuncs.pfnHookEvent("events/explode/tf_concuss.sc", EV_TFC_Concussion);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acwu.sc", EV_TFC_Assault_WindUp);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acwd.sc", EV_TFC_Assault_WindDown);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acstart.sc", EV_TFC_Assault_Start);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acfire.sc", EV_TFC_Assault_Fire);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acspin.sc", EV_TFC_Assault_Spin);
    gEngfuncs.pfnHookEvent("events/wpn/tf_acsspin.sc", EV_TFC_Assault_StartSpin);
    gEngfuncs.pfnHookEvent("events/wpn/tf_axedecal.sc", EV_TFC_AxeDecal);
    gEngfuncs.pfnHookEvent("events/explode/tf_mirvmain.sc", EV_TFC_MirvGrenadeMain);
    gEngfuncs.pfnHookEvent("events/explode/tf_mirv.sc", EV_TFC_MirvGrenade);
    gEngfuncs.pfnHookEvent("events/explode/tf_fire.sc", EV_TFC_NapalmFire);
    gEngfuncs.pfnHookEvent("events/explode/tf_burn.sc", EV_TFC_NapalmBurn);
    gEngfuncs.pfnHookEvent("events/explode/tf_emp.sc", EV_TFC_EMP);
    gEngfuncs.pfnHookEvent("events/wpn/tf_flame.sc", EV_TFC_Flame_Fire);
    gEngfuncs.pfnHookEvent("events/wpn/tf_rail.sc", EV_TFC_Railgun);
    gEngfuncs.pfnHookEvent("events/wpn/tf_tranq.sc", EV_TFC_Tranquilizer);
    gEngfuncs.pfnHookEvent("events/explode/tf_ng.sc", EV_TFC_NailGrenade);

    cl_gibcount = gEngfuncs.pfnRegisterVariable("cl_gibcount", "4", 1);
    cl_giblife = gEngfuncs.pfnRegisterVariable("cl_giblife", "25", 1);
    cl_gibvelscale = gEngfuncs.pfnRegisterVariable("cl_gibvelscale", "1.0", 1);
    cl_localblood = gEngfuncs.pfnRegisterVariable("cl_lb", "0.0", 3);
}