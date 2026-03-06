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
#include "hud_sprite.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

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
	m_flBat = 1.0f;
}

int CHudFlashlight::VidInit( void )
{
	int HUD_flash_empty = gHUD.GetSpriteIndex( "flash_empty" );
	int HUD_flash_full = gHUD.GetSpriteIndex( "flash_full" );
	int HUD_flash_beam = gHUD.GetSpriteIndex( "flash_beam" );

	m_hSprite1 = gHUD.GetSprite( HUD_flash_empty );
	m_hSprite2 = gHUD.GetSprite( HUD_flash_full );
	m_hBeam = gHUD.GetSprite( HUD_flash_beam );
	m_prc1 = &gHUD.GetSpriteRect( HUD_flash_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_flash_full );
	m_prcBeam = &gHUD.GetSpriteRect(HUD_flash_beam);
	m_iWidth = m_prc2->right - m_prc2->left;

	return 1;
}

int CHudFlashlight::MsgFunc_FlashBat( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0f;

	return 1;
}

int CHudFlashlight::MsgFunc_Flashlight( const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	int x = READ_BYTE();
	m_iBat = x;
	m_flBat = ( (float)x ) / 100.0f;

	return 1;
}

int CHudFlashlight::Draw( float flTime )
{
	static bool show = ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) );
	if( show != !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) ) )
	{
		show = !( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) );
		if( gMobileEngfuncs )
		{
			gMobileEngfuncs->pfnTouchHideButtons( "flashlight", !show );
		}
	}
	if( !show )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	if( gEngfuncs.IsSpectateOnly() )
		return 1;

	if( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) ) )
		return 1;

	if( m_fOn )
		a = 225;
	else
		a = MIN_ALPHA;

	if( m_flBat < 0.20f )
		UnpackRGB( r,g,b, RGB_REDISH );
	else
		UnpackRGB(r,g,b, gHUD.m_iHUDColor);

	ScaleColors( r, g, b, a );

	y = ( m_prc1->bottom - m_prc2->top ) / 2;
	x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth - m_iWidth / 2 ;

	// Draw the flashlight casing
	ScaledRenderer::Instance().SPR_Set(m_hSprite1, r, g, b );
	ScaledRenderer::Instance().SPR_DrawAdditive( 0,  x, y, m_prc1);

	if( m_fOn )
	{
		// draw the flashlight beam
		x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth/2;

		ScaledRenderer::Instance().SPR_Set( m_hBeam, r, g, b );
		ScaledRenderer::Instance().SPR_DrawAdditive( 0, x, y, m_prcBeam );
	}

	// draw the flashlight energy level
	x = ScaledRenderer::Instance().ScreenWidthScaled() - m_iWidth - m_iWidth/2 ;
	int iOffset = m_iWidth * ( 1.0f - m_flBat );
	if( iOffset < m_iWidth )
	{
		rc = *m_prc2;
		rc.left += iOffset;

		ScaledRenderer::Instance().ScaledRenderer::Instance().SPR_Set(m_hSprite2, r, g, b );
		ScaledRenderer::Instance().SPR_DrawAdditive( 0, x + iOffset, y, &rc);
	}

	return 1;
}

DECLARE_MESSAGE( m_Nightvision, Nightvision )

#define NIGHTVISION_SPRITE_NAME "sprites/visor.spr"
#define NIGHTVISION_FRAME_UPDATE_DELAY 0.1f

int CHudNightvision::Init(void)
{
	m_fOn = 0;

	HOOK_MESSAGE(Nightvision);

	m_iFlags |= HUD_ACTIVE;

	m_pLight = 0;
	m_frameUpdateTime = 0;

	return 1;
}

void CHudNightvision::Reset(void)
{
	m_fOn = 0;
}

int CHudNightvision::VidInit(void)
{
	m_hSprite = LoadSprite(NIGHTVISION_SPRITE_NAME);

	// Get the number of frames available in this sprite.
	m_nFrameCount = SPR_Frames(m_hSprite);

	// current frame.
	m_iFrame = 0;
	m_frameUpdateTime = 0;
	return 1;
}


int CHudNightvision::MsgFunc_Nightvision(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_fOn = READ_BYTE();
	if (!m_fOn) {
		RemoveDlight();
	}

	return 1;
}

int CHudNightvision::Draw(float flTime)
{
	if( gEngfuncs.IsSpectateOnly() )
	{
		return 1;
	}

	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_FLASHLIGHT | HIDEHUD_ALL))
		return 1;

	// Only display this if the player is equipped with the suit.
	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	if (m_fOn) {
			DrawNVG(flTime);
	}
	return 1;
}

void CHudNightvision::DrawNVG(float flTime)
{
	int r, g, b, x, y;

	r = 255;
	g = 255;
	b = 255;
	int a = 255;

	ScaleColors(r, g, b, a);

	// Top left of the screen.
	x = y = 0;

	// Reset the number of frame if we are at last frame.
	if (m_iFrame >= m_nFrameCount)
		m_iFrame = 0;

	const int nvgSpriteWidth = SPR_Width(m_hSprite, 0);
	const int nvgSpriteHeight = SPR_Height(m_hSprite, 0);

	const int colCount = (int)ceil(ScreenWidth / (float)nvgSpriteWidth);
	const int rowCount = (int)ceil(ScreenHeight / (float)nvgSpriteHeight);

	//
	// draw nightvision scanlines sprite.
	//
	SPR_Set(m_hSprite, r, g, b);

	int i, j;
	for (i = 0; i < rowCount; ++i) // height
	{
		for (j = 0; j < colCount; ++j) // width
		{
			SPR_DrawAdditive(m_iFrame, x + (j * 256), y + (i * 256), NULL);
		}
	}

	// Increase sprite frame.
	if (flTime > m_frameUpdateTime)
	{
		m_iFrame++;
		m_frameUpdateTime = flTime + NIGHTVISION_FRAME_UPDATE_DELAY;
	}

	if( !m_pLight || m_pLight->die < flTime )
	{
		r = 250;
		g = 250;
		b = 250;
		m_pLight = MakeDynLight(flTime, r, g, b);
	}
	UpdateDynLight( m_pLight, NvgRadius(), gHUD.m_vecOrigin + Vector(0.0f, 0.0f, 32.0f ) );
}

dlight_t* CHudNightvision::MakeDynLight(float flTime, int r, int g, int b)
{
	dlight_t* dLight = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );

	// I hope no one is crazy so much to keep NVG for 9999 seconds
	dLight->die = flTime + 9999.0f;
	dLight->color.r = r;
	dLight->color.g = g;
	dLight->color.b = b;

	return dLight;
}

void CHudNightvision::UpdateDynLight(dlight_t *dynLight, float radius, const Vector &origin)
{
	if( dynLight )
	{
		dynLight->origin = origin;
		dynLight->radius = radius;
	}
}

void CHudNightvision::RemoveDlight()
{
	if( m_pLight )
	{
		m_pLight->die = 0;
		m_pLight = NULL;
	}
}

#define NVG_RADIUS_MIN 400
#define NVG_RADIUS_MAX 1000

float CHudNightvision::NvgRadius()
{
	extern cvar_t *cl_nvgradius;
	float radius = cl_nvgradius && cl_nvgradius->value > 0 ? cl_nvgradius->value : 450;
	if (radius < NVG_RADIUS_MIN)
		return NVG_RADIUS_MIN;
	else if (radius > NVG_RADIUS_MAX)
		return NVG_RADIUS_MAX;
	return radius;
}

bool CHudNightvision::IsOn()
{
	return m_fOn != 0;
}