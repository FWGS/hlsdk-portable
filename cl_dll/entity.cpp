//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Client side entity management functions

#include <memory.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_types.h"
#include "studio_event.h" // def. of mstudioevent_t
#include "r_efx.h"
#include "event_api.h"
#include "pm_defs.h"
#include "pmtrace.h"	
#include "pm_shared.h"
#include "game_fx.h"
#include "com_model.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBlastCone.h"
#include "PSGravityPart.h"
#include "RSSprite.h"
#include "PSBubbles.h"

void Game_AddObjects( void );

extern vec3_t v_origin;

int g_iAlive = 1;
double g_cl_gravity;// XDM3035: value is same as sv_gravity (800)

extern "C"
{
	int DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
	void DLLEXPORT HUD_CreateEntities( void );
	void DLLEXPORT HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
	void DLLEXPORT HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
	void DLLEXPORT HUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
	void DLLEXPORT HUD_TxferPredictionData ( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
	void DLLEXPORT HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ), void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
	struct cl_entity_s DLLEXPORT *HUD_GetUserEntity( int index );
}

/*
========================
HUD_AddEntity
	Return 0 to filter entity from visible list for rendering
========================
*/
int DLLEXPORT HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname )
{
	switch( type )
	{
	case ET_NORMAL:
	case ET_PLAYER:
	case ET_BEAM:
	case ET_TEMPENTITY:
	case ET_FRAGMENTED:
	default:
		break;
	}
	// each frame every entity passes this function, so the overview hooks it to filter the overview entities
	// in spectator mode:
	// each frame every entity passes this function, so the overview hooks 
	// it to filter the overview entities

	if( g_iUser1 )
	{
		gHUD.m_Spectator.AddOverviewEntity( type, ent, modelname );

		if( (	g_iUser1 == OBS_IN_EYE || gHUD.m_Spectator.m_pip->value == INSET_IN_EYE ) &&
				ent->index == g_iUser2 )
			return 0;	// don't draw the player we are following in eye
	}

	return 1;
}

/*
=========================
HUD_TxferLocalOverrides

The server sends us our origin with extra precision as part of the clientdata structure, not during the normal
playerstate update in entity_state_t.  In order for these overrides to eventually get to the appropriate playerstate
structure, we need to copy them into the state structure at this point.
=========================
*/
void DLLEXPORT HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client )
{
	VectorCopy( client->origin, state->origin );

	// Spectator
	state->iuser1 = client->iuser1;
	state->iuser2 = client->iuser2;

	// Duck prevention
	state->iuser3 = client->iuser3;

	// Fire prevention
	state->iuser4 = client->iuser4;
}

/*
=========================
HUD_ProcessPlayerState

We have received entity_state_t for this player over the network.  We need to copy appropriate fields to the
playerstate structure
=========================
*/
void DLLEXPORT HUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src )
{
	// Copy in network data
	VectorCopy( src->origin, dst->origin );
	VectorCopy( src->angles, dst->angles );

	VectorCopy( src->velocity, dst->velocity );

	dst->frame				= src->frame;
	dst->modelindex				= src->modelindex;
	dst->skin				= src->skin;
	dst->effects				= src->effects;
	dst->weaponmodel			= src->weaponmodel;
	dst->movetype				= src->movetype;
	dst->sequence				= src->sequence;
	dst->animtime				= src->animtime;
	
	dst->solid				= src->solid;
	
	dst->rendermode				= src->rendermode;
	dst->renderamt				= src->renderamt;	
	dst->rendercolor.r			= src->rendercolor.r;
	dst->rendercolor.g			= src->rendercolor.g;
	dst->rendercolor.b			= src->rendercolor.b;
	dst->renderfx				= src->renderfx;

	dst->framerate				= src->framerate;
	dst->body				= src->body;

	memcpy( &dst->controller[0], &src->controller[0], 4 * sizeof(byte) );
	memcpy( &dst->blending[0], &src->blending[0], 2 * sizeof(byte) );

	VectorCopy( src->basevelocity, dst->basevelocity );

	dst->friction				= src->friction;
	dst->gravity				= src->gravity;
	dst->gaitsequence			= src->gaitsequence;
	dst->spectator				= src->spectator;
	dst->usehull				= src->usehull;
	dst->playerclass			= src->playerclass;
	dst->team				= src->team;
	dst->colormap				= src->colormap;

	// Save off some data so other areas of the Client DLL can get to it
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();	// Get the local player's index
	if( dst->number == player->index )
	{
		g_iPlayerClass = dst->playerclass;
		g_iTeamNumber = dst->team;

		g_iUser1 = src->iuser1;
		g_iUser2 = src->iuser2;
		g_iUser3 = src->iuser3;
	}
}

/*
=========================
HUD_TxferPredictionData

Because we can predict an arbitrary number of frames before the server responds with an update, we need to be able to copy client side prediction data in
 from the state that the server ack'd receiving, which can be anywhere along the predicted frame path ( i.e., we could predict 20 frames into the future and the server ack's
 up through 10 of those frames, so we need to copy persistent client-side only state from the 10th predicted frame to the slot the server
 update is occupying.
=========================
*/
void DLLEXPORT HUD_TxferPredictionData( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd )
{
	ps->oldbuttons				= pps->oldbuttons;
	ps->flFallVelocity			= pps->flFallVelocity;
	ps->iStepLeft				= pps->iStepLeft;
	ps->playerclass				= pps->playerclass;

	pcd->viewmodel				= ppcd->viewmodel;
	pcd->m_iId				= ppcd->m_iId;
	pcd->ammo_shells			= ppcd->ammo_shells;
	pcd->ammo_nails				= ppcd->ammo_nails;
	pcd->ammo_cells				= ppcd->ammo_cells;
	pcd->ammo_rockets			= ppcd->ammo_rockets;
	pcd->m_flNextAttack			= ppcd->m_flNextAttack;
	pcd->fov				= ppcd->fov;
	pcd->weaponanim				= ppcd->weaponanim;
	pcd->tfstate				= ppcd->tfstate;
	pcd->maxspeed				= ppcd->maxspeed;

	pcd->deadflag				= ppcd->deadflag;

	// Spectating or not dead == get control over view angles.
	g_iAlive = ( ppcd->iuser1 || ( pcd->deadflag == DEAD_NO ) ) ? 1 : 0;

	// Spectator
	pcd->iuser1					= ppcd->iuser1;
	pcd->iuser2					= ppcd->iuser2;

	// Duck prevention
	pcd->iuser3 = ppcd->iuser3;

	if( gEngfuncs.IsSpectateOnly() )
	{
		// in specator mode we tell the engine who we want to spectate and how
		// iuser3 is not used for duck prevention (since the spectator can't duck at all)
		pcd->iuser1 = g_iUser1;	// observer mode
		pcd->iuser2 = g_iUser2; // first target
		pcd->iuser3 = g_iUser3; // second target
	}

	// Fire prevention
	pcd->iuser4 					= ppcd->iuser4;

	pcd->fuser2					= ppcd->fuser2;
	pcd->fuser3					= ppcd->fuser3;

	VectorCopy( ppcd->vuser1, pcd->vuser1 );
	VectorCopy( ppcd->vuser2, pcd->vuser2 );
	VectorCopy( ppcd->vuser3, pcd->vuser3 );
	VectorCopy( ppcd->vuser4, pcd->vuser4 );

	memcpy( wd, pwd, MAX_WEAPONS * sizeof(weapon_data_t) );
}

/*
//#define TEST_IT	1
#if TEST_IT

cl_entity_t mymodel[9];

void MoveModel( void )
{
	cl_entity_t *player;
	int i, j;
	int modelindex;
	struct model_s *mod;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if( !player )
		return;

	mod = gEngfuncs.CL_LoadModel( "models/sentry3.mdl", &modelindex );
	for( i = 0; i < 3; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			// Don't draw over ourself...
			if( ( i == 1 ) && ( j == 1 ) )
				continue;

			mymodel[i * 3 + j] = *player;

			mymodel[i * 3 + j].player = 0;

			mymodel[i * 3 + j].model = mod;
			mymodel[i * 3 + j].curstate.modelindex = modelindex;
		
			// Move it out a bit
			mymodel[i * 3 + j].origin[0] = player->origin[0] + 50 * ( 1 - i );
			mymodel[i * 3 + j].origin[1] = player->origin[1] + 50 * ( 1 - j );

			gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, &mymodel[i * 3 + j] );
		}
	}
}
#endif

//#define TRACE_TEST	1
#if TRACE_TEST

extern int hitent;

cl_entity_t hit;

void TraceModel( void )
{
	cl_entity_t *ent;

	if( hitent <= 0 )
		return;

	// Load it up with some bogus data
	ent = gEngfuncs.GetEntityByIndex( hitent );
	if( !ent )
		return;

	hit = *ent;
	//hit.curstate.rendermode = kRenderTransTexture;
	//hit.curstate.renderfx = kRenderFxGlowShell;
	//hit.curstate.renderamt = 100;

	hit.origin[2] += 40;

	gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, &hit );
}
#endif
*/

/*
void ParticleCallback( struct particle_s *particle, float frametime )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		particle->org[i] += particle->vel[i] * frametime;
	}
}

cvar_t *color = NULL;
void Particles( void )
{
	static float lasttime;
	float curtime;

	curtime = gEngfuncs.GetClientTime();

	if( ( curtime - lasttime ) < 2.0f )
		return;

	if( !color )
	{
		color = gEngfuncs.pfnRegisterVariable( "color", "255 0 0", 0 );
	}

	lasttime = curtime;

	// Create a few particles
	particle_t *p;
	int i, j;

	for( i = 0; i < 1000; i++ )
	{
		int r, g, b;
		p = gEngfuncs.pEfxAPI->R_AllocParticle( ParticleCallback );
		if( !p )
			break;

		for( j = 0; j < 3; j++ )
		{
			p->org[j] = v_origin[j] + gEngfuncs.pfnRandomFloat( -32.0f, 32.0f );
			p->vel[j] = gEngfuncs.pfnRandomFloat( -100.0f, 100.0f );
		}

		if( color )
		{
			sscanf( color->string, "%i %i %i", &r, &g, &b );
		}
		else
		{
			r = 192;
			g = 0;
			b = 0;
		}

		p->color = gEngfuncs.pEfxAPI->R_LookupColor( r, g, b );
		gEngfuncs.pEfxAPI->R_GetPackedColor( &p->packedColor, p->color );

		// p->die is set to current time so all you have to do is add an additional time to it
		p->die += 3.0f;
	}
}
*/

/*
void TempEntCallback( struct tempent_s *ent, float frametime, float currenttime )
{
	int i;

	for( i = 0; i < 3; i++ )
	{
		ent->entity.curstate.origin[i] += ent->entity.baseline.origin[i] * frametime;
	}
}

void TempEnts( void )
{
	static float lasttime;
	float curtime;

	curtime = gEngfuncs.GetClientTime();

	if( ( curtime - lasttime ) < 10.0f )
		return;

	lasttime = curtime;

	TEMPENTITY *p;
	int i, j;
	struct model_s *mod;
	vec3_t origin;
	int index;

	mod = gEngfuncs.CL_LoadModel( "sprites/laserdot.spr", &index );

	for( i = 0; i < 100; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			origin[j] = v_origin[j];
			if( j != 2 )
			{
				origin[j] += 75;
			}
		}

		p = gEngfuncs.pEfxAPI->CL_TentEntAllocCustom( (float *)&origin, mod, 0, TempEntCallback );
		if( !p )
			break;

		for( j = 0; j < 3; j++ )
		{
			p->entity.curstate.origin[j] = origin[j];

			// Store velocity in baseline origin
			p->entity.baseline.origin[j] = gEngfuncs.pfnRandomFloat( -100.0f, 100.0f );
		}

		// p->die is set to current time so all you have to do is add an additional time to it
		p->die += 10.0f;
	}
}
*/

#if BEAM_TEST
// Note can't index beam[0] in Beam callback, so don't use that index
// Room for 1 beam ( 0 can't be used )
static cl_entity_t beams[2];

void BeamEndModel( void )
{
	cl_entity_t *player, *model;
	int modelindex;
	struct model_s *mod;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if( !player )
		return;

	mod = gEngfuncs.CL_LoadModel( "models/sentry3.mdl", &modelindex );
	if( !mod )
		return;

	// Slot 1
	model = &beams[1];

	*model = *player;
	model->player = 0;
	model->model = mod;
	model->curstate.modelindex = modelindex;

	// Move it out a bit
	model->origin[0] = player->origin[0] - 100;
	model->origin[1] = player->origin[1];

	model->attachment[0] = model->origin;
	model->attachment[1] = model->origin;
	model->attachment[2] = model->origin;
	model->attachment[3] = model->origin;

	gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, model );
}

void Beams( void )
{
	static float lasttime;
	float curtime;
	struct model_s *mod;
	int index;

	BeamEndModel();

	curtime = gEngfuncs.GetClientTime();
	float end[3];

	if( ( curtime - lasttime ) < 10.0 )
		return;

	mod = gEngfuncs.CL_LoadModel( "sprites/laserbeam.spr", &index );
	if( !mod )
		return;

	lasttime = curtime;

	end[0] = v_origin.x + 100;
	end[1] = v_origin.y + 100;
	end[2] = v_origin.z;

	BEAM *p1;
	p1 = gEngfuncs.pEfxAPI->R_BeamEntPoint( -1, end, index,
		10.0, 2.0, 0.3, 1.0, 5.0, 0.0, 1.0, 1.0, 1.0, 1.0 );
}
#endif

/*
=========================
HUD_CreateEntities
	
Gives us a chance to add additional entities to the render this frame
=========================
*/
void DLLEXPORT HUD_CreateEntities( void )
{
	// e.g., create a persistent cl_entity_t somewhere.
	// Load an appropriate model into it ( gEngfuncs.CL_LoadModel )
	// Call gEngfuncs.CL_CreateVisibleEntity to add it to the visedicts list
/*
#if TEST_IT
	MoveModel();
#endif
#if TRACE_TEST
	TraceModel();
#endif
*/
/*
	Particles();
*/
/*
	TempEnts();
*/
#if BEAM_TEST
	Beams();
#endif
	// Add in any game specific objects
	Game_AddObjects();

	GetClientVoiceMgr()->CreateEntities();
}

//================//
//Gun Barrel smoke//
//================//
void EV_GunSmoke2(vec3_t origin, int iSmokeType)
{
	switch (iSmokeType)
	{
	case GUNSMOKE_WHITE_SMALLEST:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(2, 18, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 5, 255,255,255, 0.2, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(1,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_WHITE_SMALL:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 15, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 10, 255,255,255, 0.2, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(2,4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_WHITE_MEDIUM:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 15, 255,255,255, 0.25, -0.09, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(2,5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_WHITE_LARGE:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 20, 255,255,255, 0.3, -0.09, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(3,6), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;

	case GUNSMOKE_BLACK_SMALLEST:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 15, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 5, 0,0,0, 0.1, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(1,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_BLACK_SMALL:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 10, 0,0,0, 0.1, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(2,4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), 0, -1);
	break;
	case GUNSMOKE_BLACK_MEDIUM:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 15, 0,0,0, 0.15, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(2,5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_BLACK_LARGE:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 20, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(3,6), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	}
}

/*
=========================
HUD_StudioEvent

The entity's studio model description indicated an event was
fired during this frame, handle the event by it's tag ( e.g., muzzleflash, sound )
=========================
*/
#define FRAMERATE		20//10
#define RENDER_VALUE	50
#define SCALE_VALUE		0.5
#define COLOR_VALUE		30//25
#define FADESPEED		2//1

void DLLEXPORT HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity )
{
	void VectorAngles( const float *forward, float *angles );


	//TO DO: make 3d Muzzeflashes? How do I can attach some model by 0.1 seconds? 
	int iSmoke = CVAR_GET_FLOAT("cl_gunsmoke");

	TEMPENTITY *pMuzz1TempEnt;
	TEMPENTITY *pMuzz2TempEnt;
	TEMPENTITY *pMuzz3TempEnt;

	int  iMuzz1 = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/muzz1.spr");
	int  iMuzz2 = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/muzz2.spr");
	int  iMuzz3 = gEngfuncs.pEventAPI->EV_FindModelIndex ("sprites/muzz3.spr");

	vec3_t up, right, forward, angles;

	cl_entity_t *ent = gEngfuncs.GetEntityByIndex( entity->index );

    angles =  ent->curstate.angles;

    AngleVectors( entity->angles, forward, up, right );

	switch( event->event )
	{
	case 5001:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[0], atoi( event->options ) );
		DynamicLight((float *)&entity->attachment[0], 100, 250,200,150, 0.1, 0.0);
			if ( iSmoke != 0 )
			{
				EV_GunSmoke2((float*)&entity->attachment[0], GUNSMOKE_WHITE_SMALLEST);
			}
		break;

	case 5000:
		{
			float R, G, B, Radius;
			sscanf(event->options, "%f %f %f %f", &R, &G, &B, &Radius );
			DynamicLight((float *)&entity->attachment[0], Radius, R,G,B, 0.1, 0.0);
		}
		break;

	case 5011:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[1], atoi( event->options ) );
		DynamicLight((float *)&entity->attachment[1], 100, 250,200,150, 0.1, 0.0);
		if ( iSmoke != 0 )
		{
			EV_GunSmoke2((float*)&entity->attachment[1], GUNSMOKE_WHITE_SMALLEST);
		}
		break;

		case 5012: //muzzle flashes (attach 0)
		{//Params: 1.Sprite (0-4), 2.Frame, 3.Scale
			int Spr, Frame;
			float Scale;
			sscanf(event->options, "%i %i %f", &Spr, &Frame, &Scale );
	
		        switch (Spr)
		        {
		                case 0 : Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"); break;
		                case 1 : Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"); break;
		                case 2 : Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"); break;
		                case 3 : Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"); break;
		                case 4 : Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"); break;
				default: Spr = gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"); break;
		        }
			g_pRenderManager->AddSystem(new CRSSprite((float*)&entity->attachment[0], Vector(0,0,0), Spr, Frame, kRenderTransAdd, 255,255,255, 0.8, -1.0, Scale, -(Scale*2), 0.0, 0.05), 0, -1);
			break;
		}

	case 5021:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[2], atoi( event->options ) );
		DynamicLight((float *)&entity->attachment[2], 100, 250,200,150, 0.1, 0.0);
		if ( iSmoke != 0 )
		{
			EV_GunSmoke2((float*)&entity->attachment[2], GUNSMOKE_WHITE_SMALLEST);
		}
		break;

	case 5031:
		gEngfuncs.pEfxAPI->R_MuzzleFlash( (float *)&entity->attachment[3], atoi( event->options ) );
		DynamicLight((float *)&entity->attachment[3], 100, 250,200,150, 0.1, 0.0);
			if ( iSmoke != 0 )
			{
				EV_GunSmoke2((float*)&entity->attachment[3], GUNSMOKE_WHITE_SMALLEST);
			}	
		break;

	case 5002:
		gEngfuncs.pEfxAPI->R_SparkEffect( (float *)&entity->attachment[0], atoi( event->options ), -100, 100 );
		break;
	// Client side sound
	case 5004:		
		gEngfuncs.pfnPlaySoundByNameAtLocation( (char *)event->options, 1.0, (float *)&entity->attachment[0] );
		break;

	case 5005:
		{
			float R, G, B, Radius;
			sscanf(event->options, "%f %f %f %f", &R, &G, &B, &Radius );
			DynamicLight((float *)&entity->attachment[0], Radius, R,G,B, 0.1, 0.0);
		}
		break;

	case 5006:
		{
			float R, G, B, Radius;
			sscanf(event->options, "%f %f %f %f", &R, &G, &B, &Radius );
			DynamicLight((float*)&entity->attachment[1], Radius, R,G,B, 0.1, 0.0);
		}
		break;

	case 5007:
		{
			float R, G, B, Radius;
			sscanf(event->options, "%f %f %f %f", &R, &G, &B, &Radius );
			DynamicLight((float*)&entity->attachment[2], Radius, R,G,B, 0.1, 0.0);
		}
		break;

	case 5008:
		{
			float R, G, B, Radius;
			sscanf(event->options, "%f %f %f %f", &R, &G, &B, &Radius );
			DynamicLight((float*)&entity->attachment[3], Radius, R,G,B, 0.1, 0.0);
		}
		break;

	case 5149:
		{

			//DynamicLight((float *)&entity->attachment[0], 120, 200,100,250, 0.1, 0.0);

			pMuzz1TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0],  forward * gEngfuncs.pfnRandomLong(-10,10) + right * gEngfuncs.pfnRandomLong(-10,10) + up * gEngfuncs.pfnRandomLong(-10,10),
				0.5,//scale
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.05,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz1TempEnt)
			{ 
				pMuzz1TempEnt->fadeSpeed = 10;
				pMuzz1TempEnt->entity.curstate.framerate = 10;//20
				pMuzz1TempEnt->entity.curstate.renderamt = 222;
				pMuzz1TempEnt->entity.curstate.rendercolor.r = 200;
				pMuzz1TempEnt->entity.curstate.rendercolor.g = 100;
				pMuzz1TempEnt->entity.curstate.rendercolor.b = 250;
			}
			//				2 MUZZLE FLASH
			pMuzz2TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0], forward * gEngfuncs.pfnRandomLong(-15,15) + right * gEngfuncs.pfnRandomLong(-15,15) + up * gEngfuncs.pfnRandomLong(-15,15),
				0.3,
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.05,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz2TempEnt)
			{ 
				pMuzz2TempEnt->fadeSpeed = 10;
				pMuzz2TempEnt->entity.curstate.framerate = 10;//20
				pMuzz2TempEnt->entity.curstate.renderamt = 222;
				pMuzz2TempEnt->entity.curstate.rendercolor.r = 200;
				pMuzz2TempEnt->entity.curstate.rendercolor.g = 100;
				pMuzz2TempEnt->entity.curstate.rendercolor.b = 250;
			}

		}
		break;

	case 5150:
		{
			EV_GunSmoke2((float*)&entity->attachment[3], GUNSMOKE_WHITE_SMALLEST);

			pMuzz1TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0],  forward * gEngfuncs.pfnRandomLong(-10,10) + right * gEngfuncs.pfnRandomLong(-10,10) + up * gEngfuncs.pfnRandomLong(-10,10),
				0.3,//scale
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.05,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz1TempEnt)
			{ 
				pMuzz1TempEnt->fadeSpeed = 10;
				pMuzz1TempEnt->entity.curstate.framerate = 10;//20
				pMuzz1TempEnt->entity.curstate.renderamt = 222;
				pMuzz1TempEnt->entity.curstate.rendercolor.r = 250;
				pMuzz1TempEnt->entity.curstate.rendercolor.g = 150;
				pMuzz1TempEnt->entity.curstate.rendercolor.b = 100;
			}
			//				2 MUZZLE FLASH
			pMuzz2TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0], forward * gEngfuncs.pfnRandomLong(-15,15) + right * gEngfuncs.pfnRandomLong(-15,15) + up * gEngfuncs.pfnRandomLong(-15,15),
				0.2,
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.05,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz2TempEnt)
			{ 
				pMuzz2TempEnt->fadeSpeed = 10;
				pMuzz2TempEnt->entity.curstate.framerate = 10;//20
				pMuzz2TempEnt->entity.curstate.renderamt = 222;
				pMuzz2TempEnt->entity.curstate.rendercolor.r = 250;
				pMuzz2TempEnt->entity.curstate.rendercolor.g = 150;
				pMuzz2TempEnt->entity.curstate.rendercolor.b = 100;
			}

		}
		break;

	case 5151:
		{
			EV_GunSmoke2((float*)&entity->attachment[0], GUNSMOKE_WHITE_MEDIUM);
		}
		break;

	case 5187:
		{

			DynamicLight((float *)&entity->attachment[0], 90, 180,90,240, 0.1, 0.0);

			pMuzz1TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0],  forward * 12 + right * gEngfuncs.pfnRandomLong(-3,3) + up * gEngfuncs.pfnRandomLong(-3,3),
				0.5,//scale
				iMuzz2,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.05,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz1TempEnt)
			{ 
				pMuzz1TempEnt->fadeSpeed = 15;
				pMuzz1TempEnt->entity.curstate.framerate = 20;//20
				pMuzz1TempEnt->entity.curstate.renderamt = 240;
				pMuzz1TempEnt->entity.curstate.rendercolor.r = 180;
				pMuzz1TempEnt->entity.curstate.rendercolor.g = 90;
				pMuzz1TempEnt->entity.curstate.rendercolor.b = 240;
			}

		}
		break;

	case 5009:
		{
			//**********************************
			//**** MUZZLE FLASHES CODE *********
			//**********************************
			DynamicLight((float *)&entity->attachment[0], 120, 200,200,250, 0.1, 0.0);
			if ( iSmoke != 0 )
			{
				EV_GunSmoke2((float*)&entity->attachment[0], GUNSMOKE_WHITE_MEDIUM);
			}
			//				1 MUZZLE FLASH
			pMuzz1TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0],  forward * 5 + right * 0 + up * 0,
				0.15,//scale
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz1TempEnt)
			{ 
				pMuzz1TempEnt->fadeSpeed = 10;
				pMuzz1TempEnt->entity.curstate.framerate = 10;//20
				pMuzz1TempEnt->entity.curstate.renderamt = 222;
				pMuzz1TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz1TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz1TempEnt->entity.curstate.rendercolor.b = 255;
			}
			//				2 MUZZLE FLASH
			pMuzz2TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0], forward * 10 + right * 0 + up * 0,
				0.10,
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz2TempEnt)
			{ 
				pMuzz2TempEnt->fadeSpeed = 10;
				pMuzz2TempEnt->entity.curstate.framerate = 10;//20
				pMuzz2TempEnt->entity.curstate.renderamt = 222;
				pMuzz2TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz2TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz2TempEnt->entity.curstate.rendercolor.b = 255;
			}
			//				3 MUZZLE FLASH
			pMuzz3TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[0], forward * 15 + right * 0 + up * 0,
				0.05,
				iMuzz3,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz3TempEnt)
			{ 
				pMuzz3TempEnt->fadeSpeed = 10;
				pMuzz3TempEnt->entity.curstate.framerate = 10;//20
				pMuzz3TempEnt->entity.curstate.renderamt = 222;
				pMuzz3TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz3TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz3TempEnt->entity.curstate.rendercolor.b = 255;
			}
			//**********************************
			//**** MUZZLE FLASHES CODE *********
			//**********************************

		}
		break;

	case 5010:
		{
			//**********************************
			//**** MUZZLE FLASHES CODE *********
			//**********************************

			//				1 MUZZLE FLASH
			pMuzz1TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[1],  forward * 5 + right * 0 + up * 0,
				0.15,//scale
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz1TempEnt)
			{ 
				pMuzz1TempEnt->fadeSpeed = 10;
				pMuzz1TempEnt->entity.curstate.framerate = 10;//20
				pMuzz1TempEnt->entity.curstate.renderamt = 222;
				pMuzz1TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz1TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz1TempEnt->entity.curstate.rendercolor.b = 155;
			}
			//				2 MUZZLE FLASH
			pMuzz2TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[1], forward * 10 + right * 0 + up * 0,
				0.10,
				iMuzz1,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz2TempEnt)
			{ 
				pMuzz2TempEnt->fadeSpeed = 10;
				pMuzz2TempEnt->entity.curstate.framerate = 10;//20
				pMuzz2TempEnt->entity.curstate.renderamt = 222;
				pMuzz2TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz2TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz2TempEnt->entity.curstate.rendercolor.b = 155;
			}
			//				3 MUZZLE FLASH
			pMuzz3TempEnt = gEngfuncs.pEfxAPI->R_TempSprite( (float *)&entity->attachment[1], forward * 15 + right * 0 + up * 0,
				0.05,
				iMuzz3,
				kRenderTransAdd,
				kRenderFxNone,
				1,//alpha?
				0.1,//life
				FTENT_SPRANIMATE| FTENT_FADEOUT);

			if(pMuzz3TempEnt)
			{ 
				pMuzz3TempEnt->fadeSpeed = 10;
				pMuzz3TempEnt->entity.curstate.framerate = 10;//20
				pMuzz3TempEnt->entity.curstate.renderamt = 222;
				pMuzz3TempEnt->entity.curstate.rendercolor.r = 155;
				pMuzz3TempEnt->entity.curstate.rendercolor.g = 155;
				pMuzz3TempEnt->entity.curstate.rendercolor.b = 155;
			}
			//**********************************
			//**** MUZZLE FLASHES CODE *********
			//**********************************

		}
		break;

	case 5014:
		{
			cl_entity_t *view = gEngfuncs.GetViewModel();
			 view->curstate.skin = atoi(event->options);
		}
		break;

	case 5015: 
		gEngfuncs.pEfxAPI->R_Implosion( (float *)&entity->attachment[0], 50, 10, 0.2 );
		break;

	default:
		break;
	}
}

/*
=================
CL_UpdateTEnts

Simulation and cleanup of temporary entities
=================
*/
void DLLEXPORT HUD_TempEntUpdate (
	double frametime,   // Simulation time
	double client_time, // Absolute time on client
	double cl_gravity,  // True gravity on client
	TEMPENTITY **ppTempEntFree,   // List of freed temporary ents
	TEMPENTITY **ppTempEntActive, // List 
	int		( *Callback_AddVisibleEntity )( cl_entity_t *pEntity ),
	void	( *Callback_TempEntPlaySound )( TEMPENTITY *pTemp, float damp ) )
{
	static int gTempEntFrame = 0;
	int			i;
	TEMPENTITY	*pTemp, *pnext, *pprev;
	float		/*freq,*/ gravity, gravitySlow, life, fastFreq;
	g_cl_gravity = cl_gravity;// XDM3035

	// Nothing to simulate
	if( !*ppTempEntActive )	
		return;

	// in order to have tents collide with players, we have to run the player prediction code so
	// that the client has the player list. We run this code once when we detect any COLLIDEALL 
	// tent, then set this BOOL to true so the code doesn't get run again if there's more than
	// one COLLIDEALL ent for this update. (often are).
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( -1 );

	// !!!BUGBUG	-- This needs to be time based
	gTempEntFrame = ( gTempEntFrame + 1 ) & 31;

	pTemp = *ppTempEntActive;

	// !!! Don't simulate while paused....  This is sort of a hack, revisit.
	if( frametime <= 0 )
	{
		while( pTemp )
		{
			if( !( pTemp->flags & FTENT_NOMODEL ) )
			{
				Callback_AddVisibleEntity( &pTemp->entity );
			}
			pTemp = pTemp->next;
		}
		goto finish;
	}

	pprev = NULL;
	//freq = client_time * 0.01;
	fastFreq = client_time * 5.5;
	gravity = -frametime * cl_gravity;
	gravitySlow = gravity * 0.5f;

	while( pTemp )
	{
		int active;

		active = 1;

		life = pTemp->die - (float)client_time;
		pnext = pTemp->next;
		if( life < 0 )
		{
			if( pTemp->flags & FTENT_FADEOUT )
			{
				if( pTemp->entity.curstate.rendermode == kRenderNormal)
					pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.renderamt = pTemp->entity.baseline.renderamt * ( 1 + life * pTemp->fadeSpeed );
				if( pTemp->entity.curstate.renderamt <= 0 )
					active = 0;
			}
			else 
				active = 0;
		}
		if( !active )		// Kill it
		{
			pTemp->next = *ppTempEntFree;
			*ppTempEntFree = pTemp;
			if( !pprev )	// Deleting at head of list
				*ppTempEntActive = pnext;
			else
				pprev->next = pnext;
		}
		else
		{
			pprev = pTemp;

			VectorCopy( pTemp->entity.origin, pTemp->entity.prevstate.origin );

			if( pTemp->flags & FTENT_SPARKSHOWER )
			{
				// Adjust speed if it's time
				// Scale is next think time
				if( client_time > pTemp->entity.baseline.scale )
				{
					// Show Sparks
					gEngfuncs.pEfxAPI->R_SparkEffect( pTemp->entity.origin, 8, -200, 200 );

					// Reduce life
					pTemp->entity.baseline.framerate -= 0.1f;

					if( pTemp->entity.baseline.framerate <= 0.0f )
					{
						pTemp->die = client_time;
					}
					else
					{
						// So it will die no matter what
						pTemp->die = client_time + 0.5;

						// Next think
						pTemp->entity.baseline.scale = client_time + 0.1;
					}
				}
			}
			else if( pTemp->flags & FTENT_PLYRATTACHMENT )
			{
				cl_entity_t *pClient;

				pClient = gEngfuncs.GetEntityByIndex( pTemp->clientIndex );

				VectorAdd( pClient->origin, pTemp->tentOffset, pTemp->entity.origin );
			}
			else if( pTemp->flags & FTENT_SINEWAVE )
			{
				pTemp->x += pTemp->entity.baseline.origin[0] * (float)frametime;
				pTemp->y += pTemp->entity.baseline.origin[1] * (float)frametime;

				pTemp->entity.origin[0] = pTemp->x + sin( pTemp->entity.baseline.origin[2] + client_time * pTemp->entity.prevstate.frame ) * ( 10 * pTemp->entity.curstate.framerate );
				pTemp->entity.origin[1] = pTemp->y + sin( pTemp->entity.baseline.origin[2] + fastFreq + 0.7f ) * ( 8 * pTemp->entity.curstate.framerate );
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * frametime;
			}
			else if( pTemp->flags & FTENT_SPIRAL )
			{
				/*float s, c;
				s = sin( pTemp->entity.baseline.origin[2] + fastFreq );
				c = cos( pTemp->entity.baseline.origin[2] + fastFreq );*/

				pTemp->entity.origin[0] += pTemp->entity.baseline.origin[0] * (float)frametime + 8 * sin( client_time * 20 + (size_t)pTemp );
				pTemp->entity.origin[1] += pTemp->entity.baseline.origin[1] * (float)frametime + 4 * sin( client_time * 30 + (size_t)pTemp );
				pTemp->entity.origin[2] += pTemp->entity.baseline.origin[2] * (float)frametime;
			}
			else 
			{
				for( i = 0; i < 3; i++ )
					pTemp->entity.origin[i] += pTemp->entity.baseline.origin[i] * (float)frametime;
			}
			
			if( pTemp->flags & FTENT_SPRANIMATE )
			{
				pTemp->entity.curstate.frame += (float)frametime * pTemp->entity.curstate.framerate;
				if( pTemp->entity.curstate.frame >= pTemp->frameMax )
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)( pTemp->entity.curstate.frame );

					if( !( pTemp->flags & FTENT_SPRANIMATELOOP ) )
					{
						// this animating sprite isn't set to loop, so destroy it.
						pTemp->die = client_time;
						pTemp = pnext;
						continue;
					}
				}
			}
			else if( pTemp->flags & FTENT_SPRCYCLE )
			{
				pTemp->entity.curstate.frame += frametime * 10;
				if( pTemp->entity.curstate.frame >= pTemp->frameMax )
				{
					pTemp->entity.curstate.frame = pTemp->entity.curstate.frame - (int)( pTemp->entity.curstate.frame );
				}
			}
// Experiment
#if 0
			if( pTemp->flags & FTENT_SCALE )
				pTemp->entity.curstate.framerate += 20.0 * ( frametime / pTemp->entity.curstate.framerate );
#endif

			if( pTemp->flags & FTENT_ROTATE )
			{
				pTemp->entity.angles[0] += pTemp->entity.baseline.angles[0] * (float)frametime;
				pTemp->entity.angles[1] += pTemp->entity.baseline.angles[1] * (float)frametime;
				pTemp->entity.angles[2] += pTemp->entity.baseline.angles[2] * (float)frametime;

				VectorCopy( pTemp->entity.angles, pTemp->entity.latched.prevangles );
			}

			if( pTemp->flags & ( FTENT_COLLIDEALL | FTENT_COLLIDEWORLD ) )
			{
				vec3_t	traceNormal( 0.0f, 0.0f, 0.0f );
				float	traceFraction = 1;

				if( pTemp->flags & FTENT_COLLIDEALL )
				{
					pmtrace_t pmtrace;
					physent_t *pe;

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

					gEngfuncs.pEventAPI->EV_PlayerTrace( pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX, -1, &pmtrace );

					if( pmtrace.fraction != 1 )
					{
						pe = gEngfuncs.pEventAPI->EV_GetPhysent( pmtrace.ent );

						if( !pmtrace.ent || ( pe->info != pTemp->clientIndex ) )
						{
							traceFraction = pmtrace.fraction;
							VectorCopy( pmtrace.plane.normal, traceNormal );

							if( pTemp->hitcallback )
							{
								(*pTemp->hitcallback)( pTemp, &pmtrace );
							}
						}
					}
				}
				else if( pTemp->flags & FTENT_COLLIDEWORLD )
				{
					pmtrace_t pmtrace;

					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

					gEngfuncs.pEventAPI->EV_PlayerTrace( pTemp->entity.prevstate.origin, pTemp->entity.origin, PM_STUDIO_BOX | PM_WORLD_ONLY, -1, &pmtrace );

					if( pmtrace.fraction != 1 )
					{
						traceFraction = pmtrace.fraction;
						VectorCopy( pmtrace.plane.normal, traceNormal );

						if( pTemp->flags & FTENT_SPARKSHOWER )
						{
							// Chop spark speeds a bit more
							//
							VectorScale( pTemp->entity.baseline.origin, 0.6f, pTemp->entity.baseline.origin );

							if( Length( pTemp->entity.baseline.origin ) < 10 )
							{
								pTemp->entity.baseline.framerate = 0.0;								
							}
						}

						if( pTemp->hitcallback )
						{
							(*pTemp->hitcallback)( pTemp, &pmtrace );
						}
					}
				}
				
				if( traceFraction != 1 )	// Decent collision now, and damping works
				{
					float  proj, damp;

					// Place at contact point
					VectorMA( pTemp->entity.prevstate.origin, traceFraction * (float)frametime, pTemp->entity.baseline.origin, pTemp->entity.origin );
					// Damp velocity
					damp = pTemp->bounceFactor;
					if( pTemp->flags & ( FTENT_GRAVITY | FTENT_SLOWGRAVITY ) )
					{
						damp *= 0.5f;
						if( traceNormal[2] > 0.9f )		// Hit floor?
						{
							if( pTemp->entity.baseline.origin[2] <= 0 && pTemp->entity.baseline.origin[2] >= gravity*3 )
							{
								damp = 0;		// Stop
								pTemp->flags &= ~( FTENT_ROTATE | FTENT_GRAVITY | FTENT_SLOWGRAVITY | FTENT_COLLIDEWORLD | FTENT_SMOKETRAIL);
								pTemp->entity.angles[0] = 0;
								pTemp->entity.angles[2] = 0;
							}
						}
					}

					if( pTemp->hitSound )
					{
						Callback_TempEntPlaySound( pTemp, damp );
					}

					if( pTemp->flags & FTENT_COLLIDEKILL )
					{
						// die on impact
						pTemp->flags &= ~FTENT_FADEOUT;	
						pTemp->die = client_time;			
					}
					else
					{
						// Reflect velocity
						if( damp != 0 )
						{
							proj = DotProduct( pTemp->entity.baseline.origin, traceNormal );
							VectorMA( pTemp->entity.baseline.origin, -proj * 2, traceNormal, pTemp->entity.baseline.origin );
							// Reflect rotation (fake)

							pTemp->entity.angles[1] = -pTemp->entity.angles[1];
						}
						
						if( damp != 1 )
						{

							VectorScale( pTemp->entity.baseline.origin, damp, pTemp->entity.baseline.origin );
							VectorScale( pTemp->entity.angles, 0.9, pTemp->entity.angles );
						}
					}
				}
			}

			if( ( pTemp->flags & FTENT_FLICKER ) && gTempEntFrame == pTemp->entity.curstate.effects )
			{
				dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				VectorCopy( pTemp->entity.origin, dl->origin );
				dl->radius = 60;
				dl->color.r = 255;
				dl->color.g = 120;
				dl->color.b = 0;
				dl->die = client_time + 0.01;
			}

			if( pTemp->flags & FTENT_SMOKETRAIL )
			{
				gEngfuncs.pEfxAPI->R_RocketTrail( pTemp->entity.prevstate.origin, pTemp->entity.origin, 1 );
			}

			if( pTemp->flags & FTENT_GRAVITY )
				pTemp->entity.baseline.origin[2] += gravity;
			else if( pTemp->flags & FTENT_SLOWGRAVITY )
				pTemp->entity.baseline.origin[2] += gravitySlow;

			if( pTemp->flags & FTENT_CLIENTCUSTOM )
			{
				if( pTemp->callback )
				{
					(*pTemp->callback)( pTemp, frametime, client_time );
				}
			}

			// Cull to PVS (not frustum cull, just PVS)
			if( !( pTemp->flags & FTENT_NOMODEL ) )
			{
				if( !Callback_AddVisibleEntity( &pTemp->entity ) )
				{
					if( !( pTemp->flags & FTENT_PERSIST ) )
					{
						pTemp->die = client_time;			// If we can't draw it this frame, just dump it.
						pTemp->flags &= ~FTENT_FADEOUT;	// Don't fade out, just die
					}
				}
			}
		}
		pTemp = pnext;
	}
finish:
	// Restore state info
	gEngfuncs.pEventAPI->EV_PopPMStates();
}

/*
=================
HUD_GetUserEntity

If you specify negative numbers for beam start and end point entities, then
  the engine will call back into this function requesting a pointer to a cl_entity_t 
  object that describes the entity to attach the beam onto.

Indices must start at 1, not zero.
=================
*/
cl_entity_t DLLEXPORT *HUD_GetUserEntity( int index )
{
#if BEAM_TEST
	// None by default, you would return a valic pointer if you create a client side
	//  beam and attach it to a client side entity.
	if( index > 0 && index <= 1 )
	{
		return &beams[index];
	}
	else
	{
		return NULL;
	}
#else
	return NULL;
#endif
}

//Fixes Server and Client ent.indexes! Ghoul [BB], XASH
cl_entity_t *UTIL_GetClientEntityWithServerIndex( int sv_index )
{
	cl_entity_t *e;

	for (int ic=1;ic<MAX_EDICTS;ic++)
	{
		e = gEngfuncs.GetEntityByIndex( ic );
		if (!e)
			break;

		if (!e->model)
			continue;

		if (e->curstate.colormap == sv_index)
			return e;
	}
	return NULL;
}