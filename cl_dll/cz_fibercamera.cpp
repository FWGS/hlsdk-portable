/*
cz_fibercamera.cpp - CZ:DS fiber optic camera and info_camera views
Copyright (C) 2026 sergiopoverony

The CZ:DS server (CBasePlayer::StartFiberCamera) sends "FiberCamera" with the
origin and angles of the camera, sets the player's iuser1 to OBS_CHASE_FREE and
the view entity to the camera. The original client (CHudFiberCamera, view.cpp
V_CalcSpectatorRefdef) puts the view at the camera, lets the mouse turn it and
draws the lens frame and the noise over the picture. The Counter-Strike client
took iuser1 for spectator mode and flew the view to the world origin.
*/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "ref_params.h"
#include "com_model.h"
#include "triangleapi.h"
#include "cz_fibercamera.h"

#include <string.h>

static struct
{
	bool on;
	bool firstFrame;
	Vector origin;
	Vector angles;

	HSPRITE noise;
	HSPRITE frame[3][8];
	HSPRITE small[4];
	HSPRITE horizontal, vertical;
	bool loaded;
} g_czFiber;

extern vec3_t v_origin, v_angles, v_cl_angles;

static int CzFiber_Msg( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	g_czFiber.on = reader.ReadByte() != 0;
	reader.ReadShort(); // the camera entity

	g_czFiber.origin.x = reader.ReadCoord();
	g_czFiber.origin.y = reader.ReadCoord();
	g_czFiber.origin.z = reader.ReadCoord();
	g_czFiber.angles.x = reader.ReadCoord();
	g_czFiber.angles.y = reader.ReadCoord();
	g_czFiber.angles.z = reader.ReadCoord();

	if( g_czFiber.angles.x > 180 )
		g_czFiber.angles.x -= 360;
	if( g_czFiber.angles.x < -180 )
		g_czFiber.angles.x += 360;

	g_czFiber.firstFrame = g_czFiber.on;

	return 1;
}

void CzFiber_Init( void )
{
	memset( &g_czFiber, 0, sizeof( g_czFiber ));
	gEngfuncs.pfnHookUserMsg( "FiberCamera", CzFiber_Msg );
}

void CzFiber_VidInit( void )
{
	g_czFiber.on = false;
	g_czFiber.loaded = false;
}

bool CzFiber_IsOn( void )
{
	return g_czFiber.on;
}

bool CzFiber_CalcRefdef( struct ref_params_s *pparams )
{
	if( !g_czFiber.on )
		return false;

	// the camera's own direction first, then the mouse turns it
	if( g_czFiber.firstFrame )
	{
		gEngfuncs.SetViewAngles( g_czFiber.angles );
		VectorCopy( g_czFiber.angles, pparams->cl_viewangles );
		g_czFiber.firstFrame = false;
	}

	VectorCopy( g_czFiber.origin, pparams->vieworg );
	VectorCopy( pparams->cl_viewangles, pparams->viewangles );

	VectorCopy( pparams->vieworg, v_origin );
	VectorCopy( pparams->viewangles, v_angles );
	VectorCopy( pparams->cl_viewangles, v_cl_angles );

	// no weapon in the picture
	cl_entity_t *view = gEngfuncs.GetViewModel();
	if( view )
		view->model = NULL;

	return true;
}

static void CzFiber_Load( void )
{
	static const char *names[3][8] =
	{
		{ "top_left", "top", "top_right", "left", "right", "bottom_left", "bottom", "bottom_right" },
		{ "top_left2", "top2", "top_right2", "left2", "right2", "bottom_left2", "bottom2", "bottom_right2" },
		{ "top_left3", "top3", "top_right3", "left3", "right3", "bottom_left3", "bottom3", "bottom_right3" },
	};
	static const char *smallNames[4] = { "top_left_small", "top_right_small", "bottom_left_small", "bottom_right_small" };
	char path[64];

	for( int group = 0; group < 3; group++ )
	{
		for( int i = 0; i < 8; i++ )
		{
			snprintf( path, sizeof( path ), "sprites/%s.spr", names[group][i] );
			g_czFiber.frame[group][i] = SPR_Load( path );
		}
	}

	for( int i = 0; i < 4; i++ )
	{
		snprintf( path, sizeof( path ), "sprites/%s.spr", smallNames[i] );
		g_czFiber.small[i] = SPR_Load( path );
	}

	g_czFiber.horizontal = SPR_Load( "sprites/horizontal.spr" );
	g_czFiber.vertical = SPR_Load( "sprites/vertical.spr" );
	g_czFiber.noise = SPR_Load( "sprites/fo-noise.spr" );
	g_czFiber.loaded = true;
}

static void CzFiber_Piece( HSPRITE spr, int x, int y, int w, int h )
{
	if( !spr )
		return;

	wrect_t rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = w;
	rc.bottom = h;

	SPR_Set( spr, 255, 255, 255 );
	SPR_DrawHoles( 0, x, y, &rc );
}

// the lens frame of the original CHudFiberCamera::Draw (sprites cut for 640, 800 and 1024 wide screens,
// the screen around them is covered with the horizontal and vertical strips)
void CzFiber_Draw( float flTime )
{
	if( !g_czFiber.on )
		return;

	if( !g_czFiber.loaded )
		CzFiber_Load();

	if( g_czFiber.noise )
	{
		// GoldSrc blends it with the given factors (1, 1 = GL_ONE, GL_ONE: added to
		// the picture). Xash ignores them and draws in the last render mode, after
		// SPR_DrawHoles that is the opaque one, so the noise hid the room.
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		SPR_Set( g_czFiber.noise, 255, 0, 0 );
		gEngfuncs.pfnSPR_DrawGeneric( (int)( flTime * 20 ) % 16, 0, 0, NULL, 1, 1, ScreenWidth, ScreenHeight );
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}

	int centerX = ScreenWidth / 2;
	int centerY = ScreenHeight / 2;

	if( ScreenWidth < 640 )
	{
		for( int i = 0; i < 4; i++ )
		{
			int x = ( i & 1 ) ? centerX : ( ScreenWidth <= 511 ? -48 : 0 );
			int y = i >= 2 ? centerY - ( ScreenWidth <= 511 ? 32 : 0 ) : 0;
			CzFiber_Piece( g_czFiber.small[i], x, y, 256, 256 );
		}
		return;
	}

	int group = ScreenWidth > 1023 ? 2 : ScreenWidth > 799 ? 1 : 0;
	int left = group == 0 ? -320 : -384;
	int top = group == 0 ? -240 : group == 1 ? -300 : -384;
	static const int columns[8] = { 0, 1, 2, 0, 2, 0, 1, 2 };
	static const int rows[8] = { 0, 0, 0, 1, 1, 2, 2, 2 };
	int xs[3] = { centerX + left, centerX - 128, centerX + 128 };
	int ys[3] = { centerY + top, centerY - 128, centerY + 128 };

	for( int i = 0; i < 8; i++ )
	{
		int w = ( group == 0 && i != 1 && i != 6 ) ? 192 : 256;
		int h = ( group == 2 || i == 3 || i == 4 ) ? 256 : group == 0 ? 112 : 172;
		CzFiber_Piece( g_czFiber.frame[group][i], xs[columns[i]], ys[rows[i]], w, h );
	}

	if( ScreenWidth <= 640 )
		return;

	int leftMask = centerX - ( ScreenWidth > 799 ? 384 : 320 );
	int rightMask = centerX + ( ScreenWidth > 799 ? 384 : 320 );

	if( ScreenWidth > 1024 )
	{
		int count = ScreenWidth / 256 + 1;

		for( int y = centerY - 416; y > 0; y -= 32 )
			for( int i = 0; i < count; i++ )
				CzFiber_Piece( g_czFiber.horizontal, i * 256, y, 256, 32 );

		for( int i = 0; i < count; i++ )
			CzFiber_Piece( g_czFiber.horizontal, i * 256, 0, 256, 32 );

		for( int y = centerY + 384; y < ScreenHeight; y += 32 )
			for( int i = 0; i < count; i++ )
				CzFiber_Piece( g_czFiber.horizontal, i * 256, y, 256, 32 );
	}

	int count = ScreenHeight / 256 + 1;

	for( int x = leftMask - 40; x > 0; x -= 40 )
		for( int i = 0; i < count; i++ )
			CzFiber_Piece( g_czFiber.vertical, x, i * 256, 40, 256 );

	for( int i = 0; i < count; i++ )
		CzFiber_Piece( g_czFiber.vertical, 0, i * 256, 40, 256 );

	for( int x = rightMask; x < ScreenWidth; x += 40 )
		for( int i = 0; i < count; i++ )
			CzFiber_Piece( g_czFiber.vertical, x, i * 256, 40, 256 );
}
