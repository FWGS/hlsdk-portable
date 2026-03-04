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

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "usercmd.h"
#include "pm_defs.h"
#include "pm_materials.h"

#include "eventscripts.h"
#include "ev_hldm.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

// XaeroX: for event-based brass shell ejection
extern "C" int CL_IsThirdPerson();

extern engine_studio_api_t IEngineStudio;

static int g_tracerCount[32];

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;

extern "C"
{
// HLDM
void EV_FireGlock1( struct event_args_s *args );
void EV_FireGlock2( struct event_args_s *args );
void EV_FireShotGunSingle( struct event_args_s *args );
void EV_FireShotGunDouble( struct event_args_s *args );
void EV_FireMP5( struct event_args_s *args );
void EV_FireMP52( struct event_args_s *args );
void EV_FireMP53( struct event_args_s *args  );
void EV_FirePython( struct event_args_s *args );
void EV_FireGauss( struct event_args_s *args );
void EV_SpinGauss( struct event_args_s *args );
void EV_Crowbar( struct event_args_s *args );
void EV_FireCrossbow( struct event_args_s *args );
void EV_FireCrossbow2( struct event_args_s *args );
void EV_FireRpg( struct event_args_s *args );
void EV_EgonFire( struct event_args_s *args );
void EV_EgonStop( struct event_args_s *args );
void EV_HornetGunFire( struct event_args_s *args );
void EV_TripmineFire( struct event_args_s *args );
void EV_SnarkFire( struct event_args_s *args );
// Delta Particles Weapons
void EV_PipeWrench( struct event_args_s *args );
void EV_Sniperrifle( struct event_args_s *args  );
void EV_Deagle( struct event_args_s *args  );
void EV_SMG( struct event_args_s *args  );
void EV_SMG2( struct event_args_s *args  );

void EV_TrainPitchAdjust( struct event_args_s *args );
void EV_VehiclePitchAdjust( event_args_t *args );
}

#define VECTOR_CONE_0DEGREES Vector( 0.00000, 0.00000, 0.00000 )
#define VECTOR_CONE_1DEGREES Vector( 0.00873f, 0.00873f, 0.00873f )
#define VECTOR_CONE_2DEGREES Vector( 0.01745f, 0.01745f, 0.01745f )
#define VECTOR_CONE_3DEGREES Vector( 0.02618f, 0.02618f, 0.02618f )
#define VECTOR_CONE_4DEGREES Vector( 0.03490f, 0.03490f, 0.03490f )
#define VECTOR_CONE_5DEGREES Vector( 0.04362f, 0.04362f, 0.04362f )
#define VECTOR_CONE_6DEGREES Vector( 0.05234f, 0.05234f, 0.05234f )
#define VECTOR_CONE_7DEGREES Vector( 0.06105f, 0.06105f, 0.06105f )	
#define VECTOR_CONE_8DEGREES Vector( 0.06976f, 0.06976f, 0.06976f )
#define VECTOR_CONE_9DEGREES Vector( 0.07846f, 0.07846f, 0.07846f )
#define VECTOR_CONE_10DEGREES Vector( 0.08716f, 0.08716f, 0.08716f )
#define VECTOR_CONE_15DEGREES Vector( 0.13053f, 0.13053f, 0.13053f )
#define VECTOR_CONE_20DEGREES Vector( 0.17365f, 0.17365f, 0.17365f )

static bool DidHitSky(pmtrace_t *ptr, float *vecSrc, float *vecEnd)
{
	int entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );
	if( entity == 0 )
	{
		const char* pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		if( pTextureName && strcmp( pTextureName, "sky" ) == 0 )
		{
			return true;
		}
	}
	return false;
}

char EV_HLDM_GetTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType, bool& isSky )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;

	int entity;
	const char *pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//
	chTextureType = 0;
	isSky = false;

	// Player
	if( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else
	{
		physent_t *pe = NULL;
		if (entity)
			pe = gEngfuncs.pEventAPI->EV_GetPhysent( ptr->ent );

		if (entity == 0 || (pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP )))
		{
			// get texture from entity or world (world is ent(0))
			pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );

			if ( pTextureName )
			{
				strcpy( texname, pTextureName );
				pTextureName = texname;

				if( strcmp( pTextureName, "sky" ) == 0 )
				{
					isSky = true;
				}

				strcpy( texname, pTextureName );
				pTextureName = texname;

				// strip leading '-0' or '+0~' or '{' or '!'
				if (*pTextureName == '-' || *pTextureName == '+')
				{
					pTextureName += 2;
				}

				if (*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
				{
					pTextureName++;
				}

				// '}}'
				strcpy( szbuffer, pTextureName );
				szbuffer[ CBTEXTURENAMEMAX - 1 ] = 0;
					
				// get texture type
				chTextureType = PM_FindTextureType( szbuffer );	
			}
		}
	}

	return chTextureType;
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( pmtrace_t *ptr, char chTextureType, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	float fvol;
	float fvolbar;
	char* rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;

	// FIXME check if playtexture sounds movevar is set
	//
	switch (chTextureType)
	{
	default:
	case CHAR_TEX_CONCRETE:
		fvol = 0.9;
		fvolbar = 0.6;
		rgsz[0] = "player/pl_step1.wav";
		rgsz[1] = "player/pl_step2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_METAL:
		fvol = 0.9;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_metal1.wav";
		rgsz[1] = "player/pl_metal2.wav";
		cnt = 2;
		break;
	case CHAR_TEX_DIRT:
		fvol = 0.9;
		fvolbar = 0.1;
		rgsz[0] = "player/pl_dirt1.wav";
		rgsz[1] = "player/pl_dirt2.wav";
		rgsz[2] = "player/pl_dirt3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_VENT:
		fvol = 0.5;
		fvolbar = 0.3;
		rgsz[0] = "player/pl_duct1.wav";
		rgsz[1] = "player/pl_duct1.wav";
		cnt = 2;
		break;
	case CHAR_TEX_GRATE:
		fvol = 0.9;
		fvolbar = 0.5;
		rgsz[0] = "player/pl_grate1.wav";
		rgsz[1] = "player/pl_grate4.wav";
		cnt = 2;
		break;
	case CHAR_TEX_TILE:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "player/pl_tile1.wav";
		rgsz[1] = "player/pl_tile3.wav";
		rgsz[2] = "player/pl_tile2.wav";
		rgsz[3] = "player/pl_tile4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_SLOSH:
		fvol = 0.9;
		fvolbar = 0.0;
		rgsz[0] = "player/pl_slosh1.wav";
		rgsz[1] = "player/pl_slosh3.wav";
		rgsz[2] = "player/pl_slosh2.wav";
		rgsz[3] = "player/pl_slosh4.wav";
		cnt = 4;
		break;
	case CHAR_TEX_WOOD:
		fvol = 0.9;
		fvolbar = 0.2;
		rgsz[0] = "debris/wood1.wav";
		rgsz[1] = "debris/wood2.wav";
		rgsz[2] = "debris/wood3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		fvol = 0.8;
		fvolbar = 0.2;
		rgsz[0] = "debris/glass1.wav";
		rgsz[1] = "debris/glass2.wav";
		rgsz[2] = "debris/glass3.wav";
		cnt = 3;
		break;
	case CHAR_TEX_FLESH:
		if( iBulletType == BULLET_PLAYER_CROWBAR )
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;
		fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong( 0, cnt - 1 )], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	return fvolbar;
}

char *EV_HLDM_DamageDecal( physent_t *pe )
{
	static char decalname[32];
	int idx;

	if( pe->classnumber == 1 )
	{
		idx = gEngfuncs.pfnRandomLong( 0, 2 );
		sprintf( decalname, "{break%i", idx + 1 );
	}
	else if( pe->rendermode != kRenderNormal )
	{
		strcpy( decalname, "{bproof1" );
	}
	else
	{
		idx = gEngfuncs.pfnRandomLong( 0, 4 );
		sprintf( decalname, "{shot%i", idx + 1 );
	}
	return decalname;
}

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName )
{
	int iRand;
	physent_t *pe;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong( 0, 0x7FFF );
	if( iRand < ( 0x7fff / 2 ) )// not every bullet makes a sound.
	{
		switch( iRand % 5 )
		{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 3:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		case 4:
			gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, "weapons/ric5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			break;
		}
	}

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if(  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot(
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ),
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

void EV_WallPuff_Wind( struct tempent_s *te, float frametime, float currenttime )
{
	static bool xWindDirection = true;
	static bool yWindDirection = true;
	static float xWindMagnitude;
	static float yWindMagnitude;

	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin.x = 0.97 * te->entity.baseline.origin.x;
		te->entity.baseline.origin.y = 0.97 * te->entity.baseline.origin.y;
		te->entity.baseline.origin.z = 0.97 * te->entity.baseline.origin.z + 0.7;
		if ( te->entity.baseline.origin.z > 70.0 )
			te->entity.baseline.origin.z = 70.0;
	}

	if ( te->entity.curstate.frame > 6.0 )
	{
		xWindMagnitude += 0.075;
		if ( xWindMagnitude > 5.0 )
			xWindMagnitude = 5.0;

		yWindMagnitude += 0.075;
		if ( yWindMagnitude > 5.0 )
			yWindMagnitude = 5.0;

		if( xWindDirection )
			te->entity.baseline.origin.x += xWindMagnitude;
		else
			te->entity.baseline.origin.x -= xWindMagnitude;

		if( yWindDirection )
			te->entity.baseline.origin.y += yWindMagnitude;
		else
			te->entity.baseline.origin.y -= yWindMagnitude;

		if ( !gEngfuncs.pfnRandomLong(0, 10) && yWindMagnitude > 3.0 )
		{
			yWindMagnitude = 0;
			yWindDirection = !yWindDirection;
		}
		if ( !gEngfuncs.pfnRandomLong(0, 10) && xWindMagnitude > 3.0 )
		{
			xWindMagnitude = 0;
			xWindDirection = !xWindDirection;
		}
	}
}

void EV_SmokeRise( struct tempent_s *te, float frametime, float currenttime )
{
	if ( te->entity.curstate.frame > 7.0 )
	{
		te->entity.baseline.origin = 0.97f * te->entity.baseline.origin;
		te->entity.baseline.origin.z += 0.7f;

		if( te->entity.baseline.origin.z > 70.0f )
			te->entity.baseline.origin.z = 70.0f;
	}
}

void EV_HugWalls(TEMPENTITY *te, pmtrace_s *ptr)
{
	Vector norm = te->entity.baseline.origin.Normalize();
	float len = te->entity.baseline.origin.Length();

	/*const Vector innerNormal = ptr->plane.normal;
	Vector v = CrossProduct(norm, innerNormal);
	Vector projection = CrossProduct(innerNormal, v);*/
	Vector projection = CrossProduct( CrossProduct(norm, ptr->plane.normal), ptr->plane.normal);

	/*if( len <= 2000.0f )
		len *= 1.5;
	else len = 3000.0f;*/

	te->entity.baseline.origin = projection * len;
}

void EV_CreateShotSmoke(int type, Vector origin, Vector dir, int speed, float scale, int r, int g, int b, bool wind, Vector velocity = Vector(0,0,0), int framerate = 35 )
{
	TEMPENTITY *te = NULL;
	void ( *callback )( struct tempent_s *ent, float frametime, float currenttime ) = NULL;
	model_t* wallPuffSprite = NULL;

	switch( type )
	{
	case SMOKE_WALLPUFF:
		wallPuffSprite = gHUD.wallPuffs[gEngfuncs.pfnRandomLong(0, (sizeof(gHUD.wallPuffs)/sizeof(gHUD.wallPuffs[0]))-1)];
		break;
	default:
		gEngfuncs.Con_DPrintf("Unknown smoketype %d\n", type);
		return;
	}

	if( wind )
		callback = EV_WallPuff_Wind;
	else
		callback = EV_SmokeRise;


	te = gEngfuncs.pEfxAPI->CL_TempEntAlloc( origin, wallPuffSprite );

	if( te )
	{
		te->callback = callback;
		te->hitcallback = EV_HugWalls;
		te->flags |= FTENT_SPRANIMATE | FTENT_COLLIDEALL | FTENT_CLIENTCUSTOM;
		te->entity.curstate.rendermode = kRenderTransAdd;
		te->entity.curstate.rendercolor.r = r;
		te->entity.curstate.rendercolor.g = g;
		te->entity.curstate.rendercolor.b = b;
		te->entity.curstate.renderamt = gEngfuncs.pfnRandomLong( 120, 180 );
		te->entity.curstate.scale = scale;
		te->entity.baseline.origin = speed * dir;
		te->entity.curstate.framerate = framerate;
		te->frameMax = wallPuffSprite->numframes;
		te->die = gEngfuncs.GetClientTime() + (float)te->frameMax / framerate;
		te->entity.curstate.frame = 0;

		if( velocity != Vector(0,0,0) )
		{
			velocity.x *= 0.9;
			velocity.y *= 0.9;
			velocity.z *= 0.5;
			te->entity.baseline.origin = te->entity.baseline.origin + velocity;
		}
	}
}

extern cvar_t* cl_weapon_wallpuff;

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType, char cTextureType, bool isSky )
{
	physent_t *pe;

	if (isSky)
		return;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_PLAYER_556MM:
		case BULLET_PLAYER_45ACP:
		case BULLET_PLAYER_14MM:
		case BULLET_PLAYER_44:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
		}

		if (cl_weapon_wallpuff && cl_weapon_wallpuff->value)
		{
			int r, g, b;
			r = g = b = 50;
			if (cTextureType == CHAR_TEX_WOOD)
			{
				r = 75;
				g = 42;
				b = 15;
			}
			float scale = 0.5f;
			switch (iBulletType)
			{
			case BULLET_PLAYER_14MM:
				scale = 1.0f;
				break;
			case BULLET_PLAYER_357:
				scale = 0.75f;
				break;
			case BULLET_PLAYER_44:
				scale = 0.6f;
				break;
			case BULLET_PLAYER_556MM:
				scale = 0.6f;
				break;
			default:
				break;
			}
			EV_CreateShotSmoke( SMOKE_WALLPUFF, pTrace->endpos + pTrace->plane.normal * 5, pTrace->plane.normal, 25, scale, r, g, b, true );
		}
	}
}

int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount )
{
	int tracer = 0;
	int i;
	qboolean player = idx >= 1 && idx <= gEngfuncs.GetMaxClients() ? true : false;

	if( iTracerFreq != 0 && ( (*tracerCount)++ % iTracerFreq ) == 0 )
	{
		vec3_t vecTracerSrc;

		if( player )
		{
			vec3_t offset( 0, 0, -4 );

			// adjust tracer position for player
			for( i = 0; i < 3; i++ )
			{
				vecTracerSrc[i] = vecSrc[i] + offset[i] + right[i] * 2 + forward[i] * 16;
			}
		}
		else
		{
			VectorCopy( vecSrc, vecTracerSrc );
		}

		if( iTracerFreq != 1 )		// guns that always trace also always decal
			tracer = 1;

		switch( iBulletType )
		{
		case BULLET_PLAYER_556MM:
		case BULLET_PLAYER_14MM:
		case BULLET_PLAYER_44:
		case BULLET_PLAYER_45ACP:
		case BULLET_MONSTER_MP5:
		case BULLET_MONSTER_9MM:
		case BULLET_MONSTER_12MM:
		default:
			EV_CreateTracer( vecTracerSrc, end );
			break;
		}
	}

	return tracer;
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY )
{
	int i;
	pmtrace_t tr;
	int iShot;
	int tracer;
	bool isSky = false;

	for( iShot = 1; iShot <= cShots; iShot++ )	
	{
		vec3_t vecDir, vecEnd;
		float x, y, z;

		//We randomize for the Shotgun.
		if( iBulletType == BULLET_PLAYER_BUCKSHOT )
		{
			do{
				x = gEngfuncs.pfnRandomFloat( -0.5, 0.5 ) + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				y = gEngfuncs.pfnRandomFloat( -0.5, 0.5 ) + gEngfuncs.pfnRandomFloat( -0.5, 0.5 );
				z = x * x + y * y;
			}while( z > 1 );

			for( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + x * flSpreadX * right[i] + y * flSpreadY * up [i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}//But other guns already have their spread randomized in the synched spread.
		else
		{
			for( i = 0 ; i < 3; i++ )
			{
				vecDir[i] = vecDirShooting[i] + flSpreadX * right[i] + flSpreadY * up [i];
				vecEnd[i] = vecSrc[i] + flDistance * vecDir[i];
			}
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();

		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if( tr.fraction != 1.0f )
		{
			bool shouldPlayTextureSound = true;
			bool shouldPlayGunshotEffect = true;

			switch(iBulletType)
			{
			default:
			case BULLET_PLAYER_9MM:
				break;
			case BULLET_PLAYER_556MM:
				shouldPlayTextureSound = !tracer;
				break;
			case BULLET_PLAYER_45ACP:
				shouldPlayTextureSound = !tracer;
				break;
			case BULLET_PLAYER_14MM:
				shouldPlayTextureSound = !tracer;
				break;
			case BULLET_PLAYER_BUCKSHOT:
				shouldPlayTextureSound = iShot == 1;
				break;
			case BULLET_PLAYER_357:
				break;
			case BULLET_PLAYER_44:
				break;
			}

			if ( shouldPlayTextureSound || shouldPlayGunshotEffect )
			{
				const char cTextureType = EV_HLDM_GetTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType, isSky );
				if ( shouldPlayTextureSound )
				{
					EV_HLDM_PlayTextureSound(&tr, cTextureType, iBulletType);
				}
				if ( shouldPlayGunshotEffect )
				{
					EV_HLDM_DecalGunshot( &tr, iBulletType, cTextureType, isSky );
				}
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

//======================
//	    GLOCK START
//======================
// Shared Glock fire implementation for EV_FireGlock1 and EV_FireGlock2.
static void EV_FireGlock_Impl( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t shellRight;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shell.mdl" );// brass shell

	if( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_SHOOT_EMPTY : GLOCK_SHOOT, 0 );

		V_PunchAxis( 0, -2.0 );
	}

	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 17, -7, -6 );

	EV_EjectBrass( ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pl_gun3.wav", gEngfuncs.pfnRandomFloat( 0.92, 1.0 ), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &g_tracerCount[idx - 1], args->fparam1, args->fparam2 );
}

void EV_FireGlock1( event_args_t *args )
{
	EV_FireGlock_Impl( args );
}

void EV_FireGlock2( event_args_t *args )
{
	EV_FireGlock_Impl( args );
}
//======================
//	   GLOCK END
//======================

//======================
//	  SHOTGUN START
//======================
void EV_FireShotGunDouble( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	int j;
	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	vec3_t vecSrc, vecAiming;
	//vec3_t vecSpread;
	vec3_t up, right, forward;
	vec3_t shellRight;
	//float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	if( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE2, 0 );
		V_PunchAxis( 0, -5.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 32, -12, 6 );

	// XaeroX: don't eject brass shell in first-person mode
	// but instead, store shell velocity in viewmodel, for a studio event
	if ( !CL_IsThirdPerson() ) 
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();
		if ( view ) {
			++view->curstate.iuser4;
			VectorCopy( ShellVelocity, view->curstate.vuser1 );
		}
	}
	else
	{
		// XaeroX: thirdperson mode, eject immediately
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], 
						gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" ), TE_BOUNCE_SHOTSHELL ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.98, 1.0), ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );
	
	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &g_tracerCount[idx-1], 0.17365, 0.04362 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &g_tracerCount[idx-1], 0.08716, 0.08716 );
	}
}

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	vec3_t vecSrc, vecAiming;
	//vec3_t vecSpread;
	vec3_t up, right, forward;
	vec3_t shellRight;
	//float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	if( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE, 0 );

		V_PunchAxis( 0, -5.0 );
	}

	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 32, -12, 6 );

	// XaeroX: don't eject brass shell in first-person mode
	// but instead, store shell velocity in viewmodel, for a studio event
	if ( !CL_IsThirdPerson() ) 
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();
		if ( view ) {
			++view->curstate.iuser4;
			VectorCopy( ShellVelocity, view->curstate.vuser1 );
		}
	}
	else
	{
		// XaeroX: thirdperson mode, eject immediately
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], 
						gEngfuncs.pEventAPI->EV_FindModelIndex( "models/shotgunshell.mdl" ), TE_BOUNCE_SHOTSHELL ); 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat( 0.95, 1.0 ), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 4, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &g_tracerCount[idx - 1], 0.08716, 0.04362 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &g_tracerCount[idx - 1], 0.08716, 0.08716 );
	}
}
//======================
//	   SHOTGUN END
//======================

//======================
//	    M4A1 START
//======================
void EV_FireMP5( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	vec3_t shellRight;
	//float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/rifleshell.mdl");// brass shell

	if( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_FIRE1, 2 );

		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -4, 2 ) );
	}

	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 11, -6, -10 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[YAW], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/556mm_fire.wav", 1, 0.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_556MM, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}

// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP52( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_LAUNCH_GREN, 2 );
		V_PunchAxis( 0, -10 );
	}
	
	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}
}
// We only predict the animation and sound
// The grenade is still launched from the server.
void EV_FireMP53( event_args_t *args )
{
	int idx;
	vec3_t origin;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( M4A1_LAST_GREN, 2 );
		V_PunchAxis( 0, -10 );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glauncher2.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}
}
//======================
//		 M4A1 END
//======================

//======================
//	   PYTHON START 
//	     ( .357 )
//======================
void EV_FirePython( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	//float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if( EV_IsLocal( idx ) )
	{
		// Python uses different body in multiplayer versus single player
		int multiplayer = gEngfuncs.GetMaxClients() == 1 ? 0 : 1;

		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( PYTHON_FIRE1, multiplayer ? 1 : 0 );

		V_PunchAxis( 0, -2.5 );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat( 0.8, 0.9 ), ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/357_shot2.wav", gEngfuncs.pfnRandomFloat( 0.8, 0.9 ), ATTN_NORM, 0, PITCH_NORM );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );

	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 0, &g_tracerCount[idx - 1], args->fparam1, args->fparam2 );
}
//======================
//	    PHYTON END 
//	     ( .357 )
//======================

//======================
//	   LIGHTGUN START 
//======================
#define SND_STOP			(1 << 5)
#define SND_CHANGE_PITCH	(1 << 7)		// duplicated in protocol.h change sound pitch

void EV_SpinGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int iSoundState = 0;

	int pitch;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	pitch = args->iparam1;

	iSoundState = args->bparam1 ? SND_CHANGE_PITCH : 0;
	iSoundState = args->bparam2 ? SND_STOP : iSoundState;

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "ambience/pulsemachine.wav", 1.0, ATTN_NORM, iSoundState, pitch );
}

/*
==============================
EV_StopPreviousGauss

==============================
*/
void EV_StopPreviousGauss( int idx )
{
	// Make sure we don't have a gauss spin event in the queue for this guy
	gEngfuncs.pEventAPI->EV_KillEvents( idx, "events/gaussspin.sc" );
	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_WEAPON, "ambience/pulsemachine.wav" );
}

extern float g_flApplyVel;

void EV_FireGauss( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	float flDamage = args->fparam1;
	//int primaryfire = args->bparam1;

	int m_fPrimaryFire = args->bparam1;
	//int m_iWeaponVolume = GAUSS_PRIMARY_FIRE_VOLUME;
	vec3_t vecSrc;
	vec3_t vecDest;
	//edict_t		*pentIgnore;
	pmtrace_t tr, beam_tr;
	float flMaxFrac = 1.0;
	//int nTotal = 0;
	int fHasPunched = 0;
	int fFirstBeam = 1;
	int nMaxHits = 10;
	physent_t *pEntity;
	int m_iBeam, m_iGlow, m_iBalls;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	//Con_Printf( "Firing gauss with %f\n", flDamage );
	EV_GetGunPosition( args, vecSrc, origin );

	m_iBeam = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );
	m_iBalls = m_iGlow = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/hotglow.spr" );

	AngleVectors( angles, forward, right, up );

	VectorMA( vecSrc, 8192, forward, vecDest );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0.0f, -2.0f );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GAUSS_FIRE2, 0 );

		if( m_fPrimaryFire == false )
			 g_flApplyVel = flDamage; 
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/lightgun_ray1.wav", 0.875, ATTN_NORM, 0, 85 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

	while( flDamage > 10 && nMaxHits > 0 )
	{
		nMaxHits--;

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
		
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecDest, PM_NORMAL, -1, &tr );

		gEngfuncs.pEventAPI->EV_PopPMStates();

		if( tr.allsolid )
			break;

		if( fFirstBeam )
		{
			if( EV_IsLocal( idx ) )
			{
				// Add muzzle flash to current weapon model
				EV_MuzzleFlash();
			}
			fFirstBeam = 0;

			gEngfuncs.pEfxAPI->R_BeamEntPoint( 
				idx | 0x1000,
				tr.endpos,
				m_iBeam,
				0.1f,
				m_fPrimaryFire ? 2.0f : 2.5f,
				0.0f,
				(m_fPrimaryFire ? 128.0f : flDamage) / 255.0f,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0f,
				(m_fPrimaryFire ? 255 : 255) / 255.0f,
				(m_fPrimaryFire ? 255 : 255) / 255.0f
			);
		}
		else
		{
			gEngfuncs.pEfxAPI->R_BeamPoints( vecSrc,
				tr.endpos,
				m_iBeam,
				0.1f,
				m_fPrimaryFire ? 2.0f : 2.5f,
				0.0f,
				(m_fPrimaryFire ? 128.0f : flDamage) / 255.0f,
				0,
				0,
				0,
				(m_fPrimaryFire ? 255 : 255) / 255.0f,
				(m_fPrimaryFire ? 255 : 255) / 255.0f,
				(m_fPrimaryFire ? 255 : 255) / 255.0f
			);
		}

		pEntity = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );
		if( pEntity == NULL )
			break;

		if( pEntity->solid == SOLID_BSP )
		{
			float n;

			//pentIgnore = NULL;

			n = -DotProduct( tr.plane.normal, forward );

			bool isSky = DidHitSky(&tr, vecSrc, vecDest);

			if (!isSky)
			{
				EV_HLDM_DecalGunshot( &tr, BULLET_MONSTER_12MM );

				gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 0.58, 6.0, FTENT_FADEOUT );

				// limit it to one hole punch
				if (fHasPunched)
				{
					break;
				}
				fHasPunched = 1;
				
				vec3_t start;

				VectorMA( tr.endpos, 8.0, forward, start );

				// Store off the old count
				gEngfuncs.pEventAPI->EV_PushPMStates();
							
				// Now add in all of the players.
				gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );

				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
				gEngfuncs.pEventAPI->EV_PlayerTrace( start, vecDest, PM_STUDIO_BOX, -1, &beam_tr );

				if ( !beam_tr.allsolid )
				{
					vec3_t delta;
					float n;

					// trace backwards to find exit point

					gEngfuncs.pEventAPI->EV_PlayerTrace( beam_tr.endpos, tr.endpos, PM_STUDIO_BOX, -1, &beam_tr );

					VectorSubtract( beam_tr.endpos, tr.endpos, delta );
							
					n = Length( delta );
				}
				else
				{
					flDamage = 0;
				}

				gEngfuncs.pEventAPI->EV_PopPMStates();
				gEngfuncs.pEfxAPI->R_TempSprite(tr.endpos, vec3_origin, 0.2, m_iGlow, kRenderGlow, kRenderFxNoDissipation, 200.0 / 255.0, 0.3, FTENT_FADEOUT);
			}

			flDamage = 0;
		}
		else
		{
			VectorAdd( tr.endpos, forward, vecSrc );
		}
	}
}
//======================
//	   LIGHTGUN END 
//======================

//======================
//	   CROWBAR START
//======================
enum crowbar_e
{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
#if !CROWBAR_IDLE_ANIM
	CROWBAR_ATTACK3HIT
#else
	CROWBAR_ATTACK3HIT,
	CROWBAR_IDLE2,
	CROWBAR_IDLE3
#endif
};

int g_iSwing;

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Crowbar( event_args_t *args )
{
	int idx;
	vec3_t origin;
	//vec3_t angles;
	//vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM ); 

	if( EV_IsLocal( idx ) )
	{
		switch( (g_iSwing++) % 3 )
		{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( CROWBAR_ATTACK1MISS, 0 );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( CROWBAR_ATTACK2MISS, 0 );
				break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( CROWBAR_ATTACK3MISS, 0 );
				break;
		}
	}
}
//======================
//	   CROWBAR END 
//======================

//======================
//	  CROSSBOW START
//======================
enum crossbow_e
{
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2	// empty
};

//=====================
// EV_BoltCallback
// This function is used to correct the origin and angles 
// of the bolt, so it looks like it's stuck on the wall.
//=====================
void EV_BoltCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	ent->entity.origin = ent->entity.baseline.vuser1;
	ent->entity.angles = ent->entity.baseline.vuser2;
}

void EV_FireCrossbow2( event_args_t *args )
{
	vec3_t vecSrc, vecEnd;
	vec3_t up, right, forward;
	pmtrace_t tr;

	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	EV_GetGunPosition( args, vecSrc, origin );

	VectorMA( vecSrc, 8192, forward, vecEnd );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0xF ) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat( 0.95f, 1.0f ), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0xF ) );

	if( EV_IsLocal( idx ) )
	{
		if( args->iparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE1, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE3, 0 );
	}

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

	//We hit something
	if( tr.fraction < 1.0f )
	{
		physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent ); 

		//Not the world, let's assume we hit something organic ( dog, cat, uncle joe, etc ).
		if( pe->solid != SOLID_BSP )
		{
			switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, tr.endpos, CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			}
		}
		//Stick to world but don't stick to glass, it might break and leave the bolt floating. It can still stick to other non-transparent breakables though.
		else if( pe->rendermode == kRenderNormal ) 
		{
			gEngfuncs.pEventAPI->EV_PlaySound( 0, tr.endpos, CHAN_BODY, "weapons/xbow_hit1.wav", gEngfuncs.pfnRandomFloat( 0.95f, 1.0f ), ATTN_NORM, 0, PITCH_NORM );

			//Not underwater, do some sparks...
			if( gEngfuncs.PM_PointContents( tr.endpos, NULL ) != CONTENTS_WATER )
				 gEngfuncs.pEfxAPI->R_SparkShower( tr.endpos );

			vec3_t vBoltAngles;
			int iModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/crossbow_bolt.mdl" );

			VectorAngles( forward, vBoltAngles );

			TEMPENTITY *bolt = gEngfuncs.pEfxAPI->R_TempModel( tr.endpos - forward * 10, Vector( 0, 0, 0 ), vBoltAngles , 5, iModelIndex, TE_BOUNCE_NULL );

			if( bolt )
			{
				bolt->flags |= ( FTENT_CLIENTCUSTOM ); //So it calls the callback function.
				bolt->entity.baseline.vuser1 = tr.endpos - forward * 10; // Pull out a little bit
				bolt->entity.baseline.vuser2 = vBoltAngles; //Look forward!
				bolt->callback = EV_BoltCallback; //So we can set the angles and origin back. (Stick the bolt to the wall)
			}
		}
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

//TODO: Fully predict the fliying bolt.
void EV_FireCrossbow( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/xbow_fire1.wav", 1, ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0xF ) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/xbow_reload1.wav", gEngfuncs.pfnRandomFloat( 0.95f, 1.0f ), ATTN_NORM, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0xF ) );

	//Only play the weapon anims if I shot it. 
	if( EV_IsLocal( idx ) )
	{
		if( args->iparam1 )
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE1, 0 );
		else
			gEngfuncs.pEventAPI->EV_WeaponAnimation( CROSSBOW_FIRE3, 0 );

		V_PunchAxis( 0.0f, -2.0f );
	}
}
//======================
//	   CROSSBOW END 
//======================

//======================
//	    RPG START 
//======================
enum rpg_e
{
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL	// unloaded fidget
};

void EV_FireRpg( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, ATTN_NORM, 0, PITCH_NORM );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "weapons/glauncher.wav", 0.7, ATTN_NORM, 0, PITCH_NORM );

	//Only play the weapon anims if I shot it. 
	if( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( RPG_FIRE2, 0 );

		V_PunchAxis( 0, -5.0 );
	}
}
//======================
//	     RPG END 
//======================

//======================
//	    EGON START 
//======================
enum egon_e
{
	EGON_IDLE1 = 0,
	EGON_FIDGET1,
	EGON_ALTFIREON,
	EGON_ALTFIRECYCLE,
	EGON_ALTFIREOFF,
	EGON_FIRE1,
	EGON_FIRE2,
	EGON_FIRE3,
	EGON_FIRE4,
	EGON_DRAW,
	EGON_HOLSTER
};

int g_fireAnims1[] = { EGON_FIRE1, EGON_FIRE2, EGON_FIRE3, EGON_FIRE4 };
int g_fireAnims2[] = { EGON_ALTFIRECYCLE };

enum EGON_FIRESTATE
{
	FIRE_OFF,
	FIRE_CHARGE
};

enum EGON_FIREMODE
{
	FIRE_NARROW,
	FIRE_WIDE
};

#define	EGON_PRIMARY_VOLUME		450
#define EGON_BEAM_SPRITE		"sprites/xbeam1.spr"
#define EGON_FLARE_SPRITE		"sprites/XSpark1.spr"
#define EGON_SOUND_OFF			"weapons/egon_off1.wav"
#define EGON_SOUND_RUN			"weapons/egon_run3.wav"
#define EGON_SOUND_STARTUP		"weapons/egon_windup2.wav"

#if !defined(ARRAYSIZE)
#define ARRAYSIZE(p)		( sizeof(p) /sizeof(p[0]) )
#endif

BEAM *pBeam;
BEAM *pBeam2;
TEMPENTITY *pFlare;	// Vit_amiN: egon's beam flare

void EV_EgonFlareCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	float delta = currenttime - ent->tentOffset.z;	// time past since the last scale
	if( delta >= ent->tentOffset.y )
	{
		ent->entity.curstate.scale += ent->tentOffset.x * delta;
		ent->tentOffset.z = currenttime;
	}
}

void EV_EgonFire( event_args_t *args )
{
	int idx, /*iFireState,*/ iFireMode;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	//iFireState = args->iparam1;
	iFireMode = args->iparam2;
	int iStartup = args->bparam1;

	if( iStartup )
	{
		if( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_STARTUP, 0.9, ATTN_NORM, 0, 100 );
	}
	else
	{
		// If there is any sound playing already, kill it. - Solokiller
		// This is necessary because multiple sounds can play on the same channel at the same time.
		// In some cases, more than 1 run sound plays when the egon stops firing, in which case only the earliest entry in the list is stopped.
		// This ensures no more than 1 of those is ever active at the same time.
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );

		if( iFireMode == FIRE_WIDE )
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.98, ATTN_NORM, 0, 125 );
		else
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, EGON_SOUND_RUN, 0.9, ATTN_NORM, 0, 100 );
	}

	//Only play the weapon anims if I shot it.
	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( g_fireAnims1[gEngfuncs.pfnRandomLong( 0, 3 )], 0 );

	if( iStartup == 1 && EV_IsLocal( idx ) && !( pBeam || pBeam2 || pFlare ) && cl_lw->value ) //Adrian: Added the cl_lw check for those lital people that hate weapon prediction.
	{
		vec3_t vecSrc, vecEnd, angles, forward, right, up;
		pmtrace_t tr;

		cl_entity_t *pl = gEngfuncs.GetEntityByIndex( idx );

		if( pl )
		{
			VectorCopy( gHUD.m_vecAngles, angles );

			AngleVectors( angles, forward, right, up );

			EV_GetGunPosition( args, vecSrc, pl->origin );

			VectorMA( vecSrc, 2048, forward, vecEnd );

			gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

			// Store off the old count
			gEngfuncs.pEventAPI->EV_PushPMStates();

			// Now add in all of the players.
			gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

			gEngfuncs.pEventAPI->EV_PopPMStates();

			int iBeamModelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_BEAM_SPRITE );

			float r = 50.0f;
			float g = 50.0f;
			float b = 125.0f;

			// if( IEngineStudio.IsHardware() )
			{
				r /= 255.0f;
				g /= 255.0f;
				b /= 255.0f;
			}

			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 3.5, 0.2, 0.7, 55, 0, 0, r, g, b );

			if( pBeam )
				 pBeam->flags |= ( FBEAM_SINENOISE );

			pBeam2 = gEngfuncs.pEfxAPI->R_BeamEntPoint( idx | 0x1000, tr.endpos, iBeamModelIndex, 99999, 5.0, 0.08, 0.7, 25, 0, 0, r, g, b );

			// Vit_amiN: egon beam flare
			pFlare = gEngfuncs.pEfxAPI->R_TempSprite( tr.endpos, vec3_origin, 1.0, gEngfuncs.pEventAPI->EV_FindModelIndex( EGON_FLARE_SPRITE ), kRenderGlow, kRenderFxNoDissipation, 1.0, 99999, FTENT_SPRCYCLE | FTENT_PERSIST );
		}
	}

	if( pFlare )	// Vit_amiN: store the last mode for EV_EgonStop()
	{
		pFlare->tentOffset.x = ( iFireMode == FIRE_WIDE ) ? 1.0f : 0.0f;
	}
}

void EV_EgonStop( event_args_t *args )
{
	int idx;
	vec3_t origin;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EGON_SOUND_RUN );

	if( args->iparam1 )
		 gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, EGON_SOUND_OFF, 0.98, ATTN_NORM, 0, 100 );

	if( EV_IsLocal( idx ) )
	{
		if( pBeam )
		{
			pBeam->die = 0.0f;
			pBeam = NULL;
		}

		if( pBeam2 )
		{
			pBeam2->die = 0.0f;
			pBeam2 = NULL;
		}

		if( pFlare )	// Vit_amiN: egon beam flare
		{
			pFlare->die = gEngfuncs.GetClientTime();

			if( gEngfuncs.GetMaxClients() == 1 || !(pFlare->flags & FTENT_NOMODEL) )
			{
				if( pFlare->tentOffset.x != 0.0f )	// true for iFireMode == FIRE_WIDE
				{
					pFlare->callback = &EV_EgonFlareCallback;
					pFlare->fadeSpeed = 2.0;			// fade out will take 0.5 sec
					pFlare->tentOffset.x = 10.0;		// scaling speed per second
					pFlare->tentOffset.y = 0.1;			// min time between two scales
					pFlare->tentOffset.z = pFlare->die;	// the last callback run time
					pFlare->flags = FTENT_FADEOUT | FTENT_CLIENTCUSTOM;
				}
			}

			pFlare = NULL;
		}
	}
}
//======================
//	    EGON END 
//======================

//======================
//	   HORNET START
//======================
enum hgun_e
{
	HGUN_IDLE1 = 0,
	HGUN_FIDGETSWAY,
	HGUN_FIDGETSHAKE,
	HGUN_DOWN,
	HGUN_UP,
	HGUN_SHOOT
};

void EV_HornetGunFire( event_args_t *args )
{
	int idx; //, iFireMode;
	vec3_t origin, angles; //, vecSrc, forward, right, up;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	//iFireMode = args->iparam1;

	//Only play the weapon anims if I shot it.
	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, gEngfuncs.pfnRandomLong( 0, 2 ) );
		gEngfuncs.pEventAPI->EV_WeaponAnimation( HGUN_SHOOT, 0 );
	}

	switch( gEngfuncs.pfnRandomLong( 0, 2 ) )
	{
		case 0:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire1.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 1:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire2.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 2:
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "agrunt/ag_fire3.wav", 1, ATTN_NORM, 0, 100 );
			break;
	}
}
//======================
//	   HORNET END
//======================

//======================
//	   TRIPMINE START
//======================
enum tripmine_e
{
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND
};

//We only check if it's possible to put a trip mine
//and if it is, then we play the animation. Server still places it.
void EV_TripmineFire( event_args_t *args )
{
	int idx;
	vec3_t vecSrc, angles, view_ofs, forward;
	pmtrace_t tr;

	idx = args->entindex;
	const bool last = args->bparam1 != 0;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, NULL, NULL );

	if( !EV_IsLocal ( idx ) )
		return;

	// Grab predicted result for local player
	gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );

	vecSrc = vecSrc + view_ofs;

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecSrc + forward * 128.0f, PM_NORMAL, -1, &tr );

	//Hit something solid
	if( tr.fraction < 1.0f && !last )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation ( TRIPMINE_DRAW, 0 );
	
	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   TRIPMINE END
//======================

//======================
//	   SQUEAK START
//======================
enum squeak_e
{
	SQUEAK_IDLE1 = 0,
	SQUEAK_FIDGETFIT,
	SQUEAK_FIDGETNIP,
	SQUEAK_DOWN,
	SQUEAK_UP,
	SQUEAK_THROW
};

#define VEC_HULL_MIN		Vector( -16, -16, -36 )
#define VEC_DUCK_HULL_MIN	Vector( -16, -16, -18 )

void EV_SnarkFire( event_args_t *args )
{
	int idx;
	vec3_t vecSrc, angles, /*view_ofs,*/ forward;
	pmtrace_t tr;

	idx = args->entindex;
	VectorCopy( args->origin, vecSrc );
	VectorCopy( args->angles, angles );

	AngleVectors( angles, forward, NULL, NULL );

	if( !EV_IsLocal ( idx ) )
		return;

	if( args->ducking )
		vecSrc = vecSrc - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc + forward * 20, vecSrc + forward * 64, PM_NORMAL, -1, &tr );

	//Find space to drop the thing.
	if( tr.allsolid == 0 && tr.startsolid == 0 && tr.fraction > 0.25f )
		 gEngfuncs.pEventAPI->EV_WeaponAnimation( SQUEAK_THROW, 0 );

	gEngfuncs.pEventAPI->EV_PopPMStates();
}
//======================
//	   SQUEAK END
//======================

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	const char *pszSound;

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0f;
	noise = (int)( ( ( us_params ) >> 12 ) & 0x0007 );
	pitch = (int)( 10.0f * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch( noise )
	{
	case 1:
		pszSound = "plats/ttrain1.wav";
		break;
	case 2:
		pszSound = "plats/ttrain2.wav";
		break;
	case 3:
		pszSound = "plats/ttrain3.wav";
		break; 
	case 4:
		pszSound = "plats/ttrain4.wav";
		break;
	case 5:
		pszSound = "plats/ttrain6.wav";
		break;
	case 6:
		pszSound = "plats/ttrain7.wav";
		break;
	default:
		// no sound
		return;
	}

	if( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, pszSound );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, pszSound, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

void EV_VehiclePitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	const char *pszSound;

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0f;
	noise = (int)( ( ( us_params ) >> 12 ) & 0x0007 );
	pitch = (int)( 10.0f * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch( noise )
	{
	case 1:
		pszSound = "plats/vehicle1.wav";
		break;
	case 2:
		pszSound = "plats/vehicle2.wav";
		break;
	case 3:
		pszSound = "plats/vehicle3.wav";
		break;
	case 4:
		pszSound = "plats/vehicle4.wav";
		break;
	case 5:
		pszSound = "plats/vehicle6.wav";
		break;
	case 6:
		pszSound = "plats/vehicle7.wav";
		break;
	default:
		// no sound
		return;
	}

	if( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, pszSound );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, pszSound, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}

//============================================
//	   DELTA PARTICLES WEAPONS
//============================================

//======================
//	   PIPEWRENCH START
//======================
enum pwrench_e
{
	PIPEWRENCH_IDLE1 = 0,
	PIPEWRENCH_IDLE2,
	PIPEWRENCH_IDLE3,
	PIPEWRENCH_DRAW,
	PIPEWRENCH_HOLSTER,
	PIPEWRENCH_ATTACK1HIT,
	PIPEWRENCH_ATTACK1MISS,
	PIPEWRENCH_ATTACK2HIT,
	PIPEWRENCH_ATTACK2MISS,
	PIPEWRENCH_ATTACK3HIT,
	PIPEWRENCH_ATTACK3MISS,
	PIPEWRENCH_ATTACKBIGWIND,
	PIPEWRENCH_ATTACKBIGHIT,
	PIPEWRENCH_ATTACKBIGMISS,
	PIPEWRENCH_ATTACKBIGLOOP
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_PipeWrench( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	if( EV_IsLocal( idx ) )
	{
		if( args->iparam1 ) // Is primary attack?
		{
			//Play Swing sound
			switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
			{
			case 0:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pwrench_miss1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pwrench_miss2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
				break;
			}

			//gEngfuncs.pEventAPI->EV_WeaponAnimation( PIPEWRENCH_ATTACK1MISS, 1 );

			// Send weapon anim.
			switch( ( g_iSwing++ ) % 3 )
			{
			case 0:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( PIPEWRENCH_ATTACK1MISS, 0 );
				break;
			case 1:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( PIPEWRENCH_ATTACK2MISS, 0 );
				break;
			case 2:
				gEngfuncs.pEventAPI->EV_WeaponAnimation( PIPEWRENCH_ATTACK3MISS, 0 );
				break;
			}
		}
		else
		{
			// Play Swing sound
			gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, PITCH_NORM );

			// Send weapon anim.
			gEngfuncs.pEventAPI->EV_WeaponAnimation( PIPEWRENCH_ATTACKBIGMISS, 0 );
		}
	}
}
//======================
//	   PIPEWRENCH END 
//======================

//======================
//	    SNIPERRIFLE START
//======================
void EV_Sniperrifle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;

	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	vec3_t shellRight;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );
	empty = args->bparam1;
	
	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/snipershell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? SNIPERRIFLE_FIRE2 : SNIPERRIFLE_FIRE1, 2 );

		V_PunchAxis( 0, -15 );
	}
		
	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 20, -12, 4 );

	// XaeroX: don't eject brass shell in first-person mode
	// but instead, store shell velocity in viewmodel, for a studio event
	if ( !CL_IsThirdPerson() ) 
	{
		cl_entity_t *view = gEngfuncs.GetViewModel();
		if ( view ) {
			++view->curstate.iuser4;
			VectorCopy( ShellVelocity, view->curstate.vuser1 );
		}
	}
	else
	{
		// XaeroX: thirdperson mode, eject immediately
		EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], 
						gEngfuncs.pEventAPI->EV_FindModelIndex( "models/snipershell.mdl" ), TE_BOUNCE_SHELL ); 
	}

	switch( gEngfuncs.pfnRandomLong( 0, 1 ) )
	{
	case 0:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	case 1:
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		break;
	}

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	if ( gEngfuncs.GetMaxClients() > 1 )
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_14MM, 0, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
	}
	else
	{
		EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_14MM, 0, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
	}
}
//======================
//		 SNIPERRIFLE END
//======================

//======================
//	    44DESERT_EAGLE START
//======================
void EV_Deagle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	int empty;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	vec3_t shellRight;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/44shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? DEAGLE_SHOOT_EMPTY : DEAGLE_SHOOT, 2 );

		V_PunchAxis( 0, -6 );
	}

	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 15, -5, -13 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/44_gun_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_44, 0, 0, args->fparam1, args->fparam2 );
}
//======================
//	   44DESERT_EAGLE END
//======================

//======================
//	    SMG START
//======================
void EV_SMG( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	vec3_t shellRight;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/45acp_shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SMG_FIRE1,	args->iparam1 );
		
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 15, -7, -4 );
	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/smg_fire1.wav", 1, 0.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_45ACP, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}

void EV_SMG2( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	vec3_t shellRight;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );
	VectorScale( right, -1.0f, shellRight );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/45acp_shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SMG_FIRE2,	args->iparam1 );
		V_PunchAxis( 0, gEngfuncs.pfnRandomFloat( -2, 2 ) );
	}
	// XaeroX: shellRight is actually left
	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, shellRight, up, 15, -7, -4 );
	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/smg_fire2.wav", 1, 0.6, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_45ACP, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//		 SMG END
//======================