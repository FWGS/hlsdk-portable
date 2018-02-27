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
//
// neffect.cpp
//
// implementation of CHudNoiseEffect class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#define MAX_NOISE_EFFECT_FRAMES	13

int grgNEffectFrame[MAX_NOISE_EFFECT_FRAMES] = { 1, 2, 3, 4, 3, 2, 1, 2, 3, 4, 3, 2, 1 };

extern cvar_t	*cl_noiseeffect;

int CHudNoiseEffect::Init(void)
{
	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

int CHudNoiseEffect::VidInit(void)
{
	int HUD_neffect = gHUD.GetSpriteIndex( "neffect" );
	m_hSprite = gHUD.GetSprite( HUD_neffect );
	m_prc = &gHUD.GetSpriteRect( HUD_neffect );
	m_iWidth = m_prc->right - m_prc->left;

	return 1;
};

int CHudNoiseEffect::Draw (float flTime)
{
	int r, g, b, x, y, sliceCountX, sliceCountY, frame;

	if( !cl_noiseeffect->value )
		return 1;

	if( !( gHUD.m_iHideHUDDisplay & HIDEHUD_NOISEEFFECT ) )
		return 1;

	frame = grgNEffectFrame[(int)(flTime * 20) % MAX_NOISE_EFFECT_FRAMES] - 1;
	sliceCountX = ScreenWidth / m_iWidth;
	sliceCountY = ScreenHeight / m_iWidth;

	if( ScreenWidth > m_iWidth * sliceCountX )
		sliceCountX++;

	if( ScreenHeight > m_iWidth * sliceCountY )
                sliceCountY++;

	UnpackRGB( r,g,b, gHUD.m_iHUDColor );

	SPR_Set( m_hSprite, r, g, b );

	for( int j = 0; j < sliceCountY; j++ )
	{
		y = j * m_iWidth;
		for( int i = 0; i < sliceCountX; i++ )
		{
			x = i * m_iWidth;
			SPR_DrawAdditive( gEngfuncs.pfnRandomLong( 0, 1 ) ? 3 - frame : frame, x, y, m_prc );
		}
	}
	return 1;
}
