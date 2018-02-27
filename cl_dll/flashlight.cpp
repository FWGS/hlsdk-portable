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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_Flash, FlashBat )
DECLARE_MESSAGE( m_Flash, Flashlight )

#define BAT_NAME "sprites/%d_Flashlight.spr"

int CHudFlashlight::Init( void )
{
	m_fFade = 0;
	m_fOn = 0;

	HOOK_MESSAGE( Flashlight );
	HOOK_MESSAGE( FlashBat );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem( this );

	return 1;
}

void CHudFlashlight::Reset( void )
{
	m_fFade = 0;
	m_fOn = 0;
	m_iBat = 100;
	m_flBat = 1.0;
}

int CHudFlashlight::VidInit( void )
{
	// int HUD_flash_empty = gHUD.GetSpriteIndex( "flash_empty" );
	// int HUD_flash_beam = gHUD.GetSpriteIndex( "flash_beam" );
	int HUD_flash_on = gHUD.GetSpriteIndex( "flash_on" );
	int HUD_flash_off = gHUD.GetSpriteIndex( "flash_off" );

	m_hSprite1 = gHUD.GetSprite( HUD_flash_on );
	m_hSprite2 = gHUD.GetSprite( HUD_flash_off );
	// m_hBeam = gHUD.GetSprite( HUD_flash_beam );
	m_prc1 = &gHUD.GetSpriteRect( HUD_flash_on );
	m_prc2 = &gHUD.GetSpriteRect( HUD_flash_off );
	// m_prcBeam = &gHUD.GetSpriteRect( HUD_flash_beam );
	m_iWidth = m_prc2->right - m_prc2->left;

	return 1;
}

int CHudFlashlight::MsgFunc_FlashBat( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0;

	return 1;
}

int CHudFlashlight::MsgFunc_Flashlight( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0;

	return 1;
}

int CHudFlashlight::Draw( float flTime )
{
	static bool show = ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL | HIDEHUD_ALL_EXCLUDEMESSAGE ) );
	if( show != !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL | HIDEHUD_ALL_EXCLUDEMESSAGE ) ) )
	{
		show = !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL | HIDEHUD_ALL_EXCLUDEMESSAGE ) );
		if( gMobileEngfuncs )
		{
			gMobileEngfuncs->pfnTouchHideButtons( "flashlight", !show );
		}
	}
	if( !show )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	if( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_FLASHLIGHT ) ) ) )
		return 1;

	//if( m_fOn )
		a = 225;
	//else
	//	a = MIN_ALPHA;

	/*if( m_flBat < 0.20 )
		UnpackRGB( r, g, b, RGB_REDISH );
	else*/
		UnpackRGB( r, g, b, gHUD.m_iHUDColor );

	ScaleColors( r, g, b, a );

	y = ( m_prc1->bottom - m_prc2->top ) / 2;
	x = ScreenWidth - m_iWidth - m_iWidth / 2;

	// Draw the flashlight casing
	if( m_fOn )
	{
		SPR_Set( m_hSprite1, r, g, b );
		SPR_DrawAdditive( 0, x, y, m_prc1 );
	}
	else
	{
		SPR_Set( m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x, y, m_prc2 );
	}

	return 1;
}
