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
#include "ev_tfc.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

static int tracerCount[32];

extern "C" char PM_FindTextureType( char *name );

void V_PunchAxis( int axis, float punch );
void VectorAngles( const float *forward, float *angles );

extern "C" float anglemod( float a );

struct eventnode_s
{
    event_args_t *data;
    eventnode_s *prev;
    eventnode_s *next;
};
typedef eventnode_s eventnode_t;

pmtrace_t *gp_tr_decal[33];
extern float g_flSpinDownTime[33];
extern int g_bACSpinning[33];
extern float g_flSpinUpTime[33];
int m_iSwing;
extern cvar_t *cl_lw;
BEAM *pBeam;
BEAM *pBeam2;

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

#define VECTOR_CONE_1DEGREES Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES Vector( 0.17365, 0.17365, 0.17365 )

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play
float EV_HLDM_PlayTextureSound( int idx, pmtrace_t *ptr, float *vecSrc, float *vecEnd, int iBulletType )
{
	// hit the world, try to play sound based on texture material type
	char chTextureType = CHAR_TEX_CONCRETE;
	float fvol;
	float fvolbar;
	char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	char *pTextureName;
	char texname[64];
	char szbuffer[64];

	entity = gEngfuncs.pEventAPI->EV_IndexFromTrace( ptr );

	// FIXME check if playtexture sounds movevar is set
	//
	chTextureType = 0;

	// Player
	if( entity >= 1 && entity <= gEngfuncs.GetMaxClients() )
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
		sprintf( decalname, "{bproof1" );
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
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

		tracer = EV_HLDM_CheckTracer( idx, vecSrc, tr.endpos, forward, right, iBulletType, iTracerFreq, tracerCount );

		// do damage, paint decals
		if( tr.fraction != 1.0 )
		{
			switch( iBulletType )
			{
			default:
			case BULLET_PLAYER_9MM:
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_MP5:
				if( !tracer )
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

#define SND_CHANGE_PITCH	(1 << 7)

void EV_TrainPitchAdjust( event_args_t *args )
{
	int idx;
	vec3_t origin;

	unsigned short us_params;
	int noise;
	float m_flVolume;
	int pitch;
	int stop;

	char sz[256];

	idx = args->entindex;

	VectorCopy( args->origin, origin );

	us_params = (unsigned short)args->iparam1;
	stop = args->bparam1;

	m_flVolume = (float)( us_params & 0x003f ) / 40.0;
	noise = (int)( ( ( us_params ) >> 12 ) & 0x0007 );
	pitch = (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	switch( noise )
	{
	case 1:
		strcpy( sz, "plats/ttrain1.wav" );
		break;
	case 2:
		strcpy( sz, "plats/ttrain2.wav" );
		break;
	case 3:
		strcpy( sz, "plats/ttrain3.wav" );
		break;
	case 4:
		strcpy( sz, "plats/ttrain4.wav");
		break;
	case 5:
		strcpy( sz, "plats/ttrain6.wav");
		break;
	case 6:
		strcpy( sz, "plats/ttrain7.wav");
		break;
	default:
		// no sound
		strcpy( sz, "" );
		return;
	}

	if( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, sz );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, sz, m_flVolume, ATTN_NORM, SND_CHANGE_PITCH, pitch );
	}
}

int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 )
{
	return 0;
}

enum tfc_axe_e
{
    AXE_IDLE1 = 0,
    AXE_DRAW,
    AXE_HOLSTER,
    AXE_ATTACK1,
    AXE_ATTACK1MISS,
    AXE_ATTACK2,
    AXE_ATTACK2HIT,
    AXE_ATTACK3,
    AXE_ATTACK3HIT,
    AXE_IDLE2,
    AXE_IDLE3
};

int EV_TFC_PlayCrowbarAnim( int iAnimType )
{
	m_iSwing++;

	if( iAnimType == 1 )
	{
		if( m_iSwing % 2 == 0 )
			return AXE_ATTACK2HIT;
		else
			return AXE_ATTACK3HIT;
	}
	else
	{
		switch( m_iSwing % 3 )
		{
		case 0:
			return AXE_ATTACK1;
		case 1:
			return AXE_ATTACK2;
		case 2:
			return AXE_ATTACK3;
		}
	}
}

#define CLASS_SPY 0
#define CLASS_ENGINEER 1
#define CLASS_MEDIC 2
#define CLASS_DEFAULT 3

void EV_TFC_Axe( event_args_t *args )
{
    int idx;
    float fSoundData;
    float *vecc;
    signed int classid;
    int ent;
    pmtrace_t tr;
    pmtrace_t tr2;
    vec3_t vecSrc, vecEnd;
    vec3_t right, forward, up;
    vec3_t origin, angles;
	unsigned short m_usAxeDecal;

    idx = args->entindex;
    classid = args->iparam1;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	AngleVectors(angles, forward, right, up);
    EV_GetGunPosition(args, vecSrc, origin);
    VectorMA(vecSrc, 32.0, forward, vecEnd);
    gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(0, 1);
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_NORMAL, -1, &tr);

    if (tr.fraction >= 1.0)
    {
        gEngfuncs.pEventAPI->EV_SetTraceHull(1);
        gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr2 );

        if(tr2.fraction < 1.0)
            tr = tr2;
    }

    gEngfuncs.pEventAPI->EV_PopPMStates();
    gp_tr_decal[idx-1] = 0;

    if( tr.fraction >= 1.0 )
    {
        if ( !EV_IsLocal(idx) )
        {
            EV_TFC_PlayAxeSound( idx, classid, origin, 0, 0.0 );
            return;
        }

        switch( classid )
        {
		case CLASS_DEFAULT:
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( EV_TFC_PlayCrowbarAnim( 0 ), 2 );
			EV_TFC_PlayAxeSound( idx, classid, origin, 0, 1.0 );
			return;
		}
        default:
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ( classid == CLASS_ENGINEER ) ? 1 : 2, 2 );
			EV_TFC_PlayAxeSound( idx, classid, origin, 0, 1.0 );
			return;
		}
		}
    }

	gp_tr_decal[idx-1] = &tr; //Velaron: add g_tr_decal?
	ent = gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr );

    if( EV_IsLocal(idx) )
    {
		switch( classid )
        {
		case CLASS_DEFAULT:
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( EV_TFC_PlayCrowbarAnim( 1 ), 2 );
			break;
		}
        default:
		{
			gEngfuncs.pEventAPI->EV_WeaponAnimation( ( classid == CLASS_SPY ) ? 2 : 3, 2 );
			break;
		}
		}
	}

	if( EV_IsLocal(idx) )
    {
		switch( classid )
        {
		case CLASS_ENGINEER:
		{
			if ( !EV_TFC_IsAlly( idx, ent ) )
				if ( cl_localblood && cl_localblood->value != 0.0 )
					EV_TFC_TraceAttack(idx, forward, &tr, 20);
			break;
		}
		case CLASS_MEDIC:
		{
			gp_tr_decal[idx-1] = 0;
			EV_TFC_Medkit( idx, origin, forward, right, ent, forward, &tr );
			break;
		}
        default:
		{
			EV_TFC_AxeHit( idx, origin, forward, right, ent, forward, &tr );
			break;
		}
		}
	}

	if ( gp_tr_decal[idx-1] )
	{
		m_usAxeDecal = gEngfuncs.pEventAPI->EV_PrecacheEvent( 1, "events/wpn/tf_axedecal.sc" );
		gEngfuncs.pEventAPI->EV_PlaybackEvent( idx, 0, m_usAxeDecal, 0.2, (float *)&origin, (float *)&angles, 0, 0, 0, 0, 0, 0 );
	}

	if ( EV_IsPlayer(ent) )
	{
		EV_TFC_PlayAxeSound( idx, classid, origin, 1, 0.0 );
		return;
	}

	fSoundData = 1.0;
	if ( gEngfuncs.GetMaxClients() < 2 )
		fSoundData = EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecSrc, 5 );
	EV_TFC_PlayAxeSound( idx, classid, origin, 2, fSoundData );
}

enum tfc_shotgun_e
{
    TFCSHOTGUN_IDLE = 0,
    TFCSHOTGUN_SHOOT,
    TFCSHOTGUN_SHOOT_BIG,
    TFCSHOTGUN_RELOAD,
    TFCSHOTGUN_PUMP,
    TFCSHOTGUN_STARTRELOAD,
    TFCSHOTGUN_DRAW,
    TFCSHOTGUN_REHOLSTER,
    TFCSHOTGUN_IDLE4,
    TFCSHOTGUN_DEEPIDLE
};

void EV_FireTFCShotgun(event_args_t *args)
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;

	idx = args->entindex;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl");

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	if(EV_IsLocal(idx))
	{
	    EV_MuzzleFlash();
	    gEngfuncs.pEventAPI->EV_WeaponAnimation(TFCSHOTGUN_SHOOT, 2);
        V_PunchAxis(0, -2.0);
	}

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles.y, shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/sbarrel1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), 0.8, 0, gEngfuncs.pfnRandomLong(0, 31) + 93);
	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 6, vecSrc, vecAiming, 2048.0, BULLET_PLAYER_BUCKSHOT, 0, tracerCount + idx, 0.04, 0.04);
}

void EV_ReloadTFCShotgun(event_args_t *args)
{
    int idx;
    vec3_t origin;
    const char* sound;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(TFCSHOTGUN_RELOAD, 2);
    if(gEngfuncs.pfnRandomLong(0, 1))
        sound = "weapons/reload3.wav";

    else
        sound = "weapons/reload1.wav";
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, sound, 1.0, 0.8, 0, gEngfuncs.pfnRandomLong(0, 31) + 85);
}

void EV_PumpTFCShotgun(event_args_t *args)
{
    int full;
    int idx;
    vec3_t origin;

    full = args->bparam1;
    idx = args->entindex;
    VectorCopy(args->origin, origin);
    if(full && EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(TFCSHOTGUN_PUMP, 2);
  gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/scock1.wav", 1.0, 0.8, 0, gEngfuncs.pfnRandomLong(0, 31) + 95);
}

enum tf_ng_e
{
    NG_LONGIDLE = 0,
    NG_IDLE1,
    NG_GRENADE,
    NG_RELOAD,
    NG_DEPLOY,
    NG_SHOOT1,
    NG_SHOOT2,
    NG_SHOOT3
};

void EV_FireTFCNailgun(event_args_t *args)
{
  	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;

	idx = args->entindex;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/nail.mdl");

    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(NG_SHOOT1, 2);
    EV_GetGunPosition(args, ShellOrigin, origin);
    VectorMA(ShellOrigin, -4.0, up, ShellOrigin);
    VectorMA(ShellOrigin, 2.0, right, ShellOrigin);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/airgun_1.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), 0.8, 0, gEngfuncs.pfnRandomLong(0, 31) + 93);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(ShellOrigin, velocity, shell, 6, idx, EV_TFC_NailTouch);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -1.0);
}

void EV_TFC_NailTouch(struct tempent_s *ent, pmtrace_t *ptr)
{
  physent_t *pe;
  char name;

    pe = gEngfuncs.pEventAPI->EV_GetPhysent(ptr->ent);
    if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
    {
        sprintf(&name, "{shot%i", gEngfuncs.pfnRandomLong(0, 4) + 1);
        if(name)
        {
            if (ptr->fraction != 1.0)
                EV_HLDM_GunshotDecalTrace(ptr, &name);
        }
    }
}

void EV_FireTFCSuperNailgun(event_args_t *args)
{
  	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;

	idx = args->entindex;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/nail.mdl");

    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(NG_SHOOT1, 2);
    EV_GetGunPosition(args, ShellOrigin, origin);
    VectorMA(ShellOrigin, -4.0, up, ShellOrigin);
    VectorMA(ShellOrigin, 2.0, right, ShellOrigin);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/spike2.wav", 1.0, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(ShellOrigin, velocity, shell, 6, idx, EV_TFC_NailTouch);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -1.0);
}

void EV_FireTFCSuperShotgun(event_args_t *args)
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t up, right, forward;

	vec3_t ShellVelocity;
	vec3_t ShellOrigin;
	int shell;
	vec3_t vecSrc, vecAiming;

	idx = args->entindex;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
	VectorCopy(args->velocity, velocity);

	AngleVectors(angles, forward, right, up);

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shotgunshell.mdl");

	EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20, -12, 4);

	if(EV_IsLocal(idx))
	{
	    EV_MuzzleFlash();
	    gEngfuncs.pEventAPI->EV_WeaponAnimation(TFCSHOTGUN_SHOOT, 2);
        V_PunchAxis(0, -4.0);
	}

	EV_EjectBrass(ShellOrigin, ShellVelocity, angles.y, shell, TE_BOUNCE_SHELL);

	gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/shotgn2.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), 0.8, 0, gEngfuncs.pfnRandomLong(0, 31) + 93);
	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	EV_HLDM_FireBullets(idx, forward, right, up, 14, vecSrc, vecAiming, 2048.0, BULLET_PLAYER_BUCKSHOT, 0, tracerCount + idx, 0.04, 0.04);
}

enum tfc_sniper_e
{
    SNIPER_IDLE = 0,
    SNIPER_AIM,
    SNIPER_FIRE,
    SNIPER_DRAW,
    SNIPER_HOLSTER,
    SNIPER_AUTOIDLE,
    SNIPER_AUTOFIRE,
    SNIPER_AUTODRAW,
    SBIPER_AUTOHOLSTER
};

void EV_FireTFCAutoRifle(event_args_t *args)
{
    int idx;
    physent_s *pe;
    pmtrace_t tr;

  	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;

	vec3_t vecSrc, vecAiming, vecEnd;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
    AngleVectors(angles, forward, right, up);
    if (EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(SNIPER_AUTOFIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/sniper.wav", 0.9, 0.8, 0, 100);
    EV_GetGunPosition(args, vecSrc, origin);
    vecAiming = forward;
    VectorMA(vecSrc, 8192.0, vecAiming, vecEnd);
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

    if ( tr.fraction != 1.0 )
    {
        if(cl_localblood->value != 0.0)
            EV_TFC_TraceAttack(idx, vecAiming, &tr, 8.0);
        EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, BULLET_PLAYER_357);
        pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
        if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
            EV_HLDM_DecalGunshot(&tr, BULLET_PLAYER_357);
    }
    gEngfuncs.pEventAPI->EV_PopPMStates();
}

void EV_TFC_TraceAttack(int idx, float *vecDir, pmtrace_t *ptr, float flDamage)
{
    int index;
    signed int iPlayer1;
    signed int iPlayer2;
    vec3_t dir;
    vec3_t vecOrigin;

    VectorScale(vecDir, -4.0, dir);
    vecOrigin.x = (int)ptr + 20 - dir.x;
    vecOrigin.y = (int)ptr + 24 - dir.y;
    vecOrigin.z = (int)ptr + 28 - dir.z;
    index = gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr);
    if(index > 0 && index <= gEngfuncs.GetMaxClients())
    {
        iPlayer2 = -1;
            if(GetEntity(index))
                iPlayer2 = GetEntity(index)->curstate.team;
    }
    if(idx > 0 && idx <= gEngfuncs.GetMaxClients())
    {
        iPlayer1 = -1;
            if(GetEntity(idx))
                iPlayer1 = GetEntity(idx)->curstate.team;
    }
    if ( iPlayer1 != iPlayer2 )
        EV_TFC_BloodDrips(vecOrigin, (signed int)flDamage, vecOrigin.z);
}

void EV_TFC_BloodDrips(float *vecOrigin, signed int iDamage, long double height)
{
    int modelIndex;
    int modelIndex2;
    float scale;

    if (gEngfuncs.pfnGetCvarFloat("violence_hblood") != 0.0 && iDamage > 0)
    {
        if (gEngfuncs.GetMaxClients() > 2)
            iDamage *= 2;
        if (iDamage > 255)
    {
        modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/bloodspray.spr");
        scale = 16.0;
        modelIndex2 = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/blood.spr");
    }
    else
    {
        modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/bloodspray.spr");
        modelIndex2 = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/blood.spr");
        if ( iDamage > 159 )
            scale = 16.0;
        else
        {
            scale = 3.0;
            if ( iDamage > 29 )
                scale = iDamage / 10;
        }
    }
    gEngfuncs.pEfxAPI->R_BloodSprite(vecOrigin, 47, modelIndex, modelIndex2, scale);
    }
}

enum tf_ac_e
{
    AC_IDLE = 0,
    AC_IDLE2,
    AC_SPINUP,
    AC_SPINDOWN,
    AC_FIRE,
    AC_DRAW,
    AC_HOLSTER
};

void EV_TFC_Assault_WindUp(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    g_flSpinDownTime[idx - 1] = 0;
    gEngfuncs.GetClientTime();
    g_flSpinUpTime[idx - 1] = 0.0 + 3.5;
    if (EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(AC_SPINUP, 2);
    g_bACSpinning[idx - 1] = 0;
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan2.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan4.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_WEAPON, "weapons/asscan3.wav");
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/asscan1.wav", 0.98, 0.8, 0, 125);
}

void EV_TFC_Assault_WindDown(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    if(g_flSpinUpTime[idx - 1] == 0.0)
    {
        gEngfuncs.GetClientTime();
        g_flSpinDownTime[idx - 1] = 0.0 + 3.0;
        if(EV_IsLocal(idx))
            gEngfuncs.pEventAPI->EV_WeaponAnimation(AC_SPINDOWN, 2);
    }
    else if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(AC_SPINDOWN, 2);

    g_bACSpinning[idx - 1] = 0;
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan2.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan4.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_WEAPON, "weapons/asscan1.wav");
    if(!args->bparam1)
        gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/asscan3.wav", 0.98, 0.8, 0, 125);
}

void EV_TFC_Assault_Start(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    g_flSpinDownTime[idx - 1] = 0;
    g_flSpinUpTime[idx - 1] = 0;
    g_bACSpinning[idx - 1] = 0;
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan2.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan4.wav");
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, "weapons/asscan3.wav", 0.98, 0.8, 0, 125);
}

void EV_TFC_Assault_Fire(event_args_t *args)
{
    int idx;
    int oddammo;
    int shell;
    vec3_t ShellOrigin, ShellVelocity;
    vec3_t up, right, forward;
    vec3_t vecSrc, vecAiming;
    vec3_t origin, angles, velocity;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    VectorCopy(args->angles, angles);
    VectorCopy(args->velocity, velocity);
    oddammo = args->bparam1;
    shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/shell.mdl");
    gEngfuncs.pfnAngleVectors(angles, forward, right, up);
    if(EV_IsLocal(idx))
    {
        EV_MuzzleFlash();
        gEngfuncs.pEventAPI->EV_WeaponAnimation(AC_FIRE, 2);
    }
    g_bACSpinning[idx - 1] = 0;
    if(oddammo)
    {
        EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 15.0, -25.0, 6.0);
        EV_EjectBrass(ShellOrigin, ShellVelocity, angles.y, shell, TE_BOUNCE_SHELL);
    }
    EV_GetGunPosition(args, vecSrc, origin);
    vecAiming = forward;
    vecSrc.x = right.x + right.x + up.x * -4.0 + vecSrc.x;
    vecSrc.y = right.y + right.y + up.y * -4.0 + vecSrc.y;
    vecSrc.z = right.z + right.z + -4.0 * up.z + vecSrc.z;
    EV_HLDM_FireBullets(idx, forward, right, up, 5, vecSrc, vecAiming, 0.1, 8192.0, BULLET_MONSTER_MP5, &tracerCount[idx - 1], 1.0, 8.0);
}

void EV_TFC_Assault_Spin(event_args_t *args)
{
    int idx;

    idx = args->entindex;
    if (EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(AC_FIRE, 2);
    g_bACSpinning[idx - 1] = 1;
}

void EV_TFC_Assault_StartSpin(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    g_bACSpinning[idx - 1] = 1;
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan2.wav");
    gEngfuncs.pEventAPI->EV_StopSound(idx, CHAN_STATIC, "weapons/asscan4.wav");
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, "weapons/asscan4.wav", 0.98, 0.8, 0, 125);
}

void EV_TFC_Gas(event_args_t *args)
{
  	vec3_t origin;
    VectorCopy(args->origin, origin);
    gEngfuncs.pEfxAPI->R_ParticleBurst(origin, 240, 195, 2.0);
}

void EV_TFC_DoorGoUp(event_args_t *args)
{
    int index;
    char sound[255];
    char num;

    index = args->iparam1;
    num = (char)index;

    sprintf(sound,"doors/doormove%d.wav", num);
    gEngfuncs.pEventAPI->EV_StopSound(-1, CHAN_STATIC, sound);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, args->origin, CHAN_STATIC, sound, 1.0, 1.0, 0, 100);
}

void EV_TFC_DoorGoDown(event_args_t *args)
{
    int index;
    char sound[255];
    char num;

    index = args->iparam1;
    num = (char)index;

    sprintf(sound,"doors/doormove%d.wav", num);
    gEngfuncs.pEventAPI->EV_StopSound(-1, CHAN_STATIC, sound);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, args->origin, CHAN_STATIC, sound, 1.0, 1.0, 0, 100);
}

void EV_TFC_DoorHitTop(event_args_t *args)
{
    int index;
    char sound[255];
    char num;

    index = args->iparam1;
    num = (char)index;

    sprintf(sound,"doors/doormove%d.wav", num);
    gEngfuncs.pEventAPI->EV_StopSound(-1, CHAN_STATIC, sound);
    sound[255];
    sprintf(sound,"doors/doorstop%d.wav", num);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, args->origin, CHAN_STATIC, sound, 1.0, 1.0, 0, 100);
}

void EV_TFC_DoorHitBottom(event_args_t *args)
{
    int index;
    char sound[255];
    char num;

    index = args->iparam1;
    num = (char)index;

    sprintf(sound,"doors/doormove%d.wav", num);
    gEngfuncs.pEventAPI->EV_StopSound(-1, CHAN_STATIC, sound);
    sound[255];
    sprintf(sound,"doors/doorstop%d.wav", num);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, args->origin, CHAN_STATIC, sound, 1.0, 1.0, 0, 100);
}

void EV_TFC_Explode(float *org, int dmg, pmtrace_t *pTrace, qboolean bDontSpark)
{
    int pc;
    bool outside;
    int explosion;
    float exp_scale;
    float veca, vecb;
    int fireball;
    int decalindex;
    physent_s *pe;
    const char *vecc;
    int explosion_outside;
    int damage;
    vec3_t origin;
    float fExplosionScale;

    VectorCopy(org, origin);

    if(pTrace->fraction != 1.0)
    {
        if(dmg <= 23)
            damage = dmg;
        else
            damage = dmg - 24;
        fireball = (float)damage * 0.6;
        VectorMA(pTrace->endpos, fireball, pTrace->plane.normal, origin);
    }
    pc = gEngfuncs.PM_PointContents(origin, 0);
    if (pc != CONTENTS_SOLID)
    {
        outside = pc != -3;
        explosion_outside = 0;
    }
    else
    {
        outside = 1;
        explosion_outside = 1;
    }
    fExplosionScale = (float)(dmg - 50) * 0.6;
    explosion = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/explode01.spr");
    exp_scale = 0.1 * fExplosionScale;
    vecb = exp_scale;
    gEngfuncs.pEfxAPI->R_Explosion(origin, explosion, vecb, 15, 3);
    if ( !explosion_outside )
    {
        veca = pTrace->ent;
        if(gEngfuncs.pfnRandomFloat(0, 1.0) >= 0.5)
        {
            pe = gEngfuncs.pEventAPI->EV_GetPhysent((int)veca);
            if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
                if(gEngfuncs.pfnGetCvarFloat("r_decals") != 0.0)
                    decalindex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName("{scorch2");
        }
        else
        {
            pe = gEngfuncs.pEventAPI->EV_GetPhysent((int)veca);
            if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
                if ( exp_scale != 0.0 )
                    decalindex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName("{scorch1");
        }
        gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(decalindex), gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace), 0, pTrace->endpos, 0);
    }
    long rand = gEngfuncs.pfnRandomLong(0, 2);
    if(rand == 1)
        vecc = "weapons/debris2.wav";
    if(rand == 2)
        vecc = "weapons/debris3.wav";
    if(!rand)
        vecc = "weapons/debris1.wav";
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_VOICE, vecc, 0.55, 0.8, 0, 100);

    if ( outside && !bDontSpark )
    {
        long rand2 = gEngfuncs.pfnRandomLong(0, 3);
        if ( rand2 > 0 )
            for(int x = 0; x < rand2; x++)
                gEngfuncs.pEfxAPI->R_SparkShower(origin);
    }
}

void EV_TFC_Explosion(event_args_t *args)
{
    pmtrace_t tr;
    vec3_t vecEnd;
    vec3_t vecSpot;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    EV_TFC_Explode(origin, 120, &tr, 0.0);
}

void EV_TFC_Grenade(event_args_t *args)
{
    pmtrace_t tr;
    vec3_t vecEnd;
    vec3_t vecSpot;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    EV_TFC_Explode(origin, 120, &tr, 0.0);
}

void EV_TFC_NormalGrenade(event_args_t *args)
{
    pmtrace_t tr;
    vec3_t vecEnd;
    vec3_t vecSpot;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    EV_TFC_Explode(origin, 180, &tr, 0.0);
}

enum tfc_tfcrpg_e
{
    TFCRPG_IDLE = 0,
    TFCRPG_FIDGET,
    TFCRPG_FIRE,
    TFCRPG_HOLSTER1,
    TFCRPG_DRAW1,
    TFCRPG_HOLSTER2,
    TFCRPG_DRAW2,
    TFCRPG_RELSTART,
    TFCRPG_RELCYCLE,
    TFCRPG_RELEND,
    TFCRPG_IDLE2,
    TFCRPG_FIDGET2
};

void EV_TFC_FireRPG(event_args_t *args)
{
    int idx;
    vec_t v2;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);

    if(EV_IsLocal(idx))
    gEngfuncs.pEventAPI->EV_WeaponAnimation(TFCRPG_FIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/rocketfire1.wav", 0.9, 0.8, 0, 100);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/glauncher.wav", 0.7, 0.8, 0, 100);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -5.0);
}

void EV_FireTFCSniper(event_args_t *args)
{
    int idx;
    physent_s *pe;
    pmtrace_t tr;
    int iDamage;

  	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;

	vec3_t vecSrc, vecDir, vecEnd;

    idx = args->entindex;
    iDamage = args->iparam1;
    VectorCopy(args->origin, origin);
	VectorCopy(args->angles, angles);
    AngleVectors(angles, forward, right, up);
    if (EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(SNIPER_FIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "ambience/rifle1.wav", 0.9, 0.8, 0, 100);
    EV_GetGunPosition(args, vecSrc, origin);
    vecDir = forward;
    VectorMA(vecSrc, 2.0, up, vecSrc);
    VectorMA(vecSrc, 8192.0, vecDir, vecEnd);
    gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, (pmtrace_s*)&up);
    gEngfuncs.pEventAPI->EV_PopPMStates();

    if ( tr.fraction != 1.0 )
    {
        if(cl_localblood->value != 0.0)
            EV_TFC_TraceAttack(idx, vecDir, &tr, iDamage);
        EV_HLDM_PlayTextureSound(idx, &tr, vecSrc, vecEnd, BULLET_PLAYER_357);
        pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
        if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
            EV_HLDM_DecalGunshot(&tr, BULLET_PLAYER_357);
    }
}

void EV_TFC_SniperHit(event_args_s *args)
{
    int idx;
    float volume;
    char *sound;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    idx = args->entindex;
    volume = (float)args->iparam1 / 100.0;
    if (args->bparam2)
    {
        sound = "common/bodysplat.wav";
        volume = 1.0;
    }
    else if (args->bparam1)
        sound = "weapons/bullet_hit2.wav";
    else
        sound = "weapons/bullet_hit1.wav";
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_STATIC, sound, volume, 0.8, 0, 100);
}

enum tf_ic_e
{
	IC_IDLE = 0,
	IC_FIDGET,
	IC_RELOAD,
	IC_FIRE,
	IC_HOLSTER,
	IC_DRAW,
	IC_HOLSTER2,
	IC_DRAW2,
	IC_IDLE2,
	IC_FIDGET2,
};

void EV_TFC_FireIC(event_args_t *args)
{
    int idx;
    vec_t v2;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);

    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(IC_FIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/sgun1.wav", 0.9, 0.8, 0, 100);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -5.0);
}

void EV_TFC_NailgrenadeNail(event_args_t *args)
{
    int nail;
    vec3_t origin, angles, forward, velocity, org;

    angles.x = 0;
    angles.z = 0;
    angles.y = (float)(args->iparam1 & 2047) * 0.25;
    VectorCopy(args->origin, org);
    nail = gEngfuncs.pEventAPI->EV_FindModelIndex("models/nail.mdl");
    angles.y = anglemod(args->fparam1 + angles.y);
    gEngfuncs.pfnAngleVectors(angles, forward, 0, 0);
    VectorMA(org, 12.0, forward, origin);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(origin, velocity, nail, 6, (args->iparam1 >> 11) & 31, EV_TFC_NailTouch);
    if(!gEngfuncs.pfnRandomLong(0, 5))
    {
        if (gEngfuncs.pfnRandomLong(0, 1) == 0)
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/airgun_1.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
        else
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/spike2.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
    }
    angles.y = anglemod(args->fparam1 + angles.y);
    gEngfuncs.pfnAngleVectors(angles, forward, 0, 0);
    VectorMA(org, 12.0, forward, origin);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(origin, velocity, nail, 6, (args->iparam1 >> 11) & 31, EV_TFC_NailTouch);
    if(!gEngfuncs.pfnRandomLong(0, 5))
    {
        if (gEngfuncs.pfnRandomLong(0, 1) == 0)
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/airgun_1.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
        else
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/spike2.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
    }
    angles.y = anglemod(args->fparam1 + angles.y);
    gEngfuncs.pfnAngleVectors(angles, forward, 0, 0);
    VectorMA(org, 12.0, forward, origin);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(origin, velocity, nail, 6, (args->iparam1 >> 11) & 31, EV_TFC_NailTouch);
    if(!gEngfuncs.pfnRandomLong(0, 5))
    {
        if (gEngfuncs.pfnRandomLong(0, 1) == 0)
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/airgun_1.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
        else
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/spike2.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
    }
    angles.y = anglemod(args->fparam1 + angles.y);
    gEngfuncs.pfnAngleVectors(angles, forward, 0, 0);
    VectorMA(org, 12.0, forward, origin);
    VectorScale(forward, 1000.0, velocity);
    gEngfuncs.pEfxAPI->R_Projectile(origin, velocity, nail, 6, (args->iparam1 >> 11) & 31, EV_TFC_NailTouch);
    if(!gEngfuncs.pfnRandomLong(0, 5))
    {
        if (gEngfuncs.pfnRandomLong(0, 1) == 0)
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/airgun_1.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
        else
            gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/spike2.wav", 1, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
    }
}

enum tf_gl_e
{
    GL_IDLE = 0,
    PL_IDLE,
    GL_FIRE,
    PL_FIRE,
    GL_RELOAD1,
    GL_RELOAD2,
    PL_RELOAD1,
    PL_RELOAD2,
    GL_DRAW,
    PL_DRAW,
    GL_HOLSTER,
    PL_HOLSTER
};

void EV_TFC_GrenadeLauncher(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(GL_FIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/glauncher.wav", 0.7, 0.8, 0, 100);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -2.0);
}

void EV_TFC_PipeLauncher(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(PL_FIRE, 2);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_WEAPON, "weapons/glauncher.wav", 0.7, 0.8, 0, 100);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -2.0);
}

void EV_TFC_NormalShot(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_ITEM, "items/medshot4.wav", 1, 0.8, 0, 100);
}


void EV_TFC_SuperShot(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_ITEM, "items/medshot5.wav", 1, 0.8, 0, 100);
}

void EV_TFC_SteamShot(event_args_t *args)
{
    int idx;
    vec3_t origin;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_ITEM, "ambience/steamburst1.wav", 1, 0.8, 0, 100);
}

void EV_TFC_AxeDecal( event_args_t *args )
{
    int idx;
    pmtrace_t *tr;
    physent_s *pe;

    idx = args->entindex;
    tr = gp_tr_decal[idx-1];

    if ( tr )
    {
        pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr->ent );
        if ( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
            EV_HLDM_DecalGunshot( tr, BULLET_PLAYER_CROWBAR );
        tr = 0;
    }
}

void EV_TFC_EngineerGrenade(event_args_t *args)
{
    pmtrace_t tr;
    vec3_t vecEnd;
    vec3_t vecSpot;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    EV_TFC_Explode(origin, 180, &tr, 1.0);
}

void EV_TFC_Concussion(event_args_t *args)
{
    int wave;
    long rand;
    const char *sound;
    pmtrace_t tr;
    vec3_t vecEnd, vecSpot, origin;

    VectorCopy(args->origin, origin)
    wave = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/shockwave.spr");
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    rand = gEngfuncs.pfnRandomLong(0, 2);
    if(rand == 1)
        sound = "weapons/concgren_blast2.wav";
    if(rand == 2)
        sound = "weapons/concgren_blast3.wav";
    if (rand = 0)
        sound = "weapons/concgren_blast1.wav";
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_VOICE, sound, 1, 0.8, 0, 100);
    vecSpot.z = origin.z + 16.0;
    vecEnd.z = origin.z + 16.0 + 600.0;
    gEngfuncs.pEfxAPI->R_BeamCirclePoints(21, vecSpot, vecEnd, wave, 0.2, 70, 0, 1.0, 0, 0, 0, 1.0, 1.0, 1.0);
}

void EV_TFC_MirvGrenadeMain(event_args_t *args)
{
    EV_TFC_NormalGrenade(args);
}

void EV_TFC_MirvGrenade(event_args_t *args)
{
    EV_TFC_NormalGrenade(args);
}

void EV_TFC_NapalmFire(event_args_t *args)
{
    int explosion;
    physent_s *pe;
    int decalindex;
    int decal;
    const char *sound;
    pmtrace_t tr;
    vec3_t origin, vecSpot, vecEnd;
    vec3_t org;

    VectorCopy(args->origin, origin)
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    if(tr.fraction != 1.0)
        VectorMA(tr.endpos, 12.0, tr.plane.normal, org);
    if(gEngfuncs.PM_PointContents(org, 0) == CONTENTS_SOLID && gEngfuncs.PM_PointContents(origin, 0) == CONTENTS_SOLID)
        return;
    explosion = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/explode01.spr");
    gEngfuncs.pEfxAPI->R_Explosion(org, explosion, 3, 15, 3);
    if(gEngfuncs.pfnRandomFloat(0, 1.0) < 0.5)
    {
        pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
        if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
            if(gEngfuncs.pfnGetCvarFloat("r_decals") != 0.0)
                decalindex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName("{scorch1");
    }
    else
    {
        pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
        if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
            if(gEngfuncs.pfnGetCvarFloat("r_decals") != 0.0)
                decalindex = gEngfuncs.pEfxAPI->Draw_DecalIndexFromName("{scorch2");
    }
    if(decalindex != 0)
    {
        decal = gEngfuncs.pEfxAPI->Draw_DecalIndex(decalindex);
        gEngfuncs.pEfxAPI->R_DecalShoot(decal, gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), 0, tr.endpos, 0);
    }

    switch(gEngfuncs.pfnRandomLong(0, 2))
    {
        case 0:
            sound = "weapons/debris1.wav";
            break;
        case 1:
            sound = "weapons/debris2.wav";
            break;
        case 2:
            sound = "weapons/debris3.wav";
            break;
    }
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_VOICE, sound, 0.55, 0.8, 0, 100);
}

void EV_TFC_NapalmBurn(event_args_t *args)
{
    int sprite;
    vec3_t origin;

    VectorCopy(args->origin, origin);
    sprite = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/playerflame.spr");
    gEngfuncs.pEfxAPI->R_FireField(origin, 100, sprite, 12, 18, 0.8);
}

void EV_TFC_EMP(event_args_t *args)
{
    int sprite;
    pmtrace_t tr;
    vec3_t origin, vecSpot, vecEnd;

    sprite = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/shockwave.spr");
    VectorCopy(args->origin, origin)
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    gEngfuncs.pEventAPI->EV_PlaySound(-1, origin, CHAN_VOICE, "weapons/emp_1.wav", 1.0, 0.8, 0, 100);
    vecSpot.z = origin.z + 16.0;
    vecEnd.z = origin.z + 16.0 + 600.0;
    gEngfuncs.pEfxAPI->R_BeamCirclePoints(21, vecSpot, vecEnd, sprite, 0.2, 70, 0, 1.0, 0, 0, 0, 1.0, 1.0, 0.0);
}

enum tf_ft_e
{
	FT_IDLE = 0,
	FT_FIDGET,
	FT_ALTON,
	FT_ALTCYCLE,
	FT_ALTOFF,
	FT_FIRE1,
	FR_FIRE2,
	FT_FIRE3,
	FT_FIRE4,
	FT_DRAW,
	FT_HOLSTER
};

void EV_TFC_Flame_Fire(event_args_t *args)
{
    int idx;
    float height;
    long double height2;
    int underwater;
    int shell;
    int bubble;
    vec3_t ShellOrigin, BubbleSpot, up, right, forward, vecVelocity, origin, angles;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    VectorCopy(args->angles, angles);
    underwater = args->bparam1;
    shell = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fthrow.spr");
    bubble = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/bubble.spr");
    gEngfuncs.pfnAngleVectors(angles, forward, right, up);
    if(EV_IsLocal(idx))
        gEngfuncs.pEventAPI->EV_WeaponAnimation(FT_FIRE1, 2);
    EV_GetGunPosition(args, ShellOrigin, origin);
    VectorMA(ShellOrigin, -8.0, up, ShellOrigin);
    VectorMA(ShellOrigin, 8.0, right, ShellOrigin);
    VectorMA(ShellOrigin, 16.0, forward, ShellOrigin);
    if (!underwater)
    {
        VectorScale(forward, 600.0, vecVelocity);
        gEngfuncs.pEfxAPI->R_Projectile(ShellOrigin, vecVelocity, shell, 1, idx, 0);
        gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/flmfire2.wav", 0.9, 0.8, 0, 100);
        return;
    }
    VectorMA(ShellOrigin, 128.0, forward, BubbleSpot);
    height = EV_TFC_WaterLevel(ShellOrigin, ShellOrigin.z, ShellOrigin.z + 256.0);
    if(height >= 8.0)
    {
        gEngfuncs.pEfxAPI->R_BubbleTrail(ShellOrigin, BubbleSpot, EV_TFC_WaterLevel(ShellOrigin, ShellOrigin.z, ShellOrigin.z + 256.0) - ShellOrigin.z, bubble, 4, 8.0);
        return;
    }
    height2 = EV_TFC_WaterLevel(BubbleSpot, BubbleSpot.z, BubbleSpot.z + 256.0) - BubbleSpot.z;
    if (height2 >= 8.0)
    {
        height = height2 + BubbleSpot.z - ShellOrigin.z;
        gEngfuncs.pEfxAPI->R_BubbleTrail(ShellOrigin, BubbleSpot, EV_TFC_WaterLevel(ShellOrigin, ShellOrigin.z, ShellOrigin.z + 256.0) - ShellOrigin.z, bubble, 4, 8.0);
        return;
    }
}

long double EV_TFC_WaterLevel(float *position, float minz, float maxz)
{
    long double result;
    long double dif;
    long double dif2;
    float diff;
    float minz_1;

    minz_1 = minz;
    result = minz;
    if(gEngfuncs.PM_PointContents(position, 0) == -3 )
    {
        minz_1 = maxz;
        if(gEngfuncs.PM_PointContents(position, 0) == -3 )
            result = maxz;
    }
    else
    {
        diff = minz;
        dif = maxz - minz;
        if(dif > 1.0)
        {
            while(1)
            {
                minz_1 = dif * 0.5 + diff;
                if(gEngfuncs.PM_PointContents(position, 0) == -3)
                {
                    result = minz_1;
                    diff = minz_1;
                }
                else
                {
                    result = minz_1;
                    maxz = minz_1;
                }
                dif2 = maxz - diff;
                if (dif2 <= 1.0)
                    break;
                dif = dif2;
            }
        }
        else
            result = minz_1;
    }
  return result;
}

void EV_TFC_Railgun(event_args_t *args)
{
    int idx;
    pmtrace_t tr;
    vec3_t ShellOrigin, vecSrc, vecEnd, tracerVelocity, right, up, forward, delta, origin, angles;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    VectorCopy(args->angles, angles);
    gEngfuncs.pfnAngleVectors(angles, forward, right, up);
    gEngfuncs.pEventAPI->EV_FindModelIndex("models/nail.mdl");
    if(EV_IsLocal(idx))
    {
        EV_MuzzleFlash();
        gEngfuncs.pEventAPI->EV_WeaponAnimation(1, 2);
    }
    EV_GetGunPosition(args, ShellOrigin, origin);
    VectorMA(ShellOrigin, -4.0, up, ShellOrigin);
    VectorMA(ShellOrigin, 2.0, right, ShellOrigin);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/railgun.wav", 0.9, 0.8, 0, 100);
    vecSrc = ShellOrigin;
    VectorMA(vecSrc, 4096.0, forward, vecEnd);
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetSolidPlayers(idx - 1);
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);
    delta.x = tr.endpos.x - vecSrc.x;
    delta.y = tr.endpos.y - vecSrc.y;
    delta.z = tr.endpos.z - vecSrc.z;
    VectorScale(forward, 1500.0, tracerVelocity);
    gEngfuncs.pEfxAPI->R_UserTracerParticle(ShellOrigin, tracerVelocity, Length(delta) / 1500.0, 2, 1.0, idx, EV_TFC_RailDie);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -2.0);
}

void EV_TFC_RailDie(particle_s *particle)
{
    physent_s *pe;
    int idx2;
    pmtrace_t tr;
    char decalname[32];
    vec3_t forward, back, vecSrc, vecEnd;

    if (particle)
    {
        forward.x = particle->vel.x;
        forward.y = particle->vel.y;
        forward.z = particle->vel.z;
        VectorNormalize(forward);
        back = forward;
        VectorInverse(back);
        VectorMA(particle->org, 128.0, back, vecSrc);
        VectorMA(vecSrc, 256.0, forward, vecEnd);
        gEngfuncs.pEventAPI->EV_PushPMStates();
        gEngfuncs.pEventAPI->EV_SetSolidPlayers(particle->context - 1);
        gEngfuncs.pEventAPI->EV_SetTraceHull(2);
        gEngfuncs.pEventAPI->EV_PlayerTrace(vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr);

        if(tr.ent != -1)
        {
            pe = gEngfuncs.pEventAPI->EV_GetPhysent(tr.ent);
            idx2 = gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr);
            if(!EV_IsPlayer(idx2) || EV_TFC_IsAlly(gEngfuncs.pEventAPI->EV_IndexFromTrace(&tr), particle->context))
            {
                if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
                {
                    decalname[0] = 0;
                    sprintf(decalname, "{shot%i", gEngfuncs.pfnRandomLong(0, 4) + 1);
                    if(decalname[0])
                    {
                        if(tr.fraction != 1.0)
                            EV_HLDM_GunshotDecalTrace(&tr, decalname);
                    }
                }
            }
            else if (cl_localblood->value != 0.0)
            {
                EV_TFC_BloodDrips(tr.endpos, 25, 0.0);
            }
        }
        gEngfuncs.pEventAPI->EV_PopPMStates();
    }
}

int EV_TFC_IsAlly(int idx1, int idx2)
{
    bool err;
    signed int iPlayer1;
    signed int iPlayer2;
    int result;
    long v10;
    if (idx1 > 0 && idx1 <= gEngfuncs.GetMaxClients() && GetEntity(idx1) != 0 )
    {
        iPlayer1 = GetEntity(idx1)->curstate.team;
        err = iPlayer1 == -1;
    }
    else
    {
        err = 1;
        iPlayer1 = -1;
    }
    if (idx1 < 0 || idx1 > gEngfuncs.GetMaxClients() || GetEntity(idx1) == 0 )
        return 0;
    iPlayer2 = GetEntity(idx2)->curstate.team;
    if(iPlayer2 == -1 || err || !iPlayer2 || !iPlayer1)
        return 0;
    result = 0;
    if (!gEngfuncs.GetLocalPlayer())
        return result;
    if(iPlayer2 > 4 || iPlayer2 > 4)
        return 0;
    v10 = strtol(gEngfuncs.PhysInfo_ValueForKey("ta"), 0, 10);
    result = 1;
    if (!(((v10 & 0x1F) + iPlayer1) >> (iPlayer2 - 1) & 1));
        result = iPlayer1 == iPlayer2;
    return result;
}

enum tf_tranq_e
{
    TRANQ_IDLE1 = 0,
    TRANQ_IDLE2,
    TRANQ_IDLE3,
    TRANQ_SHOOT,
    TRANQ_SHOOT_EMPTY,
    TRANQ_RELOAD,
    TRANQ_DRAW,
    TRANQ_HOLSTER,
    TRANQ_ADD_SILENCER,
};

void EV_TFC_Tranquilizer(event_args_t *args)
{
    int idx;
    int shell;
    vec3_t ShellOrigin, vecVelocity, right, up, forward, origin, angles;

    idx = args->entindex;
    VectorCopy(args->origin, origin);
    VectorCopy(args->angles, angles);
    gEngfuncs.pfnAngleVectors(angles, forward, right, up);
    shell = gEngfuncs.pEventAPI->EV_FindModelIndex("models/nail.mdl");
    if(EV_IsLocal(idx))
    {
        EV_MuzzleFlash();
        gEngfuncs.pEventAPI->EV_WeaponAnimation(TRANQ_SHOOT, 1);
    }
    EV_GetGunPosition(args, ShellOrigin, origin);
    VectorMA(ShellOrigin, -4.0, up, ShellOrigin);
    VectorMA(ShellOrigin, 2.0, right, ShellOrigin);
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/dartgun.wav", 0.9, 0.8, 0, 100);
    VectorScale(forward, 1500.0, vecVelocity);
    gEngfuncs.pEfxAPI->R_Projectile(ShellOrigin, vecVelocity, shell, 6, idx, EV_TFC_TranqNailTouch);
    if(EV_IsLocal(idx))
        V_PunchAxis(0, -2.0);
}

void EV_TFC_TranqNailTouch(tempent_s *ent, pmtrace_t *ptr)
{
    physent_s *pe;
    char decalname[32];

    if(gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr) > 0 && gEngfuncs.pEventAPI->EV_IndexFromTrace(ptr) <= gEngfuncs.GetMaxClients() )
    {
        decalname[0] = 0;
        sprintf(decalname, "{shot%i", gEngfuncs.pfnRandomLong(0, 4) + 1);
    }
    else
    {
        decalname[0] = 0;
        sprintf(decalname, "{shot%i", gEngfuncs.pfnRandomLong(0, 4) + 1);
        pe = gEngfuncs.pEventAPI->EV_GetPhysent(ptr->ent);
        if(pe && (pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP))
        {
            if (decalname[0])
            {
                if(ptr->fraction != 1.0)
                    EV_HLDM_GunshotDecalTrace(ptr, decalname);
            }
        }
    }
}

void EV_TFC_NailGrenade(event_args_t *args)
{
    pmtrace_t tr;
    vec3_t vecEnd, vecSpot, origin;

    VectorCopy(args->origin, origin)
    vecSpot.x = origin.x;
    vecSpot.y = origin.y;
    vecSpot.z = origin.z + 8.0;
    vecEnd.x = origin.x;
    vecEnd.y = origin.y;
    vecEnd.z = origin.z + 8.0 - 40.0;
    gEngfuncs.pEventAPI->EV_PushPMStates();
    gEngfuncs.pEventAPI->EV_SetTraceHull(2);
    gEngfuncs.pEventAPI->EV_PlayerTrace(vecSpot, vecEnd, PM_STUDIO_BOX, -1, &tr);
    gEngfuncs.pEventAPI->EV_PopPMStates();
    EV_TFC_Explode(origin, 180, &tr, 78);
}

void EV_TFC_GibCallback(tempent_s *ent, float frametime)
{
    float vel = Length(ent->entity.baseline.origin);
    if(ent->entity.curstate.playerclass == 2)
    {
        ent->entity.curstate.playerclass = 3;
        gEngfuncs.GetClientTime();
        ent->entity.curstate.renderamt = 255;
        ent->entity.curstate.rendermode = kRenderFxPulseFast;
        ent->die = vel + 2.5;
    }
}

//Velaron: unfinished code
tempent_s* EV_TFC_CreateGib(float *origin, float *attackdir, int multiplier, int ishead)
{
    int modelindex;
    tempent_s* ent;
    float vmultiple;
    float mins0[3];
    float maxs0[3];
    //TEMPENTITY tmpent;

    model_s* gib = gEngfuncs.CL_LoadModel("models/hgibs.mdl", &modelindex);

    if(!gib)
        return 0;

    if(multiplier < 0)
        vmultiple = 0.7 * cl_gibvelscale->value;
    else
        vmultiple = multiplier * cl_gibvelscale->value;

    gEngfuncs.pEventAPI->EV_LocalPlayerBounds(0, mins0, maxs0);
    //ent = gEngfuncs.pEfxAPI->CL_TentEntAllocCustom(origin, gib, 0, EV_TFC_GibCallback);

    if(ent)
    {
        if(ishead)
            ent->entity.curstate.body = 0;
        else
            ent->entity.curstate.body = gEngfuncs.pfnRandomLong(1, 5);

        if(ishead)
        {

        }
    }
}

void EV_TFC_Gibs(event_args_t *args)
{
    int idx;
    int multiplier;
    vec3_t origin, attackdir;
    int gibcount;

    idx = args->entindex;
    VectorCopy(args->origin, origin)
    VectorCopy(args->angles, attackdir)
    multiplier = args->iparam1;
    gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "common/bodysplat.wav", 1.0, 0.8, 0, 100);
    gibcount = cl_gibcount->value;
    if(gibcount > 64)
        gibcount = 64;
    for(int x = 0; x < gibcount; x++)
        EV_TFC_CreateGib(origin, attackdir, multiplier, 0);
    EV_TFC_CreateGib(origin, attackdir, multiplier, 1);
}

void EV_TFC_PlayAxeSound(int idx, int classid, float *origin, int iSoundType, float fSoundData)
{
    if(classid == 2)
        return;

    switch(iSoundType)
    {
    case 1:
        {
            switch(gEngfuncs.pfnRandomLong(0, 2))
            {
            case 1:
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod2.wav", 1.0, 0.8, 0, 100);
                break;
            case 2:
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod3.wav", 1.0, 0.8, 0, 100);
                break;
            default:
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, 0.8, 0, 100);
                break;
            }
        }
        break;
    case 2:
        {
            switch(gEngfuncs.pfnRandomLong(0, 1))
            {
            case 1:
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_hit2", fSoundData, 0.8, 0, gEngfuncs.pfnRandomLong(0, 3) + 98);
                break;
            default:
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_hit1", fSoundData, 0.8, 0, gEngfuncs.pfnRandomLong(0, 3) + 98);
                break;
            }
        }
        break;
    default:
        gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, CHAN_WEAPON, "weapons/cbar_miss1.wav", 1.0, 0.8, 0, gEngfuncs.pfnRandomLong(0, 15) + 94);
        break;
    }
}

int EV_TFC_AxeHit(int idx, float *origin, float *forward, float *right, int entity, float *vecDir, pmtrace_t *ptr)
{
    cl_entity_s *ent;
    Vector tf_0, tu, tr;

    if(entity > 0 && entity <= gEngfuncs.GetMaxClients())
        ent = GetEntity(entity - 1);

    if (entity > 0
    && entity <= gEngfuncs.GetMaxClients()
    && ent != 0)
    {
        AngleVectors(ent->curstate.angles, tf_0, tu, tr);
        if (tr.x * forward[1] - *forward * tr.y <= 0.0)
        {
            if (cl_localblood->value != 0.0)
                EV_TFC_TraceAttack(idx, vecDir, ptr, 40.0);
            if (EV_IsLocal(idx))
                gEngfuncs.pEventAPI->EV_WeaponAnimation(2, 2);
        }
        else
        {
            if (cl_localblood->value != 0.0)
                EV_TFC_TraceAttack(idx, vecDir, ptr, 120.0);
            if (EV_IsLocal(idx))
                gEngfuncs.pEventAPI->EV_WeaponAnimation(3, 2);
        }
    }
    else if (cl_localblood->value != 0.0)
        EV_TFC_TraceAttack(idx, vecDir, ptr, 20.0);
    return 1;
}

int EV_TFC_Medkit(int idx, float *origin, float *forward, float *right, int entity, float *vecDir, pmtrace_t *ptr)
{
    cl_entity_s *ent;

    if (entity > 0 && entity <= gEngfuncs.GetMaxClients())
    {
        ent = GetEntity(entity - 1);
        if (!EV_TFC_IsAlly(idx, entity))
        {
            if (cl_localblood->value != 0.0)
                EV_TFC_TraceAttack(idx, vecDir, ptr, 10.0);
            if (ent->curstate.playerclass != 5)
                gEngfuncs.pEventAPI->EV_PlaySound(idx, origin, 2, "player/death2.wav", 1.0, 0.8, 0, 100);
        }
    }
  return 1;
}

void ClearEventList()
{
}

void RunEventList()
{
}
