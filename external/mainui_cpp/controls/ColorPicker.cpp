/*
ColorPicker.cpp - color picker widget
Copyright (C) 2026 $_Vladislav

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <math.h>

#include "ColorPicker.h"
#include "BaseMenu.h"
#include "extdll_menu.h"
#include "utlvector.h"

CMenuColorPicker::CMenuColorPicker()
	: m_hue( 0.f ), m_sat( 1.f ), m_val( 1.f ), m_svTex( 0 ), m_hueTex( 0 ),
	  m_cursorTex( 0 ), m_draggingSV( false ), m_draggingHue( false )
{
	size.w = 246;
	size.h = 220;
}

CMenuColorPicker::~CMenuColorPicker()
{
	EngFuncs::PIC_Free( "#cp_sv.tga" );
	EngFuncs::PIC_Free( "#cp_hue.tga" );
	EngFuncs::PIC_Free( "#cp_cursor.tga" );
}

void CMenuColorPicker::VidInit()
{
	CMenuBaseItem::VidInit();

	const int hueW = 20 * uiStatic.scaleX;
	const int gap  = 6 * uiStatic.scaleX;

	int side = Q_min( m_scSize.w - hueW - gap, m_scSize.h );

	m_svSize = { side, side };
	m_svPos  = { m_scPos.x, m_scPos.y };

	m_hueSize = { hueW, side };
	m_huePos  = { m_scPos.x + side + gap, m_scPos.y };

	BuildHueTexture();
	BuildCursorTexture();
	RebuildSVTexture();
}

void CMenuColorPicker::RebuildSVTexture()
{
	if( m_svSize.w <= 0 || m_svSize.h <= 0 )
	{
		return;
	}

	const int W       = Q_max( 1, m_svSize.w );
	const int H       = Q_max( 1, m_svSize.h );
	const int bufSize = sizeof( tga_t ) + W * H * 3;
	CUtlVector<byte> buf;
	buf.SetCount( bufSize );
	memset( buf.Base(), 0, bufSize );

	tga_t *hdr      = (tga_t *)buf.Base();
	hdr->image_type = 2; // uncompressed true-color
	hdr->width      = W;
	hdr->height     = H;
	hdr->pixel_size = 24;
	hdr->attributes = 0x20; // descriptor: origin upper left

	byte *pixels = buf.Base() + sizeof( tga_t );

	for( int y = 0; y < H; y++ )
	{
		float val = H > 1 ? 1.f - y / (float)( H - 1 ) : 1.f;
		for( int x = 0; x < W; x++ )
		{
			float sat = W > 1 ? x / (float)( W - 1 ) : 1.f;
			byte  r, g, b;
			ColorUtils::HSVtoRGB( m_hue, sat, val, r, g, b );
			int i = ( y * W + x ) * 3;
			pixels[i + 0] = b;
			pixels[i + 1] = g;
			pixels[i + 2] = r;
		}
	}

	EngFuncs::PIC_Free( "#cp_sv.tga" );
	m_svTex = EngFuncs::PIC_Load( "#cp_sv.tga", buf.Base(), bufSize, PIC_NOMIPMAP | PIC_NEAREST );
}

void CMenuColorPicker::BuildHueTexture()
{
	if( m_hueSize.h <= 0 )
	{
		return;
	}

	const int W      = 4;
	const int H      = Q_max( 1, m_hueSize.h );
	const int bufSize = sizeof( tga_t ) + W * H * 3;
	CUtlVector<byte> buf;
	buf.SetCount( bufSize );
	memset( buf.Base(), 0, bufSize );

	tga_t *hdr      = (tga_t *)buf.Base();
	hdr->image_type = 2; // uncompressed true-color
	hdr->width      = W;
	hdr->height     = H;
	hdr->pixel_size = 24;
	hdr->attributes = 0x20; // descriptor: origin upper left

	byte *pixels = buf.Base() + sizeof( tga_t );

	for( int y = 0; y < H; y++ )
	{
		float hue = H > 1 ? ( 1.f - y / (float)( H - 1 ) ) * 360.f : 0.f;
		byte  r, g, b;
		ColorUtils::HSVtoRGB( hue, 1.f, 1.f, r, g, b );

		for( int x = 0; x < W; x++ )
		{
			int i = ( y * W + x ) * 3;
			pixels[i + 0] = b;
			pixels[i + 1] = g;
			pixels[i + 2] = r;
		}
	}

	EngFuncs::PIC_Free( "#cp_hue.tga" );
	m_hueTex = EngFuncs::PIC_Load( "#cp_hue.tga", buf.Base(), bufSize, PIC_NOMIPMAP | PIC_NEAREST );
}

void CMenuColorPicker::BuildCursorTexture()
{
	const int SIZE    = 17;
	const int bufSize = sizeof( tga_t ) + SIZE * SIZE * 4;
	CUtlVector<byte> buf;
	buf.SetCount( bufSize );
	memset( buf.Base(), 0, bufSize );

	tga_t *hdr      = (tga_t *)buf.Base();
	hdr->image_type = 2; // uncompressed true-color
	hdr->width      = SIZE;
	hdr->height     = SIZE;
	hdr->pixel_size = 32;
	hdr->attributes = 0x28; // 8-bit alpha, descriptor: origin upper left

	byte *pixels = buf.Base() + sizeof( tga_t );

	const float cx      = ( SIZE - 1 ) * 0.5f;
	const float cy      = ( SIZE - 1 ) * 0.5f;
	const float r_mid   = 6.0f;
	const float w_white = 0.75f;
	const float w_black = 0.9f;
	const float aa      = 0.5f;

	for( int y = 0; y < SIZE; y++ )
	{
		for( int x = 0; x < SIZE; x++ )
		{
			float dist = sqrtf( ( x - cx ) * ( x - cx ) + ( y - cy ) * ( y - cy ) );
			float d    = fabsf( dist - r_mid );

			float white_mask  = Q_max( 0.f, Q_min( 1.f, ( w_white - d ) / aa + 0.5f ) );
			float cursor_mask = Q_max( 0.f, Q_min( 1.f, ( ( w_white + w_black ) - d ) / aa + 0.5f ) );

			int i = ( y * SIZE + x ) * 4;
			pixels[i + 0] = (byte)( white_mask * 255.f + 0.5f );
			pixels[i + 1] = (byte)( white_mask * 255.f + 0.5f );
			pixels[i + 2] = (byte)( white_mask * 255.f + 0.5f );
			pixels[i + 3] = (byte)( cursor_mask * 255.f + 0.5f );
		}
	}

	EngFuncs::PIC_Free( "#cp_cursor.tga" );
	m_cursorTex = EngFuncs::PIC_Load( "#cp_cursor.tga", buf.Base(), bufSize, PIC_NOMIPMAP | PIC_HAS_ALPHA );
}

void CMenuColorPicker::Draw()
{
	if( m_svTex )
	{
		EngFuncs::PIC_EnableScissor( m_svPos.x, m_svPos.y, m_svSize.w, m_svSize.h );

		EngFuncs::PIC_Set( m_svTex, 255, 255, 255 );
		EngFuncs::PIC_DrawTrans( m_svPos, m_svSize );

		if( m_cursorTex )
		{
			const int CS = 17;
			int       cx = m_svPos.x + (int)( m_sat * Q_max( 0, m_svSize.w - 1 ) );
			int       cy = m_svPos.y + (int)( ( 1.f - m_val ) * Q_max( 0, m_svSize.h - 1 ) );
			EngFuncs::PIC_Set( m_cursorTex, 255, 255, 255 );
			EngFuncs::PIC_DrawTrans( Point( cx - CS / 2, cy - CS / 2 ), Size( CS, CS ) );
		}

		UI_DrawRectangle( m_svPos, m_svSize, PackRGBA( 255, 255, 255, 64 ) );
		EngFuncs::PIC_DisableScissor();
	}

	if( m_hueTex )
	{
		EngFuncs::PIC_EnableScissor( m_huePos.x, m_huePos.y, m_hueSize.w, m_hueSize.h );

		EngFuncs::PIC_Set( m_hueTex, 255, 255, 255 );
		EngFuncs::PIC_DrawTrans( m_huePos, m_hueSize );

		int hy = m_huePos.y + (int)( ( 1.f - m_hue / 360.f ) * Q_max( 0, m_hueSize.h - 1 ) );
		EngFuncs::FillRGBA( m_huePos.x, hy - 1, m_hueSize.w, 1, 0, 0, 0, 255 );
		EngFuncs::FillRGBA( m_huePos.x, hy, m_hueSize.w, 2, 255, 255, 255, 255 );
		EngFuncs::FillRGBA( m_huePos.x, hy + 2, m_hueSize.w, 1, 0, 0, 0, 255 );

		UI_DrawRectangle( m_huePos, m_hueSize, PackRGBA( 255, 255, 255, 64 ) );
		EngFuncs::PIC_DisableScissor();
	}
}

bool CMenuColorPicker::MouseMove( int x, int y )
{
	if( m_draggingSV || m_draggingHue )
	{
		if( !EngFuncs::KEY_IsDown( K_MOUSE1 ) )
		{
			m_draggingSV = m_draggingHue = false;
			return false;
		}
	}

	if( m_draggingSV )
	{
		const float w = m_svSize.w > 1 ? (float)( m_svSize.w - 1 ) : 1.f;
		const float h = m_svSize.h > 1 ? (float)( m_svSize.h - 1 ) : 1.f;
		m_sat = Q_max( 0.f, Q_min( 1.f, ( x - m_svPos.x ) / w ) );
		m_val = Q_max( 0.f, Q_min( 1.f, 1.f - ( y - m_svPos.y ) / h ) );
		_Event( QM_CHANGED );
		return true;
	}
	if( m_draggingHue )
	{
		const float h = m_hueSize.h > 1 ? (float)( m_hueSize.h - 1 ) : 1.f;
		float       t = ( y - m_huePos.y ) / h;
		m_hue = ( 1.f - Q_max( 0.f, Q_min( 1.f, t ) ) ) * 360.f;
		RebuildSVTexture();
		_Event( QM_CHANGED );
		return true;
	}
	return false;
}

bool CMenuColorPicker::KeyDown( int key )
{
	if( key == K_MOUSE1 )
	{
		if( UI_CursorInRect( m_svPos, m_svSize ) )
		{
			m_draggingSV = true;
			MouseMove( uiStatic.cursorX, uiStatic.cursorY );
			return true;
		}
		if( UI_CursorInRect( m_huePos, m_hueSize ) )
		{
			m_draggingHue = true;
			MouseMove( uiStatic.cursorX, uiStatic.cursorY );
			return true;
		}
	}
	return false;
}

bool CMenuColorPicker::KeyUp( int key )
{
	if( key == K_MOUSE1 && ( m_draggingSV || m_draggingHue ) )
	{
		m_draggingSV = m_draggingHue = false;
		return true;
	}
	return false;
}

void CMenuColorPicker::GetRGB( byte &r, byte &g, byte &b ) const
{
	ColorUtils::HSVtoRGB( m_hue, m_sat, m_val, r, g, b );
}

void CMenuColorPicker::SetRGB( byte r, byte g, byte b, bool rebuild )
{
	ColorUtils::RGBtoHSV( r, g, b, m_hue, m_sat, m_val );
	if( rebuild )
		RebuildSVTexture();
}

void CMenuColorPicker::SetHSV( float h, float s, float v )
{
	bool hueChanged = ( fabsf( h - m_hue ) > 0.001f );
	m_hue = h;
	m_sat = s;
	m_val = v;
	if( hueChanged )
		RebuildSVTexture();
}
