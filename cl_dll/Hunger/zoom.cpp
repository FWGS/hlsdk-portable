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
#include "ammohistory.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Zoom, Zoom)

int CHudZoom::Init(void)
{
	m_fOn = 0;
	m_pWeapon = NULL;

	HOOK_MESSAGE(Zoom);

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

void CHudZoom::Reset(void)
{
	m_fOn = 0;
}

int CHudZoom::VidInit(void)
{
	m_pWeapon = NULL;

	return 1;
};

int CHudZoom::MsgFunc_Zoom(const char *pszName, int iSize, void *pbuf)
{
	int iId;

	BEGIN_READ(pbuf, iSize);
	m_fOn = READ_BYTE();
	iId = READ_BYTE();

	WEAPON* pWeapon = gWR.GetWeapon(iId);
	if (pWeapon)
	{
		m_pWeapon = pWeapon;
	}

	return 1;
}

int CHudZoom::Draw(float flTime)
{
	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return 1;

	if (!m_fOn)
	{
		if (m_pWeapon)
		{
			SetCrosshair(m_pWeapon->hCrosshair, m_pWeapon->rcCrosshair, 255, 0, 0);
		}
	}
	else
	{
		int r, g, b, a;

		r = b = 0;
		g = 128;
		a = 192;

		ScaleColors(r, g, b, a);

		FillRGBA(0, 0, ScreenWidth, ScreenHeight, r, g, b, a);

		if (m_pWeapon)
		{
			SetCrosshair(m_pWeapon->hZoomedCrosshair, m_pWeapon->rcZoomedCrosshair, 255, 0, 0);
		}
	}

	return 1;
}