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

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Keycard, HudKeycard);

int CHudKeycard::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(HudKeycard);

	InitHUDData();

	return 1;
}

void CHudKeycard::InitHUDData(void)
{
	m_iFlags |= HUD_ACTIVE;
}

int CHudKeycard::VidInit(void)
{
	m_HUD_sec_card = gHUD.GetSpriteIndex("sec_card");

	m_hSprite = gHUD.GetSprite(m_HUD_sec_card);
	return 1;
}

int CHudKeycard::MsgFunc_HudKeycard(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_BYTE();
	return 1;
}

int CHudKeycard::Draw(float flTime)
{
    int width = gHUD.GetSpriteRect(m_HUD_sec_card).right - gHUD.GetSpriteRect(m_HUD_sec_card).left;
    int height = gHUD.GetSpriteRect(m_HUD_sec_card).bottom - gHUD.GetSpriteRect(m_HUD_sec_card).top;
    int x, y;

    if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL || !gHUD.HasKeycard())
        return 0;

    int r, g, b;
    UnpackRGB(r, g, b, RGB_YELLOWISH);

    // Top right corner of the screen.
    if (ScreenWidth > 2560)
    {
        x = ScreenWidth - width - width / 2;
        y = height / 10;
    }
    else
    {
        x = ScreenWidth - width - width / 10;
        y = height / 2;
    }

    SPR_Set(m_hSprite, r, g, b);

	// Check if we bring something along.
    y = GetKeycardPosition();

    SPR_DrawAdditive(0, x, y, NULL);

    return 1;
}

int CHudKeycard::GetKeycardPosition()
{
    int height = gHUD.GetSpriteRect(m_HUD_sec_card).bottom - gHUD.GetSpriteRect(m_HUD_sec_card).top;

    if (!gHUD.HasFlashlight() && !gHUD.HasRedcard() && !gHUD.HasC4())
    {
		// If we don't have the flashlight, nor the red keycard
		// nor the C4, place it at the top.
        return height / 2;
    }
    else if (gHUD.HasFlashlight() && gHUD.HasRedcard() && gHUD.HasC4())
    {
        // If we have all the items, move the element 32 pixels in 640 
        // further down with each item -- also scale with different resolutions.
        return GetScreenPosition(120, 240, 360);
    }
    else if (gHUD.HasFlashlight() && gHUD.HasRedcard())
    {
		// If we only have the flashlight and red keycard.
        return GetScreenPosition(88, 176, 264);
    }
    else if (gHUD.HasFlashlight() && gHUD.HasC4())
    {
        // If we only have the flashlight and C4.
        return GetScreenPosition(88, 176, 264);
    }
    else if (!gHUD.HasFlashlight() && gHUD.HasRedcard() && gHUD.HasC4())
    {
		// If we only have the red keycard and C4.
        return GetScreenPosition(88, 176, 264);
    }
    else
    {
		// If we only have the flashlight.
        return GetScreenPosition(56, 112, 168);
    }
}

int CHudKeycard::GetScreenPosition(int small, int medium, int large)
{
    if (ScreenWidth < 640)
        return small;
    else if (ScreenWidth < 1280)
        return small;
    else if (ScreenWidth <= 2560)
        return medium;
    else
        return large;
}