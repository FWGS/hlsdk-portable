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

extern engine_studio_api_t IEngineStudio;

static int g_tracerCount[32];

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern cvar_t *cl_lw;

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
void EV_VehiclePitchAdjust( event_args_t *args );
}

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

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	const char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	cl_entity_t *ent;
	char *pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//
	chTextureType = 0;

	// Player
	if( ( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
	    || ( ( ent = gEngfuncs.GetEntityByIndex( entity )) && ( ent->curstate.eflags & EFLAG_FLESH_SOUND )))
	{
		// hit body
		chTextureType = CHAR_TEX_FLESH;
	}
	else if( entity == 0 )
	{
		// get texture from entity or world (world is ent(0))
		pTextureName = (char *)gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );

		if ( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if( *pTextureName == '-' || *pTextureName == '+' )
			{
				pTextureName += 2;
			}

			if( *pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ' )
			{
				pTextureName++;
			}

			// '}}'
			strcpy( szbuffer, pTextureName );
			szbuffer[CBTEXTURENAMEMAX - 1] = 0;

			// get texture type
			chTextureType = PM_FindTextureType( szbuffer );
		}
	}
	
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

void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType )
{
	physent_t *pe;

	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		switch( iBulletType )
		{
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_HLDM_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal( pe ) );
			break;
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
		case BULLET_PLAYER_MP5:
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

		// tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if( tr.fraction != 1.0f )
		{
			switch( iBulletType )
			{
			default:
			case BULLET_PLAYER_9MM:
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_MP5:
				//if( !tracer )
				{
					EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
					EV_HLDM_DecalGunshot( &tr, iBulletType );
				}
				break;
			case BULLET_PLAYER_BUCKSHOT:
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_357:
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

#define SND_CHANGE_PITCH	(1 << 7) // duplicated in protocol.h change sound pitch

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

//======================
//	   KNIFE START
//======================
enum knife_e
{
	KNIFE_IDLE = 0,
	KNIFE_DRAW,
	KNIFE_HOLSTER,
	KNIFE_SLASH1,
	KNIFE_SLASH2
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Knife( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/knife_swing1.wav", 1, ATTN_NORM, 0, PITCH_LOW + gEngfuncs.pfnRandomLong( -1, 0xf ) ); 

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( KNIFE_SLASH1 + gEngfuncs.pfnRandomLong( 0, 1 ), 1 );
	}
}
//======================
//	   KNIFE END 
//======================

//======================
//	   AXE START
//======================
enum axe_e
{
	AXE_IDLE = 0,
	AXE_SLASH,
	AXE_DRAW
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Axe( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/axe_swing.wav", 1, ATTN_NORM, 0, PITCH_LOW + gEngfuncs.pfnRandomLong( -1, 0xf ) );

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( AXE_SLASH, 1 );
	}
}
//======================
//	   AXE END 
//======================

//======================
//	   HAMMER START
//======================
enum hammer_e
{
	HAMMER_IDLE = 0,
	HAMMER_WHACK,
	HAMMER_DRAW
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Hammer( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	
	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/hammer_swing.wav", 1, ATTN_NORM, 0, PITCH_LOW + gEngfuncs.pfnRandomLong( -1, 0xf ) );

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( HAMMER_WHACK, 1 );
	}
}
//======================
//	   HAMMER END 
//======================

//======================
//	   SPEAR START
//======================
enum spear_e
{
	SPEAR_IDLE = 0,
	SPEAR_STAB_START,
	SPEAR_STAB_MISS,
	SPEAR_STAB,
	SPEAR_DRAW,
	SPEAR_ELECTROCUTE
};

//Only predict the miss sounds, hit sounds are still played 
//server side, so players don't get the wrong idea.
void EV_Spear( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	idx = args->entindex;
	VectorCopy( args->origin, origin );

	//Play Swing sound
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/spear_swing.wav", 1, ATTN_NORM, 0, PITCH_LOW + gEngfuncs.pfnRandomLong( -1, 0xf ) );

	if ( EV_IsLocal( idx ) )
	{
		gEngfuncs.pEventAPI->EV_WeaponAnimation ( SPEAR_STAB_MISS, 1 );
	}
}
//======================
//	   SPEAR END 
//======================

//======================
//	    GLOCK START
//======================
enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_FIRE1,
	GLOCK_FIRE2,
	GLOCK_FIRE_LOAD,
	GLOCK_RELOAD,
	GLOCK_RELOAD_EMPTY,
	GLOCK_DRAW
};

void EV_FireGlock1( event_args_t *args )
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
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_FIRE_LOAD : GLOCK_FIRE1, 2 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 60, -8, 20 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}

void EV_FireGlock2( event_args_t *args )
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
	vec3_t vecSpread;
	vec3_t up, right, forward;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? GLOCK_FIRE_LOAD : GLOCK_FIRE2, 2 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 60, -8, 20 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/glock_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
	
}
//======================
//	   GLOCK END
//======================

//======================
//	    BERETTA START
//======================
enum beretta_e
{
	BERETTA_IDLE1 = 0,
	BERETTA_IDLE2,
	BERETTA_FIRE1,
	BERETTA_FIRE2,
	BERETTA_FIRE_LOAD,
	BERETTA_RELOAD,
	BERETTA_RELOAD_EMPTY,
	BERETTA_DRAW
};

void EV_FireBeretta( event_args_t *args )
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
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? BERETTA_FIRE_LOAD : BERETTA_FIRE1, 2 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 60, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ber_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}
//======================
//	   BERETTA END
//======================

//======================
//	    P228 START
//======================
enum p228_e
{
	P228_IDLE1 = 0,
	P228_IDLE2,
	P228_FIRE1,
	P228_FIRE2,
	P228_FIRE_LOAD,
	P228_RELOAD,
	P228_RELOAD_EMPTY,
	P228_DRAW
};

void EV_FireP228( event_args_t *args )
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
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? P228_FIRE_LOAD : P228_FIRE1, 2 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 70, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/p228_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}
//======================
//	   P228 END
//======================

//======================
//	    DEAGLE START
//======================
enum deagle_e
{
	DEAGLE_IDLE1 = 0,
	DEAGLE_IDLE2,
	DEAGLE_FIRE1,
	DEAGLE_FIRE2,
	DEAGLE_FIRE_LOAD,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_EMPTY,
	DEAGLE_DRAW
};

void EV_FireDeagle( event_args_t *args )
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
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	empty = args->bparam1;
	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell

	if ( EV_IsLocal( idx ) )
	{
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( empty ? DEAGLE_FIRE_LOAD : DEAGLE_FIRE1 , 2 );

		V_PunchAxis( 0, -2.0 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 60, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/deagle_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_9MM, 0, 0, args->fparam1, args->fparam2 );
}
//======================
//	   DEAGLE END
//======================

//======================
//	REVOLVER START 
//======================
enum revolver_e
{
	REVOLVER_IDLE = 0,
	REVOLVER_SHOOT1,
	REVOLVER_SHOOT2,
	REVOLVER_SHOOT_EMPTY,
	REVOLVER_RELOAD,
	REVOLVER_DRAW
};

void EV_FireRevolver( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;

	vec3_t vecSrc, vecAiming;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( REVOLVER_SHOOT1, 0 );

		V_PunchAxis( 0, -2.0 );
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/revolver_fire.wav", 0.8, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_357, 0, 0, args->fparam1, args->fparam2 );
}
//======================
//	  REVOLVER END 
//======================

//======================
//	  SHOTGUN START
//======================
enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_IDLE2,
	SHOTGUN_FIRE,
	SHOTGUN_RELOAD_START,
	SHOTGUN_RELOAD_INS,
	SHOTGUN_RELOAD_END,
	SHOTGUN_DRAW
};

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( SHOTGUN_FIRE, 2 );
	}

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/shotgun_fire.wav", 1.0, ATTN_NORM, 0, PITCH_LOW + gEngfuncs.pfnRandomLong( 0, 9 ) );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 8, vecSrc, vecAiming, 2048, BULLET_PLAYER_BUCKSHOT, 0, &g_tracerCount[idx-1], 0.08716, 0.08716 );
}
//======================
//	   SHOTGUN END
//======================

//======================
//	    MP5K START
//======================
enum mp5_e
{
	MP5K_IDLE = 0,
	MP5K_FIDGET,
	MP5K_FIRE1,
	MP5K_FIRE2,
	MP5K_RELOAD,
	MP5K_DRAW
};

void EV_FireMP5K( event_args_t *args )
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
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( MP5K_FIRE1 + gEngfuncs.pfnRandomLong(0,1), 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 30, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/mp5k_fire.wav", 1, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//	  MP5K END
//======================

//======================
//	  UZI START
//======================
enum uzi_e
{
	UZI_IDLE = 0,
	UZI_RELOAD,
	UZI_DRAW,
	UZI_FIRE1,
	UZI_FIRE2,
	UZI_FIRE3
};

void EV_FireUzi( event_args_t *args )
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
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if ( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( UZI_FIRE1 + gEngfuncs.pfnRandomLong(0,2), 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 30, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/uzi_fire.wav", 1, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//		UZI END
//======================

//======================
//	GMGENERAL START
//======================
enum gmgeneral_e
{
	GMGENERAL_IDLE,
	GMGENERAL_FIRE1,
	GMGENERAL_FIRE2,
	GMGENERAL_RELOAD,
	GMGENERAL_DRAW
};

void EV_FireGMGeneral( event_args_t *args )
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
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex ("models/shell.mdl");// brass shell
	
	if( EV_IsLocal( idx ) )
	{
		// Add muzzle flash to current weapon model
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation( GMGENERAL_FIRE1 + gEngfuncs.pfnRandomLong(0,1), 2 );
	}

	EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 30, -12, 30 );

	EV_EjectBrass ( ShellOrigin, ShellVelocity, angles[ YAW ], shell, TE_BOUNCE_SHELL ); 

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "gmgeneral/gm_expell.wav", 1, ATTN_NORM, 0, PITCH_NORM );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	EV_HLDM_FireBullets( idx, forward, right, up, 1, vecSrc, vecAiming, 8192, BULLET_PLAYER_MP5, 2, &g_tracerCount[idx-1], args->fparam1, args->fparam2 );
}
//======================
//	GMGENERAL END
//======================

