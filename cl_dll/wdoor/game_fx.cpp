#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"
#include "con_nprint.h"
#include "event_api.h"
#include "pm_defs.h"
#include "in_defs.h"
#include "pm_materials.h"
#include "game_fx.h"
#include "rain.h"
#include "r_particle.h"

#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
#include "ParticleSystem.h"
#include "RSBeamStar.h"
#include "RSCylinder.h"
#include "RSTeleparts.h"
#include "RSDisk.h"
#include "PSSparks.h"
#include "RSLight.h"
#include "PSTrail.h"
#include "PSDrips.h"
#include "PSFlatTrail.h"
#include "PSBubbleTrail.h"
#include "RSBeam.h"
#include "PSBlood.h"
#include "PSSpawnEffect.h"
#include "PSBubbles.h"
#include "PSBlastCone.h"
#include "PSGravityPart.h"
#include "PSWallGibs.h"
#include "PSSparkShower.h"
#include "RSSprite.h"

vec3_t forward, right, up;//I can't make these vectors local, because I got a bug with shell-casings and clip casings!!
vec3_t rotate;//used for water splashes, MUST be global now (till we fix it)

void V_PunchAxis( int axis, float punch );

//===============================
// Client-side water splash
// Uses "rotate" as return value
//===============================
int WaterSurfaceCoords ( vec3_t start, vec3_t end, int StartEnv, int bubbles = 1 )
{
	vec3_t a, b, dir;
	int EndEnv;

	if ( (EndEnv = gEngfuncs.PM_PointContents(end, NULL)) != StartEnv )
	{
		VectorSubtract(end, start, dir);
		float length = Length(dir);
		VectorCopy(start, a);
		do
		{
			VectorScale (dir, 0.5, dir);
			length *= 0.5;
			VectorAdd(a, dir, b);

			if ( gEngfuncs.PM_PointContents(b, NULL) == StartEnv )
				VectorCopy(b, a);
		}
		while( length > 5 );

		if (bubbles)
		{
			if (StartEnv == CONTENTS_WATER)
				g_pRenderManager->AddSystem(new CPSBubbleTrail(start, a, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, gEngfuncs.pfnRandomFloat(3,9), gEngfuncs.pfnRandomFloat(15,40), 0));
			else
				g_pRenderManager->AddSystem(new CPSBubbleTrail(a, end, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, gEngfuncs.pfnRandomFloat(3,9), gEngfuncs.pfnRandomFloat(15,40), 0));
		}
		VectorCopy (a, rotate);
		return EndEnv;
	}
	else if (bubbles && (StartEnv == CONTENTS_WATER))
	{
		g_pRenderManager->AddSystem(new CPSBubbleTrail(start, end, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, gEngfuncs.pfnRandomFloat(3,9), gEngfuncs.pfnRandomFloat(15,40), 0));
	}
	return 0;
}

//===========================
//Draws dynamic scaling model
// used for explosions
//===========================
void EV_BlastModel( float *origin, int body, float StartScale, float scaleADD, float renderamtADD )
{
	TEMPENTITY *BlastModel = NULL;
	BlastModel = gEngfuncs.pEfxAPI->R_TempModel( origin, Vector(0,0,0), Vector(0,0,0), 20, gEngfuncs.pEventAPI->EV_FindModelIndex("models/explosions.mdl"), TE_BOUNCE_NULL );

	if(!BlastModel)
		return;

	BlastModel->entity.curstate.body = body;			
	BlastModel->entity.curstate.scale = StartScale;
	BlastModel->entity.curstate.rendermode = kRenderTransAdd;
	BlastModel->entity.curstate.renderamt = 255;
	BlastModel->entity.baseline.animtime = scaleADD;
	BlastModel->entity.baseline.framerate = renderamtADD;

	BlastModel->flags &= ~FTENT_GRAVITY;
	BlastModel->flags |= ( FTENT_CLIENTCUSTOM );
	BlastModel->callback = EV_BlastModelCallback;
}

void EV_BlastModelCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	ent->entity.curstate.scale += ent->entity.baseline.animtime;
	ent->entity.curstate.renderamt -= ent->entity.baseline.framerate;

	if ( ent->entity.curstate.renderamt < 5 )
	{
		ent->die = gEngfuncs.GetClientTime();
	}
}

void EV_GibTouch ( struct tempent_s *ent, struct pmtrace_s *ptr )
{
	if (ent->entity.baseline.iuser1 <=0 )
		return;

	ent->entity.baseline.iuser1--;

	if (ent->entity.baseline.iuser2 == GIBBED_BODY)
	{
		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(2,3), gEngfuncs.pfnRandomFloat(35,70), ptr->endpos, Vector(0,0,1), Vector(0.2,0.2,1.0), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		if (gEngfuncs.pfnRandomLong(0, 3) > 0)
		return;

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/bustflesh1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/bustflesh2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}

	if (ent->entity.baseline.iuser2 == GIBBED_FROZEN)
	{
		if (gEngfuncs.pfnRandomLong(0, 3) > 0)
		return;

		switch (gEngfuncs.pfnRandomLong(0,2))
		{
			case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/glass1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/glass2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
			case 2 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/glass3.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}
	if (ent->entity.baseline.iuser2 == GIBBED_IGNITE)
	{
		return;
	}
}

//======================
//Bullet impact wallgibs
//======================
void EV_Wallgib( float *origin, float random, float scale, float life, int count, int skin, char flags)
{
	vec3_t velocity;

	for (int i = 0; i < count; i++)
	{
		vec3_t dir = (VectorRandom() + VectorRandom());
		VectorNormalize(dir);
		VectorScale(dir, random, velocity);

		TEMPENTITY *WallGib = gEngfuncs.pEfxAPI->R_TempModel(origin, velocity, vec3_origin, life, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), 0);
		WallGib->entity.curstate.skin = skin;			
		WallGib->entity.curstate.scale = scale;
		WallGib->flags |= (flags);

	    if (WallGib->entity.curstate.skin == GIB_WOOD)
		{
			WallGib->entity.curstate.body = gEngfuncs.pfnRandomLong(8,13);			
			WallGib->entity.baseline.sequence = 70;
		}
		else if (WallGib->entity.curstate.skin == GIB_FLESH)
		{
			WallGib->entity.curstate.body = gEngfuncs.pfnRandomLong(0,7);			
			WallGib->entity.baseline.sequence = 666;
			WallGib->entity.baseline.iuser2 = GIBBED_BODY;
			WallGib->entity.baseline.iuser1 = 5;
			WallGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			WallGib->hitcallback = EV_GibTouch;
		}
		else if (WallGib->entity.curstate.skin == GIB_METALL)
		{
			WallGib->entity.curstate.body = gEngfuncs.pfnRandomLong(20,27);			
		}
		else if (WallGib->entity.curstate.skin == GIB_GRATE)
		{
			WallGib->entity.curstate.body = gEngfuncs.pfnRandomLong(28,35);			
		}
		else
			WallGib->entity.curstate.body = gEngfuncs.pfnRandomLong(0,7);			
	}
}

//===================================
//func_breakable & func_pushable gibs
//===================================
void EV_BreakableGib( float *origin, float random, float scale, float life, int skin, int count, char flags)
{
	vec3_t velocity;

	for (int i = 0; i < count; i++)
	{
		vec3_t dir = (VectorRandom() + VectorRandom());
		VectorNormalize(dir);
		VectorScale(dir, random, velocity);

		TEMPENTITY *BreakGib = gEngfuncs.pEfxAPI->R_TempModel(origin, velocity, vec3_origin, life, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_worldgibs.mdl"), 0);
		BreakGib->entity.curstate.scale = scale;
		BreakGib->entity.curstate.skin = skin;
		BreakGib->flags |= (flags);

		if (BreakGib->entity.curstate.skin == SHARD_GLASS || BreakGib->entity.curstate.skin == SHARD_UNBR_GLASS)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(0,5);			
			BreakGib->entity.curstate.rendermode = kRenderTransTexture;
			BreakGib->entity.curstate.renderamt = 128;
		}
		else if (BreakGib->entity.curstate.skin == SHARD_WOOD)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(6,11);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_METALL)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(12,21);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_FLESH)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(22,25);			
			BreakGib->entity.baseline.iuser2 = GIBBED_BODY;
			BreakGib->entity.baseline.sequence = 666;
			BreakGib->entity.baseline.iuser1 = 5;
			BreakGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			BreakGib->hitcallback = EV_GibTouch;
		}
		else if (BreakGib->entity.curstate.skin == SHARD_CONCRETE_BLOCK)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(26,31);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_CEILING_TILE)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(32,35);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_COMPUTER)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(36,45);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_ROCK)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(46,51);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_GRATE)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(52,57);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_VENT)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(58,65);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_BRICK)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(66,73);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_CONCRETE)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(74,81);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_ICE)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(82,86);			
		}
		else if (BreakGib->entity.curstate.skin == SHARD_SANDWALL)
		{
			BreakGib->entity.curstate.body = gEngfuncs.pfnRandomLong(87,94);			
		}
	}
}

//====================
//Bullet shell casings
//====================
void EV_EjectBrass( float *origin, float rotation, int soundtype, int body, int iLife )
{
	vec3_t ShellVelocity, src;

	float fR, fU;
	fR = gEngfuncs.pfnRandomFloat(-60, -100);
	fU = gEngfuncs.pfnRandomFloat(100, 140);

	VectorClear(src);
	src[1] = rotation;
	AngleVectors (src, forward, right, up);

	for (int i = 0; i < 3; i++ )
		ShellVelocity[i] = -forward[i] * fR + up[i] * fU + right[i] * gEngfuncs.pfnRandomFloat(50,80);

	TEMPENTITY *shell = gEngfuncs.pEfxAPI->R_TempModel( origin, ShellVelocity, src, 3.0, gEngfuncs.pEventAPI->EV_FindModelIndex ("models/w_shells_all.mdl"), soundtype );
	shell->entity.curstate.body = body;			

	//if (gHUD.SmokingShells->value > 0)
//	{
	//	shell->flags |= ( FTENT_CLIENTCUSTOM );
//		shell->callback = EV_EjectBrassCallback;
//		shell->entity.curstate.framerate = gEngfuncs.pfnRandomLong(70,120);
//	}
}

void EV_EjectBrassCallback ( struct tempent_s *ent, float frametime, float currenttime )
{
	if ( ent->entity.curstate.framerate <= 1)
	return;

	if ( gEngfuncs.PM_PointContents(ent->entity.origin, NULL ) == CONTENTS_WATER)
	return;

	g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, ent->entity.origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 6, 15, 100,100,100, 0.08, -0.09, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 0.1), 0, -1);
	ent->entity.curstate.framerate -=1;
}

//=================
//Empty gun clips
//=================
void EV_BrassGunClip( float *origin, int body )
{
	vec3_t ShellVelocity, src;

	float fR, fU;
	fR = gEngfuncs.pfnRandomFloat( 50, 70 );
	fU = gEngfuncs.pfnRandomFloat( 100, 150 );

	for (int i = 0; i < 3; i++ )
		ShellVelocity[i] = right[i] * fR + up[i] * fU + forward[i] * 35;

	VectorClear(src);
	src[1] = gEngfuncs.pfnRandomLong(-20,50);
	TEMPENTITY *clip = gEngfuncs.pEfxAPI->R_TempModel( origin, ShellVelocity, src, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex ("models/w_clips_all.mdl"), TE_BOUNCE_NULL );
	clip->entity.curstate.body = body;			

	clip->entity.baseline.iuser1 = 15;
	clip->flags |= (FTENT_COLLIDEWORLD | FTENT_CLIENTCUSTOM);
	clip->hitcallback = EV_ClipTouch;
}

void EV_ClipTouch ( struct tempent_s *ent, struct pmtrace_s *ptr )
{
	if (ent->entity.baseline.iuser1 <=0 )
		return;

	ent->entity.baseline.iuser1--;

        switch (gEngfuncs.pfnRandomLong(0, 2))
        {
                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/metal1.wav", 0.5, ATTN_NORM, 0, 125 ); break;
                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/metal2.wav", 0.5, ATTN_NORM, 0, 125 ); break;
                case 2 : gEngfuncs.pEventAPI->EV_PlaySound( 0, ptr->endpos, CHAN_STATIC, "debris/metal3.wav", 0.5, ATTN_NORM, 0, 125 ); break;
	}
}

//================//
//Gun Barrel smoke//
//================//
void EV_GunSmoke(vec3_t origin, int iSmokeType)
{
	switch (iSmokeType)
	{
	case GUNSMOKE_WHITE_SMALLEST:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(4, 15, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 5, 255,255,255, 0.2, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(1,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_WHITE_SMALL:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(4, 20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 10, 255,255,255, 0.2, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
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
			g_pRenderManager->AddSystem(new CPSBlastCone(4, 15, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 5, 0,0,0, 0.1, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			g_pRenderManager->AddSystem(new CPSBubbles(20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(1,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	break;
	case GUNSMOKE_BLACK_SMALL:
		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new CPSBlastCone(4, 20, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 10, 0,0,0, 0.1, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
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

//============================//
//Rocket & Grenade hit effects//
//============================//
float SpawnShards(char chTextureType, vec3_t origin, int RocketType, int InWater)
{
	switch (chTextureType)
	{
	case CHAR_TEX_ENERGYSHIELD:
		switch (RocketType)
		{
			case BULLET_BOLT:
				g_pRenderManager->AddSystem(new CPSBlastCone(10, 100, origin, origin, Vector(1,1,1), 20, 50, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;

			case BULLET_SMALEXP:
				g_pRenderManager->AddSystem(new CPSBlastCone(15, 130, origin, origin, Vector(1,1,1), 25, 75, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;

			case BULLET_NORMEXP:
				g_pRenderManager->AddSystem(new CPSBlastCone(20, 150, origin, origin, Vector(1,1,1), 25, 100, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;
			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				g_pRenderManager->AddSystem(new CPSBlastCone(25, 175, origin, origin, Vector(1,1,1), 35, 100, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;
		}
		break;
	case CHAR_TEX_ASPHALT:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_ASPHALT, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_BRICK:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_BRICK, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_SAND:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_SAND, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_SANDWALL:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_SANDWALL, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_ROCK:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_ROCK, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_BP_CONCRETE:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_CONCRETE, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_METAL:
	case CHAR_TEX_BP_METAL:
	case CHAR_TEX_COMPUTER:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(50, 100),-400,400);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(100, 150),-600,600);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(150, 180),-800,800);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(3.6,5.2), gHUD.TempEntLifeCvar->value, 7, GIB_METALL, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(200, 230),-1000,1000);
			break;
		}
		break;

	case CHAR_TEX_VENT:
	case CHAR_TEX_GRATE:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(50, 80),-400,400);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(80, 120),-600,600);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(120, 150),-800,800);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(3.6,5.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRATE, 0);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(150, 180),-1000,1000);
			break;
		}
		break;

	case CHAR_TEX_WOOD:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(50, 80),-400,400);
				if (!InWater)
				{
					g_pRenderManager->AddSystem(new PSGravityPart(7, 5, gEngfuncs.pfnRandomFloat(80, 120), origin, origin, Vector(1,1,1), 4, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(7, 7.5, gEngfuncs.pfnRandomFloat(110, 150), origin, origin, Vector(1,1,1), 6, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(80, 120),-600,600);
				if (!InWater)
				{
					g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(100, 200), origin, origin, Vector(1,1,1), 4, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(10, 7.5, gEngfuncs.pfnRandomFloat(150, 300), origin, origin, Vector(1,1,1), 6, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(10, 10, gEngfuncs.pfnRandomFloat(200, 400), origin, origin, Vector(1,1,1), 8, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(120, 150),-800,800);
				if (!InWater)
				{
					g_pRenderManager->AddSystem(new PSGravityPart(7, 5, gEngfuncs.pfnRandomFloat(100, 200), origin, origin, Vector(1,1,1), 4, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(7, 7.5, gEngfuncs.pfnRandomFloat(150, 300), origin, origin, Vector(1,1,1), 6, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(7, 10, gEngfuncs.pfnRandomFloat(200, 400), origin, origin, Vector(1,1,1), 8, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(7, 12.5, gEngfuncs.pfnRandomFloat(250, 500), origin, origin, Vector(1,1,1), 10, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(3.6,5.2), gHUD.TempEntLifeCvar->value, 7, GIB_WOOD, FTENT_SMOKETRAIL);
				gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(150, 180),-1000,1000);
				if (!InWater)
				{
					g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(100, 200), origin, origin, Vector(1,1,1), 4, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(10, 7.5, gEngfuncs.pfnRandomFloat(150, 300), origin, origin, Vector(1,1,1), 6, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(10, 10, gEngfuncs.pfnRandomFloat(200, 400), origin, origin, Vector(1,1,1), 8, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(10, 12.5, gEngfuncs.pfnRandomFloat(250, 500), origin, origin, Vector(1,1,1), 10, -0.8, 255,255,255, 1, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			break;
		}
		break;

	case CHAR_TEX_GLASS:
	case CHAR_TEX_BP_GLASS:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(1.2,2.2), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(1.8,3.0), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(2.4,3.8), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(3.0,4.6), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(3.6,5.2), gHUD.TempEntLifeCvar->value, 7, GIB_GLASS, 0);
			break;
		}
		break;

	case CHAR_TEX_DIRT:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_DIRT, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_SNOW:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_SNOW, 0);
			break;
		}
		break;

	case CHAR_TEX_SNOWROCK:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_SNOWROCK, 0);
			break;
		}
		break;

	case CHAR_TEX_GRASS:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_GRASS, 0);
			break;
		}
		break;

		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_LEAVES, 0);
			break;
		}
		break;

	case CHAR_TEX_TILE:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_TILE, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_FLESH:
		switch (RocketType)
		{
			case BULLET_BOLT:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
			break;

			case BULLET_SMALEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
			break;

			case BULLET_NORMEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
			break;

			case BULLET_HIGHEXP:
			case BULLET_MEGAEXP:
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(300,500), gEngfuncs.pfnRandomFloat(0.4,0.8), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(350,600), gEngfuncs.pfnRandomFloat(0.6,1.2), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(400,700), gEngfuncs.pfnRandomFloat(0.8,1.6), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(450,800), gEngfuncs.pfnRandomFloat(1.0,2.0), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
				EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(1.2,2.4), gHUD.TempEntLifeCvar->value, 7, GIB_FLESH, FTENT_SMOKETRAIL);
			break;
		}
		break;

	case CHAR_TEX_EMPTY:
		break;
	default:
		return 0;
		break;
	}
	return 1;
}

//=====================//
//Bullet impact effects//
//=====================//
float PlayCrowbarSound(vec3_t origin, float volume)
{
       gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/cbar_hit1.wav", volume, ATTN_NORM, 0, PITCH_NORM );
	return 1;
}

float PlayKnifeSound(vec3_t origin, float volume)
{
       gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/knife_hitwall1.wav", volume, ATTN_NORM, 0, PITCH_NORM );
	return 1;
}

float Impact(char chTextureType, vec3_t origin, vec3_t dir, int iBulletType, int InWater)
{
	if (InWater)
		g_pRenderManager->AddSystem(new CPSBubbles(gEngfuncs.pfnRandomLong(14,21), origin, Vector(0,0,1), Vector(0.2,0.2,0.7), gEngfuncs.pfnRandomFloat(50,100), gEngfuncs.pfnRandomFloat(2,5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

	switch (chTextureType)
	{
	case CHAR_TEX_ENERGYSHIELD:
     	g_pRenderManager->AddSystem(new CPSBlastCone(3, 30, origin, origin, Vector(1,1,1), 10, 25, 128,128,128, 0.5, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		if (!InWater)
		{
		g_pRenderManager->AddSystem(new CPSSparks(5, origin, 0.5, 0.01, 200, 0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_ASPHALT:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
			g_pRenderManager->AddSystem(new CPSBlastCone(3, gEngfuncs.pfnRandomFloat(100,120), origin, dir, Vector(0.2,0.2,0.6), 5, 30, 0,0,0, 0.2, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_BRICK:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
			g_pRenderManager->AddSystem(new CPSBlastCone(3, gEngfuncs.pfnRandomFloat(110,130), origin, dir, Vector(0.2,0.2,0.2), 8, 45, 131,45,27, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_SAND:
		if (!InWater)
		g_pRenderManager->AddSystem(new CPSBlastCone(3, gEngfuncs.pfnRandomFloat(80,100), origin, dir, Vector(0.2,0.2,0.2), 5, 40, 210,140,10, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_8, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		break;

	case CHAR_TEX_SANDWALL:
		if (!InWater)
		g_pRenderManager->AddSystem(new CPSBlastCone(3, gEngfuncs.pfnRandomFloat(110,140), origin, dir, Vector(0.2,0.2,0.2), 5, 30, 212,145,96, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		break;

	case CHAR_TEX_ROCK:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_BulletImpactParticles(origin);
	     	g_pRenderManager->AddSystem(new CPSBlastCone(3, 13, origin, origin, Vector(1,1,1), 8, 15, 73,32,40, 0.2, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_CONCRETE:
	case CHAR_TEX_BP_CONCRETE:
		if (!InWater){
		g_pRenderManager->AddSystem(new CPSBlastCone(3, 120, origin, dir, Vector(0.2,0.2,0.2), 5, 40, 100,100,100, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_VENT:
	case CHAR_TEX_METAL:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(80, 100),-400,400);
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
			DynamicLight(origin, 60, 254,110,25, 0.15, 0.0);
		}
		break;

	case CHAR_TEX_BP_METAL:
	case CHAR_TEX_BP_GLASS:
		if (!InWater)
		{	
			gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(50,70),-200,200);
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
			DynamicLight(origin, 60, 254,110,25, 0.15, 0.0);
		}
		break;

	case CHAR_TEX_GRATE:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(30,50),-50,50);
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
			DynamicLight(origin, 50, 254,110,25, 0.15, 0.0);
		}
		break;

	case CHAR_TEX_WOOD:
		if (!InWater)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(2, 110, origin, dir, Vector(0.2,0.2,0.2), 8, 15, 100,60,0, 0.2, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(2, 110, origin, dir, Vector(0.2,0.2,0.2), 8, 20, 100,60,0, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_COMPUTER:
		if (!InWater)
		{
			gEngfuncs.pEfxAPI->R_StreakSplash(origin, vec3_origin, 6, gEngfuncs.pfnRandomFloat(25,40), 56, -256, 256);
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
			DynamicLight(origin, 60, 254,160,25, 0.15, 0.0);
		}
		break;

	case CHAR_TEX_GLASS:
		if (!InWater)
			gEngfuncs.pEfxAPI->R_SparkShower(origin);
		break;

	case CHAR_TEX_DIRT:
		if (!InWater)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(2, 110, origin, dir, Vector(0.2,0.2,0.2), 8, 15, 100,60,0, 0.2, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(2, 110, origin, dir, Vector(0.2,0.2,0.2), 8, 20, 100,60,0, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_SNOW:
		if (!InWater)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(2, gEngfuncs.pfnRandomFloat(30,50), origin, dir, Vector(0.2,0.2,0.5), 8, 25, 200,200,200, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		break;

	case CHAR_TEX_GRASS:
		break;

	case CHAR_TEX_LEAVES:
		break;

	case CHAR_TEX_TILE:
		if (!InWater)
	    g_pRenderManager->AddSystem(new CPSBlastCone(4, 12, origin, dir, Vector(0.2,0.2,0.4), 8, 14, 128,128,128, 0.2, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		break;

	case CHAR_TEX_SNOWROCK:
		if (!InWater)
		gEngfuncs.pEfxAPI->R_SparkStreaks(origin,gEngfuncs.pfnRandomFloat(10,15),-30,30);
		break;

	case CHAR_TEX_SLOSH:
		break;

	case CHAR_TEX_FLESH:
     	g_pRenderManager->AddSystem(new CPSBlastCone(4, 20, origin, origin, Vector(1,1,1), 5, 25, 90,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		break;

	case CHAR_TEX_EMPTY://No effects!
		break;
	default:
		return 0.0;
		break;
	}

	return 1;
}


//=========//
//DECALS!!!//
//=========//
void EV_DecalTrace(pmtrace_t *pTrace, int index)
{
	if (CVAR_GET_FLOAT("r_decals") > 0 && index > 0)
	{
		if (gEngfuncs.GetEntityByIndex(gEngfuncs.pEventAPI->EV_IndexFromTrace(pTrace))->model->type == mod_brush)
		gEngfuncs.pEfxAPI->R_DecalShoot(gEngfuncs.pEfxAPI->Draw_DecalIndex(index), gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0);
	}
}

int EV_SmalExpDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{ofscorch%d", gEngfuncs.pfnRandomLong(1,6));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_NormExpDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{scorch%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_HighExpDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{bigscorch%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_MegaExpDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{blow", 0);
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_BoltDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{exp_scorch%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_EgonDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{gaussshot1");
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_TeslaDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{ofsmscorch%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_BigshotDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{bigshot%d", gEngfuncs.pfnRandomLong(1,5));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_GaussDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{gausshot%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_LightsaberDecal(char chTextureType)
{
	char decalname[16];
	sprintf(decalname, "{scratch%d", gEngfuncs.pfnRandomLong(1,3));
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}

int EV_BulletDecal(char chTextureType)
{
	char decalname[16];
	switch (chTextureType)
	{
		case CHAR_TEX_BRICK:
		case CHAR_TEX_ROCK:
			sprintf(decalname, "{shot_brick%d", gEngfuncs.pfnRandomLong(1,2));
		break;
		case CHAR_TEX_SAND:
		case CHAR_TEX_SANDWALL:
			sprintf(decalname, "{shot_sand%d", gEngfuncs.pfnRandomLong(1,2));
		break;
		case CHAR_TEX_CONCRETE:
		case CHAR_TEX_BP_CONCRETE:
			sprintf(decalname, "{shot_concrete%d", gEngfuncs.pfnRandomLong(1,5));
		break;
		case CHAR_TEX_DIRT:
		case CHAR_TEX_GRASS:
		case CHAR_TEX_LEAVES:
			sprintf(decalname, "{shot_dirt%d", gEngfuncs.pfnRandomLong(1,2));
		break;
		case CHAR_TEX_METAL:
		case CHAR_TEX_VENT:
		case CHAR_TEX_BP_METAL:
		case CHAR_TEX_BP_GLASS:
			sprintf(decalname, "{shot_metal%d", gEngfuncs.pfnRandomLong(1,4));
		break;
		case CHAR_TEX_GRATE:
		case CHAR_TEX_TILE:
			sprintf(decalname, "{shot%d", gEngfuncs.pfnRandomLong(1, 5));
		break;
		case CHAR_TEX_ASPHALT:
			sprintf(decalname, "{bigshot%d", gEngfuncs.pfnRandomLong(1, 5));
		break;
		case CHAR_TEX_COMPUTER:
			sprintf(decalname, "{break%d", gEngfuncs.pfnRandomLong(1, 3));
		break;
		case CHAR_TEX_GLASS:
			sprintf(decalname, "{shot_glass%d", gEngfuncs.pfnRandomLong(1,5));
		break;
		case CHAR_TEX_WOOD:
			sprintf(decalname, "{shot_wood%d", gEngfuncs.pfnRandomLong(1,2));
		break;
		case CHAR_TEX_SNOW:
		case CHAR_TEX_SNOWROCK:
			sprintf(decalname, "{shot_snow%d", gEngfuncs.pfnRandomLong(1,2));
		break;
		case CHAR_TEX_FLESH:
			sprintf(decalname, "{shot_flesh%d", gEngfuncs.pfnRandomLong(1, 4));
		break;
		case CHAR_TEX_ENERGYSHIELD:
		case CHAR_TEX_EMPTY:
		break;

		default:
		break;
	}
	return gEngfuncs.pEfxAPI->Draw_DecalIndexFromName(decalname);
}


//======================================================================//
// used for WORLD explosion effects (exp.objects, crates, barrels, etc.)//
//======================================================================//
int __MsgFunc_WorldExp(const char *pszName, int iSize, void *pbuf)
{
	pmtrace_t tr;
	vec3_t origin;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	int Scale = READ_SHORT();
	int Smoke = READ_BYTE();
	int Sparks = READ_BYTE();
	int Fireball = READ_BYTE();
	int Ring = READ_BYTE();
	int Decal = READ_BYTE();

	if (Fireball) //Draw normal explosion
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(20+Scale/6, Scale*3, origin, origin, Vector(1,1,1), 10, Scale*2, 100,0,0, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15+Scale/6, Scale*3.5, origin, origin, Vector(1,1,1), 10, Scale*3, 255,167,17, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10+Scale/6, Scale*4, origin, origin, Vector(1,1,1), 10, Scale*3.5, 128,128,128, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		DynamicLight(origin, Scale, 200,128,0, 0.6, 200.0);
		gEngfuncs.pEfxAPI->R_ParticleExplosion( origin );
	}
	else // No FIREBALL
	{
		DynamicLight(origin, Scale, 200,128,0, 0.6, 200.0);
		gEngfuncs.pEfxAPI->R_ParticleExplosion2( origin, 111, 8 );
	}

	if (Sparks)// draw SPARKS
	{
		g_pRenderManager->AddSystem(new CPSSparks(15+Scale/6, origin, 1, 0.02, Scale*3, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(250, 300), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	if (Smoke)// draw SMOKE
		g_pRenderManager->AddSystem(new CPSBlastCone(8, Scale/2, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, Scale, 0,0,0, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

	if (Ring)// draw Ring
		g_pRenderManager->AddSystem(new CRSCylinder(origin, Scale/10, Scale*6, Scale/2, Scale/3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_STEAM, kRenderTransAdd, 215,140,0, 1, -0.5, 0.5));

	if (Decal)// draw DECAL
	{
		vec3_t dir;
		dir.z=125;
		dir=dir.Normalize();
	//	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);	
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(origin, origin+dir*-125, PM_WORLD_ONLY, -1, &tr);
		EV_DecalTrace(&tr, EV_NormExpDecal(CHAR_TEX_CONCRETE));
	}
return 1;
}

//==================//
//Bullet hit effects//
//==================//
int __MsgFunc_ImpBullet(const char *pszName, int iSize, void *pbuf)
{
	pmtrace_s tr;
	vec3_t normal, nail_angles, origin, src, end, angles;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	angles.x = READ_COORD();//this is startpos
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	normal.x = READ_COORD();//this is normal
	normal.y = READ_COORD();
	normal.z = READ_COORD();
	int Surface = READ_BYTE();
	int BulletType = READ_BYTE();
	int TexType = READ_BYTE();

	int iRand = gEngfuncs.pfnRandomLong(0,0x7FFF);
	int InWater = (gEngfuncs.PM_PointContents(origin, NULL ) == CONTENTS_WATER);
	int StartEnv = gEngfuncs.PM_PointContents(angles, NULL);
	int contents = WaterSurfaceCoords(angles, origin, StartEnv);

	int iBlood = CVAR_GET_FLOAT("cl_bloodsmoke");

	if (BulletType == 114){
			if (contents == CONTENTS_WATER || (StartEnv == CONTENTS_WATER && contents == CONTENTS_EMPTY))//water->air or air->water
			{
				g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_12, 0, kRenderTransAdd, 255,255,255, 1.0,-0.7, 8.0, 90.0, 0.0));
				switch (gEngfuncs.pfnRandomLong(0, 3))
				{
				case 0 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh1.wav", 0.4, ATTN_NORM, 0, 100 );break;
				case 1 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh2.wav", 0.4, ATTN_NORM, 0, 100 );break;
				case 2 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh3.wav", 0.4, ATTN_NORM, 0, 100 );break;
				case 3 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh4.wav", 0.4, ATTN_NORM, 0, 100 );break;
				}
			}
			return 1;
	}
	if (BulletType == 191){
			if (contents == CONTENTS_WATER || (StartEnv == CONTENTS_WATER && contents == CONTENTS_EMPTY))//water->air or air->water
			{
				g_pRenderManager->AddSystem(new CPSBlastCone(15, 90, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.2, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_9, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(15, 75, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.4, -0.35, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_12, 0, kRenderTransAdd, 255,255,255, 1.0,-0.5, 16.0, 180.0, 0.0));
				gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/water_small_splash.wav", 0.6, ATTN_NORM, 0, 100 );
			}
			return 1;
	}

	if (contents == CONTENTS_WATER || (StartEnv == CONTENTS_WATER && contents == CONTENTS_EMPTY))//water->air or air->water
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(3, 90, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.15, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_9, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(3, 75, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.3, -0.35, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_12, 0, kRenderTransAdd, 255,255,255, 1.0,-0.7, 8.0, 90.0, 0.0));

	        switch (gEngfuncs.pfnRandomLong(0, 3))
	        {
			case 0 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh1.wav", 0.5, ATTN_NORM, 0, 100 );break;
			case 1 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh2.wav", 0.5, ATTN_NORM, 0, 100 );break;
			case 2 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh3.wav", 0.5, ATTN_NORM, 0, 100 );break;
			case 3 : gEngfuncs.pEventAPI->EV_PlaySound(0, rotate, CHAN_STATIC, "player/pl_slosh4.wav", 0.5, ATTN_NORM, 0, 100 );break;
			}
	}

	else if (contents == CONTENTS_LAVA || (StartEnv == CONTENTS_LAVA && contents == CONTENTS_EMPTY))//same for lava
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(12, 60, rotate, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 30, 110,110,110, 0.5, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), 0, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_13, 0, kRenderTransAdd, 255,120,0, 1.0,-0.7, 8.0, 90.0, 0.0));
	}


	VectorMA(origin, 4.0, normal, src);//4 units backward
	VectorMA(origin, -4.0, normal, end);// forward (into the brush)
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_IGNORE, -1, &tr);

	//Tracer bullets, every 2nd bullet makes a trace
	/*
	if (BulletType == BULLET_32mm || BulletType == BULLET_57mm || BulletType == BULLET_9MM || BulletType == BULLET_12MM || BulletType == BULLET_14MM)
	{
		if (iRand < (0x7fff/2))
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), 0, kRenderTransAdd, 255,255,255, 1.0, 0, 1, 0, 0.1));
	}
	*/

	//M72 trails
	/*
	if (BulletType == 682)
		g_pRenderManager->AddSystem(new CPSFlatTrail(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, kRenderTransAdd, 255,255,255, 1.0, -0.3, 0.1, 2.0, 15.0, 0));

	if (BulletType == BULLET_2MM_QUAD)
		g_pRenderManager->AddSystem(new CPSFlatTrail(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_5, kRenderTransAdd, 255,255,255, 1.0, -0.3, 0.1, 2.0, 15.0, 0));

	*/

	if (gEngfuncs.PM_PointContents(origin, NULL) == CONTENTS_SKY)
		return 1;

	//These types of bullets have their own decals
//	if (BulletType != BULLET_BFG || BulletType != BULLET_127MM)
	//	EV_DecalTrace(&tr, EV_BulletDecal(TexType));

	switch (BulletType)
	{
		case BULLET_BFG:
		{
			if (Surface == SURFACE_WORLDBRUSH || Surface == SURFACE_BREAKABLE)
				EV_DecalTrace(&tr, EV_BoltDecal(TexType));

			if (InWater == 1)// In water
			{
				DynamicLight(src, 350, 128,128,0, 0.2, 300.0);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(20, 170, src, src, Vector(1,1,1), 5, 150, 128,128,128, 0.7, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(20, 220, src, src, Vector(1,1,1), 5, 125, 128,128,128, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_1, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBubbles(20, src, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(7,11), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

				if (iRand < (0x7fff/2))
					gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_STATIC, "weapons/explode5.wav", 1, ATTN_LOW, 10, 170 );
			}
			else
			{
				DynamicLight(src, 500, 128,128,0, 0.3, 250.0);
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(4,6), 3, gEngfuncs.pfnRandomFloat(120, 160), src, src, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,6), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(25, 250, src, src, Vector(1,1,1), 5, 150, 128,128,128, 0.7, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(25, 300, src, src, Vector(1,1,1), 5, 125, 128,128,128, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_1, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

				if (iRand < (0x7fff/3))
					gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_STATIC, "weapons/explode5.wav", 1, ATTN_LOW, 10, 90 );
			}
		}
		break;

		case BULLET_127MM:
		{
			if (Surface == SURFACE_WORLDBRUSH || Surface == SURFACE_BREAKABLE)
			{
				EV_DecalTrace(&tr, EV_BoltDecal(TexType));
				SpawnShards(TexType, src, BULLET_BOLT, InWater);
			}
			if (InWater == 1)
			{
				g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, src, src, Vector(1,1,1), 10, 20, 255,255,255, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(60, 200, src, src, Vector(1,1,1), 10, 50, 255,255,255, 0.5, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(40, 75, src, src, Vector(1,1,1), 10, 100, 150,80,0, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_9, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_VOICE, "weapons/barett_explode.wav", 1, ATTN_LOW, 0, 200);
				DynamicLight(src, 200, 128,128,0, 0.6, 200.0);
				g_pRenderManager->AddSystem(new CPSBubbles(50, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(7,11), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else
			{
				g_pRenderManager->AddSystem(new CPSBlastCone(50, 85, src, src, Vector(1,1,1), 10, 60, 0,0,0, 0.6, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(50, 400, src, src, Vector(1,1,1), 10, 20, 255,255,255, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(80, 300, src, src, Vector(1,1,1), 10, 50, 255,255,255, 0.5, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(50, 150, src, src, Vector(1,1,1), 10, 100, 150,80,0, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_9, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_VOICE, "weapons/tank_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
				gEngfuncs.pEfxAPI->R_ParticleExplosion2( src, 111, 8 );
				DynamicLight(src, 300, 128,128,0, 0.6, 200.0);
				g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(2,4), gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(300, 450), SPARKSHOWER_SPARKS, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,4), gEngfuncs.pfnRandomFloat(1.5,2.0), gEngfuncs.pfnRandomFloat(340, 480), SPARKSHOWER_FIREEXP, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
		}
		break;

		case BULLET_NAILGUN:
		{
			if (Surface == SURFACE_WORLDBRUSH)
			{
				Impact(TexType, end, normal, BulletType, InWater);

				VectorMA(origin, -12, normal, src);
				VectorAngles(normal, nail_angles);
				TEMPENTITY *bolt = gEngfuncs.pEfxAPI->R_TempModel( src, Vector(0,0,0), nail_angles, 15, gEngfuncs.pEventAPI->EV_FindModelIndex("models/projectiles.mdl"), TE_BOUNCE_NULL );
				bolt->entity.curstate.body = 6;			
				bolt->flags &= ~FTENT_GRAVITY;
				if (iRand < (0x7fff/2))
					gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_BODY, "weapons/nail_hitwall.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			}
			else if (Surface == SURFACE_BREAKABLE || Surface == SURFACE_ARMOR || Surface == SURFACE_ENERGYARMOR)
			{
				Impact(TexType, end, normal, BulletType, InWater);
			}
			else if (Surface == SURFACE_FLESH)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(7,10), gEngfuncs.pfnRandomFloat(60,90), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(4, 24, src, src, Vector(1,1,1), 5, 25, 90,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
		}
		break;

		default:
			if (Surface == SURFACE_WORLDBRUSH || Surface == SURFACE_BREAKABLE || Surface == SURFACE_ARMOR || Surface == SURFACE_ENERGYARMOR)
			{
				Impact(TexType, src, normal, BulletType, InWater);
			}
			else if (Surface == SURFACE_FLESH && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(1,2), gEngfuncs.pfnRandomFloat(70,100), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		        g_pRenderManager->AddSystem(new CPSBlastCone(3, 25, src, src, Vector(1,1,1), 5, 10, 255,35,35, 0.6, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (Surface == SURFACE_FLESH_YELLOW && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(1,2), gEngfuncs.pfnRandomFloat(70,100), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(3, 25, src, src, Vector(1,1,1), 5, 10, 255,255,35, 0.6, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (Surface == SURFACE_FLESH_LEVEL2 && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(2,4), gEngfuncs.pfnRandomFloat(110,140), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(5, 25, src, src, Vector(1,1,1), 8, 16, 255,35,35, 0.6, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (Surface == SURFACE_FLESH_YELLOW_LEVEL2 && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(2,4), gEngfuncs.pfnRandomFloat(110,140), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(5, 25, src, src, Vector(1,1,1), 8, 16, 255,255,35, 0.6, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (Surface == SURFACE_FLESH_LEVEL3 && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(4,6), gEngfuncs.pfnRandomFloat(110,140), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(8, 24, src, src, Vector(1,1,1), 10, 20, 255,35,35, 0.75, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (Surface == SURFACE_FLESH_YELLOW_LEVEL3 && iBlood != 0)
			{
				g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(4,6), gEngfuncs.pfnRandomFloat(110,140), origin, normal, Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     	g_pRenderManager->AddSystem(new CPSBlastCone(8, 24, src, src, Vector(1,1,1), 10, 20, 255,255,35, 0.75, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
		break;
	}
return 1;
}

//===============================//
//Rocket & Grenade impact effects//
//===============================//
int __MsgFunc_ImpRocket(const char *pszName, int iSize, void *pbuf)
{
	pmtrace_t tr;
	vec3_t origin, src, end, angles;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	int IsBsp = READ_BYTE();
	int RocketType = READ_BYTE();
	int TexType = READ_BYTE();

	int InWater = (gEngfuncs.PM_PointContents(origin, NULL ) == CONTENTS_WATER);

	if (IsBsp > 0)// IsBSPModel
	{
		VectorMA(origin, 10.0, angles, src);// 14 units backward
		VectorMA(origin, -8.0, angles, end);// forward (into the brush)
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);

		switch (RocketType)
		{
			case BULLET_SMALEXP:
				EV_DecalTrace(&tr, EV_SmalExpDecal(TexType));
				SpawnShards(TexType, src, RocketType, InWater);
			break;

			case BULLET_NORMEXP:
				EV_DecalTrace(&tr, EV_NormExpDecal(TexType));
				SpawnShards(TexType, src, RocketType, InWater);
			break;

			case BULLET_HIGHEXP:
				EV_DecalTrace(&tr, EV_HighExpDecal(TexType));
				SpawnShards(TexType, src, RocketType, InWater);
			break;

			case BULLET_MEGAEXP:
				EV_DecalTrace(&tr, EV_MegaExpDecal(TexType));
				SpawnShards(TexType, src, RocketType, InWater);
			break;

			case BULLET_BOLT:
				EV_DecalTrace(&tr, EV_BoltDecal(TexType));
				SpawnShards(TexType, src, RocketType, InWater);
			break;

			default:
			break;
		}
	}
return 1;
}

//===================//
//Beam Impact effects//
//===================//
int __MsgFunc_ImpBeam(const char *pszName, int iSize, void *pbuf)
{
	pmtrace_t tr;
	vec3_t exp_angles, origin, src, end, angles;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	int Surface = READ_BYTE();
	int BeamType = READ_BYTE();

	int InWater = (gEngfuncs.PM_PointContents(origin, NULL ) == CONTENTS_WATER);

	switch (BeamType)
	{
		case IMPBEAM_PLASMABALL:
			VectorMA (origin, 2, angles, src);
			VectorMA (origin, 2, angles, end);
			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
			EV_DecalTrace(&tr, EV_SmalExpDecal(CHAR_TEX_CONCRETE));

			VectorAngles(angles, exp_angles);
			exp_angles[0] = -exp_angles[0];
			g_pRenderManager->AddSystem(new CRenderSystem(src, Vector(0,0,0), exp_angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_5, 0, kRenderTransAdd, 255,255,255, 1.0, -0.6, 10, 25, 0.0));
			break;

		case IMPBEAM_HELLVORTB:
			VectorMA (origin, 2, angles, src);
			VectorMA (origin, 2, angles, end);
			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
			EV_DecalTrace(&tr, EV_SmalExpDecal(CHAR_TEX_CONCRETE));

			VectorAngles(angles, exp_angles);
			exp_angles[0] = -exp_angles[0];
			g_pRenderManager->AddSystem(new CRenderSystem(src, Vector(0,0,0), exp_angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_GREEN_7, 0, kRenderTransAdd, 255,255,255, 1.0, -0.6, 10, 25, 0.0));
			break;

		case IMPBEAM_PBOLT:
			VectorMA (origin, 2, angles, src);
			VectorMA (origin, 2, angles, end);
			gEngfuncs.pEventAPI->EV_SetTraceHull(2);
			gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
			EV_DecalTrace(&tr, EV_EgonDecal(CHAR_TEX_CONCRETE));

			VectorAngles(angles, exp_angles);
			exp_angles[0] = -exp_angles[0];

			g_pRenderManager->AddSystem(new CRenderSystem(src, Vector(0,0,0), exp_angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_2, 0, kRenderTransAdd, 255,255,255, 1.0, 0, 10, 25, 0.9));
			//gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_STATIC, "weapons/bfg_dryfire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
			DynamicLight(origin, 180, 240,170,30, 0.3, 100.0);

			if (Surface == SURFACE_WORLDBRUSH || Surface == SURFACE_BREAKABLE || Surface == SURFACE_ARMOR)//hit world
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(6,12), 5, gEngfuncs.pfnRandomFloat(200, 240), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.3,2.5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), TRUE, -1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		break;
	}

	if (Surface == SURFACE_WORLDBRUSH || Surface == SURFACE_BREAKABLE || Surface == SURFACE_ARMOR)
	{
		VectorMA(origin, 6.0, angles, src);// 6 units backward
		VectorMA(origin, -4.0, angles, end);// forward (into the brush)
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);

		if (gEngfuncs.PM_PointContents(origin, NULL) == CONTENTS_SKY)
			return 1;

		if (InWater == 1)// In water
		{
        	if (BeamType == IMPBEAM_EGON)
			{
				EV_DecalTrace(&tr, EV_EgonDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new CPSBlastCone(3, 90, src, Vector(0,0,1), Vector(0.2,0.2,0.4), 6, 17, 255,255,255, 0.4, -0.18, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBubbles(3, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else if (BeamType == IMPBEAM_BLASTERBEAM)
			{
				EV_DecalTrace(&tr, EV_BigshotDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new CPSBlastCone(2, 80, src, Vector(0,0,1), Vector(0.2,0.2,0.4), 6, 17, 255,255,255, 0.4, -0.18, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBubbles(2, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
			else
			{
				EV_DecalTrace(&tr, EV_GaussDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new CPSBlastCone(8, 70, src, Vector(0,0,1), Vector(0.2,0.2,0.4), 6, 23, 255,255,255, 0.4, -0.13, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBubbles(30, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			}
		}
		else
		switch (BeamType)
		{
			case IMPBEAM_BLASTER:
			break;

			case IMPBEAM_GAUSS:
			break;

			case IMPBEAM_GAUSSCHARGED:
				if (Surface == SURFACE_WORLDBRUSH)
				{
					g_pRenderManager->AddSystem(new CPSTrail(50, 3, 4, 30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 3, 8, 126,120,120, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_8, kRenderTransAdd, gEngfuncs.pfnRandomFloat(5,8)), 0, -1);
					g_pRenderManager->AddSystem(new PSGravityPart(75, 5, gEngfuncs.pfnRandomFloat(140, 180), origin, Vector(0,0,1), Vector(0.1,0.1,0.6), gEngfuncs.pfnRandomFloat(2,3), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, gEngfuncs.pfnRandomFloat(3,5)), 0, -1);
				}
				EV_DecalTrace(&tr, EV_GaussDecal(CHAR_TEX_CONCRETE));
				DynamicLight(src, 180, 255,255,50, 0.15, 300.0);
				g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, 150, kRenderTransAdd, 230,140,0, 0.8, -0.1, 5, 6, 0.0));
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(15,20), 5, gEngfuncs.pfnRandomFloat(140, 180), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(3,6), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;

			case IMPBEAM_PHOTONGUN:
				EV_DecalTrace(&tr, EV_GaussDecal(CHAR_TEX_CONCRETE));
				DynamicLight(src, 125, 0,190,255, 0.4, 200.0);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(10, 10, src, src, Vector(1,1,1), 5, 25, 30,200,250, 0.2, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(10, 12, src, src, Vector(1,1,1), 5, 20, 128,128,128, 0.3, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		     		g_pRenderManager->AddSystem(new CPSBlastCone(10, 5, src, src, Vector(1,1,1), 1, 10, 128,128,128, 0.8, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			break;

			case IMPBEAM_TAUCANNON:
				EV_DecalTrace(&tr, EV_GaussDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				DynamicLight(src, 200, 160,0,210, 0.2, 250.0);
				g_pRenderManager->AddSystem(new CRSBeamStar(src, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), 0, 60, kRenderTransAdd, 125,0,255, 1.0, -0.3, 10, 25, 5.0));
			    //gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/bfg_fire_sunofgod.wav", 1.0, ATTN_NORM, 0, 105 );
			break;

			case IMPBEAM_TAUCANNON2:
				EV_DecalTrace(&tr, EV_GaussDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,0, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,0, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				DynamicLight(src, 200, 160,0,210, 0.2, 250.0);
				g_pRenderManager->AddSystem(new CRSBeamStar(src, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0, 60, kRenderTransAdd, 255,255,125, 1.0, -0.3, 10, 25, 5.0));
			    //gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/bfg_fire_sunofgod.wav", 1.0, ATTN_NORM, 0, 105 );
			break;

			case IMPBEAM_IONTURRET:
				EV_DecalTrace(&tr, EV_SmalExpDecal(CHAR_TEX_CONCRETE));
				DynamicLight(src, 220, 0,90,240, 0.2, 250.0);
				g_pRenderManager->AddSystem(new CPSBlastCone(35, 80, src, Vector(0,0,1), Vector(0.3,0.3,0.4), 10, 80, 0,40,250, 0.7, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(10, 60, src, Vector(0,0,1), Vector(0.2,0.2,0.4), 10, 50, 0,0,0, 0.4, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(20, 100, src, src, Vector(1,1,1), 5, 100, 0,100,250, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_17, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			        gEngfuncs.pEventAPI->EV_PlaySound( 0, src, CHAN_VOICE, "weapons/chronosceptor_fire.wav", 1.0, ATTN_LOW_HIGH, 0, 150 );
			break;

			case IMPBEAM_BLASTERBEAM:
				EV_DecalTrace(&tr, EV_BigshotDecal(CHAR_TEX_CONCRETE));
				g_pRenderManager->AddSystem(new CPSBlastCone(2, 25, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 20, 255,255,255, 0.8, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 3, 16, 126,120,120, 0.4, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 0.1), 0, -1);
				DynamicLight(src, 100, 250,0,0, 0.01, 0.0);
			break;

			case IMPBEAM_EGON:
		     	g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, src, src, Vector(1,1,1), 5, 30, 128,128,128, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, src, src, Vector(1,1,1), 5, 30, 160,160,160, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
			break;

			case IMPBEAM_EGON2:
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, src, src, Vector(1,1,1), 5, 30, 128,128,128, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
			break;
		}
	}
	else if (InWater != 1 && Surface == SURFACE_FLESH)
	{
		if (BeamType == IMPBEAM_BLASTERBEAM)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(3, 80, origin, origin, Vector(1,1,1), 5, 40, 255,140,0, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(5, 125, origin, origin, Vector(1,1,1), 3, 1, 0,0,0, 1, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		else
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(20, 80, origin, origin, Vector(1,1,1), 5, 40, 255,140,0, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(20, 125, origin, origin, Vector(1,1,1), 3, 1, 0,0,0, 1, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
	}
return 1;
}

//================//
//Gun Beam Effects//
//================//
int __MsgFunc_FireBeam( const char *pszName, int iSize, void *pbuf )
{
	pmtrace_t tr;
	vec3_t normal, exp_angles, origin, src, end, angles;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();//Beam start point i.e. Gun Barrel
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	angles.x = READ_COORD();//Beam End point
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	normal.x = READ_COORD();//Normal
	normal.y = READ_COORD();
	normal.z = READ_COORD();
	int Type = READ_BYTE();

	int StartEnv = gEngfuncs.PM_PointContents(origin, NULL);
	int contents = WaterSurfaceCoords(origin, angles, StartEnv, 0);
	int InWater = (gEngfuncs.PM_PointContents(angles, NULL ) == CONTENTS_WATER);
	int InSky = (gEngfuncs.PM_PointContents(angles, NULL ) == CONTENTS_SKY);

	if (contents == CONTENTS_WATER || (StartEnv == CONTENTS_WATER && contents == CONTENTS_EMPTY))//water->air or air->water
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 70, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.15, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_9, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 60, rotate, Vector(0,0,1), Vector(0.2,0.2,1.0), 5, 25, 255,255,255, 0.3, -0.35, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_13, 0, kRenderTransAdd, 255,255,255, 1.0,-0.7, 8.0, 90.0, 0.0));
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 60, rotate, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 15, 255,255,255, 0.5, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, rotate, CHAN_VOICE, "weapons/plasma_hitwall.wav", 1, ATTN_NORM, 0, 200 );
	}
	else if (contents == CONTENTS_LAVA || (StartEnv == CONTENTS_LAVA && contents == CONTENTS_EMPTY))//same for lava
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(12, 60, rotate, Vector(0,0,1), Vector(0.2,0.2,0.4), 10, 30, 110,110,110, 0.5, 0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), 0, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(rotate, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_13, 0, kRenderTransAdd, 255,120,0, 1.0,-0.7, 8.0, 90.0, 0.0));
	}

	switch (Type)
	{
		case BEAM_BLASTER:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 0, kRenderTransAdd, 200,0,0, 1.0, -1.0, 2, 1, 0.0));
		break;

		case BEAM_BLASTER_EXIT:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 0, kRenderTransAdd, 200,0,0, 1.0, -1.0, 2, 1, 0.0));
		break;

		case BEAM_GAUSS:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 0.9, -1.1, 1, 0, 0.1));
		break;

		case BEAM_NOBITABEAM:
		{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_SHOCKWAVE, kRenderTransAdd, 255,255,255, 1.0, -1.1, 3.0, 0, 0.0));
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_STEAM, kRenderTransAdd, 255,255,255, 1.0, -0.33, 3.0, 1, 0.0));
		}
		break;

		case BEAM_GAUSS_EXIT:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 0.9, -1.1, 1, 0, 0.1));
		break;

		case BEAM_GAUSSCHARGED:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 1.0, -1.1, 1.5, 0, 0.0));
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 128,128,128, 1.0, -0.33, 1.5, 1, 0.0));
		break;

		case BEAM_STONEBEAM:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 1.0, -1.1, 3.0, 0, 0.0));
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 128,128,128, 1.0, -0.33, 3.0, 1, 0.0));
		break;

		case BEAM_GAUSSCHARGED_EXIT:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 1.0, -1.1, 1.5, 0, 0.0));
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 128,128,128, 1.0, -0.33, 1.5, 1, 0.0));
		break;

		case BEAM_PHOTONGUN:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.07, 1.3, 0, 250, 30, 0, 25, 255, 255, 255 );
		break;

		case BEAM_PHOTONGUN_EXIT:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.07, 1.3, 0, 250, 30, 0, 25, 255, 255, 255 );
		break;

		case BEAM_TAUCANNON:
		{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_TAUBEAM, kRenderTransAdd, 255,255,255, 1.0, -0.5, 3, 1, 0.0));

			//BEAM *pBeam;
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 1.2, 1, 0.08, 255, 15, BLAST_SKIN_TAUBEAM, 0, 255, 255, 255 );
			//pBeam->flags |= ( FBEAM_SINENOISE); ���ܵ�������BUG
		}
		break;

		case BEAM_TAUCANNON2:
			{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,0, 1.0, -0.4, 9, 1, 0.0));

			//BEAM *pBeam;
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 1.5, 3, 0.08, 255, 15, BLAST_SKIN_GAUSSBEAM, 0, 255, 255, 0 );
			//pBeam->flags |= ( FBEAM_SINENOISE); ���ܵ�������BUG
			}
		break;

		case BEAM_TAUCANNON_EXIT:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_TAUBEAM, kRenderTransAdd, 255,255,255, 1.0, -0.5, 3, 1, 0.0));
		break;

		case BEAM_IONTURRET:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.1, 7, 0, 128, 50, 0, 25, 255, 255, 255);
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 255,255,255, 1.0, -0.5, 8, 1, 0.0));
		break;

		case BEAM_ZDEADEYE:
			{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), 2, kRenderTransAdd, 255,255,255, 1.0, -0.4, 24, 1, 0.0));

			//BEAM *pBeam;
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), 1.5, 12, 0.08, 255, 15, 2, 0, 255, 255, 255 );
			//pBeam->flags |= ( FBEAM_SINENOISE); ���ܵ�������BUG
			}
		break;

		case 24:
			{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 2, kRenderTransAdd, 255,255,255, 1.0, -0.3, 18, 1, 0.0));
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 2, kRenderTransAdd, 255,255,255, 1.0, -0.3, 18, 1, 0.0));
			}
		break;

		case BEAM_DOMABOSS:
		{
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_TAUBEAM, kRenderTransAdd, 255,255,255, 1.0, -0.3, 30, 1, 0.0));
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 1.2, 5, 0.08, 255, 15, BLAST_SKIN_TAUBEAM, 0, 255, 255, 255 );
		}
		break;

		case BEAM_IONTURRET_EXIT:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.1, 7, 0, 128, 50, 0, 25, 255, 255, 255);
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 255,255,255, 1.0, -0.5, 8, 1, 0.0));
		break;

		case BEAM_PHOTONGUN_EXP:
			if (!InSky)
			{
				VectorMA (angles, 2, normal, src);
				VectorMA (angles, 2, normal, end);
				gEngfuncs.pEventAPI->EV_SetTraceHull(2);
				gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
				EV_DecalTrace(&tr, EV_TeslaDecal(CHAR_TEX_CONCRETE));

				if (!InWater)
				{
					VectorAngles(normal, exp_angles);
					exp_angles[0] = -exp_angles[0];
					g_pRenderManager->AddSystem(new CRenderSystem(src, Vector(0,0,0), exp_angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 25, kRenderTransAdd, 255,255,255, 1.0, -0.75, 10, 15, 0.0), 0, -1);
					g_pRenderManager->AddSystem(new CPSSparks(gEngfuncs.pfnRandomLong(8,16), src, 0.4, 0.02, 150, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/photon_hitwall.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
					DynamicLight(angles, 120, 140,90,250, 0.3, 200.0);
				}
				else
				{
					gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/photon_hitwall.wav", 1.0, ATTN_NORM, 0, 200 );
					g_pRenderManager->AddSystem(new CPSBlastCone(12, 80, src, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 40, 255,255,255, 0.4, -0.13, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_8, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					g_pRenderManager->AddSystem(new CPSBubbles(30, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			}
		break;

		case BEAM_TESLAGUN:
			if (!InSky)
			{
				VectorMA (angles, 2, normal, src);
				VectorMA (angles, 2, normal, end);
				gEngfuncs.pEventAPI->EV_SetTraceHull(2);
				gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
				EV_DecalTrace(&tr, EV_TeslaDecal(CHAR_TEX_CONCRETE));

				if (!InWater)
				{
					VectorAngles(normal, exp_angles);
					exp_angles[0] = -exp_angles[0];
					g_pRenderManager->AddSystem(new CRenderSystem(src, Vector(0,0,0), exp_angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0, 25, kRenderTransAdd, 255,255,255, 1.0, -1.2, 25, -12, 0.0), RENDERSYSTEM_FLAG_RANDOMFRAME, -1);
					g_pRenderManager->AddSystem(new CPSSparks(20, src, 1.2, 0.04, 140, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					DynamicLight(angles, 100, 220,220,0, 0.25, 250.0);

					switch (gEngfuncs.pfnRandomLong(0,2))
					{
						case 0:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
						case 1:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro5.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
						case 2:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro6.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
					}
				}
				else
				{
					g_pRenderManager->AddSystem(new CPSBubbles(20, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

					switch (gEngfuncs.pfnRandomLong(0,2))
					{
						case 0:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro4.wav", 1.0, ATTN_NORM, 0, 200 ); break;
						case 1:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro5.wav", 1.0, ATTN_NORM, 0, 200 ); break;
						case 2:	gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/electro6.wav", 1.0, ATTN_NORM, 0, 200 ); break;
					}
				}
			}
		break;

		case BEAM_PULSERIFLE:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.15, 2.5, 0.8, 200, 15, BLAST_SKIN_LIGHTNING, 0, 255, 255, 255 );

			if (!InSky)
			{
				VectorMA (angles, 8, normal, src);
				VectorMA (angles, 2, normal, end);
				gEngfuncs.pEventAPI->EV_SetTraceHull(2);
				gEngfuncs.pEventAPI->EV_PlayerTrace(src, end, PM_STUDIO_BOX, -1, &tr);
				EV_DecalTrace(&tr, EV_SmalExpDecal(CHAR_TEX_CONCRETE));

				if (!InWater)
				{
					gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/pulse_hitwall.wav", 1, ATTN_NORM, 0, PITCH_NORM );
					g_pRenderManager->AddSystem(new CRSLight(src, 0,0,0, 1, 1.8, BEAM_PULSERIFLE, -1));
					DynamicLight(src, 300, 240,240,50, 1.8, 80.0);
              				g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(12,15), gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(400, 450), SPARKSHOWER_SPARKS2, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
					gEngfuncs.pEfxAPI->R_StreakSplash ( src, vec3_origin, 0, gEngfuncs.pfnRandomFloat(50,80), 500, -300, 300);
				}
				else
				{
					gEngfuncs.pEventAPI->EV_PlaySound( 0, angles, CHAN_VOICE, "weapons/pulse_hitwall.wav", 1, ATTN_NORM, 0, 200 );
					g_pRenderManager->AddSystem(new CPSBubbles(35, src, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
				}
			}
		break;

		case BEAM_PSP:
			g_pRenderManager->AddSystem(new CRSBeam(origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_PSPBEAM, kRenderTransAdd, 200,200,200, 1.0, -0.8, 2, 1, 0.0));
		break;

		case BEAM_LGTNGBALL:
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, angles, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.3, 1, 1.5, 255, 15, BLAST_SKIN_LIGHTNING, 0, 255, 255, 255 );
		break;

		case BEAM_BLOODSTREAM:
		//	g_pRenderManager->AddSystem(new CPSBlood(6, gEngfuncs.pfnRandomFloat(90,150), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			gEngfuncs.pEfxAPI->R_BloodStream(origin, angles, (unsigned short)73, gEngfuncs.pfnRandomFloat(100,200) );
		break;

		case BEAM_BLOODSTREAM233:
		//	g_pRenderManager->AddSystem(new CPSBlood(6, gEngfuncs.pfnRandomFloat(90,150), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			gEngfuncs.pEfxAPI->R_BloodStream(origin, angles, (unsigned short)58, gEngfuncs.pfnRandomFloat(100,200) );
		break;
	}
return 1;
}

//=============================//
//change body for weapon models//
//=============================//
int __MsgFunc_SetBody(const char *pszName, int iSize, void *pbuf)
{
	gHUD.m_iBody = READ_BYTE();
	cl_entity_s *view = gEngfuncs.GetViewModel();
	view->curstate.body = gHUD.m_iBody;
return 1;
}

//=============================//
//change skin for weapon models//
//=============================//
int __MsgFunc_SetSkin(const char *pszName, int iSize, void *pbuf)
{
	gHUD.m_iSkin = READ_BYTE();
	cl_entity_s *view = gEngfuncs.GetViewModel();
	view->curstate.skin = gHUD.m_iSkin;
return 1;
}

//============//
// rain & snow//
//============//
extern rain_properties Rain;
int __MsgFunc_Rain( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	Rain.dripsPerSecond =	READ_SHORT();
	Rain.distFromPlayer =	READ_COORD();
	Rain.windX =		READ_COORD();
	Rain.windY =		READ_COORD();
	Rain.randX =		READ_COORD();
	Rain.randY =		READ_COORD();
	Rain.weatherMode =	READ_BYTE();
	Rain.globalHeight =	READ_COORD();
return 1;
}

int __MsgFunc_Explosion( const char *pszName, int iSize, void *pbuf )
{
	pmtrace_t tr;
	vec3_t SplashSpread, dir, origin;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	int Type = READ_BYTE();

	int InWater = (gEngfuncs.PM_PointContents(origin, NULL ) == CONTENTS_WATER);

	if (Type == EXPLOSION_C4)
	{
		SplashSpread[0] = 0.5;
		SplashSpread[1] = 0.5;
		SplashSpread[2] = 1;

		g_pRenderManager->AddSystem(new CPSBlastCone(70, 480, origin, origin, Vector(1,1,1), 5, 250, 128,128,128, 0.7, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 400, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 200, 255,115,0, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 350, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 75, 160,0,217, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(45, 420, origin, origin, Vector(1,1,1), 5, 100, 0,0,217, 0.8, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, origin, Vector(0,0,1), SplashSpread, 5, 180, 255,255,255, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 425, origin, Vector(0,0,1), SplashSpread, 5, 300, 240,80,50, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 425, origin, Vector(0,0,1), SplashSpread, 5, 250, 128,128,128, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 420, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 220,150,0, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		DynamicLight(origin, 800, 240,170,30, 0.6, 150.0);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(12,16), gEngfuncs.pfnRandomFloat(1.7,2.4), gEngfuncs.pfnRandomFloat(400, 600), SPARKSHOWER_SPARKSMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 2400, 80, 80, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_C4, kRenderTransAdd, 255,255,255, 1, -0.5, 0.5));
	}
	else if (Type == EXPLOSION_BIOMASS)
	{
		DynamicLight(origin, 200, 255,255,255, 0.5, 80.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "rmxp/111-Heal07.wav", 1, 0.5, 0, 120 );
	}
	else if (Type == EXPLOSION_SLEEP_Z)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 20, origin, Vector(1,1,1), Vector(1,1,1), 10, 0, 255,255,255, 2.0, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/sleep_z.spr"), FALSE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "weapons/medkit_heal.wav", 1, 0.5, 0, 120 );
	}
	else if (Type == EXPLOSION_DEFINE_SHIELD)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(90, 300, origin, origin, Vector(3,3,3), 5, 125, 64,255,128, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(90, 300, origin, origin, Vector(3,3,3), 3, 10, 64,255,128, 0.6, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_BOLT)
	{
	
			gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/xbolt_explode.wav", 1, 0.6, 0, 100 );
	//		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,5), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(200, 250), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSSparks(30, origin, 3, 0.03, 120, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(30, 250, origin, origin, Vector(1,1,1), 5, 100, 255,115,0, 0.7, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(30, 150, origin, origin, Vector(1,1,1), 5, 70, 220,100,0, 0.5, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(30, 70, origin, origin, Vector(1,1,1), 5, 40, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(10, 30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 25, 0,0,0, 0.15, -0.04, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	DynamicLight(origin, 280, 128,140,0, 0.6, 70.0);

	}
	else if ( Type == EXPLOSION_TURRET)
	{
		EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(250,400), gEngfuncs.pfnRandomFloat(3,5), gHUD.TempEntLifeCvar->value, 12, GIB_METALL, 0);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,8), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(250, 300), SPARKSHOWER_STREAKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,3), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(350, 400), SPARKSHOWER_EXP, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/explosion_flakbomb.wav", 1, ATTN_NORM, 0, 150);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 25, 0,0,0, 0.25, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 130, origin, origin, Vector(1,1,1), 15, 50, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_18, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 150, origin, origin, Vector(1,1,1), 15, 51, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_20, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 180, 128,140,0, 0.5, 50.0);
	}
	else if ( Type == EXPLOSION_TANK)
	{
		EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(5,8), gHUD.TempEntLifeCvar->value, 12, GIB_METALL, 0);
		EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(500,900), gEngfuncs.pfnRandomFloat(10,14), gHUD.TempEntLifeCvar->value, 12, GIB_METALL, 0);

		DynamicLight(origin, 350, 128,140,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 15, 32, 0,0,0, 0.15, -0.04, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(2.4,3.0), gEngfuncs.pfnRandomFloat(350, 400), SPARKSHOWER_LAVA_FLAME, origin, origin, Vector(2,2,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(2,4), gEngfuncs.pfnRandomFloat(1.8,2.5), gEngfuncs.pfnRandomFloat(300, 450), SPARKSHOWER_FLICKER, origin, origin, Vector(2,2,3), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 330, origin, origin, Vector(1,1,1), 25, 100, 255,255,255, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_17, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, origin, origin, Vector(1,1,1), 25, 100, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_16, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 270, origin, origin, Vector(1,1,1), 15, 70, 255,255,255, 0.6, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 900, 60, 60, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_C4, kRenderTransAdd, 255,255,255, 1, -1, 0.3));
	}
	else if ( Type == EXPLOSION_MACHINEGUN)
	{
		EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(250,400), gEngfuncs.pfnRandomFloat(3,5), gHUD.TempEntLifeCvar->value, 12, GIB_METALL, 0);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 32, 0,0,0, 0.15, -0.04, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,3), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(350, 400), SPARKSHOWER_FIRESMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,6), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(200, 250), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 100, origin, origin, Vector(2,2,1), 15, 70, 255,255,255, 0.8, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_21, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 170, origin, origin, Vector(1,1,1), 15, 40, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_19, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 250, 128,140,0, 0.5, 100.0);
	}
	else if ( Type == EXPLOSION_MEDKIT)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 5, 125, 64,255,64, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 64,255,64, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "rmxp/105-Heal01.wav", 1, 0.5, 0, 100 );
	}
	else if ( Type == EXPLOSION_HEAL_HOLY)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 150, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 75, 0,220,160, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 5, 125, 64,255,64, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 64,255,64, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "rmxp/111-Heal07.wav", 1, 0.5, 0, 100 );
	}
	else if ( Type == EXPLOSION_HEVCHARGER)
	{
	//	SpawnShards(CHAR_TEX_METAL,origin,BULLET_NORMEXP,InWater);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 250, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 120, 205,100,105, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 200, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 75, 217,0,160, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 5, 125, 255,32,32, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,32,32, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "weapons/chronoclip_explode.wav", 1, ATTN_LOW, 0, 120 );
		DynamicLight(origin, 200, 255,0,32, 0.5, 80.0);
	}
	else if ( Type == EXPLOSION_YELOOW)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,64, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(90, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,64, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 200, 255,192,32, 0.5, 120.0);
	}
	else if ( Type == EXPLOSION_PSHIELD)
	{
		if (InWater == 1)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 120, 150,100,205, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(25, 150, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 75, 160,0,217, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(50, 130, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall.wav", 1, ATTN_LOW_HIGH, 0, 200 );
			g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(8,12), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			DynamicLight(origin, 180, 160,0,220, 0.5, 60.0);
		}
		else
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 250, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 120, 150,100,205, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 75, 160,0,217, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(80, 100, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall.wav", 1, ATTN_LOW_HIGH, 0, 150 );
			DynamicLight(origin, 230, 160,0,220, 0.6, 80.0);
		}
	}
	else if ( Type == EXPLOSION_TRIPMINE)
	{
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 420, origin, origin, Vector(1,1,1), 10, 170, 255,115,0, 0.7, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_8, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(60, 330, origin, origin, Vector(1,1,1), 5, 150, 190,130,0, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 250, origin, origin, Vector(1,1,1), 10, 110, 255,255,255, 0.5, -6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_8, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        	g_pRenderManager->AddSystem(new CPSBlastCone(10, 50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 15, 35, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSSparks(60, origin, 5, 0.01, 500, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			DynamicLight(origin, 400, 128,128,40, 0.6, 125.0);
	}
	else if ( Type == EXPLOSION_GRENADE)
	{
		if (InWater == 1)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(50, 150, origin, Vector(0,0,1), Vector(1,1,1), 5, 125, 255,255,255, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(50, 260, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 240,80,50, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(50, 260, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 128,128,128, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash22.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBubbles(80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(8,12), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			DynamicLight(origin, 200, 128,128,0, 0.6, 200.0);
		}
		else
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 150, origin, Vector(0,0,1), Vector(3,3,1), 5, 0, 255,255,255, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 150, origin, Vector(0,0,1), Vector(3,3,1), 7, 0, 255,255,255, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_23, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 200, origin, Vector(0,0,1), Vector(1,1,1), 5, 125, 255,255,255, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 325, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 240,80,50, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(35, 325, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 128,128,128, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash22.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        	g_pRenderManager->AddSystem(new CPSBlastCone(35, 310, origin, Vector(0,0,1), Vector(1,1,1), 10, 220, 0,0,0, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        	g_pRenderManager->AddSystem(new CPSBlastCone(10, 80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 50, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), 0, -1);
			g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0, 0, kRenderTransAdd, 255,106,30, 1.0, -0.5, 10, 80, 0.0));

		//	gEngfuncs.pEfxAPI->R_ParticleExplosion( origin );
			DynamicLight(origin, 300, 128,128,0, 0.6, 200.0);
			g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,5), gEngfuncs.pfnRandomFloat(1.8,2.2), gEngfuncs.pfnRandomFloat(350, 420), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		}
	}
	else if ( Type == EXPLOSION_FLASHBANG)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 5, 20, 255,255,255, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_TANKPROJ)
	{
		if (InWater == 1)
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(40, 220, origin, origin, Vector(1,1,1), 5, 145, 130,0,217, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(40, 250, origin, origin, Vector(1,1,1), 5, 145, 0,0,215, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        		g_pRenderManager->AddSystem(new CPSBlastCone(50, 300, origin, origin, Vector(1,1,1), 10, 195, 0,0,0, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        		g_pRenderManager->AddSystem(new CPSBlastCone(70, 360, origin, origin, Vector(1,1,1), 10, 240, 110,110,110, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/grenade_explode.wav", 1, ATTN_LOW, 0, 200 );
			DynamicLight(origin, 300, 220,140,0, 0.4, 250.0);
			g_pRenderManager->AddSystem(new CPSBubbles(75, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(8,12), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		}
		else
		{
			g_pRenderManager->AddSystem(new CPSBlastCone(45, 270, origin, origin, Vector(1,1,1), 5, 145, 130,0,217, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(45, 300, origin, origin, Vector(1,1,1), 5, 145, 0,0,215, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        		g_pRenderManager->AddSystem(new CPSBlastCone(50, 350, origin, origin, Vector(1,1,1), 10, 195, 0,0,0, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        		g_pRenderManager->AddSystem(new CPSBlastCone(70, 410, origin, origin, Vector(1,1,1), 10, 240, 110,110,110, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSSparks(70, origin, 6, 0.01, 500, 0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/grenade_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
        		g_pRenderManager->AddSystem(new CPSBlastCone(12, 80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 70, 0,0,0, 0.15, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_7, kRenderTransAlpha, 1), 0, -1);
			g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,7), gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(400, 450), SPARKSHOWER_STREAKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(2,5), gEngfuncs.pfnRandomFloat(1.6,2.1), gEngfuncs.pfnRandomFloat(400, 450), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			DynamicLight(origin, 450, 220,140,0, 0.5, 250.0);
		}
	}
	else if ( Type == EXPLOSION_SATCHEL)
	{
	//	g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(30,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 25, kRenderTransAdd, 196,156,15, 1.0, -0.3, 20, 150, 0.0));
		g_pRenderManager->AddSystem(new CPSBlastCone(80, 200, origin, Vector(0,0,1), Vector(1,1,3), 5, 200, 196,156,13, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 175, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 0.8, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       	g_pRenderManager->AddSystem(new CPSBlastCone(40, 180, origin, Vector(0,0,1), Vector(1,1,3), 5, 200, 0,0,0, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_9, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 40, origin, Vector(1,1,1), Vector(5,5,1), 5, 100, 255,150,0, 0.3, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 450, 196,156,13, 0.6, 150.0);
	}
	else if ( Type == EXPLOSION_SATELLITE)
	{
		SplashSpread[0] = 0.5;
		SplashSpread[1] = 0.5;
		SplashSpread[2] = 1;

		dir.z=50;
		dir=dir.Normalize();
	//	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);	
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
		gEngfuncs.pEventAPI->EV_PlayerTrace(origin, origin+dir*-50, PM_WORLD_ONLY, -1, &tr);
		EV_DecalTrace(&tr, EV_MegaExpDecal(CHAR_TEX_CONCRETE));

       		g_pRenderManager->AddSystem(new CPSBlastCone(40, 150, origin, Vector(0,0,1), SplashSpread, 10, 120, 110,110,110, 0.6, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(40, 175, origin, Vector(0,0,1), SplashSpread, 10, 120, 110,110,110, 0.6, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 400, origin, Vector(0,0,1), SplashSpread, 50, 5, 255,255,255, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_RANDOMFRAME | RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 450, origin, Vector(0,0,1), SplashSpread, 50, 5, 255,255,255, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_RANDOMFRAME | RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 250, origin, origin, Vector(1,1,1), 50, 80, 185,0,255, 0.9, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_RANDOMFRAME | RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(50, 350, origin, origin, Vector(1,1,1), 10, 150, 195,0,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(50, 400, origin, origin, Vector(1,1,1), 10, 150, 110,110,110, 0.7, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_1, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(45, 120, origin, Vector(0,0,1), SplashSpread, 5, 40, 220,120,250, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSCylinder(origin+dir*100, 20, 900, 200, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 255,255,255, 1, -1, 0.8));
		g_pRenderManager->AddSystem(new CRenderSystem(tr.endpos+dir*3, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_2, 0, kRenderTransAdd, 255,255,255, 1.0, -0.08, 20, 80, 0.0));

		EV_BlastModel(tr.endpos, BLAST_MDL_CONE, 1, 0.05, 1.5 );
		DynamicLight(origin, 1200, 225,150,0, 1.5, 400.0);

		g_pRenderManager->AddSystem(new CRSBeam(origin, origin+dir*4000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 0, kRenderTransAdd, 255,100,0, 1.0, -0.2, 30, 2, 0.0));
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(10,15), 5, gEngfuncs.pfnRandomFloat(200, 460), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,6), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(10,15), 5, gEngfuncs.pfnRandomFloat(200, 460), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,6), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(10,15), 5, gEngfuncs.pfnRandomFloat(200, 460), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,6), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(10,15), 5, gEngfuncs.pfnRandomFloat(200, 460), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,6), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_MORTAR)
	{
		dir.z=50;
		dir=dir.Normalize();

		SplashSpread[0] = 0.5;
		SplashSpread[1] = 0.5;
		SplashSpread[2] = 1;

	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/mortarhit.wav", 1, ATTN_LOW, 0, PITCH_NORM );
	//	g_pRenderManager->AddSystem(new CRSCylinder(origin+dir*20, 20, 2500, 30, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_C4, kRenderTransAdd, 255,255,255, 1, -1, 0.25));
	//	g_pRenderManager->AddSystem(new CRSBeam(origin, origin+dir*2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 0, kRenderTransAdd, 180,120,0, 1.0, -0.5, 0.1, 15, 0.2));
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, origin, Vector(0,0,1), SplashSpread, 5, 180, 255,255,255, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 425, origin, Vector(0,0,1), SplashSpread, 5, 300, 240,80,50, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 425, origin, Vector(0,0,1), SplashSpread, 5, 250, 128,128,128, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(40, 340, origin, Vector(0,0,1), SplashSpread, 10, 300, 0,0,0, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_8, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       		g_pRenderManager->AddSystem(new CPSBlastCone(10, 85, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 80, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_7, kRenderTransAlpha, 1), 0, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 420, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 220,150,0, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 350, 196,156,13, 0.6, 120.0);
	}
	else if ( Type == EXPLOSION_CHRONOCLIP)
	{
		SplashSpread[0] = 0.3;
		SplashSpread[1] = 0.3;
		SplashSpread[2] = 1;

		//EV_BlastModel(origin, BLAST_MDL_HALFSPHERE, 0.5, 0.08, 2.4 );
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(-30,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, 0, kRenderTransAdd, 255,1,255, 0.9, -0.4, 20, 500, 0.0));
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(30,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, 0, kRenderTransAdd, 0,40,255, 0.8, -0.4, 20, 800, 0.0));
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_12, 0, kRenderTransAdd, 0,90,250, 1.0, -0.5, 25, 1000, 0.0));
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 200, origin, origin, Vector(3,3,1), 3, 0, 255,255,255, 1, -0.51, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 225, origin, origin, Vector(3,3,1), 3, 0, 255,255,255, 1, -0.51, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 250, origin, origin, Vector(3,3,1), 4, 0, 255,255,255, 1, -0.51, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 450, origin, Vector(0,0,1), SplashSpread, 3, 0, 255,255,255, 1, -0.51, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 450, origin, Vector(0,0,1), SplashSpread, 4, 0, 255,255,255, 1, -0.51, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 350, origin, Vector(0,0,1), SplashSpread, 10, 200, 255,255,255, 0.5, -0.26, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/chronoclip_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
	}
	else if ( Type == EXPLOSION_SPARKSHOWER)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_SPARKSHOWER_SND)
	{
		gEngfuncs.pEfxAPI->R_SparkShower( origin );
		switch (gEngfuncs.pfnRandomLong(0,5))
		{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark1.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark2.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark3.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark4.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark5.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
			case 5:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "buttons/spark6.wav", gEngfuncs.pfnRandomFloat(0.25,0.75), ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}
	else if ( Type == EXPLOSION_BEAM_FLESHIMPACT)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(6, 40, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 2, 30, 0,0,0, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 300, origin, origin, Vector(1,1,1), 2, 0, 255,255,255, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_SHIELDIMPACT)
	{
			gEngfuncs.pEfxAPI->R_SparkShower( origin );

     		g_pRenderManager->AddSystem(new CPSBlastCone(8, 20, origin, origin, Vector(1,1,1), 10, 25, 128,128,128, 0.5, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_ARMORIMPACT)
	{
		if (InWater == 1)
			g_pRenderManager->AddSystem(new CPSBubbles(15, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,120), gEngfuncs.pfnRandomFloat(5,9), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		else
			gEngfuncs.pEfxAPI->R_SparkShower( origin );

		switch (gEngfuncs.pfnRandomLong(0,6))
		{
			case 0:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "debris/metal_impact_bullet1.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 1:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "debris/metal_impact_bullet2.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 2:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "debris/metal_impact_bullet3.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 3:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "debris/metal_impact_bullet4.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 4:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "weapons/ric1.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 5:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "weapons/ric2.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
			case 6:	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, 0, "weapons/ric3.wav", gEngfuncs.pfnRandomFloat(0.5,1.0), ATTN_NORM, 0, PITCH_NORM ); break;
		}
	}
	else if (Type == EXPLOSION_WHITESMOKE)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 40, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 25, 128,128,128, 0.3, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_BUBBLES)
	{
		g_pRenderManager->AddSystem(new CPSBubbles(70, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,12), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_BIOMASSIMPACT)
	{
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(6,9), 5, gEngfuncs.pfnRandomFloat(200, 230), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(0.5,1.2), 0, 0,150,0, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 232)
	{
		g_pRenderManager->AddSystem(new CPSBlood(5, gEngfuncs.pfnRandomFloat(70,100), origin, Vector(0,0,1), Vector(0.2,0.2,0.2), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(1, gEngfuncs.pfnRandomFloat(70,100), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 233)
	{
		g_pRenderManager->AddSystem(new CPSBlood(5, gEngfuncs.pfnRandomFloat(70,100), origin, Vector(0,0,1), Vector(0.2,0.2,0.2), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(1, gEngfuncs.pfnRandomFloat(70,100), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 234)
	{
		g_pRenderManager->AddSystem(new CPSBlood(8, gEngfuncs.pfnRandomFloat(100,130), origin, Vector(0,0,1), Vector(0.2,0.2,0.2), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(2, gEngfuncs.pfnRandomFloat(100,130), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 235)
	{
		g_pRenderManager->AddSystem(new CPSBlood(8, gEngfuncs.pfnRandomFloat(100,130), origin, Vector(0,0,1), Vector(0.2,0.2,0.2), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(2, gEngfuncs.pfnRandomFloat(100,130), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 236)
	{
		g_pRenderManager->AddSystem(new CPSBlood(11, gEngfuncs.pfnRandomFloat(130,160), origin, Vector(0,0,1), Vector(0.25,0.25,0.25), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(3, gEngfuncs.pfnRandomFloat(130,160), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 237)
	{
		g_pRenderManager->AddSystem(new CPSBlood(11, gEngfuncs.pfnRandomFloat(130,160), origin, Vector(0,0,1), Vector(0.25,0.25,0.25), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(3, gEngfuncs.pfnRandomFloat(130,160), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 238)
	{
		g_pRenderManager->AddSystem(new CPSBlood(20, gEngfuncs.pfnRandomFloat(130,160), origin, Vector(0,0,1), Vector(0.25,0.25,0.25), gEngfuncs.pfnRandomFloat(1, 2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(8, gEngfuncs.pfnRandomFloat(130,160), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(6,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 239)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 20, 75, 255,32,32, 0.3, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       	g_pRenderManager->AddSystem(new CPSBlastCone(45, 250, origin, origin, Vector(1,1,1), 10, 175, 255,32,32, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 254)
	{//������������
		for (int i = 0; i < 256; i++)
		{
		gEngfuncs.pEfxAPI->R_DecalRemoveAll(i);
		}
	}
	else if ( Type == EXPLOSION_TORCH)
	{
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GAUSSBEAM, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
	  	g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 10, 175, 192,128,32, 0.25, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 300, origin, origin, Vector(1,1,1), 2, 0, 255,255,255, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_PTELEPORT)
	{
		switch (gEngfuncs.pfnRandomLong(0,2))
		{
			case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "player/pl_teleport1.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM ); break;
			case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "player/pl_teleport2.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM ); break;
			case 2 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "player/pl_teleport3.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM ); break;
		}
		gEngfuncs.pEfxAPI->R_TeleportSplash(origin);
		g_pRenderManager->AddSystem(new CPSSpawnEffect(100, origin, 4, 12, 10, 10, 15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), kRenderTransAdd, 255,255,255, 1.0, -0.25, 1), 0, -1);
	}
	else if ( Type == EXPLOSION_DISPTELEPORT)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(1, 0, origin, Vector(0,0,0), Vector(1,1,1), 40, 0, 255,255,255, 0.6, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0, 50, kRenderTransAdd, 128,128,128, 0.7, -0.1, 50, -2.5, 8.0));
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0, 64, kRenderTransAdd, 128,128,128, 1, -0.1, 90, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "weapons/displacer_teleport.wav", 1, ATTN_NORM, 0, 100);
	}
	else if ( Type == EXPLOSION_TURRET_SPAWN)
	{
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_ITEM, "turret/tu_materialize.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSSpawnEffect(100, origin, 1, 1, 5, 8, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), kRenderTransAdd, 110,160,110, 0.8, 0, 1.8), 0, -1);
		g_pRenderManager->AddSystem(new CPSSpawnEffect(100, origin, 4, 12, 10, 10, 15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), kRenderTransAdd, 255,255,255, 1.0, -0.25, 1), 0, -1);
	}
	else if (Type == EXPLOSION_PLASMABALL2)
	{
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/plasma_hitwall.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSSparks(80, origin, 0.7, 0.01, 300, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_5, 100, kRenderTransAdd, 255,255,255, 0.6, -0.3, 4, 100, 3));
		DynamicLight(origin, 330, 0,190,20, 0.8, 150.0);

	}
	else if (Type == EXPLOSION_PLASMABALL2_SPARKS)
	{
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(13,20), 5, gEngfuncs.pfnRandomFloat(320, 420), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.8,3.2), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(13,20), 5, gEngfuncs.pfnRandomFloat(320, 420), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.8,3.2), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSTrail(50, 3, 4, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 15, 0,0,0, 0.05, -0.02, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, gEngfuncs.pfnRandomFloat(3,5)), 0, -1);

	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/plasma_hitwall.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSSparks(80, origin, 0.7, 0.01, 300, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_5, 100, kRenderTransAdd, 255,255,255, 0.6, -0.3, 4, 100, 3));
		DynamicLight(origin, 330, 0,190,20, 0.8, 150.0);
	}
	else if ( Type == EXPLOSION_ENERGY_INWATER_S)
	{
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/plasma_hitwall.wav", 1, ATTN_NORM, 0, 200 );
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 60, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 25, 255,255,255, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_ENERGY_INWATER_M)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall2.wav", 1, ATTN_LOW_HIGH, 0, 200);
		g_pRenderManager->AddSystem(new CPSBlastCone(12, 75, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 45, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(60, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_ENERGY_INWATER_L)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/sunofgod_blast.wav", 1, ATTN_LOW, 0, 200);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 90, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 80, 255,255,255, 0.35, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(6,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_PBOLT)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/explode5.wav", 1, ATTN_NORM, 10, 250 );
		DynamicLight(origin, 225, 240,170,30, 0.5, 100.0);
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 2500, 30, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_ENERGYBOLT, kRenderTransAdd, 255,255,255, 1, -1, 0.07));
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(2,5), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(300, 360), SPARKSHOWER_ENERGY, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_WHL_SHARD)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 100, origin, Vector(0,0,1), Vector(2,2,1), 5, 110, 100,100,100, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_8, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 150, origin, origin, Vector(1,1,1), 5, 100, 128,140,0, 1, -1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_WILLAM_SLASH)
	{
		g_pRenderManager->AddSystem(new CPSSparks(30, origin, 3, 0.05, 120, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_WIND_EXP)
	{
		g_pRenderManager->AddSystem(new CPSSparks(45, origin, 1, 0.06, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_LIGHTARROW_EXP)
	{
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_SNAKER_EXP)
	{
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.06, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_DRAGON_SLASH)
	{
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 5, 0.08, 240, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_DRAGON_SLASH2)
	{
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 5, 0.08, 240, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_HOLY_STAR)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 20, origin, Vector(0,0,0), Vector(1,1,1), 5, 100, 255,255,0, 0.3, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(3, gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(220, 280), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_LIGHTSABER)
	{
			DynamicLight(origin, 270, 255,0,0, 0.15, 200.0);
			g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(5,8), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(5,8), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(5,8), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(5,8), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//	g_pRenderManager->AddSystem(new CPSBlastCone(8, 3, origin, origin, Vector(1,1,1), 45, 3, 128,128,128, 0.6, -0.06, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == EXPLOSION_CLUSTERBABY)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/cluster_explode.wav", 1.0, ATTN_LOW_HIGH, 0, 70 );
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(180, 250), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(4,7), 0, 255,255,255, 1, -0.13, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_5, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(190, 230), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,3), 0, 255,255,255, 1, -0.13, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 60, origin, origin, Vector(1,1,1), 5, 110, 0,90,250, 0.3, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, -1, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 80, origin, origin, Vector(1,1,1), 5, 60, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 110, origin, origin, Vector(1,1,1), 5, 80, 255,255,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 400, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 300, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 200, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_LAVA_FLAME)
	{
		g_pRenderManager->AddSystem(new PSSparkShower(1, gEngfuncs.pfnRandomFloat(2,4), gEngfuncs.pfnRandomFloat(200, 450), SPARKSHOWER_LAVA_FLAME, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == EXPLOSION_LAVA)
	{
		g_pRenderManager->AddSystem(new PSSparkShower(1, gEngfuncs.pfnRandomFloat(2,4), gEngfuncs.pfnRandomFloat(200, 450), SPARKSHOWER_FIRESMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 100)//ʥ����
	{
		g_pRenderManager->AddSystem(new CPSSparks(70, origin, 5, 0.01, 500, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,10), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,10), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,10), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,10), 5, gEngfuncs.pfnRandomFloat(100, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,4), 0, 255,255,255, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_12, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 1, 0.02, 600, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 0.8, 0.01, 800, 1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(80, 350, origin, origin, Vector(1,1,1), 10, 200, 100,0,0, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 300, origin, origin, Vector(1,1,1), 10, 125, 255,255,255, 1, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 360, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 230,140,0, 1, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 500, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 255,255,255, 0.9, -0.8, 1.8));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 400, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 255,255,255, 0.9, -0.8, 1.8));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FIREBEAM, kRenderTransAdd, 255,255,255, 0.9, -0.8, 1.8));
		
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_2, 0, kRenderTransAdd, 255,255,255, 1.0, -0.1, 30, 80, 0.0));

		for (int i = 0; i <9; i++)
		{
			dir.x=gEngfuncs.pfnRandomFloat(-720,720);
			dir.y=gEngfuncs.pfnRandomFloat(-720,720);
			dir.z=gEngfuncs.pfnRandomFloat(-720,720);
			dir=dir.Normalize();
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(100,200)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(3.0,5.0), gEngfuncs.pfnRandomFloat(1.2,1.8), 2.5, 30, 20, BLAST_SKIN_FIREBEAM, 0, 255, 255, 255);
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(100,200)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(3.0,5.0), gEngfuncs.pfnRandomFloat(0.4,1.0), 2.5, 30, 20, BLAST_SKIN_ENERGYBOLT, 0, 255, 255, 255);
		}

		DynamicLight(origin, 400, 255,128,64, 0.6, 125.0);
	}
	else if(Type == 101){
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 20, origin, origin, Vector(1,1,1), 5, 20, 200,255,200, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 102)
	{
		DynamicLight(origin, 200, 220,220,220, 0.5, 80.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_AUTO, "weapons/medkit_heal.wav", 1, 0.5, 0, 120 );
	}
	else if(Type == 103){
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 20, origin, origin, Vector(1,1,1), 5, 20, 200,200,255, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 104)//��������̤
	{
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 500, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 400, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CPSBlastCone(150, 250, origin, origin, Vector(1,1,1), 10, 60, 200,250,150, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSBlastCone(50, 100, origin, Vector(0,0,1), Vector(3,3,1), 20, 100, 255,255,255, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_6, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		DynamicLight(origin, 500, 80,240,120, 0.7, 600.0);
	}
	else if(Type == 105){//������ͷ�
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 1, 0.1, 300, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 106)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 150, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 0.8, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 107){//�ϲ�һ��
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 480, origin, origin, Vector(1,1,1), 10, 80, 170,90,250, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_9, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 250, origin, origin, Vector(1,1,1), 10, 60, 170,90,250, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(100, 420, origin, origin, Vector(1,1,1), 10, 30, 0,90,250, 1, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 1.5, 0.15, 375, 3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 600, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.7, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 500, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.7, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 400, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.7, 1.5));

		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 500, 0,90,250, 0.7, 200.0);
	}
	else if ( Type == 108)
	{
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 150, origin, Vector(0,0,1), Vector(3,3,1), 5, 0, 255,255,255, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 150, origin, Vector(0,0,1), Vector(3,3,1), 7, 0, 255,255,255, 1.0, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_23, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 200, origin, Vector(0,0,1), Vector(1,1,1), 5, 125, 255,255,255, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 325, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 240,80,50, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
			g_pRenderManager->AddSystem(new CPSBlastCone(35, 325, origin, Vector(0,0,1), Vector(1,1,1), 5, 250, 128,128,128, 0.5, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash22.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        	g_pRenderManager->AddSystem(new CPSBlastCone(35, 310, origin, Vector(0,0,1), Vector(1,1,1), 10, 220, 0,0,0, 0.4, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
        	g_pRenderManager->AddSystem(new CPSBlastCone(10, 80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 50, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), 0, -1);
		//	g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0, 0, kRenderTransAdd, 255,106,30, 1.0, -0.5, 10, 80, 0.0));

		//	gEngfuncs.pEfxAPI->R_ParticleExplosion( origin );
			DynamicLight(origin, 300, 128,128,0, 0.6, 200.0);
			g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,5), gEngfuncs.pfnRandomFloat(1.8,2.2), gEngfuncs.pfnRandomFloat(350, 420), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 120)//�ֻ��θ���
	{
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 900, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 255,255,255, 1.0, -0.8, 1.2));
	}
	else if (Type == 125){//GMAN���ߤλ���
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,0, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,0, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0, 60, kRenderTransAdd, 255,255,125, 1.0, -0.3, 10, 25, 5.0));	   
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/displacer_teleport.wav", 1, ATTN_NORM, 0, 120);
	}
	else if ( Type == 126)//GMAN�α�ը
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 420, origin, origin, Vector(1,1,1), 10, 170, 255,255,255, 0.7, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_8, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 330, origin, origin, Vector(1,1,1), 5, 150, 255,255,255, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 250, origin, origin, Vector(1,1,1), 10, 110, 255,255,255, 0.5, -6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_8, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
       	g_pRenderManager->AddSystem(new CPSBlastCone(10, 50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 15, 35, 0,0,0, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 5, 0.01, 500, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 400, 128,128,40, 0.6, 125.0);
	}
	else if ( Type == 127)//GMAN�α���
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 250, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 120, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 200, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 75, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_4, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if(Type == 128){//�˵��α�ը
		g_pRenderManager->AddSystem(new CPSBlastCone(300, 800, origin, origin, Vector(1,1,1), 50, 200, 128,128,128, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, 0, kRenderTransAdd, 255,255,255, 1.0, -0.5, 25, 1200, 0.0));
	}
	else if (Type == 129)//��ը�򻯣��޺��̣�
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/explode5.wav", 1, ATTN_NORM, 10, 250 );
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 175, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 0.8, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 340, origin, origin, Vector(1,1,1), 10, 150, 255,115,15, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 130)//��ը��2
	{
		DynamicLight(origin, 320, 128,128,40, 0.8, 150.0);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/explode5.wav", 1, 0.5, 10, 250 );
		g_pRenderManager->AddSystem(new CPSBlastCone(6, 120, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 30,30,30, 0.6, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 1, 0.02, 600, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 0.8, 0.01, 800, 1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(75, 350, origin, origin, Vector(1,1,1), 10, 200, 100,0,0, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 300, origin, origin, Vector(1,1,1), 10, 125, 255,255,255, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 360, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 230,140,0, 1, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 131)//�ϴ���ħ��ץȡ
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(90, 200, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.3, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 80, origin, Vector(0,0,1), Vector(3,3,1), 3, 0, 255,255,255, 0.8, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_3, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 132)//��ը�򻯣�����A�Τ�С�ͱ�ը��
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 180, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 0.8, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 320, origin, origin, Vector(1,1,1), 10, 150, 255,115,15, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 133)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 200, origin, origin, Vector(1,1,1), 5, 200, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 200, origin, origin, Vector(1,1,1), 5, 200, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 200, origin, origin, Vector(1,1,1), 5, 200, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 600, origin, origin, Vector(1,1,1), 5, 100, 255,255,255, 0.2, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 134)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(1, 0, origin, Vector(0,0,0), Vector(1,1,1), 40, 0, 255,255,255, 0.6, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 128,128,128, 0.7, -0.1, 50, -2.5, 8.0));
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 64, kRenderTransAdd, 128,128,128, 1, -0.1, 90, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 136)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "majo/flame_hitwall.wav", 1, 0.6, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 100, origin, origin, Vector(1,1,1), 5, 80, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 150, origin, origin, Vector(1,1,1), 10, 120, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == 137)//����������ը
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 180, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.8, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_9, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 138){//�������ߤλ���
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,255,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), 0, 60, kRenderTransAdd, 255,255,255, 1.0, -0.3, 10, 25, 5.0));	   
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/animglow01.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/displacer_teleport.wav", 1, ATTN_NORM, 0, 120);
	}
	else if (Type == PROJ_DOMA_CLAW_HIT)//DOMA����צ�����
	{
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 600, 80, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 255,128,255, 1.0, -0.6, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 45, 900, 80, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 255,128,255, 1.0, -0.6, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 60, 1200, 80, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 255,128,255, 1.0, -0.6, 1.5));
	}
	else if (Type == PROJ_DOMABEAM2_HIT){//DOMA���ߤλ���
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,192,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 220), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(1.5,4), 0, 255,192,255, 1, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 60, kRenderTransAdd, 255,192,255, 1.0, -0.3, 10, 25, 5.0));	   
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 1, 0.05, 180, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/displacer_teleport.wav", 1, ATTN_NORM, 0, 120);
	}
	else if(Type == PROJ_GOD_SWORD1){
		g_pRenderManager->AddSystem(new CPSSparks(100, origin, 5, 0.1, 300, 1.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_2, 0, kRenderTransAdd, 255,255,255, 1.0, -0.1, 30, 60, 0.0));
	}
	else if(Type == PROJ_GOD_SWORD2){
		g_pRenderManager->AddSystem(new CPSSparks(100, origin, 5, 0.1, 300, 1.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_2, 0, kRenderTransAdd, 255,255,255, 1.0, -0.1, 30, 60, 0.0));
	}

return 1;
}

//====================================//
//Xash (spirit) world particle effects//
//====================================//
int __MsgFunc_Aurora(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	int idx = READ_BYTE();
	char *sz = READ_STRING();
	CreateAurora( idx, sz );
	END_READ( pszName );
return 1;
}

//===================//
//weapon fire effects//
//===================//
int __MsgFunc_FireGun(const char *pszName, int iSize, void *pbuf)
{
	vec3_t view_ofs, angles;

	BEGIN_READ( pbuf, iSize );
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	int EntIndex = READ_BYTE();
	int Animation = READ_BYTE();
	int Special = READ_BYTE();
	int Type = READ_BYTE();

	struct cl_entity_s *gun = gEngfuncs.GetEntityByIndex(EntIndex);

	if ( EV_IsLocal(EntIndex))
	{
		if (Animation != 200) //don't play animations for some guns (server-sided, like minigun, m249)
			gEngfuncs.pEventAPI->EV_WeaponAnimation(Animation, 0 );
	}

	if (Type == FIREGUN_REMOVE)//removes attached effects
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
	}
	if (Type == FIREGUN_BANDSAW)
	{
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/bandsaw_attack.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_NORM, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALLEST);
	}
	else if (Type == FIREGUN_EPISTOL)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_3, kRenderTransAdd, 255,255,255, 1.0, -1.2, gEngfuncs.pfnRandomFloat(11,13), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 180, 240,170,30, 0.1, 300.0);
	//	gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/lightsaber_fireplasma.wav", 1, ATTN_LOW_HIGH, 0, 100);
	}
	if (Type == FIREGUN_GLOCK)
	{
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/glock_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_LOW_HIGH, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_9MM );
		DynamicLight((float*)&gun->attachment[0], 180, 180,100,0, 0.1, 300.0);
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
	}
	else if (Type == FIREGUN_GLOCKAKIMBO)
	{
		switch (Special)
		{
		case 1://right gun
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
			DynamicLight((float*)&gun->attachment[0], 180, 180,100,0, 0.1, 300.0);
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_9MM );
		break;

		case 2://left gun
			EV_GunSmoke((float*)&gun->attachment[2], GUNSMOKE_WHITE_SMALL);
			DynamicLight((float*)&gun->attachment[2], 180, 180,100,0, 0.1, 300.0);
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
			EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_9MM );
		break;
		}
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/glock_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_LOW_HIGH, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
	}
	else if (Type == FIREGUN_USP)
	{
		if (Special == 0)//no silencer
		{
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
			DynamicLight((float*)&gun->attachment[0], 200, 180,100,0, 0.1, 300.0);
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash3.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
		}
		else
		{
			EV_GunSmoke((float*)&gun->attachment[1], GUNSMOKE_WHITE_SMALLEST);
			DynamicLight((float*)&gun->attachment[1], 80, 180,100,0, 0.1, 300.0);
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(2,3), -10.0, 0.0, 0.03), 0, -1);
		}
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, Special?"weapons/usp_fire_sil.wav":"weapons/usp_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_LOW_HIGH, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
		EV_EjectBrass((float*)&gun->attachment[2], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_45ACP ); 
	}
	else if (Type == FIREGUN_DEAGLE)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(7,9), -20.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 215, 180,230,60, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_50AE ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/deagle_fire.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_LOW_HIGH, 0, PITCH_NORM );
	}
	else if (Type == FIREGUN_PYTHON)
	{
		if (Special)//brass shells
		{
			for (int j = 0; j < 6; j++ )
			EV_EjectBrass( (float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_357 ); 
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(10,12), -20.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 220, 210,230,60, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_MEDIUM);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/357_shot1.wav", gEngfuncs.pfnRandomFloat(0.8, 0.9), ATTN_LOW_HIGH, 0, PITCH_NORM );
		}
	}
	else if (Type == FIREGUN_UZI)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_5, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 180, 180,100,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALLEST);
		EV_EjectBrass((float*)&gun->attachment[2], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_8MM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/UZI_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	}
	else if (Type == FIREGUN_UZIAKIMBO)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_5, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_5, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 220, 180,100,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALLEST);
		EV_GunSmoke((float*)&gun->attachment[1], GUNSMOKE_BLACK_SMALLEST);
		EV_EjectBrass((float*)&gun->attachment[2], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_8MM ); 
		EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_8MM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/uzi_fire.wav", 1, ATTN_LOW_HIGH, 0, 100 + gEngfuncs.pfnRandomLong( 0, 15) );
	}
	else if (Type == FIREGUN_SHIELDGUN)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_6, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(10,12), -25.0, 0.0, 0.03), 0, -1);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_MEDIUM);
		DynamicLight((float*)&gun->attachment[0], 220, 180,140,0, 0.1, 300.0);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_PISTOL_12G ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/shieldgun_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	}
	else if (Type == FIREGUN_SHOTGUN)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_7, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(10,12), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 250, 180,180,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_LARGE);
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/shotgun_fire.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_LOW, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs[PITCH] -= 5;
				gEngfuncs.SetViewAngles((float *)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_SHOTGUN_BR_SHELL)
	{
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHOTSHELL, SHELL_SHOTGUN_RED ); 
	}
	else if (Type == FIREGUN_AUTOSHOTGUN)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_8, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 250, 240,150,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_MEDIUM);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHOTSHELL, SHELL_SHOTGUN_10MM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/autoshotgun_fire.wav", gEngfuncs.pfnRandomFloat(0.95, 1.0), ATTN_LOW, 0, 93 + gEngfuncs.pfnRandomLong( 0, 0x1f ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs[PITCH] -= 2.8;
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_30MMSG)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_9, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(18,20), -40.0, 0.0, 0.04), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 300, 180,120,0, 0.15, 300.0);
		gEngfuncs.pEfxAPI->R_SparkEffect((float*)&gun->attachment[0], 10, -100, 100 );
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_LARGE);
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/30mmsg_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_LOW_HIGH, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
	}
	else if (Type == FIREGUN_MP5)
	{
		if (Special)//gren.launcher
		{
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/mp5_glauncher.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_10, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -25.0, 0.0, 0.03), 0, -1);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
			DynamicLight((float*)&gun->attachment[0], 200, 180,100,0, 0.1, 300.0);
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_9MM ); 
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/mp5_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
	}
	else if (Type == FIREGUN_M16)
	{
		if (Special)//gren.launcher
		{
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/m16_glauncher.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_4, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(13,15), -25.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 230, 180,100,0, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALL);
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762NATO ); 
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/m16_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
	}
	else if (Type == FIREGUN_AKIMBOGUN_LEFT)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_11, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		EV_GunSmoke((float*)&gun->attachment[2], GUNSMOKE_WHITE_SMALL);
		DynamicLight((float*)&gun->attachment[2], 230, 180,100,0, 0.1, 300.0);
		EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762MAGNUM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/akimbogun_aug_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=0.9;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.5,0.8),gEngfuncs.pfnRandomFloat(-0.6,1.0),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_AKIMBOGUN_RIGHT)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_39, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -25.0, 0.0, 0.03), 0, -1);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
		DynamicLight((float*)&gun->attachment[0], 230, 160,130,0, 0.1, 300.0);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762MAGNUM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[2], CHAN_WEAPON, "weapons/akimbogun_sg552_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=0.9;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.5,0.8),gEngfuncs.pfnRandomFloat(-0.6,1.0),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_AKIMBOGUN_BOTH)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_11, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_39, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -25.0, 0.0, 0.03), 0, -1);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
		EV_GunSmoke((float*)&gun->attachment[2], GUNSMOKE_WHITE_SMALL);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762MAGNUM ); 
		EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762MAGNUM ); 
		DynamicLight((float*)&gun->attachment[0], 270, 160,130,0, 0.1, 300.0);

	        switch (gEngfuncs.pfnRandomLong(0,1))
	        {
			case 0 : gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/akimbogun_aug_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );break;
			case 1 : gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[2], CHAN_STATIC, "weapons/akimbogun_sg552_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );break;
		}
		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=2;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.9,1.4),gEngfuncs.pfnRandomFloat(-0.8,1.7),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_AK74)
	{
		if (Special)//gren.launcher
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_27, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(16,18), -25.0, 0.0, 0.06), 0, -1);
			DynamicLight((float*)&gun->attachment[1], 245, 180,140,20, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound( 0, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/ak74_glauncher.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_19, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 230, 180,140,0, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALL);
			EV_EjectBrass((float*)&gun->attachment[2], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762 ); 
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/AK74_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
	}
	else if (Type == FIREGUN_CROSSBOW)
	{
		if (Special)//expl.bolt
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_8, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(14,16), -25.0, 0.0, 0.04), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 150, 0,160,110, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/xbow_fire_exp.wav", 1, ATTN_NORM, 0, PITCH_NORM );
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.1, gEngfuncs.pfnRandomFloat(0.2,0.35), 0.7, 0.8, 25, BLAST_SKIN_TELEENTER, 0, 255, 255, 255);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x3000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.1, gEngfuncs.pfnRandomFloat(0.2,0.35), 0.7, 0.8, 25, BLAST_SKIN_TELEENTER, 0, 255, 255, 255);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x4000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.1, gEngfuncs.pfnRandomFloat(0.2,0.35), 0.7, 0.8, 25, BLAST_SKIN_TELEENTER, 0, 255, 255, 255);
		}
		else//accel.bolt
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_6, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(14,16), -25.0, 0.0, 0.04), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 150, 0,160,110, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/xbow_fire_accel.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		}
	}
	else if (Type == FIREGUN_G11)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_17, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,11), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 180, 180,100,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALLEST);
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/g11_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	}
	else if (Type == FIREGUN_U2)
	{
		if (Special)//gren.launcher
		{
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/u2_glauncher.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_18, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,11), -25.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 230, 180,100,0, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_SMALL);
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_556 ); 
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/U2_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
	}
	else if (Type == FIREGUN_SVD)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_40, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,11), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 280, 180,140,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_MEDIUM);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_762x54 ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/svd_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=1.3;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.7,0.9),gEngfuncs.pfnRandomFloat(-0.8,1.3),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_AWP)
	{
		if (Special!=2)
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_16, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(12,14), -25.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 270, 210,200,60, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_MEDIUM);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/awp_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

			if (EV_IsLocal(EntIndex))
			{
				if (Special==0)
				{
					gEngfuncs.GetViewAngles((float*)view_ofs);
					view_ofs.x-=3;
					view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.8,0.8),gEngfuncs.pfnRandomFloat(-1.8,1.8),0.0f);
					gEngfuncs.SetViewAngles((float*)view_ofs);
				}
			}
		}
		else if (Special==2)//brass shell
		{
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_338MAGNUM ); 
		}
	}
	else if (Type == FIREGUN_BARETT)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_20, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(17,20), -25.0, 0.0, 0.05), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 300, 210,200,60, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_LARGE);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_127MM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/barett_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=4.5;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-0.8,1.1),gEngfuncs.pfnRandomFloat(-2.2,3.0),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_M249)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_38, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,12), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 250, 180,100,0, 0.1, 300.0);
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_BLACK_SMALLEST);
		EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_556NATO ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/m249_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	}
	else if (Type == FIREGUN_MINIGUN)
	{
		EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_87MM ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/minigun_fire.wav", 0.9, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
	}
	else if (Type == FIREGUN_NAILGUN)
	{
		if (Special == 0)//right barrel
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_36, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEfxAPI->R_SparkEffect((float *)&gun->attachment[0], 25, -100, 100 );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_36, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(6,8), -20.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEfxAPI->R_SparkEffect((float *)&gun->attachment[1], 25, -100, 100 );
		}
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/nailgun_fire.wav", gEngfuncs.pfnRandomFloat(0.92, 1.0), ATTN_LOW_HIGH, 0, 98 + gEngfuncs.pfnRandomLong( 0, 3 ) );
		DynamicLight((float*)&gun->attachment[0], 150, 180,100,0, 0.1, 300.0);
	}
	else if (Type == FIREGUN_AIRGUN)
	{
		if (EV_IsLocal(EntIndex))
		{
			V_PunchAxis( 0, -4.0 );
			gEngfuncs.pEventAPI->EV_WeaponAnimation ( 5, 1 );
		}
		EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_MEDIUM);
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/airgun_fire.wav", 1, ATTN_LOW_HIGH, 0, 100 );
	}
	else if (Type == FIREGUN_FLAKCANNON)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_30, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(18,22), -25.0, 0.0, 0.03), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 320, 220,150,0, 0.1, 300.0);

		if (Special)//secondary fire
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/flakcannon_fire2.wav", 1, ATTN_LOW, 0, 100);
		else
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/flakcannon_fire.wav", 1, ATTN_LOW, 0, 100);
	}
	else if (Type == FIREGUN_MACHINEGUN)
	{
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/machinegun_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong(0,10));

		switch (Special)
		{
		case 0://left gun
		case 1:
		case 5:
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_35, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,12), -20.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 275, 220,150,0, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[0], GUNSMOKE_WHITE_MEDIUM);
			EV_EjectBrass((float*)&gun->attachment[1], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_32MM ); 
		break;
		case 2://right gun
		case 3:
		case 4:
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_35, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,12), -20.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[2], 275, 220,150,0, 0.1, 300.0);
			EV_GunSmoke((float*)&gun->attachment[2], GUNSMOKE_WHITE_MEDIUM);
			EV_EjectBrass((float*)&gun->attachment[3], angles[YAW], TE_BOUNCE_SHELL, SHELL_RIFLE_32MM ); 
		break;
		}
	}
	else if (Type == FIREGUN_BFG)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_31, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(32,36), -50.0, 0.0, 0.07), 0, -1);
		DynamicLight((float*)&gun->attachment[0], 400, 180,100,0, 0.15, 300.0);
		EV_EjectBrass((float*)&gun->attachment[2], angles[YAW], TE_BOUNCE_SHOTSHELL, SHELL_BFG ); 
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/BFG_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );

		if (EV_IsLocal(EntIndex))
		{
			if (!Special)
			{
				gEngfuncs.GetViewAngles((float*)view_ofs);
				view_ofs.x-=8.2;
				view_ofs=view_ofs+Vector(gEngfuncs.pfnRandomFloat(-4.5,4.8),gEngfuncs.pfnRandomFloat(-0.8,2.1),0.0f);
				gEngfuncs.SetViewAngles((float*)view_ofs);
			}
		}
	}
	else if (Type == FIREGUN_BFG_SEC)
	{
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 180,100,0, 240, 2.3, 0, EntIndex));
		g_pRenderManager->AddSystem(new CRSBeamStar((float*)&gun->attachment[0], gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 0, 64, kRenderTransAdd, 180,100,0, 0, 0.25, 25, 20, 2.5), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar((float*)&gun->attachment[0], gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 230,140,0, 0, 0.25, 45, 30, 2.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == FIREGUN_SMARTGUN)
	{
	//	if(Special)//full power
	//	{
	//		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_1, kRenderTransAdd, 255,255,255, 1.0, -1.2, gEngfuncs.pfnRandomFloat(14,16), -20.0, 0.0, 0.06), 0, -1);
	//		DynamicLight((float*)&gun->attachment[0], 270, 0,90,250, 0.1, 300.0);
	//		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/smartgun_fire2.wav", 1, ATTN_LOW_HIGH, 0, 100);
	//	}
	//	else
	//	{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_1, kRenderTransAdd, 255,255,255, 1.0, -1.2, gEngfuncs.pfnRandomFloat(10,12), -20.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 210, 30,200,250, 0.1, 300.0);
		//	gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/smartgun_fire.wav", 1, ATTN_NORM, 0, 100);
	//	}
	}
	else if (Type == FIREGUN_TESLAGUN)
	{
		if(Special)//gren.launcher
		{
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/tesla_launcher_fire.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), PARTICLE_VIOLET_3, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,11), -20.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/tesla_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
			DynamicLight((float*)&gun->attachment[0], 220, 220,220,0, 0.1, 300.0);
		}
	}
	else if (Type == FIREGUN_EGON)
	{
		if(Special == 0){
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, 0.0, 14, 0.0, 0.0, 120), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, 0.0, 12, 0.0, 0.0, 120), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 0,128,250, 200, 120, 0, EntIndex));
		}
		else{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), 0, kRenderTransAdd, 255,255,255, 0.8, 0.0, 14, 0.0, 0.0, 120), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), 1, kRenderTransAdd, 255,255,255, 0.8, 0.0, 12, 0.0, 0.0, 120), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[1], 200,200,0, 200, 120, 0, EntIndex));
		}
	}
	else if (Type == FIREGUN_PLASMARIFLE)
	{
		if (Special)//secondary fire
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_4, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(15,18), -50.0, 0.0, 0.07), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 280, 0,190,0, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/plasmarifle_fire2.wav", 1, ATTN_LOW_HIGH, 0, 100);
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(10,12), -25.0, 0.0, 0.03), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 230, 0,190,20, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/plasmarifle_fire.wav", 1, ATTN_LOW_HIGH, 0, 100);
		}
	}
	else if (Type == FIREGUN_PHOTONGUN)
	{
		if(Special == 0)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_2, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(7,9), -25.0, 0.0, 0.03), 0, -1);
		else if(Special == 1)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_2, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(7,9), -25.0, 0.0, 0.03), 0, -1);
		if(Special == 2)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_2, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(7,9), -25.0, 0.0, 0.03), 0, -1);
		if(Special == 3)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[3], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_2, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(7,9), -25.0, 0.0, 0.03), 0, -1);

		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/photongun_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		DynamicLight((float*)&gun->attachment[0], 320, 0,90,250, 0.15, 300.0);
	}
	else if (Type == FIREGUN_PHOTONGUN_EXP)
	{
		if(Special == 0)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 140,90,250, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		else if(Special == 1)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[1], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 140,90,250, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		if(Special == 2)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[2], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 140,90,250, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);
		if(Special == 3)
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[3], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 140,90,250, 0.8, -1.2, gEngfuncs.pfnRandomFloat(8,10), -25.0, 0.0, 0.03), 0, -1);

		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/photongun_fire2.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		DynamicLight((float*)&gun->attachment[0], 300, 140,90,250, 0.1, 300.0);
	}
	else if (Type == FIREGUN_GAUSS)
	{
		if(Special)//full power
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_2, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(14,17), -20.0, 0.0, 0.06), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/gauss_fire2.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
			DynamicLight((float*)&gun->attachment[0], 300, 255,255,50, 0.15, 300.0);
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_37, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(12,14), -20.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/gauss_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
			DynamicLight((float*)&gun->attachment[0], 250, 240,170,30, 0.1, 300.0);
		}
	}
	else if (Type == FIREGUN_TAUCANNON)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), PARTICLE_VIOLET_1, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(17,20), -25.0, 0.0, 0.06), 0, EntIndex);
		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/taucannon_fire.wav", 1, ATTN_LOW_HIGH, 0, gEngfuncs.pfnRandomLong(90, 105) );
		DynamicLight((float*)&gun->attachment[0], 350, 160,0,210, 0.15, 300.0);
	}
	else if (Type == FIREGUN_GLUONGUN)
	{
		if (Special == 0)//primary fire
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_4, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(20,25), -30.0, 0.0, 0.06), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 250, 170,90,250, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/gluongun_zap.wav", 1, ATTN_LOW_HIGH, 0, 100);
		}
		if (Special == 1)//spin up
		{
			g_pRenderManager->AddSystem(new CRSBeamStar((float*)&gun->attachment[0], gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), PARTICLE_VIOLET_3, 64, kRenderTransAdd, 255,255,255, 0.4, 0, 10, 8, 3.4), 0, EntIndex);
			g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 170,90,250, 200, 3.2, 0, EntIndex));
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/gluongun_charge.wav", 1, ATTN_NORM, 0, 100);
		}
		if (Special == 2)//secondary fire
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), PARTICLE_BLUE_4, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(20,25), -30.0, 0.0, 0.06), 0, -1);
			DynamicLight((float*)&gun->attachment[0], 300, 170,90,250, 0.1, 300.0);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/gluongun_fire.wav", 1, ATTN_LOW_HIGH, 0, 100);

			CRenderSystem *pSystem = NULL;
			while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
			{
				pSystem->m_iFollowEntity = -1;
				pSystem->m_fDieTime = 0.1f;
			}
		}
	}
	else if (Type == FIREGUN_DISPLACER)
	{
		if (Special == 0)//fire
		{
			DynamicLight((float*)&gun->attachment[0], 300, 85,250,85, 0.1, 300.0);
			V_PunchAxis( 0, -10 );
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/displacer_fire.wav", 1, ATTN_LOW_HIGH, 0, 100 + gEngfuncs.pfnRandomLong( -5, 5 ));
		}
		if (Special == 1)//spin up
		{
			g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 85,250,85, 200, 1.5, 0, EntIndex));
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 1.5, gEngfuncs.pfnRandomFloat(0.3,0.5), 0.7, 0.5, 25, 0, 15, 0, 250, 0);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x3000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 1.5, gEngfuncs.pfnRandomFloat(0.4,0.6), 0.7, 0.5, 25, 0, 15, 0, 180, 0);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x4000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 1.5, gEngfuncs.pfnRandomFloat(0.3,0.5), 0.7, 0.5, 25, 0, 15, 0, 220, 0);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/displacer_spin.wav", 1, ATTN_NORM, 0, 100);
		}
		if (Special == 2)//remove effects 
		{
			CRenderSystem *pSystem = NULL;
			while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
			{
				pSystem->m_iFollowEntity = -1;
				pSystem->m_fDieTime = 0.1f;
			}
			gEngfuncs.pEfxAPI->R_BeamKill(gun->index | 0x1000);
		}
	}
	else if (Type == 58)//VALVE SWORD
	{
		if (Special == 0)//fire
		{
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "rmxp/086-Action01.wav", 1, ATTN_NORM, 0, 100);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 3.0, gEngfuncs.pfnRandomFloat(0.5,0.7), 0.7, 0.5, 25, 0, 15, 250, 0, 0);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 3.0, gEngfuncs.pfnRandomFloat(0.5,0.7), 0.7, 0.5, 25, 0, 15, 250, 200, 0);
		}
		if (Special == 1)//spin up
		{
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 2.0, gEngfuncs.pfnRandomFloat(0.5,0.7), 0.7, 0.5, 25, 0, 15, 250, 200, 0);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), 2.0, gEngfuncs.pfnRandomFloat(0.3,0.5), 0.6, 0.4, 20, 0, 10, 250, 200, 0);
		}
		if (Special == 2)//remove effects 
		{
			CRenderSystem *pSystem = NULL;
			while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
			{
				pSystem->m_iFollowEntity = -1;
				pSystem->m_fDieTime = 0.1f;
			}
			gEngfuncs.pEfxAPI->R_BeamKill(gun->index | 0x1000);
		}
		if (Special == 3)//spin up
		{
			gEngfuncs.pEfxAPI->R_BeamFollow(gun->index | 0x1000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 0.3, 1, 250,250,0, 0.2);
		}
	}
	else if (Type == 59)//ȭͷ
	{
		if (Special == 0)//primary fire
		{
			g_pRenderManager->AddSystem(new CRSBeamStar((float*)&gun->attachment[0], gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 0, 50, kRenderTransAdd, 100,100,100, 0, 0.1, 5, 6, 5.0), 0, EntIndex);
			g_pRenderManager->AddSystem(new CRSBeamStar((float*)&gun->attachment[0], gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 50, kRenderTransAdd, 100,100,100, 0, 0.1, 5, 4, 5.0), 0, EntIndex);
		}
		if (Special == 1)//secondary fire
		{
			CRenderSystem *pSystem = NULL;
			while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
			{
				pSystem->m_iFollowEntity = -1;
				pSystem->m_fDieTime = 0.1f;
			}
		}
	}
	else if (Type == FIREGUN_BLASTER)
	{
		if(Special==0)//pulse mode
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(17,20), -20.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/blaster_fire.wav", 1, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
			DynamicLight((float*)&gun->attachment[0], 240, 240,0,0, 0.1, 300.0);
		}
		if(Special==1)//beam mode
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_0, kRenderTransAdd, 255,255,255, 0.8, 0.0, gEngfuncs.pfnRandomFloat(10,12), 0.0, 0.0, 60), 0, EntIndex);
			g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 240,0,0, 120, 210, 0, EntIndex));
		}
	}
	else if (Type == FIREGUN_PULSERIFLE)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), PARTICLE_VIOLET_4, kRenderTransAdd, 0,128,250, 0.8, -1.2, gEngfuncs.pfnRandomFloat(9,11), -25.0, 0.0, 0.04), 0, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/pulserifle_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		DynamicLight((float*)&gun->attachment[0], 280, 255,255,30, 0.15, 300.0);
	}
	else if (Type == FIREGUN_M72)
	{
		if(Special)//quad
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_3, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(12,14), -25.0, 0.0, 0.06), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/m72_fire_quad.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
		else
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_23, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(12,14), -25.0, 0.0, 0.03), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/m72_fire.wav", 1, ATTN_LOW_HIGH, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
	}
	else if (Type == FIREGUN_CHRONOSCEPTOR)
	{
		if (Special == 0)//fire
		{
			g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_8, kRenderTransAdd, 128,128,128, 0.8, -1.2, gEngfuncs.pfnRandomFloat(20,23), -50.0, 0.0, 0.05), 0, -1);
			gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/chronosceptor_fire.wav", 1, ATTN_LOW, 0, 94 + gEngfuncs.pfnRandomLong( 0, 0xf ) );
		}
		if (Special == 1)//spin up
		{
			gEngfuncs.pEfxAPI->R_Implosion( (float *)&gun->attachment[0], 50, 10, 0.2 );
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x2000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 3, gEngfuncs.pfnRandomFloat(0.18,0.3), 2, 0.8, 25, 0, 15, 0, 90, 255);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x3000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 3, gEngfuncs.pfnRandomFloat(0.18,0.3), 2, 0.8, 25, 0, 15, 0, 90, 255);
			gEngfuncs.pEfxAPI->R_BeamEnts(gun->index | 0x1000, gun->index | 0x4000, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 3, gEngfuncs.pfnRandomFloat(0.18,0.3), 2, 0.8, 25, 0, 15, 0, 90, 255);
		}
		if (Special == 2)//remove effects 
		{
			CRenderSystem *pSystem = NULL;
			while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
			{
				pSystem->m_iFollowEntity = -1;
				pSystem->m_fDieTime = 0.1f;
			}
			gEngfuncs.pEfxAPI->R_BeamKill(gun->index | 0x1000);
		}
	}
	else if (Type == FIREGUN_IONTURRET)
	{
		g_pRenderManager->AddSystem(new CRSSprite((float*)&gun->attachment[0], Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), PARTICLE_VIOLET_0, kRenderTransAdd, 255,255,255, 0.8, -1.2, gEngfuncs.pfnRandomFloat(17,21), -25.0, 0.0, 0.03), 0, -1);
		gEngfuncs.pEventAPI->EV_PlaySound(0, (float*)&gun->attachment[0], CHAN_WEAPON, "turret/tu_fire_ion.wav", 1, ATTN_LOW_HIGH, 0, 100 );
		DynamicLight((float*)&gun->attachment[0], 300, 0,90,200, 0.15, 300.0);
	}
	else if (Type == FIREGUN_TURRETKIT)
	{
		gEngfuncs.pEventAPI->EV_PlaySound(EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/psp_fire.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		DynamicLight((float*)&gun->attachment[0], 170, 0,235,155, 0.2, 250.0);
	}
	else if (Type == FIREGUN_TORCH)
	{
		g_pRenderManager->AddSystem(new CPSTrail(100, 0.8, 1.0, 10, (float*)&gun->attachment[0], Vector(0,0,-1), Vector(0.2,0.2,0.4), 2, 2, 160,160,250, 0.8, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 200), RENDERSYSTEM_FLAG_RANDOMFRAME, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 220,150,0, 60, 200, 0, EntIndex));
	}
	else if (Type == FIREGUN_LIGHTSABER)
	{
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 240,0,0, 10, 999, 0, EntIndex));
	}
	else if (Type == FIREGUN_FTHROWER)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 0.8, 1.0, 10, (float*)&gun->attachment[0], Vector(0,0,1), Vector(0.2,0.2,0.4), 2, 3, 120,120,120, 0.8, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 600), RENDERSYSTEM_FLAG_RANDOMFRAME, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight((float*)&gun->attachment[0], 220,150,0, 60, 600, 0, EntIndex));
	}
	else if (Type == FIREGUN_DEVASTATOR)
	{
		DynamicLight((float*)&gun->attachment[0], 180, 128,128,0, 0.1, 300.0);
		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/devastator_fire.wav", 1, ATTN_LOW, 0, PITCH_NORM );
	}
	else if (Type == FIREGUN_RPG)
	{
		vec3_t dir;
		VectorMA((float*)&gun->attachment[1], -1, (float*)&gun->attachment[0], dir);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 25, (float *)&gun->attachment[1], dir, Vector(0,0,0), 25, -100, 128,128,128, 1.0, -1.2f, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.2), 0, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/rocketfire1.wav", 1, ATTN_LOW, 0, PITCH_NORM );
	}
	else if (Type == FIREGUN_INCENDIARY)
	{
		DynamicLight((float*)&gun->attachment[0], 180, 220,150,0, 0.1, 300.0);
		gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, (float*)&gun->attachment[0], CHAN_WEAPON, "weapons/incendiary_fire.wav", 1, ATTN_LOW, 0, PITCH_NORM );
	}
return 1;
}

//===================//
//enpty clip ejecting//
//===================//
int __MsgFunc_BrassClip(const char *pszName, int iSize, void *pbuf)
{
	vec3_t origin, angles, forward, right, up;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	angles.x = READ_COORD();
	angles.y = READ_COORD();
	angles.z = READ_COORD();
	int EntIndex = READ_BYTE();
	int Type = READ_BYTE();

	AngleVectors( angles, forward, right, up );

	if ( Type == CLIP_GLOCK)
	{
		EV_BrassGunClip( origin+(forward*12+right*7+up*-30), CLIP_GLOCK); 
	}
	else if ( Type == CLIP_GLOCK_DUAL)
	{
		EV_BrassGunClip( origin+(forward*13+right*9+up*-30), CLIP_GLOCK); 
		EV_BrassGunClip( origin+(forward*10+right*3+up*-30), CLIP_GLOCK); 
	}
	else if ( Type == CLIP_USP)
	{
		EV_BrassGunClip( origin+(forward*12+right*6+up*-30), CLIP_USP); 
	}
	else if ( Type == CLIP_DEAGLE)
	{
		EV_BrassGunClip( origin+(forward*12+right*7+up*-30), CLIP_DEAGLE); 
	}
	else if ( Type == CLIP_UZI)
	{
		EV_BrassGunClip( origin+(forward*12+right*-7+up*-30), CLIP_UZI); 
	}
	else if ( Type == CLIP_UZI_RIGHT)
	{
		EV_BrassGunClip( origin+(forward*13+right*7+up*-30), CLIP_UZI); 
	}
	else if ( Type == CLIP_UZI_LEFT)
	{
		EV_BrassGunClip( origin+(forward*9+right*-9+up*-30), CLIP_UZI); 
	}
	else if ( Type == CLIP_SHIELDGUN )
	{
		EV_BrassGunClip( origin+(forward*6+right*-3+up*-30), CLIP_SHIELDGUN); 
	}
	else if ( Type == CLIP_MP5 )
	{
		EV_BrassGunClip( origin+(forward*12+up*-30), CLIP_MP5); 
	}
	else if ( Type == CLIP_M16 )
	{
		EV_BrassGunClip( origin+(forward*12+right*2+up*-30), CLIP_M16); 
	}
	else if ( Type == CLIP_AKIMBOGUN_SG552 )
	{
		EV_BrassGunClip( origin+(forward*13+right*7+up*-30), CLIP_AKIMBOGUN_SG552); 
	}
	else if ( Type == CLIP_AKIMBOGUN_AUG )
	{
		EV_BrassGunClip( origin+(forward*9+right*-7+up*-30), CLIP_AKIMBOGUN_AUG); 
	}
	else if ( Type == CLIP_AK74 )
	{
		EV_BrassGunClip( origin+(forward*12+up*-30), CLIP_AK74); 
	}

	else if ( Type == CLIP_CROSSBOW )
	{
		EV_BrassGunClip( origin+(forward*9+right*1+up*-30), CLIP_CROSSBOW); 
	}
	else if ( Type == CLIP_G11 )
	{
		EV_BrassGunClip( origin+(forward*12+right*3+up*-30), CLIP_G11); 
	}
	else if ( Type == CLIP_U2 )
	{
		EV_BrassGunClip( origin+(forward*12+right*-3+up*-30), CLIP_U2); 
	}
	else if ( Type == CLIP_SVD )
	{
		EV_BrassGunClip( origin+(forward*8+right*-2+up*-30), CLIP_SVD); 
	}
	else if ( Type == CLIP_AWP )
	{
		EV_BrassGunClip( origin+(forward*12+right*4+up*-30), CLIP_AWP); 
	}
	else if ( Type == CLIP_BARETT )
	{
		EV_BrassGunClip( origin+(forward*10+up*-30), CLIP_BARETT); 
	}
	else if ( Type == CLIP_M249 )
	{
		EV_BrassGunClip( origin+(forward*12+right*4+up*-30), CLIP_M249); 
	}
	else if ( Type == CLIP_MINIGUN )
	{
		EV_BrassGunClip( origin+(forward*12+right*3+up*-30), CLIP_MINIGUN); 
	}
	else if ( Type == CLIP_NAILGUN )
	{
		EV_BrassGunClip( origin+(forward*11+right*4+up*-30), CLIP_NAILGUN); 
	}
	else if ( Type == CLIP_NAILGUN_LEFT )
	{
		EV_BrassGunClip( origin+(forward*8+up*-30), CLIP_NAILGUN); 
	}
	else if ( Type == CLIP_FROSTER)
	{
		EV_BrassGunClip( origin+(forward*12+right*4+up*-30), CLIP_FROSTER); 
	}
	else if ( Type == CLIP_FLAMETHROWER )
	{
		EV_BrassGunClip( origin+(forward*12+right*4+up*-30), CLIP_FLAMETHROWER); 
	}
	else if ( Type == CLIP_MACHINEGUN )
	{
		EV_BrassGunClip( origin+(forward*11+right*7+up*-30), CLIP_MACHINEGUN); 
	}
	else if ( Type == CLIP_MACHINEGUN_LEFT)
	{
		EV_BrassGunClip( origin+(forward*7+right*-7+up*-30), CLIP_MACHINEGUN); 
	}
	else if ( Type == CLIP_BFG)
	{
		EV_BrassGunClip( origin+(forward*5+up*-30), CLIP_BFG); 
	}
	else if ( Type == CLIP_FLAKCANNON)
	{
		EV_BrassGunClip( origin+(forward*10+right*3+up*-35), CLIP_FLAKCANNON); 
	}
	else if ( Type == CLIP_INCENDIARY)
	{
		EV_BrassGunClip( origin+(forward*10+up*-30), CLIP_INCENDIARY); 
	}
	else if ( Type == CLIP_TESLAGUN)
	{
		EV_BrassGunClip( origin+(forward*10+right*-3+up*-30), CLIP_TESLAGUN); 
	}
	else if ( Type == CLIP_EGON)
	{
		EV_BrassGunClip( origin+(forward*12+right*8+up*-30), CLIP_EGON); 
	}
	else if ( Type == CLIP_EGON_MIDDLE)
	{
		EV_BrassGunClip( origin+(forward*6+up*-30), CLIP_EGON); 
	}
	else if ( Type == CLIP_EGON_LEFT)
	{
		EV_BrassGunClip( origin+(forward*10+right*-8+up*-30), CLIP_EGON); 
	}
	else if ( Type == CLIP_PLASMARIFLE)
	{
		EV_BrassGunClip( origin+(forward*8+right*-8+up*-30), CLIP_PLASMARIFLE); 
	}
	else if ( Type == CLIP_PHOTONGUN)
	{
		EV_BrassGunClip( origin+(forward*5+up*-30), CLIP_PHOTONGUN); 
	}
	else if ( Type == CLIP_GAUSS)
	{
		EV_BrassGunClip( origin+(forward*7+right*3+up*-30), CLIP_GAUSS); 
	}
	else if ( Type == CLIP_TAUCANNON)
	{
		EV_BrassGunClip( origin+(forward*10+right*3+up*-30), CLIP_TAUCANNON); 
	}
	else if ( Type == CLIP_GLUONGUN)
	{
		EV_BrassGunClip( origin+(forward*12+right*3+up*-30), CLIP_GLUONGUN); 
	}
	else if ( Type == CLIP_GLUONGUN_LEFT)
	{
		EV_BrassGunClip( origin+(forward*5+right*-3+up*-30), CLIP_GLUONGUN); 
	}
	else if ( Type == CLIP_DISPLACER)
	{
		EV_BrassGunClip( origin+(forward*10+right*3+up*-30), CLIP_DISPLACER); 
	}
	else if ( Type == CLIP_BIORIFLE)
	{
		EV_BrassGunClip( origin+(forward*10+right*3+up*-30), CLIP_BIORIFLE); 
	}
	else if ( Type == CLIP_PULSERIFLE)
	{
		EV_BrassGunClip( origin+(forward*7+right*-4+up*-30), CLIP_PULSERIFLE); 
	}
	else if ( Type == CLIP_M72)
	{
		EV_BrassGunClip( origin+(forward*2+right*2+up*-30), CLIP_M72); 
	}
	else if ( Type == CLIP_CHRONOSCEPTOR)
	{
		EV_BrassGunClip( origin+(forward*10+right*5+up*-30), CLIP_CHRONOSCEPTOR); 
	}
	else if ( Type == CLIP_CHRONOSCEPTOR_LEFT)
	{
		EV_BrassGunClip( origin+(forward*5+right*-4+up*-30), CLIP_CHRONOSCEPTOR); 
	}
return 1;
}

//==================//
//Player gib effects//
//==================//
int __MsgFunc_PlrGib(const char *pszName, int iSize, void *pbuf)
{
	vec3_t origin, gib_angles, vecScale;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	int Type = READ_BYTE();

        int i;
        TEMPENTITY *pGibHead = NULL;
        TEMPENTITY *pGib = NULL;

        gib_angles[0] = gEngfuncs.pfnRandomLong (-100, 100);
        gib_angles[1] = gEngfuncs.pfnRandomLong (-100, 100);
        gib_angles[2] = gEngfuncs.pfnRandomLong (-100, 100);

	if ( Type == GIBBED_BODY)// blasts all body
	{
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(300, 400), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 36;
			pGibHead->entity.baseline.sequence = 666;
			pGibHead->entity.baseline.iuser2 = GIBBED_BODY;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}	
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(300, 400), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 37;
			pGibHead->entity.baseline.sequence = 666;
			pGibHead->entity.baseline.iuser2 = GIBBED_BODY;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}	
	        for (i = 0; i < 12; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(500, 700), vecScale);

			pGib = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGib->entity.curstate.body = gEngfuncs.pfnRandomLong(38, 49);
			pGib->entity.baseline.sequence = 666;
			pGib->entity.baseline.iuser2 = GIBBED_BODY;
			pGib->entity.baseline.iuser1 = 15;
			pGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGib->hitcallback = EV_GibTouch;
		}

		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(25,40), gEngfuncs.pfnRandomFloat(175,220), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.8, 2.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(25,40), gEngfuncs.pfnRandomFloat(180,230), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2.2, 3.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(12, gEngfuncs.pfnRandomFloat(180,210), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(10, 12), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_3), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(8,12), gEngfuncs.pfnRandomFloat(2,3), gEngfuncs.pfnRandomFloat(250, 350), SPARKSHOWER_BLOODDRIPS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_BODY, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
	else if ( Type == GIBBED_HEAD)// blasts Head ONLY!
	{
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(90, 120), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 50;
			pGibHead->entity.baseline.sequence = 666;
			pGibHead->entity.baseline.iuser2 = GIBBED_BODY;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(90, 120), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 51;
			pGibHead->entity.baseline.sequence = 666;
			pGibHead->entity.baseline.iuser2 = GIBBED_BODY;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 15; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(120, 180), vecScale);

			pGib = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGib->entity.curstate.body = gEngfuncs.pfnRandomLong(52, 56);
			pGib->entity.curstate.scale = gEngfuncs.pfnRandomFloat(0.5, 1);
			pGib->entity.baseline.sequence = 666;
			pGib->entity.baseline.iuser2 = GIBBED_BODY;
			pGib->entity.baseline.iuser1 = 15;
			pGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGib->hitcallback = EV_GibTouch;
		}
		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(15,25), gEngfuncs.pfnRandomFloat(60,130), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1, 2.2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(10,20), gEngfuncs.pfnRandomFloat(60,130), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.9, 2.8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlood(8, gEngfuncs.pfnRandomFloat(60,80), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_3), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
     		g_pRenderManager->AddSystem(new CPSBlastCone(5, 20, origin, origin, Vector(1,1,1), 5, 35, 90,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_BODY, "common/bodysplat.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
	else if ( Type == GIBBED_FROZEN)
	{
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(250, 350), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 36;
			pGibHead->entity.curstate.rendercolor.r = 0;
			pGibHead->entity.curstate.rendercolor.g = 90;
			pGibHead->entity.curstate.rendercolor.b = 250;
			pGibHead->entity.curstate.renderfx = kRenderFxGlowShell;
			pGibHead->entity.baseline.iuser2 = GIBBED_FROZEN;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(250, 350), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 37;
			pGibHead->entity.curstate.rendercolor.r = 0;
			pGibHead->entity.curstate.rendercolor.g = 90;
			pGibHead->entity.curstate.rendercolor.b = 250;
			pGibHead->entity.curstate.renderfx = kRenderFxGlowShell;
			pGibHead->entity.baseline.iuser2 = GIBBED_FROZEN;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 12; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(400, 600), vecScale);

			pGib = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGib->entity.curstate.body = gEngfuncs.pfnRandomLong(38, 51);
			pGib->entity.curstate.rendercolor.r = 0;
			pGib->entity.curstate.rendercolor.g = 90;
			pGib->entity.curstate.rendercolor.b = 250;
			pGib->entity.curstate.renderfx = kRenderFxGlowShell;
			pGib->entity.baseline.iuser2 = GIBBED_FROZEN;
			pGib->entity.baseline.iuser1 = 15;
			pGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM);
			pGib->hitcallback = EV_GibTouch;
		}
	}
	else if ( Type == 623)
	{
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(8,13), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(250, 300), SPARKSHOWER_FIRESMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if ( Type == GIBBED_ELECTRO)
	{
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(200, 300), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 67;
			pGibHead->entity.baseline.sequence = 2;
			pGibHead->entity.baseline.iuser2 = GIBBED_IGNITE;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 1; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(200, 300), vecScale);

			pGibHead = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGibHead->entity.curstate.body = 68;
			pGibHead->entity.baseline.sequence = 2;
			pGibHead->entity.baseline.iuser2 = GIBBED_IGNITE;
			pGibHead->entity.baseline.iuser1 = 15;
			pGibHead->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGibHead->hitcallback = EV_GibTouch;
		}
	        for (i = 0; i < 12; i++)
		{
			vec3_t dir = (VectorRandom() + VectorRandom());
			VectorNormalize(dir);
			VectorScale(dir, gEngfuncs.pfnRandomFloat(300, 380), vecScale);

			pGib = gEngfuncs.pEfxAPI->R_TempModel (origin, vecScale, gib_angles, gHUD.GibsLifeCvar->value, gEngfuncs.pEventAPI->EV_FindModelIndex("models/w_gibs_all.mdl"), TE_BOUNCE_NULL);
			pGib->entity.curstate.body = gEngfuncs.pfnRandomLong(69, 75);
			pGib->entity.baseline.sequence = 2;
			pGib->entity.baseline.iuser2 = GIBBED_IGNITE;
			pGib->entity.baseline.iuser1 = 15;
			pGib->flags |= (FTENT_COLLIDEWORLD | FTENT_ROTATE | FTENT_CLIENTCUSTOM | FTENT_SMOKETRAIL);
			pGib->hitcallback = EV_GibTouch;
		}
		g_pRenderManager->AddSystem(new CPSBlastCone(6, gEngfuncs.pfnRandomFloat(50,80), origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 15, 40, 0,0,0, 0.7, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,6), 2, gEngfuncs.pfnRandomFloat(40, 60), SPARKSHOWER_FIRESMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,6), 3, gEngfuncs.pfnRandomFloat(50, 70), SPARKSHOWER_FIRESMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,6), 2, gEngfuncs.pfnRandomFloat(40, 60), SPARKSHOWER_SMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,6), 3, gEngfuncs.pfnRandomFloat(50, 70), SPARKSHOWER_SMOKE, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
return 1;
}

//===========================//
//Projectile trails & impacts//
//===========================//
int __MsgFunc_Trail(const char *pszName, int iSize, void *pbuf)
{
	vec3_t dir, origin;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	int EntIndex = READ_SHORT();
	int Type = READ_BYTE();

	if (Type == PROJ_REMOVE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
	}
	if (Type == PROJ_FLAME)
	{
	//	g_pRenderManager->AddSystem(new CPSTrail(30, 0.8, 1.1, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 25, 255,255,255, 0.8, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.8), 0, -1);
	//	gEngfuncs.pEventAPI->EV_PlaySound( EntIndex, origin, CHAN_WEAPON, "weapons/flamethrower.wav", 1, ATTN_NORM, 0, 100);
		g_pRenderManager->AddSystem(new CPSTrail(15, 2, 2, 15, origin, origin, Vector(1,1,1), 5, 25, 160,160,160, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 1.25), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(10, 3, 3, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 15, 0,0,0, 0.2, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1.25), 0, EntIndex);
	//	g_pRenderManager->AddSystem(new CRSLight(origin, 220,150,0, 250, 1.25, 0, EntIndex));
	}
	else if (Type == PROJ_KADOMA_FLY)
	{
		g_pRenderManager->AddSystem(new CPSTrail(20, 4, 4, 20, origin, origin, Vector(1,1,1), 10, 30, 255,255,255, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 1.25), 0, EntIndex);
	}
	else if (Type == PROJ_HYDRA_SHOCKEFF)
	{
		g_pRenderManager->AddSystem(new CPSTrail(15, 3, 3, 15, origin, origin, Vector(1,1,1), 5, 25, 128,255,128, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 1.25), 0, EntIndex);
	}
	else if (Type == PROJ_GMAN_BURNFIRE)
	{
		g_pRenderManager->AddSystem(new CPSTrail(15, 3, 3, 15, origin, origin, Vector(1,1,1), 5, 25, 192,192,192, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 1.25), 0, EntIndex);
	}
	else if (Type == PROJ_FLAME_DETONATE)
	{
		/*
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		*/
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "majo/flame_hitwall.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 100, origin, origin, Vector(1,1,1), 5, 80, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 180, origin, origin, Vector(1,1,1), 10, 120, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_FLAME_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "weapons/flame_hitwall.wav", 1, ATTN_NORM, 0, 200);
		g_pRenderManager->AddSystem(new CPSBubbles(10, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 90, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 15, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_ICE)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 1, 1.3, 25, origin, origin, Vector(1,1,1), 2, 50, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 10), 0, EntIndex);
	}
	else if (Type == PROJ_ICE_DETONATE)
	{
		/*
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}*/
		g_pRenderManager->AddSystem(new CPSBlastCone(16, 100, origin, origin, Vector(1,1,1), 5, 110, 255,255,255, 0.3, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 75, origin, origin, Vector(1,1,1), 5, 85, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 50, origin, origin, Vector(1,1,1), 5, 85, 255,255,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(6,9), 5, gEngfuncs.pfnRandomFloat(160, 200), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(4,7), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_5, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(6,9), 5, gEngfuncs.pfnRandomFloat(150, 190), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,3), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 400, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 300, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(10, origin, 0.5, 0.01, 200, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_ICE_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}

		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/glass1.wav", 1, ATTN_NORM, 0, 200);
		g_pRenderManager->AddSystem(new CPSBubbles(25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 80, origin, origin, Vector(1,1,1), 5, 90, 255,255,255, 0.3, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 55, origin, origin, Vector(1,1,1), 5, 75, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 40, origin, origin, Vector(1,1,1), 5, 65, 255,255,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_NERVEBLUE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.2f;
		}

		g_pRenderManager->AddSystem(new CPSTrail(60, 2.5, 4, 20, origin, Vector(1,1,1), Vector(1,1,1), 5, 25, 60,90,180, 0.6, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 15), 0, EntIndex);
	}
	else if (Type == PROJ_NERVEGREN)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.2f;
		}

		g_pRenderManager->AddSystem(new CPSTrail(120, 2.5, 4, 25, origin, Vector(1,1,1), Vector(1,1,1), 5, 30, 0,100,0, 0.6, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 15), 0, EntIndex);
	}
	else if (Type == PROJ_NERVEGREN_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}

		dir.z=10;
		dir=dir.Normalize();

		if ( gEngfuncs.PM_PointContents(origin+(dir*10), NULL ) != CONTENTS_WATER)
		{
			g_pRenderManager->AddSystem(new CPSTrail(100, 2.5, 4, gEngfuncs.pfnRandomFloat(75,150), origin+(dir*gEngfuncs.pfnRandomFloat(10,30)), Vector(0,0,1), Vector(1,1,1), 10, gEngfuncs.pfnRandomFloat(60,100), 0,100,0, 0.5, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 16), 0, EntIndex);
			g_pRenderManager->AddSystem(new CPSTrail(100, 3, 4.5, gEngfuncs.pfnRandomFloat(60,120), origin+(dir*gEngfuncs.pfnRandomFloat(10,30)), Vector(0,0,1), Vector(2,2,1), 10, gEngfuncs.pfnRandomFloat(50,90), 0,80,0, 0.8, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 16), 0, EntIndex);
		}
		else
			g_pRenderManager->AddSystem(new CPSBubbles(100, origin+(dir*10), Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(90,130), gEngfuncs.pfnRandomFloat(3,6), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 16), 0, EntIndex);
	}
	else if (Type == PROJ_30MMGREN)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 0,0,0, 1, 10, PROJ_30MMGREN, EntIndex));
	}
	else if (Type == PROJ_30MMGREN_DETONATE)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		DynamicLight(origin, 300, 128,128,40, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSSparks(100, origin, 0.5, 0.01, 350, 2.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 100, origin, origin, Vector(1,1,1), 5, 80, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 100, origin, origin, Vector(1,1,1), 5, 80, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 180, origin, origin, Vector(1,1,1), 10, 120, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 180, origin, origin, Vector(1,1,1), 10, 120, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 240, origin, origin, Vector(1,1,1), 10, 150, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 240, origin, origin, Vector(1,1,1), 10, 150, 0,0,0, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(2,5), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(250, 300), SPARKSHOWER_STREAKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_30MMGREN_DETONATE_WATER)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		DynamicLight(origin, 200, 128,128,40, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 50, origin, origin, Vector(1,1,1), 5, 80, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 50, origin, origin, Vector(1,1,1), 5, 80, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 120, origin, origin, Vector(1,1,1), 10, 120, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 120, origin, origin, Vector(1,1,1), 10, 120, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 190, origin, origin, Vector(1,1,1), 10, 150, 128,128,128, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 190, origin, origin, Vector(1,1,1), 10, 150, 0,0,0, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_AK74)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 25, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 40, 0,0,0, 0.8, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 10), 0, EntIndex);
	}
	else if (Type == PROJ_AK74_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		g_pRenderManager->AddSystem(new CPSBlastCone(6, 80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 40, 200,200,200, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_9, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_AK74_DETONATE_WATER)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/ak74gren_explode.wav", 1, ATTN_LOW, 0, 200);
		DynamicLight(origin, 250, 128,128,40, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBubbles(80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 250, origin, origin, Vector(1,1,1), 10, 120, 150,5,0, 0.6, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 300, origin, origin, Vector(1,1,1), 10, 30, 150,125,0, 0.5, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_3, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_M203)
	{
		g_pRenderManager->AddSystem(new CPSTrail(100, 2, 3, 25, origin, origin, Vector(0,0,0), 5, 20, 0,0,0, 0.4, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 10), 0, EntIndex);
	}
	else if (Type == PROJ_M203_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}

		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/m203_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		DynamicLight(origin, 350, 128,140,0, 0.6, 200.0);
	//	g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(220, 280), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 175, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 0.8, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 340, origin, origin, Vector(1,1,1), 10, 150, 255,115,15, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(45, 320, origin, origin, Vector(1,1,1), 5, 90, 185,110,25, 0.7, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 70, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 0,0,0, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_8, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	//	g_pRenderManager->AddSystem(new CPSSparks(20, origin, 0.8, 0.01, 280, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	//	g_pRenderManager->AddSystem(new CPSSparks(20, origin, 0.7, 0.03, 320, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_M203_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/m203_explode.wav", 1, ATTN_LOW, 0, 200 );
		DynamicLight(origin, 250, 128,140,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 125, origin, origin, Vector(1,1,1), 10, 125, 210,200,0, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, origin, origin, Vector(1,1,1), 10, 150, 255,115,15, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 270, origin, origin, Vector(1,1,1), 5, 90, 185,110,25, 0.7, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_6, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_MMISSILE)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 240,240,50, 300, 10, PROJ_MMISSILE, EntIndex));
	}
	else if (Type == PROJ_MMISSILE_DETONATE)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/mmissile_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		DynamicLight(origin, 350, 128,128,0, 0.6, 200.0);

		g_pRenderManager->AddSystem(new CPSBlastCone(40, 320, origin, origin, Vector(1,1,1), 10, 150, 120,0,0, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 200, origin, origin, Vector(1,1,1), 10, 95, 255,255,255, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 160, origin, Vector(0,0,1), Vector(5,5,1), 5, 0, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_3, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(45, 270, origin, origin, Vector(1,1,1), 5, 180, 100,100,100, 0.7, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 70, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 0,0,0, 0.8, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 1, 0.02, 350, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash2.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_MMISSILE_DETONATE_WATER)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/mmissile_explode.wav", 1, ATTN_LOW, 0, 200 );
		DynamicLight(origin, 250, 128,128,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 270, origin, origin, Vector(1,1,1), 10, 150, 120,0,0, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(80, 150, origin, origin, Vector(1,1,1), 10, 85, 255,255,255, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash1.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 110, origin, Vector(0,0,1), Vector(5,5,1), 5, 0, 255,255,255, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_3, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_WARHEAD)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 40, 100,100,100, 1, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight(origin, 240,240,50, 350, 10, PROJ_WARHEAD, EntIndex));
	}
	else if (Type == PROJ_WARHEAD_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/rpgrocket_explode.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 500, 80, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 0, kRenderTransAdd, 120,80,0, 1, -0.3, 1));
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,7), gEngfuncs.pfnRandomFloat(1.8,2.5), gEngfuncs.pfnRandomFloat(300, 410), SPARKSHOWER_EXP, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(1.3,2.0), gEngfuncs.pfnRandomFloat(320, 450), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(80, 280, origin, origin, Vector(1,1,1), 10, 100, 255,255,255, 0.7, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_6, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 230, origin, origin, Vector(1,1,1), 10, 60, 255,255,255, 0.8, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_6, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 250, origin, origin, Vector(1,1,1), 5, 85, 128,0,0, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 60, origin, origin, Vector(1,1,1), 5, 75, 0,0,0, 0.8, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_8, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 500, 128,128,20, 0.8, 150.0);
	}
	else if (Type == PROJ_WARHEAD_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/rpgrocket_explode.wav", 1, ATTN_NONE, 0, 200 );
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 80, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 0, kRenderTransAdd, 120,80,0, 1, -0.5, 0.8));
		g_pRenderManager->AddSystem(new CPSBlastCone(90, 220, origin, origin, Vector(1,1,1), 10, 90, 255,255,255, 0.7, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_6, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 190, origin, origin, Vector(1,1,1), 5, 85, 128,0,0, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(80, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(80,140), gEngfuncs.pfnRandomFloat(12,15), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 350, 128,128,20, 0.8, 150.0);
	}

	else if (Type == PROJ_DUMBFIRE)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 3, 4, 15, origin, origin, Vector(1,1,1), 5, 40, 100,100,100, 0.6, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_1, kRenderTransAdd, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), 7, kRenderTransAdd, 200,200,200, 0.8, 0.0, 20, 0.0, 0.0, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight(origin, 240,180,0, 270, 10, 0, EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "turret/tu_fire_missile.wav", 1, ATTN_LOW, 0, PITCH_NORM );

		BEAM *pBeamTrail;
		pBeamTrail = gEngfuncs.pEfxAPI->R_BeamFollow(EntIndex, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 2.0, 8, 1,1,1, 0.5);
		pBeamTrail->frame = BLAST_SKIN_PLASMA;
		pBeamTrail->frameRate = 0;
	}
	else if (Type == PROJ_DUMBFIRE_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "garg/garg_explode.wav", 1, ATTN_LOW, 0, 100 );
		DynamicLight(origin, 320, 128,128,40, 0.8, 150.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(6, 120, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 30,30,30, 0.6, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 1, 0.02, 600, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(20, origin, 0.8, 0.01, 800, 1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(75, 350, origin, origin, Vector(1,1,1), 10, 200, 100,0,0, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 300, origin, origin, Vector(1,1,1), 10, 125, 255,255,255, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 360, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 230,140,0, 1, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_DUMBFIRE_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/bfg_fire.wav", 1, ATTN_LOW, 0, 200 );
		DynamicLight(origin, 200, 128,128,40, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 300, origin, origin, Vector(1,1,1), 10, 200, 100,0,0, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 250, origin, origin, Vector(1,1,1), 10, 125, 255,255,255, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 310, origin, Vector(0,0,1), Vector(1,1,1), 10, 0, 230,140,0, 1, -0.6, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_NUKE)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 35, origin, origin, Vector(0.2,0.2,0.5), 5, 30, 255,255,255, 0.4, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, -1), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0, kRenderTransAdd, 255,200,200, 0.8, 0.0, 20, 0.0, 0.0, -1), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight(origin, 240,180,0, 320, 10, 0, EntIndex));
	}
	else if (Type == PROJ_NUKE_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound(0, origin, CHAN_VOICE, "weapons/redeemer_WH_explode.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		DynamicLight(origin, 1000, 229,97,0, 2.5, 250.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 100, origin, Vector(0,0,2), Vector(0.5,0.5,1.0), 10, 250, 170,80,32, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 50, origin, Vector(0,0,2), Vector(0.5,0.5,1.0), 10, 150, 170,80,32, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		dir.z=40;
		dir=dir.Normalize();
		g_pRenderManager->AddSystem(new CRSCylinder(origin+dir*150, 20, 2200, 120, 70, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 145,50,0, 1, -1, 0.6));
		EV_BlastModel(origin, BLAST_MDL_MUSHROOM, 0.5, 0.05, 1 );
	}
	else if (Type == PROJ_NUKE_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound(0, origin, CHAN_VOICE, "weapons/redeemer_WH_explode.wav", 1, ATTN_NONE, 0, 200);
		DynamicLight(origin, 600, 229,97,0, 2.0, 200.0);

		dir.z=40;
		dir=dir.Normalize();
		g_pRenderManager->AddSystem(new CRSCylinder(origin+dir*75, 20, 2200, 120, 70, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_WASTEDBEAM, kRenderTransAdd, 128,128,128, 1, -1, 0.4));
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 300, origin, Vector(0,0,2), Vector(0.2,0.2,0.8), 40, 250, 255,255,255, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(100, origin, Vector(0,0,1), Vector(0.3,0.3,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(12,15), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 100, origin, Vector(0,0,1), Vector(0.5,0.5,1.0), 10, 150, 170,80,32, 1, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_U2)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 3, 4, 15, origin, origin, Vector(0,0,0), 5, 20, 100,0,0, 0.5, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_1, kRenderTransAlpha, 5), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 3, 4, 18, origin, origin, Vector(0,0,0), 5, 25, 120,120,120, 0.75, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_8, kRenderTransAdd, 5), 0, EntIndex);
	}
	else if (Type == PROJ_U2_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/ak74_glauncher.wav", 1, ATTN_LOW, 0, 120);
		DynamicLight(origin, 300, 129,140,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(1.8,2.4), gEngfuncs.pfnRandomFloat(220, 280), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 450, 40, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_PLASMA, kRenderTransAdd, 100,100,100, 1, -1, 1));
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 70, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 0,0,0, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_1, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(70, 340, origin, origin, Vector(1,1,1), 10, 120, 100,100,100, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(30, origin, 0.8, 0.01, 320, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_U2_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/ak74_glauncher.wav", 1, ATTN_LOW, 0, 200);
		DynamicLight(origin, 200, 120,140,0, 0.6, 150.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(80, 280, origin, origin, Vector(1,1,1), 10, 120, 100,100,100, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_U2_SHARD)
	{
		gEngfuncs.pEfxAPI->R_BeamFollow(EntIndex, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 2.5, 5, 1,1,1, 0.2);
	}
	else if (Type == PROJ_U2_SHARD_DETONATE)
	{
		switch (gEngfuncs.pfnRandomLong(0,2))
		{
			case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/explode4.wav", 1.0, ATTN_LOW, 0, 125 ); break;
			case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/explode5.wav", 1.0, ATTN_LOW, 0, 125 ); break;
			case 2 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_WEAPON, "weapons/rpgrocket_explode.wav", 1.0, ATTN_LOW, 0, 125 ); break;
		}
		DynamicLight(origin, 180, 129,140,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,3), gEngfuncs.pfnRandomFloat(1.5,2.0), gEngfuncs.pfnRandomFloat(150, 200), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 240, origin, origin, Vector(1,1,1), 10, 150, 150,90,0, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 100, origin, origin, Vector(1,1,1), 10, 100, 120,0,0, 0.8, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 220, origin, origin, Vector(1,1,1), 5, 90, 0,0,0, 1, -1.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_U2_SHARD_DETONATE_WATER)
	{
//		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/m203_explode.wav", 1, ATTN_LOW, 0, 200 );
		DynamicLight(origin, 150, 129,140,0, 0.6, 100.0);
		g_pRenderManager->AddSystem(new CPSBubbles(30, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 190, origin, origin, Vector(1,1,1), 10, 150, 150,90,0, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_5, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 50, origin, origin, Vector(1,1,1), 10, 100, 120,0,0, 0.8, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_U2_DETONATE_SHARDS)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		SpawnShards(CHAR_TEX_METAL,origin,BULLET_SMALEXP, 0);
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/explosion_flakbomb.wav", 1, ATTN_NORM, 0, 150);

		if ( gEngfuncs.PM_PointContents(origin, NULL ) != CONTENTS_WATER)
			g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(3,6), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(210, 270), SPARKSHOWER_STREAKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_FLAKBOMB)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 25, origin, origin, Vector(0.2,0.2,0.5), 5, 20, 255,255,255, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 25, origin, origin, Vector(0.2,0.2,0.5), 5, 20, 150,50,10, 0.4, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 10), 0, EntIndex);
	}
	else if (Type == PROJ_FLAKBOMB_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/explosion_flakbomb.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		DynamicLight(origin, 300, 129,129,30, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 0.5, 0.01, gEngfuncs.pfnRandomFloat(800,1000), 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(75, 70, origin, Vector(1,0,0), Vector(3,3,1), 10, 100, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(65, 250, origin, origin, Vector(1,1,1), 10, 80, 229,97,0, 0.5, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 200, origin, origin, Vector(1,1,1), 10, 60, 255,255,255, 0.7, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		dir.z=40;
		dir=dir.Normalize();
		g_pRenderManager->AddSystem(new CRSCylinder(origin+dir*15, 20, 1000, 40, 70, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_PULSE, kRenderTransAdd, 128,128,128, 1, -1, 0.3));
	}
	else if (Type == PROJ_FLAKBOMB_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/explosion_flakbomb.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		DynamicLight(origin, 200, 129,129,30, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 50, origin, Vector(1,0,0), Vector(3,3,1), 10, 100, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(55, 200, origin, origin, Vector(1,1,1), 10, 80, 229,97,0, 0.5, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 150, origin, origin, Vector(1,1,1), 10, 60, 255,255,255, 0.7, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	if (Type == PROJ_GLUON)
	{
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, kRenderTransAdd, 255,200,255, 0.8, 0.0, 20, 0.0, 0.0, -1), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 180,120,230, 1, 0, 100, 0, 6), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);

		BEAM *pBeamTrail;
		pBeamTrail = gEngfuncs.pEfxAPI->R_BeamFollow(EntIndex, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.5, 10, 1,1,1, 0.2);
		pBeamTrail->frame = BLAST_SKIN_TAUBEAM;
		pBeamTrail->frameRate = 0;
	}
	else if (Type == PROJ_BLACKBALL)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 180,120,230, 1, 0, 60, 0, 8), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == PROJ_GLUON_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall2.wav", 1, ATTN_LOW_HIGH, 0, PITCH_NORM );
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 500, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 400, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_GLUON, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 250, origin, origin, Vector(1,1,1), 10, 60, 170,90,250, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 400, 0,90,250, 0.7, 200.0);
	}
	else if (Type == PROJ_GLUON_DETONATE_WATER)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall2.wav", 1, ATTN_LOW_HIGH, 0, 255);
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CPSBubbles(60, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(15, 150, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 35, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_GLUON2)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 170,90,250, 300, 10, 0, EntIndex));
	}
	else if (Type == PROJ_GLUON2_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/gluon_hitwall.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 480, origin, origin, Vector(1,1,1), 10, 80, 170,90,250, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_9, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 250, origin, origin, Vector(1,1,1), 10, 60, 170,90,250, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(100, 420, origin, origin, Vector(1,1,1), 10, 30, 0,90,250, 1, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(60, origin, 1.5, 0.15, 375, 3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(10, 5, gEngfuncs.pfnRandomFloat(200, 420), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 500, 0,90,250, 0.7, 200.0);
	}
	else if (Type == PROJ_DISPLACER)
	{
	//	g_pRenderManager->AddSystem(new CPSBlastCone(1, 0, origin, Vector(0,0,0), Vector(1,1,1), 40, 0, 255,255,255, 0.6, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0, 50, kRenderTransAdd, 128,128,128, 0.5, 0, 50, -2.5, 16.0), 0, EntIndex);
	//	g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr12.spr"), 0, 64, kRenderTransAdd, 128,128,128, 1, -0.1, 90, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);

	//	g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 0, 64, kRenderTransAdd, 180,100,0, 0, 0.25, 25, 20, 2.5), 0, EntIndex);
	//	g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 230,140,0, 0, 0.25, 45, 30, 2.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);

		g_pRenderManager->AddSystem(new CRSLight(origin, 90,250,90, 200, 10, PROJ_DISPLACER, EntIndex));
	}
	else if (Type == PROJ_DISPLACER_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/displacer_teleportblast.wav", 1, ATTN_LOW, 0, 100);
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		//g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 500, 40, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		//g_pRenderManager->AddSystem(new CRSCylinder(origin, 30, 400, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		//g_pRenderManager->AddSystem(new CRSCylinder(origin, 20, 300, 20, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 0.8, -0.8, 1.5));
		g_pRenderManager->AddSystem(new CPSBlastCone(150, 250, origin, origin, Vector(1,1,1), 10, 60, 200,250,150, 1, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr4.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(150, 320), origin, origin, Vector(1,1,1), gEngfuncs.pfnRandomFloat(2,5), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		g_pRenderManager->AddSystem(new CPSBlastCone(50, 100, origin, Vector(0,0,1), Vector(3,3,1), 20, 100, 255,255,255, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_6, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		DynamicLight(origin, 500, 80,240,120, 0.7, 600.0);
	}
	else if (Type == PROJ_DISPPOWER_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/displacer_teleportblast.wav", 1, ATTN_LOW, 0, 100);
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 1500, 80, 70, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 1, -1, 0.8));
		DynamicLight(origin, 600, 85,250,85, 0.7, 600.0);
	}
	else if (Type == PROJ_DISPLACER_DETONATE_WATER)
	{
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 300, 30, 40, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_DISPLACER, kRenderTransAdd, 255,255,255, 1, -1, 0.8));
	}
	else if (Type == PROJ_SHOCK)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 128,128,128, 0.7, -0.1, 24, -2.5, 8.0), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 64, kRenderTransAdd, 128,128,128, 1, -0.1, 40, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == PROJ_SHOCK2)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 50, kRenderTransAdd, 128,128,128, 0.7, -0.1, 24, -2.5, 8.0), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 64, kRenderTransAdd, 128,128,128, 1, -0.1, 40, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == PROJ_GOD_BALL1)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/unused_spark1.spr"), 0, 64, kRenderTransAdd, 255,32,32, 0.7, -0.1, 24, -2.5, 8.0), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/unused_spark1.spr"), 0, 80, kRenderTransAdd, 255,32,32, 1, -0.1, 40, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == PROJ_GOD_BALL2)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/unused_spark1.spr"), 0, 64, kRenderTransAdd, 32,32,255, 0.7, -0.1, 24, -2.5, 8.0), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/unused_spark1.spr"), 0, 80, kRenderTransAdd, 32,32,255, 1, -0.1, 40, -2.5, 8.0), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
	}
	else if (Type == 87)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 64,255,64, 0.7, -0.1, 16, -1.5, 8.0), 0, EntIndex);
	}
	else if (Type == PROJ_SHOCK_DETONATE)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 0.4, 0.04, 300, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEfxAPI->R_StreakSplash (origin, vec3_origin, (unsigned short)73, gEngfuncs.pfnRandomFloat(80, 100), 200, -100, 100);
		DynamicLight(origin, 250, 250,90,250, 0.2, 250.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 100, origin, origin, Vector(1,1,1), 5, 150, 255,64,255, 0.25, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		for (int i = 0; i <6; i++)
		{
			dir.x=gEngfuncs.pfnRandomFloat(-400,400);
			dir.y=gEngfuncs.pfnRandomFloat(-400,400);
			dir.z=gEngfuncs.pfnRandomFloat(-400,400);
			dir=dir.Normalize();
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(50,100)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(0.5,0.7), gEngfuncs.pfnRandomFloat(0.7,1.0), 2.5, 40, 20, BLAST_SKIN_TAUBEAM, 0, 255, 255, 255);
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(50,100)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(0.5,0.7), gEngfuncs.pfnRandomFloat(0.3,0.6), 2.5, 40, 20, BLAST_SKIN_TAUBEAM, 0, 255, 255, 255);
		}
	}
	else if (Type == PROJ_SHOCKPOWER_DETONATE)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shock_hitwall.wav", 1, ATTN_NORM, 0, 110 );
	//	g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(4,8), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(220, 300), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 0.4, 0.03, 300, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr7.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		gEngfuncs.pEfxAPI->R_StreakSplash (origin, vec3_origin, (unsigned short)73, gEngfuncs.pfnRandomFloat(80, 100), 200, -100, 100);
		DynamicLight(origin, 250, 0,90,250, 0.2, 250.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 100, origin, origin, Vector(1,1,1), 5, 120, 128,128,128, 0.25, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

		for (int i = 0; i <6; i++)
		{
			dir.x=gEngfuncs.pfnRandomFloat(-360,360);
			dir.y=gEngfuncs.pfnRandomFloat(-360,360);
			dir.z=gEngfuncs.pfnRandomFloat(-360,360);
			dir=dir.Normalize();
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(50,100)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(0.3,0.5), gEngfuncs.pfnRandomFloat(0.6,0.9), 2.5, 40, 20, BLAST_SKIN_LIGHTNING, 0, 255, 255, 255);
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(50,100)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), gEngfuncs.pfnRandomFloat(0.3,0.5), gEngfuncs.pfnRandomFloat(0.2,0.5), 2.5, 40, 20, BLAST_SKIN_FROSTGRENADE, 0, 255, 255, 255);
		}
	}
	else if (Type == 89)
	{
	g_pRenderManager->AddSystem(new CPSBlastCone(10, 120, origin, origin, Vector(1,1,1), 20, 60, 255,255,255, 0.5, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == 90)
	{
	g_pRenderManager->AddSystem(new CPSBlastCone(20, 120, origin, origin, Vector(1,1,1), 30, 60, 160,160,160, 0.5, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_SHOCK_DETONATE_WATER)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shock_hitwall.wav", 1, ATTN_NORM, 0, 200);
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CPSBubbles(35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(12, 100, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 45, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_3, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_ENERGYCHARGE_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shock_hitwall.wav", 1, ATTN_NORM, 0, 110 );
		g_pRenderManager->AddSystem(new CPSSparks(30, origin, 0.4, 0.04, 200, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 150, 35,214,177, 0.1, 300.0);

		for (int i = 0; i <2; i++)
		{
			dir.x=gEngfuncs.pfnRandomFloat(-360,360);
			dir.y=gEngfuncs.pfnRandomFloat(-360,360);
			dir.z=gEngfuncs.pfnRandomFloat(-360,360);
			dir=dir.Normalize();
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(30,70)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/plasma.spr"), gEngfuncs.pfnRandomFloat(0.15,0.3), gEngfuncs.pfnRandomFloat(0.6,0.9), 5, 1, 50, 0, 25, 35, 215, 177);
			gEngfuncs.pEfxAPI->R_BeamPoints( origin, origin+(dir*gEngfuncs.pfnRandomFloat(30,70)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/plasma.spr"), gEngfuncs.pfnRandomFloat(0.15,0.3), gEngfuncs.pfnRandomFloat(0.2,0.5), 5, 1, 50, 0, 25, 35, 215, 177);
		}
	}
	else if (Type == PROJ_ENERGYCHARGE_DETONATE_WATER)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shock_hitwall.wav", 1, ATTN_NORM, 0, 200);
		g_pRenderManager->AddSystem(new CPSBubbles(35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(12, 100, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 45, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_STONEBEAM)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 200,100,0, 0, 0.5, 10, 10, 2.5), 0, EntIndex);
	}
	else if (Type == PROJ_DOMABEAM)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 50, kRenderTransAdd, 200,100,200, 0, 0.3, 3, 3, 3.0), 0, EntIndex);
	}
	else if (Type == PROJ_KADOMA_FISTHIT){//KADOMA����ȭ
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 0, 50, kRenderTransAdd, 100,100,100, 0, 0.15, 5, 6, 6.0), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 50, kRenderTransAdd, 100,100,100, 0, 0.15, 5, 4, 6.0), 0, EntIndex);
	}
	else if (Type == PROJ_DOMABEAM_FIRE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 0.6, 0.05, 120, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_DOMABEAM2)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 64, kRenderTransAdd, 255,128,255, 0, 0.2, 6, 18, 5.0), 0, EntIndex);
	}
	else if (Type == PROJ_DOMABEAM2_FIRE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		g_pRenderManager->AddSystem(new CPSSparks(90, origin, 0.6, 0.15, 300, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_GMANBEAM)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0, 50, kRenderTransAdd, 255,255,64, 0.1, 0.3, 6, 4, 3.0), 0, EntIndex);
	}
	else if (Type == PROJ_GMANBEAM_FIRE)
	{
		g_pRenderManager->AddSystem(new CPSSparks(35, origin, 0.6, 0.05, 120, 1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_STONESHOOT)
	{
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 80, origin, Vector(0,0,1), Vector(0.3,0.3,0.4), 10, 80, 0,40,250, 0.7, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 60, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 10, 50, 0,0,0, 0.4, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_0, kRenderTransAlpha, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 100, origin, origin, Vector(1,1,1), 5, 100, 0,100,250, 1.0, -1.0, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_17, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_SUNOFGOD2)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), 0, 64, kRenderTransAdd, 128,255,128, 0, 0.15, 2, 8, 8.0), 0, EntIndex);
	}
	else if (Type == PROJ_SUNOFGOD)
	{
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr11.spr"), 0, 64, kRenderTransAdd, 180,100,0, 0.7, 0, 80, -12, 6), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0, 50, kRenderTransAdd, 230,140,0, 1, 0, 100, 0, 6), RENDERSYSTEM_FLAG_SIMULTANEOUS, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(120, 0.8, 1.0, 40, origin, Vector(0,1,0), Vector(3,3,1), 50, 100, 128,128,128, 0.8, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(120, 0.8, 1.0, 150, origin, origin, Vector(1,1,1), 50, 125, 255,255,255, 0.2, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 10), 0, EntIndex);
	}
	else if (Type == PROJ_SUNOFGOD_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/sunofgod_blast.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 750, origin, origin, Vector(1,1,1), 50, 200, 128,128,128, 1, -1.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(90,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_14, 0, kRenderTransAdd, 255,255,255, 1.0, -0.5, 25, 900, 0.0));
		DynamicLight(origin, 1500, 255,255,128, 1.0, 500.0);
	}
	else if (Type == PROJ_SUNOFGOD_DETONATE_WATER)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/sunofgod_blast.wav", 1, ATTN_NONE, 0, 200);
		g_pRenderManager->AddSystem(new CPSBubbles(200, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 300, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 75, 255,255,255, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_5, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_TELEENTER)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 65,250,85, 300, 2.5, PROJ_DISPLACER, EntIndex));
		g_pRenderManager->AddSystem(new CPSBlastCone(1, 0, origin, origin, Vector(1,1,1), 60, -5, 0,110,0, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 1), 0, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(5, 10, origin, origin, Vector(1,1,1), 90, -20, 160,160,0, 0.8, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr8.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 20, origin, origin, Vector(1,1,1), 40, 0, 0,100,80, 0.5, -0.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr3.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 60, origin, origin, Vector(1,1,1), 10, -2, 160,160,0, 0.6, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_1, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSpawnEffect(40, origin, 1, 7, 50, -5, 1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), kRenderTransAdd, 0,80,200, 0.8, -0.4, 2), 0, -1);
	}
	else if (Type == PROJ_TELEENTER_DETONATE)
	{
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/displacer_teleportblast.wav", 1, ATTN_NORM, 0, 150 );
		DynamicLight(origin, 300, 65,250,80, 0.6, 150.0);
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 1000, 40, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_TELEENTER, kRenderTransAdd, 255,255,255, 1, -1, 0.6));
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 100, origin, Vector(0,0,1), Vector(3,3,1), 20, 100, 255,255,255, 0.6, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), FALSE, PARTICLE_GREEN_6, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 200, origin, origin, Vector(1,1,1), 20, 100, 0,150,80, 0.8, -1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), FALSE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_TELEENTER_DETONATE_WATER)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/displacer_teleportblast.wav", 1, ATTN_NORM, 0, 200 );
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		g_pRenderManager->AddSystem(new CPSBubbles(40, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(8,10), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 150, origin, Vector(0,0,1), Vector(0.2,0.2,0.4), 10, 45, 255,255,255, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_BLACKHOLE)
	{
		g_pRenderManager->AddSystem(new CPSSparks(150, origin, 0.1, 0.02, -600, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/smoke.spr"), 5.0), 0, -1);
		g_pRenderManager->AddSystem(new CPSSparks(150, origin, 0.5, 0.04, -200, 1.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_gibs.spr"), 5.0), 0, -1);
		g_pRenderManager->AddSystem(new CRSBeamStar(origin, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), PARTICLE_BLACK_9, 120, kRenderTransAlpha, 0,0,0, 1, 0, 300, -10, 6.0));
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), PARTICLE_BLACK_6, kRenderTransAlpha, 0,0,0, 1.0, 0, 20, 0.0, 0.0, 4));
	}
	else if (Type == PROJ_BLACKHOLE_DETONATE)
	{
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "player/pl_teleport3.wav", 1, ATTN_LOW, 0, 150 );
		//g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_3, 0, kRenderTransAdd, 255,255,255, 1.0, -0.2, 50, 160, 0.0));
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 2000, origin, origin, Vector(1,1,1), 10, 350, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 2200, origin, origin, Vector(1,1,1), 10, 300, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 1800, origin, origin, Vector(1,1,1), 10, 320, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
		else if (Type == PROJ_BLACKHOLE_DETONATE2)
	{
		//gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "player/pl_teleport3.wav", 1, ATTN_LOW, 0, 150 );
		//g_pRenderManager->AddSystem(new CRenderSystem(origin, Vector(0,0,0), Vector(45,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_green.spr"), PARTICLE_GREEN_3, 0, kRenderTransAdd, 255,255,255, 1.0, -0.2, 50, 160, 0.0));
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 2000, origin, origin, Vector(1,1,1), 10, 350, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 2200, origin, origin, Vector(1,1,1), 10, 300, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_2, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(200, 1800, origin, origin, Vector(1,1,1), 10, 320, 255,255,255, 0.4, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_4, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	if (Type == PROJ_PLASMA)
	{
		BEAM *pBeamTrail;
		pBeamTrail = gEngfuncs.pEfxAPI->R_BeamFollow(EntIndex, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 1.6, 15, 1,1,1, 0.6);
		pBeamTrail->frame = BLAST_SKIN_ENERGYBEAM;
		pBeamTrail->frameRate = 0;
	}
	else if (Type == PROJ_RPGROCKET)
	{
		g_pRenderManager->AddSystem(new CPSTrail(180, 5, 5, 20, origin, Vector(1,1,1), Vector(0.2,0.2,0.5), 5, 20, 200,200,200, 0.8, -0.25, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAdd, 30), 0, EntIndex);
	}
	else if (Type == PROJ_RPGROCKET2)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 3, 3, 30, origin, Vector(1,1,1), Vector(0.2,0.2,0.5), 5, 10, 200,200,200, 0.6, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAdd, 30), 0, EntIndex);
	}
	else if (Type == PROJ_RPGROCKET_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		DynamicLight(origin, 350, 220,150,0, 0.7, 200.0);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,4), gEngfuncs.pfnRandomFloat(1.5,2.2), gEngfuncs.pfnRandomFloat(320, 520), SPARKSHOWER_FIREEXP, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSSparkShower(gEngfuncs.pfnRandomLong(1,5), gEngfuncs.pfnRandomFloat(1.4,2.0), gEngfuncs.pfnRandomFloat(320, 450), SPARKSHOWER_SPARKS, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(8, 65, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 0,0,0, 0.5, -0.12, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_3, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 300, origin, origin, Vector(1,1,1), 10, 110, 128,128,128, 0.8, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(60, 350, origin, origin, Vector(1,1,1), 10, 150, 128,128,128, 0.6, -0.55, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 400, origin, origin, Vector(1,1,1), 15, 5, 128,128,128, 0.5, -0.45, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash3.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(50, 450, origin, origin, Vector(1,1,1), 20, 5, 255,255,255, 0.5, -0.45, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_RPGROCKET_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/rpgrocket_explode.wav", 1, ATTN_LOW, 0, 200);
		DynamicLight(origin, 250, 220,150,0, 0.6, 200.0);
		g_pRenderManager->AddSystem(new CPSBlastCone(45, 200, origin, origin, Vector(1,1,1), 10, 100, 128,128,128, 0.8, -0.75, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_violet.spr"), FALSE, PARTICLE_VIOLET_2, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(45, 250, origin, origin, Vector(1,1,1), 10, 130, 128,128,128, 0.6, -0.55, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr10.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 300, origin, origin, Vector(1,1,1), 15, 5, 128,128,128, 0.5, -0.45, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/muzzleflash3.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(35, 350, origin, origin, Vector(1,1,1), 20, 5, 255,255,255, 0.5, -0.45, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(50, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_INCENDIARY)
	{
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_4, kRenderTransAdd, 255,200,200, 1.0, 0.0, 25, 0.0, 0.0, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 35, origin, Vector(0,0,1), Vector(0.1,0.1,0.5), 5, 20, 255,255,255, 0.4, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 25, origin, Vector(0,0,1), Vector(0.1,0.1,0.5), 5, 20, 255,255,255, 0.4, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight(origin, 220,130,0, 300, 10, 0, EntIndex));
	}
	else if (Type == PROJ_INCENDIARY2)
	{
		g_pRenderManager->AddSystem(new CRSSprite(origin, Vector(0,0,0), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), PARTICLE_RED_4, kRenderTransAdd, 255,200,200, 1.0, 0.0, 25, 0.0, 0.0, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 20, 255,255,255, 0.4, -0.08, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_2, kRenderTransAlpha, 10), 0, EntIndex);
		g_pRenderManager->AddSystem(new CRSLight(origin, 220,130,0, 300, 10, 0, EntIndex));
		gEngfuncs.pEfxAPI->R_BeamFollow(EntIndex, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/laserbeam.spr"), 2, 10, 1,1,1, 0.1);
	}
	else if (Type == PROJ_INCENDIARY_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/incrocket_explode.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 370, origin, origin, Vector(1,1,1), 5, 120, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 350, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 150, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(20, 330, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 120, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 300, origin, origin, Vector(1,1,1), 30, 5, 128,128,128, 1, -1.1, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(5, 70, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 5, 60, 0,0,0, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_4, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 450, 220,140,0, 8.2, 20.0);

		dir.z=10;
		dir=dir.Normalize();
		g_pRenderManager->AddSystem(new CPSTrail(150, 1.5, 2, gEngfuncs.pfnRandomFloat(60,100), origin+(dir*gEngfuncs.pfnRandomFloat(20,40)), Vector(1,1,1), Vector(2,2,1), 10, gEngfuncs.pfnRandomFloat(80,120), 128,128,128, 1.0, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 7), 0, EntIndex);
		g_pRenderManager->AddSystem(new CPSTrail(150, 1.5, 2, gEngfuncs.pfnRandomFloat(60,100), origin+(dir*gEngfuncs.pfnRandomFloat(20,40)), Vector(1,1,1), Vector(2,2,0), 10, gEngfuncs.pfnRandomFloat(70,110), 128,128,128, 1.0, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 7), 0, EntIndex);
	}
	else if (Type == PROJ_INCENDIARY_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/incrocket_explode.wav", 1, ATTN_LOW, 0, 200);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 270, origin, origin, Vector(1,1,1), 5, 120, 0,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_5, kRenderTransAlpha, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 250, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 150, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 230, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 120, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(40, 200, origin, origin, Vector(1,1,1), 30, 5, 128,128,128, 1, -0.7, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr9.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(40, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(6, 120, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), 10, 75, 255,255,255, 0.2, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_7, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 250, 220,140,0, 0.6, 200.0);
	}
	else if (Type == PROJ_TESLAGREN)
	{
		g_pRenderManager->AddSystem(new CPSTrail(150, 4, 5, 0, origin, origin, Vector(0,0,0), 5, 20, 0,0,0, 0.4, -0.15, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_black.spr"), FALSE, PARTICLE_BLACK_12, kRenderTransAlpha, 10), 0, EntIndex);
	}
	else if (Type == PROJ_TESLAGREN_DETONATE)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shockgrenade_explo.wav", 1, ATTN_LOW, 0, PITCH_NORM );
		g_pRenderManager->AddSystem(new CPSSparks(40, origin, 0.6, 0.05, gEngfuncs.pfnRandomFloat(200,300), 3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(4,8), 5, gEngfuncs.pfnRandomFloat(200, 320), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,3.2), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(4,8), 5, gEngfuncs.pfnRandomFloat(100, 420), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1.6,3.2), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), TRUE, 0, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 10, 100, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 230, origin, origin, Vector(1,1,1), 10, 80, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 100, origin, origin, Vector(1,1,1), 10, 50, 0,80,250, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 150, origin, origin, Vector(1,1,1), 10, 60, 0,120,250, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 10, 70, 0,160,250, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 250, origin, origin, Vector(1,1,1), 10, 80, 0,200,250, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 300, 0,230,250, 0.6, 200.0);
	}
	else if (Type == PROJ_TESLAGREN_DETONATE_WATER)
	{
		CRenderSystem *pSystem = NULL;
		while (pSystem = g_pRenderManager->FindSystemByFollowEntity(EntIndex))
		{
			pSystem->m_iFollowEntity = -1;
			pSystem->m_fDieTime = 0.1f;
		}
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/shockgrenade_explo.wav", 1, ATTN_LOW, 0, 200);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 160, origin, origin, Vector(1,1,1), 10, 70, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 200, origin, origin, Vector(1,1,1), 10, 60, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, 0, kRenderTransAdd, 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 190, origin, origin, Vector(1,1,1), 10, 30, 0,90,250, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 100, origin, origin, Vector(1,1,1), 10, 40, 0,90,250, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 150, origin, origin, Vector(1,1,1), 10, 50, 0,90,250, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(30, 200, origin, origin, Vector(1,1,1), 10, 60, 0,90,250, 1, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_11, kRenderTransAdd, 0.2), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(40, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		DynamicLight(origin, 200, 0,230,250, 0.6, 200.0);
	}
	else if (Type == PROJ_CLUSTERBOMB)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 0,0,0, 1, 5, PROJ_CLUSTERBOMB, EntIndex));
	}
	else if (Type == PROJ_CLUSTERBOMB_DETONATE)
	{
	//	g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
	//	gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/cluster_explode.wav", 1.0, ATTN_LOW_HIGH, 0, 100 );
		g_pRenderManager->AddSystem(new CRSCylinder(origin, 40, 500, 50, 50, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), BLAST_SKIN_FROSTGRENADE, kRenderTransAdd, 255,255,255, 0.75, -0.75, 0.5));
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(220, 270), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(5,8), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_5, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new PSGravityPart(gEngfuncs.pfnRandomLong(8,12), 5, gEngfuncs.pfnRandomFloat(210, 290), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(2,3), 0, 255,255,255, 1, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 150, origin, origin, Vector(1,1,1), 5, 110, 0,90,250, 0.3, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, -1, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 125, origin, origin, Vector(1,1,1), 5, 135, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 100, origin, origin, Vector(1,1,1), 5, 160, 255,255,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(12, origin, 0.5, 0.01, 400, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr2.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(12, origin, 0.5, 0.01, 400, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSSparks(12, origin, 0.5, 0.01, 300, 2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), 0.5), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_CLUSTERBOMB_DETONATE_WATER)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "weapons/cluster_explode.wav", 1.0, ATTN_LOW_HIGH, 0, 200 );
		g_pRenderManager->AddSystem(new CPSBlastCone(25, 100, origin, origin, Vector(1,1,1), 5, 100, 0,90,250, 0.3, -0.2, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr6.spr"), TRUE, -1, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 75, origin, origin, Vector(1,1,1), 5, 125, 255,255,255, 0.4, -0.3, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_5, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBlastCone(10, 50, origin, origin, Vector(1,1,1), 5, 150, 255,255,255, 0.5, -0.4, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blue.spr"), FALSE, PARTICLE_BLUE_6, kRenderTransAdd, 1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
		g_pRenderManager->AddSystem(new CPSBubbles(35, origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(60,120), gEngfuncs.pfnRandomFloat(5,8), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), PARTICLE_WHITE_11, 0.1), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
	}
	else if (Type == PROJ_GUTS)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 0,0,0, 1.0, 3, PROJ_GUTS, EntIndex));
	}
	else if (Type == PROJ_GUTS_DETONATE)
	{
		g_pRenderManager->DeleteSystem(g_pRenderManager->FindSystemByFollowEntity(EntIndex));
		EV_Wallgib(origin, gEngfuncs.pfnRandomFloat(100,120), gEngfuncs.pfnRandomFloat(0.4,1.1), gHUD.TempEntLifeCvar->value, 5, GIB_FLESH, 0);
		g_pRenderManager->AddSystem(new CPSBlood(gEngfuncs.pfnRandomLong(3,5), gEngfuncs.pfnRandomFloat(80,120), origin, Vector(0,0,1), Vector(0.2,0.2,0.5), gEngfuncs.pfnRandomFloat(1,2), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_blood.spr"), gEngfuncs.pfnRandomLong(PARTICLE_BLOOD_0, PARTICLE_BLOOD_12), 0.1,0), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);
     		g_pRenderManager->AddSystem(new CPSBlastCone(5, 30, origin, origin, Vector(1,1,1), 5, 30, 80,0,0, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_white.spr"), FALSE, PARTICLE_WHITE_6, kRenderTransAdd, 0.3), RENDERSYSTEM_FLAG_SIMULTANEOUS, -1);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "debris/bustflesh1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_STATIC, "debris/bustflesh2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == EFFECT_BURN)
	{
		g_pRenderManager->AddSystem(new CRSLight(origin, 220,160,0, 160, 10, 0, EntIndex));
		g_pRenderManager->AddSystem(new CPSTrail(100, 1, 1.5, 70, origin, Vector(0,0,1), Vector(0.1,0.1,0.5), 10, 20, 255,255,255, 0.8, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/fire.spr"), TRUE, 0, kRenderTransAdd, 10), RENDERSYSTEM_FLAG_RANDOMFRAME, EntIndex);
	}
	else if (Type == EFFECT_FREEZE)
	{
		g_pRenderManager->AddSystem(new CPSTrail(30, 3, 4, 12, origin, Vector(0,0,-1), Vector(0.2,0.2,0.3), 10, 15, 255,255,255, 0.3, -0.05, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr5.spr"), TRUE, 0, kRenderTransAdd, 13), 0, EntIndex);
	}
return 1;
}

int __MsgFunc_BreakGib(const char *pszName, int iSize, void *pbuf)
{
	vec3_t origin;

	BEGIN_READ( pbuf, iSize );
	origin.x = READ_COORD();
	origin.y = READ_COORD();
	origin.z = READ_COORD();
	float Velocity = READ_SHORT();
	int Scale = READ_BYTE();
	int Amount = READ_BYTE();
	int Type = READ_BYTE();

	if (Type == SHARD_GLASS || Type == SHARD_UNBR_GLASS)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_GLASS, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustglass1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustglass2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	if (Type == SHARD_ICE)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_ICE, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustglass1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustglass2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_WOOD)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_WOOD, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustcrate1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustcrate2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_METALL)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_METALL, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_FLESH)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_FLESH, Amount, FTENT_SMOKETRAIL);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustflesh1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustflesh2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_CONCRETE_BLOCK)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_CONCRETE_BLOCK, Amount, FTENT_SMOKETRAIL);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_BRICK)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_BRICK, Amount, FTENT_SMOKETRAIL);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_CONCRETE)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_CONCRETE, Amount, FTENT_SMOKETRAIL);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_CEILING_TILE)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_CEILING_TILE, Amount, FTENT_SMOKETRAIL);
                gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustceiling.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
	else if (Type == SHARD_SANDWALL)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_SANDWALL, Amount, FTENT_SMOKETRAIL);
                gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustceiling.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
	else if (Type == SHARD_COMPUTER)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_COMPUTER, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_ROCK)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_ROCK, Amount, FTENT_SMOKETRAIL);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustconcrete2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_GRATE)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_GRATE, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "player/pl_grate1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "player/pl_grate2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
	else if (Type == SHARD_VENT)
	{
		EV_BreakableGib( origin, Velocity, Scale*0.1, gHUD.TempEntLifeCvar->value, SHARD_VENT, Amount, 0);

	        switch (gEngfuncs.pfnRandomLong(0, 1))
	        {
	                case 0 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	                case 1 : gEngfuncs.pEventAPI->EV_PlaySound( 0, origin, CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, PITCH_NORM ); break;
	        }
	}
return 1;
}

void HookFXMessages(void)
{
	HOOK_MESSAGE( BreakGib );
	HOOK_MESSAGE( BrassClip );
	HOOK_MESSAGE( FireGun );
	HOOK_MESSAGE( FireBeam );
	HOOK_MESSAGE( Explosion );
	HOOK_MESSAGE( ImpBullet );
	HOOK_MESSAGE( ImpBeam );
	HOOK_MESSAGE( ImpRocket );
	HOOK_MESSAGE( WorldExp );
	HOOK_MESSAGE( Rain );
	HOOK_MESSAGE( Aurora );
	HOOK_MESSAGE( PlrGib );
	HOOK_MESSAGE( Trail );
}
