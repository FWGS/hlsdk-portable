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

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "triangleapi.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"

extern bool bDrawScope;
extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle
extern vec3_t v_cl_angles;	// last client/mouse angle
extern vec3_t v_sim_org;	// last sim origin
extern cvar_t *adjust_fov;

int CHudScope::Init( void )
{
	gHUD.AddHudElem( this );

	return 1;
}

int CHudScope::VidInit( void )
{
	m_hSprite = 0;

	return 1;
}

int CHudScope::DrawScope()
{
	vec3_t		 angles, forward, right, up;
	vec3_t		 dir, delta, vert;
	int		 frame;
	float		 flDist, x, y;
	pmtrace_t	*trace;

	if( !bDrawScope )
		return 1;

	if( !m_hSprite )
	{
		m_hSprite = SPR_Load( "sprites/scopeborder.spr" );
		m_hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite );
	}

	gEngfuncs.GetViewAngles( angles );
	AngleVectors( angles, forward, right, up );

	VectorMA( v_origin, 8192.0f, forward, dir );

	trace = gEngfuncs.PM_TraceLine( v_origin, dir, PM_TRACELINE_PHYSENTSONLY, 2, -1 );
	VectorSubtract( trace->endpos, v_origin, delta );

	flDist = Length( delta );

	if( flDist >= 118.0f )
		sprintf( m_szDist, "%.2f m", (double)(flDist / 39.37f) );
	else
		strcpy( m_szDist, "-.-- m" );

	x = tan( gHUD.m_iFOV / 114.6f ) * 11.0f;
	y = x / 255.0f;

	gEngfuncs.pTriAPI->RenderMode( kRenderTransTexture ); //additive
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );

	if( adjust_fov && adjust_fov->value )
		VectorMA( v_origin, 8.0f, forward, dir );
	else
		VectorMA( v_origin, 10.0f, forward, dir );

	frame = 0;

	while( true )
	{
		if( !gEngfuncs.pTriAPI->SpriteTexture( m_hSpriteModel, frame ) )
			break;

		gEngfuncs.pTriAPI->Color4f( 1.0f, 1.0f, 1.0f, 1.0f );

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->Brightness( 1.0f );
		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 1.0f );

		switch( frame )
		{
		case 0:
			VectorMA( dir, -x, right, vert );
			break;
		case 1:
			VectorCopy( dir, vert );
			break;
		case 2:
			VectorMA( dir, -x, up, vert );
			VectorMA( vert, -x, right, vert );
			break;
		case 3:
			VectorMA( dir, -x, up, vert );
			break;
		}

		gEngfuncs.pTriAPI->Vertex3fv( vert );
		gEngfuncs.pTriAPI->Brightness( 1.0f );
		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 0.0f );

		switch( frame )
		{
		case 0:
			VectorMA( dir, x, up, vert );
			VectorMA( vert, -x, right, vert );
			break;
		case 1:
			VectorMA( dir, x, up, vert );
			break;
		case 2:
			VectorMA( dir, y, up, vert );
			VectorMA( vert, -x, right, vert );
			break;
		case 3:
			VectorCopy( dir, vert );
			break;
		}

		gEngfuncs.pTriAPI->Vertex3fv( vert );
		gEngfuncs.pTriAPI->Brightness( 1.0f );
		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 0.0f );

		switch( frame )
		{
		case 0:
			VectorMA( dir, x, up, vert );
			break;
		case 1:
			VectorMA( dir, x, up, vert );
			VectorMA( vert, x, right, vert );
			break;
		case 2:
			VectorCopy( dir, vert );
			break;
		case 3:
			VectorMA( dir, y, up, vert );
			VectorMA( vert, x, right, vert );
			break;
		}

		gEngfuncs.pTriAPI->Vertex3fv( vert );
		gEngfuncs.pTriAPI->Brightness( 1.0f );
		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 1.0f );

		switch( frame )
		{
		case 0:
			VectorCopy( dir, vert );
			break;
		case 1:
			VectorMA( dir, x, right, vert );
			break;
		case 2:
			VectorMA( dir, -x, up, vert );
			break;
		case 3:
			VectorMA( dir, -x, up, vert );
			VectorMA( vert, x, right, vert );
			break;
		}

		gEngfuncs.pTriAPI->Vertex3fv( vert );
		gEngfuncs.pTriAPI->End();

		if( ++frame >= 4 )
		{
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
			cl_entity_t *viewmodel = gEngfuncs.GetViewModel();
			viewmodel->curstate.rendermode = kRenderGlow;
			viewmodel->curstate.renderamt = 5;
			break;
		}
	}

	return 1;
}

int CHudScope::DrawText()
{
	int w, h;

	if( !bDrawScope )
		return 1;

	GetConsoleStringSize( m_szDist, &w, &h );

	DrawSetTextColor( 1.0f, 0.25f, 0.25f );
	DrawConsoleString( ScreenWidth * 0.82f - w, ScreenHeight * 0.49f - h, m_szDist );

	return 1;
}
