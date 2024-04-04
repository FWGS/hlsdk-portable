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
// mode.cpp
//
// implementation of CHudModeIcon class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_ModeIcon, ChangeMode)

int CHudModeIcon::Init(void)
{
	m_fMode = 0;

	HOOK_MESSAGE(ChangeMode);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

void CHudModeIcon::Reset(void)
{
	m_fMode = 0;
}

int CHudModeIcon::VidInit(void)
{
	int HUD_mode_stand = gHUD.GetSpriteIndex( "mode_stand" );
	int HUD_mode_run = gHUD.GetSpriteIndex( "mode_run" );
	int HUD_mode_crouch = gHUD.GetSpriteIndex( "mode_crouch" );
    int HUD_mode_jump = gHUD.GetSpriteIndex( "mode_jump" );

    m_hSpriteStand = m_hSpriteRun = m_hSpriteCrouch = m_hSpriteJump = 0;

	m_hSpriteStand  = gHUD.GetSprite(HUD_mode_stand);
	m_hSpriteRun    = gHUD.GetSprite(HUD_mode_run);
	m_hSpriteCrouch = gHUD.GetSprite(HUD_mode_crouch);
	m_hSpriteJump   = gHUD.GetSprite(HUD_mode_jump);

	m_prcStand  = &gHUD.GetSpriteRect( HUD_mode_stand );
	m_prcRun    = &gHUD.GetSpriteRect( HUD_mode_run );
	m_prcCrouch = &gHUD.GetSpriteRect( HUD_mode_crouch );
	m_prcJump   = &gHUD.GetSpriteRect( HUD_mode_jump );

	return 1;
};

int CHudModeIcon:: MsgFunc_ChangeMode(const char *pszName,  int iSize, void *pbuf )
{

	BEGIN_READ( pbuf, iSize );
	m_fMode = READ_BYTE();

	return 1;
}

int CHudModeIcon::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

    switch(m_fMode){
	case 0: m_hActiveSprite = m_hSpriteStand;
		    m_prcActiveRect = m_prcStand;
		    break;
	case 1: m_hActiveSprite = m_hSpriteRun;
		    m_prcActiveRect = m_prcRun;
			break;
	case 2: m_hActiveSprite = m_hSpriteCrouch;
		    m_prcActiveRect = m_prcCrouch;
			break;
	case 3: m_hActiveSprite = m_hSpriteJump;
		    m_prcActiveRect = m_prcJump;
			break;
	}

	int r,g,b, x,y, SWidth, SHeight;

	SWidth = m_prcActiveRect->right - m_prcActiveRect->left;// SPR_Width(m_hActiveSprite,0);
	SHeight = SPR_Height(m_hActiveSprite,0);
    x = ScreenWidth - SWidth - SWidth/2;
	y =	64;

	UnpackRGB(r,g,b, gHUD.uColor);
	SPR_Set(m_hActiveSprite, r, g, b );
    SPR_DrawAdditive(0, x, y, m_prcActiveRect);
	
	//char szMes[20];
    //sprintf(szMes,"%d %d/%d", m_fMode, SWidth, SHeight);
	//gHUD.DrawHudString( 5, 5, ScreenWidth, szMes, r, g, b); 
	return 1;
}