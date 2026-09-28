//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "event_api.h"
#include "rain.h" 
#include "com_model.h"
#include "studio_util.h"
#include "r_particle.h"//xash
#include "game_fx.h"
#include "RenderManager.h"// XDM


// rain tutorial
void SetPoint( float x, float y, float z, float (*matrix)[4])
{
	vec3_t point, result;
	point[0] = x;
	point[1] = y;
	point[2] = z;

	VectorTransform(point, matrix, result);

	gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
}

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
}

//#define TEST_IT	1
#if TEST_IT

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if( gHUD.m_hsprCursor == 0 )
	{
		gHUD.m_hsprCursor = SPR_Load( "sprites/cursor.spr" );
	}

	if( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ) )
	{
		return;
	}

	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}
#endif

extern float g_fFogColor[4];
extern float g_fStartDist;
extern float g_fEndDist;
extern int g_iWaterLevel;

void RenderFog ( void )
{
	//Not in water and we want fog.
	bool bFog = g_iWaterLevel < 2 && g_fStartDist > 0 && g_fEndDist > 0;
	if (bFog)
		gEngfuncs.pTriAPI->Fog ( g_fFogColor, g_fStartDist, g_fEndDist, bFog );
//	else
//		gEngfuncs.pTriAPI->Fog ( g_fFogColor, 10000, 10001, 0 );
}


extern cl_drip FirstChainDrip;
extern rain_properties Rain;

void DrawRain( void )
{
	if (FirstChainDrip.p_Next == NULL)
		return; // no drips to draw

	float visibleHeight = Rain.globalHeight - SNOWFADEDIST;

	if (Rain.weatherMode == 0)
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( LoadSprite( "sprites/particles_white.spr" ) ), PARTICLE_WHITE_10);
	else 
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( LoadSprite( "sprites/particles_white.spr" ) ), PARTICLE_WHITE_5);

	// usual triapi stuff
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	
	// go through drips list
	cl_drip* Drip = FirstChainDrip.p_Next;
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();

	if ( Rain.weatherMode == 0 )
	{ // draw rain
		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;
					
			Vector2D toPlayer; 
			toPlayer.x = player->origin[0] - Drip->origin[0];
			toPlayer.y = player->origin[1] - Drip->origin[1];
			toPlayer = toPlayer.Normalize();
	
			toPlayer.x *= DRIP_SPRITE_HALFWIDTH;
			toPlayer.y *= DRIP_SPRITE_HALFWIDTH;

			float shiftX = (Drip->xDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
			float shiftY = (Drip->yDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;

		// --- draw triangle --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, Drip->alpha );
			gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0]-toPlayer.y - shiftX, Drip->origin[1]+toPlayer.x - shiftY,
					Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 0.5, 1 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0] + shiftX, Drip->origin[1] + shiftY,
					Drip->origin[2]-DRIP_SPRITE_HALFHEIGHT );

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				gEngfuncs.pTriAPI->Vertex3f( Drip->origin[0]+toPlayer.y - shiftX, Drip->origin[1]-toPlayer.x - shiftY,
					Drip->origin[2]+DRIP_SPRITE_HALFHEIGHT);
	
			gEngfuncs.pTriAPI->End();
		// --- draw triangle end ----------------------

			Drip = nextdDrip;
		}
	}
	else
	{ // draw snow
		vec3_t normal;
		gEngfuncs.GetViewAngles((float*)normal);
	
		float matrix[3][4];
		AngleMatrix (normal, matrix);	// calc view matrix

		while (Drip != NULL)
		{
			cl_drip* nextdDrip = Drip->p_Next;

			matrix[0][3] = Drip->origin[0]; // write origin to matrix
			matrix[1][3] = Drip->origin[1];
			matrix[2][3] = Drip->origin[2];

			// apply start fading effect
			float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha :
				((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;
					
		// --- draw quad --------------------------
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
		SetPoint(0, SNOW_SPRITE_HALFSIZE ,SNOW_SPRITE_HALFSIZE, matrix);

		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				SetPoint(0, SNOW_SPRITE_HALFSIZE ,-SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE ,-SNOW_SPRITE_HALFSIZE, matrix);

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				SetPoint(0, -SNOW_SPRITE_HALFSIZE ,SNOW_SPRITE_HALFSIZE, matrix);
				
			gEngfuncs.pTriAPI->End();
		// --- draw quad end ----------------------

			Drip = nextdDrip;
		}
	}
}

/* rain tutorial
=================================
DrawFXObjects

��ᮢ���� ����������
=================================
*/
extern cl_rainfx FirstChainFX;

void DrawFXObjects( void )
{
	if (FirstChainFX.p_Next == NULL)
		return; // no objects to draw

	float curtime = gEngfuncs.GetClientTime();

	// usual triapi stuff
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( LoadSprite( "sprites/particles_white.spr" ) ), PARTICLE_WHITE_13);
	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	
	// go through objects list
	cl_rainfx* curFX = FirstChainFX.p_Next;
	while (curFX != NULL)
	{
		cl_rainfx* nextFX = curFX->p_Next;
					
		// fadeout
		float alpha = ((curFX->birthTime + curFX->life - curtime) / curFX->life) * curFX->alpha;
		float size = (curtime - curFX->birthTime) * MAXRINGHALFSIZE;

		// --- draw quad --------------------------
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, alpha );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] - size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] + size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] + size, curFX->origin[2]);

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] - size, curFX->origin[2]);
	
		gEngfuncs.pTriAPI->End();
		// --- draw quad end ----------------------

		curFX = nextFX;
	}
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	if (gHUD.m_iIntermission == 0)
	{
		if (g_pRenderManager)// don't duplicate in overview window
			g_pRenderManager->RenderOpaque();
	}
	gHUD.m_Spectator.DrawOverview();
#if TEST_IT
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	if (gHUD.m_iIntermission == 0)
	{
		if (g_pRenderManager)// don't duplicate in overview window
			g_pRenderManager->RenderTransparent();
	}

	g_pParticleSystems->UpdateSystems();
	ProcessFXObjects();
	ProcessRain();
	DrawRain();
	DrawFXObjects();
#if TEST_IT
//	Draw_Triangles();
#endif
}
