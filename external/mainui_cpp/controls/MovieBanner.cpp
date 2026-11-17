/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "MovieBanner.h"
#include "BackgroundBitmap.h"

// In WON menus the background is always 640x480
const float splashWidth = 640.0f;
const float splashHeight = 480.0f;

void CMenuMovieBanner::VidInit()
{
	EngFuncs::PrecacheLogo( "logo.avi" );

	float xScale, yScale;
	float xOffset = 0, yOffset = 0;

	// Adapt to stretched or cropped background
	if( ui_background_stretch->value )
	{
		xScale = ScreenWidth / splashWidth;
		yScale = ScreenHeight / splashHeight;
	}
	else
	{
		if( ScreenWidth * splashHeight > ScreenHeight * splashWidth )
		{
			xScale = ScreenWidth / splashWidth;
			yScale = xScale;

			yOffset = ( ScreenHeight - 480.0f * yScale ) / 2;
		}
		else
		{
			yScale = ScreenHeight / splashHeight;
			xScale = yScale;

			xOffset = ( ScreenWidth - 640.0f * xScale ) / 2;
		}
	}

	m_scPos.x = (int)xOffset;
	m_scPos.y = (int)( yOffset + 70.0f * yScale ); // 70 is empirically determined value (magic number)

	m_scSize.w = (int)( EngFuncs::GetLogoWidth( ) * xScale );
	m_scSize.h = (int)( EngFuncs::GetLogoHeight( ) * yScale );
}

void CMenuMovieBanner::Draw()
{
	if( FBitSet( ui_background_stretch->flags, FCVAR_CHANGED ) )
	{
		ClearBits( ui_background_stretch->flags, FCVAR_CHANGED );

		// Logo needs to be reset when this CVAR changes
		VidInit();
	}

	if( !CMenuBackgroundBitmap::ShouldDrawLogoMovie() )
		return;

	if( EngFuncs::ClientInGame() && EngFuncs::GetCvarFloat( "ui_renderworld" ))
		return;

	if( EngFuncs::GetLogoLength() <= 0 || EngFuncs::GetLogoWidth() <= 32 )
		return;

	EngFuncs::DrawLogo( "logo.avi", m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
}
