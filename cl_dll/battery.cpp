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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#define ARMOR_BAR_LEFT		40
#define ARMOR_BAR_BOTTOM	96

#define ARMOR_BAR_WIDTH		20
#define ARMOR_BAR_HEIGHT	150

DECLARE_MESSAGE( m_Battery, Battery )

int CHudBattery::Init( void )
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE( Battery );

	gHUD.AddHudElem( this );

	return 1;
}

int CHudBattery::VidInit( void )
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;
	return 1;
}

int CHudBattery::MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return 1;
}

int CHudBattery::Draw( float flTime )
{
	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	if( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) ) )
		return 1;

	int r, g, b, x, y, a;
	int iWidth, iHeight;

	iWidth = ARMOR_BAR_WIDTH;
	iHeight = ARMOR_BAR_HEIGHT;

	x = ScreenWidth - ARMOR_BAR_LEFT;
	y = ScreenHeight - ARMOR_BAR_BOTTOM - iHeight;

	// Draw empty transparent bar.
	r = g = b = 200;
	a = 40;

	FillRGBA( x, y, iWidth, iHeight, r, g, b, a );

	// Draw armor level bar.
	UnpackRGB( r, g, b, RGB_YELLOWISH );

	// Has health changed? Flash the health #
	/*if( m_fFade )
	{
		if( m_fFade > FADE_TIME )
			m_fFade = FADE_TIME;

		m_fFade -= ( gHUD.m_flTimeDelta * 20 );
		if( m_fFade <= 0 )
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA + ( m_fFade / FADE_TIME ) * 128;
	}
	else*/
		a = MIN_ALPHA * 2 - 10;

	iHeight = ( m_iBat * ARMOR_BAR_HEIGHT ) / 100;

	gEngfuncs.pfnFillRGBABlend( x, y + ( ARMOR_BAR_HEIGHT - iHeight ), ARMOR_BAR_WIDTH, iHeight, r, g, b, a );

	return 1;
}
