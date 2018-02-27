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
// Blackbar.cpp
//
// implementation of CHudBlackBar class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

int CHudBlackBar::Init(void)
{
	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

int CHudBlackBar::VidInit(void)
{
	int HUD_blackbar = gHUD.GetSpriteIndex( "blackbar" );
	m_hSprite = gHUD.GetSprite( HUD_blackbar );
	m_prc = &gHUD.GetSpriteRect( HUD_blackbar );
	m_iHeight = m_prc->bottom - m_prc->top;
	m_iWidth = m_prc->right - m_prc->left;

	return 1;
};

int CHudBlackBar::Draw (float flTime)
{
	if( !( gHUD.m_iHideHUDDisplay & HIDEHUD_BLACKBARS) )
		return 1;

	int r, g, b, x, y, sliceCount;

	sliceCount = ScreenWidth / m_iWidth;

	if( ScreenWidth > m_iWidth * sliceCount )
		sliceCount++;

	UnpackRGB( r,g,b, gHUD.m_iHUDColor );

	SPR_Set( m_hSprite, r, g, b );

	y = ScreenHeight - m_iHeight;

	for( int i = 0; i < sliceCount; i++ )
	{
		x = i * m_iWidth;
		SPR_Draw( 0,  x, -1, m_prc );
		SPR_Draw( 0,  x, y, m_prc );
	}
	return 1;
}
