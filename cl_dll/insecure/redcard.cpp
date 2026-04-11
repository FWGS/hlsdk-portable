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

DECLARE_MESSAGE(m_Redcard, HudRedcard);

int CHudRedcard::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(HudRedcard);

	InitHUDData();

	return 1;
}

void CHudRedcard::InitHUDData(void)
{
	m_iFlags |= HUD_ACTIVE;
}

int CHudRedcard::VidInit(void)
{
	m_HUD_sec_card_red = gHUD.GetSpriteIndex("sec_card_red");

	m_hSprite = gHUD.GetSprite(m_HUD_sec_card_red);
	return 1;
}

int CHudRedcard::MsgFunc_HudRedcard(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int x = READ_BYTE();
	return 1;
}

int CHudRedcard::Draw(float flTime)
{
    int width = gHUD.GetSpriteRect(m_HUD_sec_card_red).right - gHUD.GetSpriteRect(m_HUD_sec_card_red).left;
    int height = gHUD.GetSpriteRect(m_HUD_sec_card_red).bottom - gHUD.GetSpriteRect(m_HUD_sec_card_red).top;
    int x, y;

    if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL || !gHUD.HasRedcard())
        return 1;

    int r, g, b;
    UnpackRGB(r, g, b, RGB_REDISH);

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

    y = GetRedcardPosition();

    SPR_DrawAdditive(0, x, y, NULL);

    return 1;
}

int CHudRedcard::GetRedcardPosition()
{
    int height = gHUD.GetSpriteRect(m_HUD_sec_card_red).bottom - gHUD.GetSpriteRect(m_HUD_sec_card_red).top;

    if (!gHUD.HasFlashlight() && !gHUD.HasC4())
    {
        // If we don't have a flashlight, draw in its place.
        return height / 2;
    }
    else if (gHUD.HasFlashlight() && gHUD.HasC4())
    {
        // If we have a flashlight AND C4
        // move a bit below it.
        return GetScreenPosition(88, 176, 264);
    }
    else
    {
        return GetScreenPosition(56, 112, 168);
    }
}

int CHudRedcard::GetScreenPosition(int small, int medium, int large)
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