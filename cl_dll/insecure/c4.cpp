/***
*
*	Copyright (c) 1996-2002, C4 LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   C4 LLC.  All other use, distribution, or modification is prohibited
*   without written permission from C4 LLC.
*
****/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_C4, HudC4 );

int CHudC4::Init( void )
{
	gHUD.AddHudElem( this );

	HOOK_MESSAGE( HudC4 );

	InitHUDData();

	return 1;
}

void CHudC4::InitHUDData(void)
{
	m_iFlags |= HUD_ACTIVE;
}

int CHudC4::VidInit(void)
{
	m_HUD_c4 = gHUD.GetSpriteIndex( "c4" );

	m_hSprite = gHUD.GetSprite( m_HUD_c4 );
	return 1;
}

int CHudC4::MsgFunc_HudC4( const char* pszName, int iSize, void* pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	return 1;
}

int CHudC4::Draw( float flTime )
{
    int width = gHUD.GetSpriteRect( m_HUD_c4 ).right - gHUD.GetSpriteRect( m_HUD_c4 ).left;
    int height = gHUD.GetSpriteRect( m_HUD_c4 ).bottom - gHUD.GetSpriteRect( m_HUD_c4 ).top;
    int x, y;

    if ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL || !gHUD.HasC4() )
        return 0;

    int r, g, b;
    UnpackRGB( r, g, b, RGB_YELLOWISH );

    // Top right corner of the screen.
    if ( ScreenWidth > 2560 )
    {
        x = ScreenWidth - width - width / 2;
        y = height / 10;
    }
    else
    {
        x = ScreenWidth - width - width / 10;
        y = height / 2;
    }

    SPR_Set( m_hSprite, r, g, b );

    y = GetC4Position();

    SPR_DrawAdditive( 0, x, y, NULL );

    return 1;
}

int CHudC4::GetC4Position()
{
    int height = gHUD.GetSpriteRect( m_HUD_c4).bottom - gHUD.GetSpriteRect(m_HUD_c4).top;

    if ( !gHUD.HasFlashlight() )
    {
        // If we don't have a flashlight, draw in its place.
        return height / 2;
    }
    else
    {
        return gHUD.GetScreenPosition( 56, 112, 168 );
    }
}