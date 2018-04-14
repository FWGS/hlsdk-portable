//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

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

// QUAKECLASSIC
#define Q_SMALL_PUNCHANGLE_KICK		-2
#define Q_BIG_PUNCHANGLE_KICK		-4

extern "C" char PM_FindTextureType( char *name );

extern void EV_Quake_DecalTrace( pmtrace_t *pTrace, const char *decalName );
extern void V_PunchAxis( int axis, float punch );
extern vec3_t v_origin;

extern "C"
{

// HLDM
void EV_FireShotGunSingle( struct event_args_s *args );
void EV_FireShotGunDouble( struct event_args_s *args );
void EV_FireAxe( struct event_args_s *args );
void EV_FireAxeSwing( struct event_args_s *args );
void EV_FireRocket( struct event_args_s *args );
void EV_FireLightning( struct event_args_s *args );
void EV_FireSpike( struct event_args_s *args );
void EV_FireSuperSpike( struct event_args_s *args );
void EV_FireGrenade( struct event_args_s *args );
void EV_Gibbed( event_args_t *args );
void EV_Teleport( event_args_t *args );
void EV_Trail( event_args_t *args );
void EV_Explosion( event_args_t *args );

void EV_PlayerPowerup( struct event_args_s *args );

void EV_DMC_DoorGoUp( struct event_args_s *args );
void EV_DMC_DoorGoDown( struct event_args_s *args );
void EV_DMC_DoorHitTop( struct event_args_s *args );
void EV_DMC_DoorHitBottom( struct event_args_s *args );

void EV_Hook( event_args_t *args );
void EV_Cable( struct event_args_s *args );
void EV_FollowCarrier( struct event_args_s *args );
void EV_FlagSpawn( struct event_args_s *args );

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
	const char *rgsz[4];
	int cnt;
	float fattn = ATTN_NORM;
	int entity;
	const char *pTextureName;
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
		pTextureName = gEngfuncs.pEventAPI->EV_TraceTexture( ptr->ent, vecSrc, vecEnd );
		
		if( pTextureName )
		{
			strcpy( texname, pTextureName );
			pTextureName = texname;

			// strip leading '-0' or '+0~' or '{' or '!'
			if(*pTextureName == '-' || *pTextureName == '+')
			{
				pTextureName += 2;
			}

			if(*pTextureName == '{' || *pTextureName == '!' || *pTextureName == '~' || *pTextureName == ' ')
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
	
	switch( chTextureType )
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
		if (iBulletType == BULLET_PLAYER_CROWBAR)
			return 0.0; // crowbar already makes this sound
		fvol = 1.0;	fvolbar = 0.2;
		rgsz[0] = "weapons/bullet_hit1.wav";
		rgsz[1] = "weapons/bullet_hit2.wav";
		fattn = 1.0;
		cnt = 2;
		break;
	}

	// play material hit sound
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, rgsz[gEngfuncs.pfnRandomLong( 0, cnt-1 )], fvol, fattn, 0, 96 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	return fvolbar;
}

//CheckPVS see if playerIndex is in same PVS as localplayer
bool CheckPVS( int playerIndex )
{
	//returns true if the player is in the same PVS
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
	cl_entity_t *player = gEngfuncs.GetEntityByIndex( playerIndex );

	if( !player || !localPlayer )
		return false;

	if( player == localPlayer )
		return true;

	if( player->curstate.messagenum < localPlayer->curstate.messagenum )
		return false;

	return true;
}

const char *EV_HLDM_RocketDamageDecal()
{
	const char *pszDecalName;
	int idx;

	idx = gEngfuncs.pfnRandomLong( 0, 2 );
	switch( idx + 1 )
	{
	case 1:
		pszDecalName = "{scorch1";
		break;
	case 2:
		pszDecalName = "{scorch2";
		break;
	case 3:
		pszDecalName = "{scorch3";
		break;
	}

	return pszDecalName;
}

const char *EV_HLDM_DamageDecal()
{
	const char *pszDecalName;
	int idx = gEngfuncs.pfnRandomLong( 0, 4 );

	switch( idx + 1 )
	{
	case 1:
		pszDecalName = "{shot1";
		break;
	case 2:
		pszDecalName = "{shot2";
		break;
	case 3:
		pszDecalName = "{shot3";
		break;
	case 4:
		pszDecalName = "{shot4";
		break;
	case 5:
		pszDecalName = "{shot5";
		break;
	}
	return pszDecalName;
}


const char *EV_Lightning_DamageDecal( void )
{
	const char *pszDecalName;
	int idx;

	idx = gEngfuncs.pfnRandomLong( 0, 2 );
	switch( idx + 1 )
	{
	case 1:
		pszDecalName = "{smscorch1";
		break;
	case 2:
		pszDecalName = "{smscorch2";
		break;
	case 3:
		pszDecalName = "{smscorch3";
		break;
	}

	return pszDecalName;
}

const char *EV_Quake_DamageDecalClub()
{
	return EV_HLDM_DamageDecal();
}

void EV_Quake_GunshotDecalTrace( pmtrace_t *pTrace, const char *decalName )
{
	int iRand;

	gEngfuncs.pEfxAPI->R_BulletImpactParticles( pTrace->endpos );

	iRand = gEngfuncs.pfnRandomLong( 0, 0x7FFF );
	if( iRand < ( 0x7fff / 2 ) )// not every bullet makes a sound.
	{
		const char *pszSound;

		switch( iRand % 5 )
		{
		case 0:
			pszSound = "weapons/ric1.wav";
			break;
		case 1:
			pszSound = "weapons/ric2.wav";
			break;
		case 2:
			pszSound = "weapons/ric3.wav";
			break;
		case 3:
			pszSound = "weapons/ric4.wav";
			break;
		case 4:
			pszSound = "weapons/ric5.wav";
			break;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( -1, pTrace->endpos, 0, pszSound, 1.0, ATTN_NORM, 0, PITCH_NORM );
	}

	EV_Quake_DecalTrace( pTrace, decalName );	
}

void EV_Quake_DecalTrace( pmtrace_t *pTrace, const char *decalName )
{
	physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if( decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
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
	// physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// if( pe && pe->solid == SOLID_BSP )
	{
		switch( iBulletType )
		{
		case BULLET_PLAYER_CROWBAR:
			EV_Quake_DecalTrace( pTrace, EV_Quake_DamageDecalClub() );
			break;
		case BULLET_PLAYER_9MM:
		case BULLET_MONSTER_9MM:
		case BULLET_PLAYER_MP5:
		case BULLET_MONSTER_MP5:
		case BULLET_PLAYER_BUCKSHOT:
		case BULLET_PLAYER_357:
		default:
			// smoke and decal
			EV_Quake_GunshotDecalTrace( pTrace, EV_HLDM_DamageDecal() );
			break;
		}
	}
}

void EV_Quake_PlayQuadSound ( int idx, float *origin, int iFlag )
{
	if( iFlag == 1 )
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_ITEM, "items/damage3.wav", 1, ATTN_NORM, 0, PITCH_NORM);
}

/*
================
FireBullets

Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void EV_Quake_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float *vecSpread )
{
	int i;
	pmtrace_t tr;
	int iShot;
	vec3_t vecRight, vecUp;

	VectorCopy( right, vecRight );
	VectorCopy( up, vecUp );

	for( iShot = 1; iShot <= cShots; iShot++ )
	{
		vec3_t vecDir, vecEnd;

		// get circular gaussian spread
		vec3_t spread;
		do
		{
			spread[0] = gEngfuncs.pfnRandomFloat( -1.0, 1.0 ) + gEngfuncs.pfnRandomFloat( -1.0, 1.0 );
			spread[1] = gEngfuncs.pfnRandomFloat( -1.0, 1.0 ) + gEngfuncs.pfnRandomFloat( -1.0, 1.0 );
			spread[2] = spread[0] * spread[0] + spread[1] *spread[1];
		}while( spread[2] > 1 );

		for( i = 0; i < 3; i++ )
		{
			vecDir[i] = vecDirShooting[i] + spread[0] * vecSpread[0] * vecRight[i] + spread[1] * vecSpread[1] * up [i];
			vecEnd[i] = vecSrc[i] + 2048.0 * vecDir[i];
		}

		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	
		// Store off the old count
		gEngfuncs.pEventAPI->EV_PushPMStates();
	
		// Now add in all of the players.
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	

		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

		int iBulletType = BULLET_PLAYER_BUCKSHOT;

		// do damage, paint decals
		if( tr.fraction != 1.0 )
		{
			switch( iBulletType )
			{
			default:
			case BULLET_PLAYER_9MM:		
			case BULLET_PLAYER_357:
			case BULLET_PLAYER_MP5:		
				EV_HLDM_PlayTextureSound( idx, &tr, vecSrc, vecEnd, iBulletType );
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			case BULLET_PLAYER_BUCKSHOT:
				EV_HLDM_DecalGunshot( &tr, iBulletType );
				break;
			}
		}

		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
}

void EV_FireShotGunDouble( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );

	AngleVectors( angles, forward, right, up );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/shotgn2.wav", 1.0, ATTN_NORM, 0, 100 );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	vecSpread[0] = 0.04;
	vecSpread[1] = 0.04;
	vecSpread[2] = 0.00;

	EV_Quake_FireBullets( idx, forward, right, up, 14, vecSrc, vecAiming, vecSpread );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_BIG_PUNCHANGLE_KICK );
	}
}

void EV_FireShotGunSingle( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	
	int i;
	vec3_t vecSrc, vecAiming;
	vec3_t vecSpread;
	vec3_t up, right, forward;
	float flSpread = 0.01;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	VectorCopy( args->velocity, velocity );
	int empty = args->bparam1;

	AngleVectors( angles, forward, right, up );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/guncock.wav", 1.0, ATTN_NORM, 0, 100 );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );

	vecSpread[0] = 0.04;
	vecSpread[1] = 0.04;
	vecSpread[2] = 0.00;

	EV_Quake_FireBullets( idx, forward, right, up, 6, vecSrc, vecAiming, vecSpread );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );
	}
}

enum soundtypes_e
{
	SOUND_MISS,
	SOUND_HIT_BODY,
	SOUND_HIT_WALL
};

void EV_Quake_PlayAxeSound( int idx, float *origin, int iSoundType )
{
	const char *pszSound;

	switch( iSoundType )
	{
	case SOUND_HIT_BODY:
		pszSound = "player/axhitbod.wav";
		break;
	case SOUND_HIT_WALL:
		pszSound = "player/axhit2.wav"; 
		break;
	default:
		return;
	}
	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, pszSound, 1, ATTN_NORM, 0, PITCH_NORM );
}

void EV_FireAxe( event_args_t *args )
{
	int ent;

	int fDidHit = 0;
	int m_bHullHit = 1;

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

	VectorMA( vecSrc, 64, forward, vecEnd );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( idx - 1 );	

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_NORMAL, -1, &tr );

	if( tr.fraction < 1.0 )
	{
		ent = gEngfuncs.pEventAPI->EV_IndexFromTrace( &tr );

		if( !EV_IsPlayer( ent ) )
		{
			EV_Quake_PlayAxeSound( idx, origin, SOUND_HIT_WALL );
			EV_HLDM_DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );
		}

		EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );
	}

	gEngfuncs.pEventAPI->EV_PopPMStates();
}

void EV_FireAxeSwing( event_args_t *args )
{
	int idx;
	vec3_t origin;
	idx = args->entindex;
	VectorCopy( args->origin, origin );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/ax1.wav", 1.0, ATTN_NORM, 0, 100 );
}

void EV_Hook( event_args_t *args  )
{
	return;
}

void EV_Cable( event_args_t *args )
{
	int idx, attached, team, modelIndex;

	float r, g, b;

	idx = args->entindex;
	attached = args->iparam1;
	team = args->iparam2;

	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/smoke.spr" );

	if( !modelIndex )
		return;

	if( team == 1 )
	{
		r = 500;
		g = 0;
		b = 0;
	}
	else if( team == 2 )
	{
		r = 0;
		g = 0;
		b = 500;
	}

	if( args->bparam1 == 1 )
		gEngfuncs.pEfxAPI->R_BeamKill( attached );
	else
		gEngfuncs.pEfxAPI->R_BeamEnts( idx, attached, modelIndex, 9999, 1, 0.001, 0.8, 0.0, 0.0, 0.0, r, g, b );
}

void EV_GenericParticleCallback( struct particle_s *particle, float frametime )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		particle->org[i] += particle->vel[i] * frametime;
	}
}

void EV_TrailCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	// If the Player is not on our PVS, then go back
	if( !CheckPVS( ent->clientIndex ) )
		return;

	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	cl_entity_t *player = gEngfuncs.GetEntityByIndex( ent->clientIndex );

	if( !player )
		return;
	   
	VectorCopy( player->origin, dl->origin );

	dl->radius = 240;
	dl->die = gEngfuncs.GetClientTime() + 0.001; // Kill it right away

	if( ent->entity.baseline.movetype == 2 )
	{
		dl->color.r = 240;
		dl->color.g = 25;
		dl->color.b = 25;
	}
	else
	{
		dl->color.r = 25;
		dl->color.g = 25;
		dl->color.b = 240;
	}

	// I know what you are thinking and yes, this was the only place I could find on where to put the timer
	// Hacky; Yes, Works; Yes!.
	if( ent->entity.baseline.animtime > gEngfuncs.GetClientTime() )
		return;

	for( int i = 0; i < 4; i++ )
	{
		particle_t *bPart = gEngfuncs.pEfxAPI->R_AllocParticle( EV_GenericParticleCallback );

		if( bPart )
		{
			VectorCopy( ent->entity.origin, bPart->org );
			bPart->org[0] += gEngfuncs.pfnRandomFloat( -2, 2 );
			bPart->org[1] += gEngfuncs.pfnRandomFloat( -2, 2 );
			bPart->org[2] += gEngfuncs.pfnRandomFloat( -2, 2 );
			bPart->vel[0] = gEngfuncs.pfnRandomFloat( -50, 50 );
			bPart->vel[1] = gEngfuncs.pfnRandomFloat( -50, 50 );
			bPart->vel[2] = gEngfuncs.pfnRandomFloat( 75, 80 );

			//Check team and color the particle correctly
			if( ent->entity.baseline.movetype == 2 )
				bPart->color = 70;
			else
				bPart->color = 43;

			bPart->type = pt_slowgrav;
			bPart->die = gEngfuncs.GetClientTime() + 0.5;
		}
	}

	ent->entity.baseline.animtime = gEngfuncs.GetClientTime() + 0.3;
}


void EV_FollowCarrier( event_args_t *args )
{
	int iEntIndex = args->iparam1;
	int iTeam = args->iparam2;
	float r, g, b;

	int modelIndex; 
	const char *model = "sprites/smoke.spr";

	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( model );

	if( iTeam == 2 )
	{
		r = 500;
		g = 0;
		b = 0;
	}
	else if( iTeam == 1 )
	{
		r = 0;
		g = 0;
		b = 500;
	}

	if( args->bparam1 == 1 )
		gEngfuncs.pEfxAPI->R_KillAttachedTents( iEntIndex );
	else
	{
		TEMPENTITY *pTrailSpawner = NULL;
		pTrailSpawner = gEngfuncs.pEfxAPI->R_TempModel( args->origin, args->velocity, args->angles, 9999, modelIndex, TE_BOUNCE_NULL );

		if( pTrailSpawner != NULL )
		{
			pTrailSpawner->flags |= ( FTENT_PLYRATTACHMENT | FTENT_PERSIST | FTENT_NOMODEL | FTENT_CLIENTCUSTOM );
			pTrailSpawner->clientIndex = iEntIndex;  

			pTrailSpawner->entity.baseline.movetype = iTeam; // Hack to store the team number on this temp ent.
			pTrailSpawner->entity.baseline.animtime = gEngfuncs.GetClientTime() + 0.3;
			pTrailSpawner->callback = EV_TrailCallback;
		}
	}
}

void EV_FlagSpawn( event_args_t *args )
{
	vec3_t origin;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEfxAPI->R_Implosion( origin, 50, 20, 0.5 );

	for( int i = 0; i < 20; i++ )
	{
		particle_t *bPart = gEngfuncs.pEfxAPI->R_AllocParticle( EV_GenericParticleCallback );

		if( bPart )
		{
			VectorCopy ( args->origin, bPart->org);
			bPart->org[0] += gEngfuncs.pfnRandomFloat( -4, 4 );
			bPart->org[1] += gEngfuncs.pfnRandomFloat( -4, 4 );
			bPart->org[2] += gEngfuncs.pfnRandomFloat( -4, 4 );
			bPart->vel[0] = gEngfuncs.pfnRandomFloat( -50, 50 );
			bPart->vel[1] = gEngfuncs.pfnRandomFloat( -50, 50 );
			bPart->vel[2] = gEngfuncs.pfnRandomFloat( 250, 350 );

			//Check team and color the particle correctly
			if( args->iparam1 == 1 )
				bPart->color = 70;
			else
				bPart->color = 43;

			bPart->type = pt_grav;
			bPart->die = gEngfuncs.GetClientTime() + 3;
		}
	}
}

void EV_PowerupCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	//If the Player is not on our PVS, then go back
	if( !CheckPVS( ent->clientIndex ) )
		return;

	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	cl_entity_t *player = gEngfuncs.GetEntityByIndex( ent->clientIndex );
	
	if( !player )
		return;

	VectorCopy( player->origin, dl->origin );

	dl->radius = 270;
	dl->dark = true;
	dl->die = gEngfuncs.GetClientTime() + 0.001; //Kill it right away

	if( ent->entity.baseline.iuser2 == 1 )
	{
		if( ent->entity.baseline.iuser1 == 1 )
		{
			dl->color.r = 255;
			dl->color.g = 128;
			dl->color.b = 128;
		}
		else
		{
			dl->color.r = 0;
			dl->color.g = 75;
			dl->color.b = 255;
		}
	}
	else if( ent->entity.baseline.iuser2 == 2 )
	{
		if( ent->entity.baseline.iuser1 == 1 )
		{
			dl->color.r = 255;
			dl->color.g = 128;
			dl->color.b = 0;
		}
		else if( ent->entity.baseline.iuser1 == 2 )
		{
			dl->color.r = 0;
			dl->color.g = 128;
			dl->color.b = 250;
		}
		else 
		{
			dl->color.r = 255;
			dl->color.g = 75;
			dl->color.b = 0;
		}
	}
	else if( ent->entity.baseline.iuser2 == 3 )
	{
		dl->color.r = 255;
		dl->color.g = 125;
		dl->color.b = 255;
	}
}

void EV_PlayerPowerup( event_args_t *args )
{
	int iEntIndex = args->iparam1;
	int iTeam = args->iparam2;
	int iPowerUp = (int)args->fparam1;

	int modelIndex; 
	const char *model = "sprites/smoke.spr";

	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( model );

	if( args->bparam1 == 1 )
		gEngfuncs.pEfxAPI->R_KillAttachedTents( iEntIndex );

	if( iPowerUp )
	{
		TEMPENTITY *pTrailSpawner = NULL;
		pTrailSpawner = gEngfuncs.pEfxAPI->R_TempModel( args->origin, args->velocity, args->angles, 9999, modelIndex, TE_BOUNCE_NULL );

		if( pTrailSpawner != NULL )
		{
			pTrailSpawner->flags |= ( FTENT_PLYRATTACHMENT | FTENT_PERSIST | FTENT_NOMODEL | FTENT_CLIENTCUSTOM );
			pTrailSpawner->clientIndex = iEntIndex;  

			pTrailSpawner->entity.baseline.iuser1 = iTeam;
			pTrailSpawner->entity.baseline.iuser2 = iPowerUp;

			pTrailSpawner->callback = EV_PowerupCallback;
		}
	}
}

float g_flLightTime;
BEAM *pBeam;

void EV_FireLightning( event_args_t *args )
{
	int idx;
	vec3_t origin, endorigin;
	vec3_t angles;
	vec3_t up, right, forward;
	int iShutDown;
	
	cl_entity_t *player;
	
	int modelIndex;

	bool bSound = false;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );
	bSound = args->bparam1 ? true : false;
	iShutDown = args->iparam2;

	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( "sprites/laserbeam.spr" );

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();

	AngleVectors( angles, forward, right, up );

	if( EV_IsLocal( idx ) )
	{
		if ( g_flLightTime <= gEngfuncs.GetClientTime() )
		{
			gEngfuncs.pfnWeaponAnim( 1, 0 );
			g_flLightTime = gEngfuncs.GetClientTime() + 0.5;
		}

		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );

		player = gEngfuncs.GetViewModel();
		origin = player->attachment[0];
	}
	else
	{
		origin = origin + Vector( 0, 0, 16 );
	}
	
	if( bSound )
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/lhit.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );
	}

	if( iShutDown == 0 && EV_IsLocal( idx ) && pBeam == NULL )
	{
		vec3_t vecSrc, vecEnd;
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
			gEngfuncs.pEventAPI->EV_SetSolidPlayers ( idx - 1 );	

			gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
			gEngfuncs.pEventAPI->EV_PlayerTrace( vecSrc, vecEnd, PM_STUDIO_BOX, -1, &tr );

			gEngfuncs.pEventAPI->EV_PopPMStates();
		
			pBeam = gEngfuncs.pEfxAPI->R_BeamEntPoint ( idx | 0x1000, tr.endpos, modelIndex, 99999, 5.0, 0.15, 2.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 );
		}
	}
	else if( iShutDown == 1 )
	{
		if( EV_IsLocal( idx ) )
		{
			if( pBeam )
			{
				pBeam->die = 0.0;
				pBeam = NULL;
			}
		}
	}
}


void EV_FireRocket( event_args_t *args )
{
	int idx;
	vec3_t origin;
	idx = args->entindex;
	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/sgun1.wav", 1.0, ATTN_NORM, 0, 100 );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );
	}
}

void EV_FireGrenade( event_args_t *args )
{
	int idx;
	vec3_t origin;
	
	idx = args->entindex;
	VectorCopy( args->origin, origin );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/grenade.wav", 1.0, ATTN_NORM, 0, 100 );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );
	}
}

void EV_Quake_NailTouch( struct tempent_s *ent, pmtrace_t *ptr )
{
	//physent_t *pe = gEngfuncs.pEventAPI->EV_GetPhysent( ptr->ent );

	//if( pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		EV_Quake_GunshotDecalTrace( ptr, EV_HLDM_DamageDecal() );
	}
}

void EV_Quake_ClientProjectile( float *vecOrigin, float *vecVelocity, short sModelIndex, int iOwnerIndex, int iLife, void (*hitcallback)( struct tempent_s *ent, pmtrace_t *ptr ) )
{
	gEngfuncs.pEfxAPI->R_Projectile( vecOrigin, vecVelocity, sModelIndex, iLife, iOwnerIndex, hitcallback );
}

void EV_FireSpike( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecVelocity;
	float offset = args->bparam1 ? 2.0 : -2.0;

	int shell;

	// gEngfuncs.Con_NPrintf( 22, "offset %f", offset );

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/spike.mdl" );
	AngleVectors( angles, forward, right, up );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/rocket1i.wav", 1.0, ATTN_NORM, 0, 100 );

	// make nails
	VectorScale( forward, 1000, vecVelocity );
	EV_Quake_ClientProjectile( origin + Vector( 0, 0, 10 ) + ( right * offset ), vecVelocity, (short)shell, idx, 6, EV_Quake_NailTouch );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );
	}
}

void EV_FireSuperSpike( event_args_t *args )
{
	int idx;
	vec3_t origin;
	vec3_t angles;
	vec3_t up, right, forward;
	vec3_t vecVelocity;

	int shell;

	idx = args->entindex;
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, angles );

	shell = gEngfuncs.pEventAPI->EV_FindModelIndex( "models/spike.mdl" );
	AngleVectors( angles, forward, right, up );

	EV_Quake_PlayQuadSound( idx, origin, args->iparam1 );

	gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, "weapons/spike2.wav", 1.0, ATTN_NORM, 0, 100 );

	// make nails
	VectorScale( forward, 1000, vecVelocity );
	EV_Quake_ClientProjectile( origin + Vector( 0, 0, 16 ), vecVelocity, (short)shell, idx, 6, EV_Quake_NailTouch );

	if( EV_IsLocal( idx ) )
	{
		V_PunchAxis( 0, Q_SMALL_PUNCHANGLE_KICK );
	}
}

#define SND_CHANGE_PITCH	( 1 << 7 )

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
	stop	  = args->bparam1;

	m_flVolume	= (float)(us_params & 0x003f)/40.0;
	noise		= (int)(((us_params) >> 12 ) & 0x0007);
	pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

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

const char *DMC_BloodDecal()
{
	const char *pszDecalName;
	int idx = gEngfuncs.pfnRandomLong( 0, 5 );
         
	switch( idx + 1 )
	{
	case 1:
		pszDecalName = "{blood1";
		break;
	case 2:
		pszDecalName = "{blood2";
		break;
	case 3:  
		pszDecalName = "{blood3";
		break;
	case 4:
		pszDecalName = "{blood4";
		break;
	case 5:
		pszDecalName = "{blood5";
		break;
	case 6:
		pszDecalName = "{blood6";
		break;
	}
	return pszDecalName;
}

void DMC_DecalTrace( pmtrace_t *pTrace, const char *decalName )
{
	EV_Quake_DecalTrace( pTrace, decalName );
}

void EV_GibTouch( struct tempent_s *ent, struct pmtrace_s *ptr )
{
	const char *pszSound;

	DMC_DecalTrace( ptr, DMC_BloodDecal() );

	// 1 in 5 chance of squishy sound
	if( gEngfuncs.pfnRandomLong( 0, 4 ) > 0 )
		return;

	switch( gEngfuncs.pfnRandomLong( 0, 5 ) )
	{
		case 0:
			pszSound = "debris/flesh1.wav";
			break;
		case 1:
			pszSound = "debris/flesh2.wav";
			break;
		case 2:
			pszSound = "debris/flesh3.wav";
			break;
		case 3:
			pszSound = "debris/flesh5.wav";
			break;
		case 4:
			pszSound = "debris/flesh6.wav";
			break;
		case 5:
			pszSound = "debris/flesh7.wav";
			break;
	}
	gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, pszSound, 1.0, ATTN_NORM, 0, PITCH_NORM );
}

void EV_GibParticleCallback( struct particle_s *particle, float frametime )
{
	for( int i = 0; i < 3; i++ )
	{
		particle->org[i] += particle->vel[i] * frametime;
	}
}

void EV_Gibbed( event_args_t *args )
{
	vec3_t origin, velocity, angles, rotate;
	int modelindex, i;
	TEMPENTITY *pGib = NULL;
	int gibs = 5;

	VectorCopy( args->origin, origin );

	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "player/gib.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );

	rotate[0] = gEngfuncs.pfnRandomLong( -100, 100 );
	rotate[1] = gEngfuncs.pfnRandomLong( -100, 100 );
	rotate[2] = gEngfuncs.pfnRandomLong( -100, 100 );

        for( i = 0; i < gibs; i++ )
        {
		const char *pszModel;

		switch( gEngfuncs.pfnRandomLong( 1, 3 ) )
		{
			//Just in case
			default:
			case 1:
				pszModel = "models/gib_1.mdl";
				break;
			case 2:
				pszModel = "models/gib_2.mdl";
				break;
			case 3:
				pszModel = "models/gib_3.mdl";
				break;
		}

		modelindex = gEngfuncs.pEventAPI->EV_FindModelIndex( pszModel );

		if( !modelindex )
			return;

                VectorCopy( args->angles, angles );

		VectorScale( angles, -1.0, angles );

		angles[0] += gEngfuncs.pfnRandomFloat( -0.30, 0.30 );
		angles[1] += gEngfuncs.pfnRandomFloat( -0.30, 0.30 );
		angles[2] += gEngfuncs.pfnRandomFloat( -0.30, 0.30 );

		VectorScale( angles, gEngfuncs.pfnRandomFloat( 500, 1200 ), velocity );
		velocity[2] += 600;

		pGib = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, rotate, 15, modelindex, TE_BOUNCE_NULL );

		if( pGib )
		{
			pGib->flags |= ( FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_FADEOUT | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL );
			pGib->hitcallback = EV_GibTouch;
		}
	}
}

//Spawns the teleport effect.
void EV_Teleport( event_args_t *args )
{
	const char *pszSound;
	vec3_t vecOrg;

	VectorCopy( args->origin, vecOrg );

	switch( gEngfuncs.pfnRandomLong( 0, 4 ) )
	{
		case 0:
			pszSound = "misc/r_tele1.wav";
			break;
		case 1:
			pszSound = "misc/r_tele2.wav";
			break;
		case 2:
			pszSound = "misc/r_tele3.wav";
			break;
		case 3:
			pszSound = "misc/r_tele4.wav";
			break;
		case 4:
			pszSound = "misc/r_tele5.wav";
			break;
	}

	gEngfuncs.pEventAPI->EV_PlaySound( 0, vecOrg, CHAN_STATIC, pszSound, 1.0, ATTN_NORM, 0, PITCH_NORM );
	gEngfuncs.pEfxAPI->R_TeleportSplash( vecOrg );
}

int ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

void EV_RocketTrailCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	if( currenttime < ent->entity.baseline.fuser1 )
		return;

	if( ent->entity.origin == ent->entity.attachment[0] )
		ent->die = gEngfuncs.GetClientTime();
	else
		VectorCopy( ent->entity.origin, ent->entity.attachment[0] );

	//Make the Rocket light up. ( And only rockets, no Grenades ).
	if( ent->entity.baseline.sequence == 70 )
	{
		dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
		VectorCopy( ent->entity.origin, dl->origin );

		dl->radius = 160;
		dl->dark = true;
		dl->die = gEngfuncs.GetClientTime() + 0.001; //Kill it right away
													 
		dl->color.r = 255;
		dl->color.g = 255;
		dl->color.b = 255;
	}
}

#define GRENADE_TRAIL 1
#define ROCKET_TRAIL 2

void EV_Trail( event_args_t *args )
{
	int iEntIndex = args->iparam1;
	TEMPENTITY *pTrailSpawner = NULL;

	pTrailSpawner = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel( args->origin );

	if( pTrailSpawner != NULL )
	{
		pTrailSpawner->flags |= ( FTENT_PLYRATTACHMENT | FTENT_COLLIDEKILL | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL | FTENT_COLLIDEWORLD );
		pTrailSpawner->callback = EV_RocketTrailCallback;
		pTrailSpawner->clientIndex = iEntIndex;

		if( args->iparam2 == GRENADE_TRAIL )
			pTrailSpawner->entity.baseline.sequence = 69;
		else if ( args->iparam2 == ROCKET_TRAIL )
			pTrailSpawner->entity.baseline.sequence = 70;

		pTrailSpawner->die = gEngfuncs.GetClientTime() + 10; // Just in case
		pTrailSpawner->entity.baseline.fuser1 = gEngfuncs.GetClientTime() + 0.5; // Don't try to die till 500ms ahead
	}
}

void EV_Explosion( event_args_t *args )
{
	vec3_t origin, scorch_origin, velocity, forward, right, up;
	int modelIndex;
	const char *model = "sprites/zerogxplode.spr";
	modelIndex = gEngfuncs.pEventAPI->EV_FindModelIndex( model );
	pmtrace_t tr;

	//Make decals and Explosions
	//Might not work for grenades.
	VectorCopy( args->origin, origin );
	VectorCopy( args->angles, velocity ); //Velocity

	scorch_origin = origin - velocity.Normalize() * 32;

	gEngfuncs.pEfxAPI->R_Explosion( origin, modelIndex, 2.5, 15, TE_EXPLFLAG_NONE );
	gEngfuncs.pEfxAPI->R_ParticleExplosion2( origin, 111, 8 );

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );

	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	gEngfuncs.pEventAPI->EV_PlayerTrace( scorch_origin, scorch_origin + velocity.Normalize() * 64, PM_STUDIO_BOX | PM_WORLD_ONLY , -1, &tr );	

	gEngfuncs.pEventAPI->EV_PopPMStates();

	DMC_DecalTrace( &tr, EV_HLDM_RocketDamageDecal() );
}

#define EV_DMC_MOVE_SOUND 0
#define EV_DMC_STOP_SOUND 1
const char *EV_DMC_LookupDoorSound( int type, int index )
{
	const char *pszSound;
	int idx;

	// Assume the worst
	pszSound = "common/null.wav";

	if( type == EV_DMC_MOVE_SOUND )
	{
		idx = ( index >> 8 ) & 0xff;

		switch( idx )
		{
		default:
		case 0:
			pszSound = "common/null.wav";
			break;
		case 1:
			pszSound = "doors/doormove1.wav";
			break;
		case 2:
			pszSound = "doors/doormove2.wav";
			break;
		case 3:
			pszSound = "doors/doormove3.wav";
			break;
		case 4:
			pszSound = "doors/doormove4.wav";
			break;
		case 5:
			pszSound = "doors/doormove5.wav";
			break;
		case 6:
			pszSound = "doors/doormove6.wav";
			break;
		case 7:
			pszSound = "doors/doormove7.wav";
			break;
		case 8:
			pszSound = "doors/doormove8.wav";
			break;
		case 9:
			pszSound = "doors/doormove9.wav";
			break;
		case 10:
			pszSound = "doors/doormove10.wav";
			break;
		}
	}
	else if( type == EV_DMC_STOP_SOUND )
	{
		idx = ( index & 0xff );

		// set the door's 'reached destination' stop sound
		switch( idx )
		{
		default:
		case 0:
			pszSound = "common/null.wav";
			break;
		case 1:
			pszSound = "doors/doorstop1.wav";
			break;
		case 2:
			pszSound = "doors/doorstop2.wav";
			break;
		case 3:
			pszSound = "doors/doorstop3.wav";
			break;
		case 4:
			pszSound = "doors/doorstop4.wav";
			break;
		case 5:
			pszSound = "doors/doorstop5.wav";
			break;
		case 6:
			pszSound = "doors/doorstop6.wav";
			break;
		case 7:
			pszSound = "doors/doorstop7.wav";
			break;
		case 8:
			pszSound = "doors/doorstop8.wav";
			break;
		}
	}
	return pszSound;
}

void EV_DMC_DoorGoUp( event_args_t *args )
{
	int idx = -1;
	int soundindex = args->iparam1;

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ));
	gEngfuncs.pEventAPI->EV_PlaySound( idx, args->origin, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ), 1.0, ATTN_NORM, 0, PITCH_NORM );
}

void EV_DMC_DoorGoDown( event_args_t *args )
{
	EV_DMC_DoorGoUp( args );
/*
	int idx = -1;
	int soundindex = args->iparam1;

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ));
	gEngfuncs.pEventAPI->EV_PlaySound( idx, args->origin, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ), 1.0, ATTN_NORM, 0, PITCH_NORM );
*/
}

void EV_DMC_DoorHitTop( event_args_t *args )
{
	int idx = -1;
	int soundindex = args->iparam1;

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, args->origin, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_STOP_SOUND, soundindex ), 1.0, ATTN_NORM, 0, PITCH_NORM );
}

void EV_DMC_DoorHitBottom( event_args_t *args )
{
	EV_DMC_DoorHitTop( args );
/*
	int idx = -1;
	int soundindex = args->iparam1;

	gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_MOVE_SOUND, soundindex ) );
	gEngfuncs.pEventAPI->EV_PlaySound( idx, args->origin, CHAN_STATIC, EV_DMC_LookupDoorSound( EV_DMC_STOP_SOUND, soundindex ), 1.0, ATTN_NORM, 0, PITCH_NORM );
*/
}
