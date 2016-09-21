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

#define SCOPE_SMALL		0
#define SCOPE_MEDIUM	1
#define SCOPE_LARGE		2

static int HUD_IsWidescreen(int iWidth, int iHeight)
{
	float ratio = iWidth / iHeight;

	return (ratio >= 1.77) ? 1 : 0;
}

DECLARE_MESSAGE(m_Scope, Scope)

int CHudScope::Init(void)
{
	HOOK_MESSAGE(Scope);

	// Make inactive.
	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);

	return 1;
};

void CHudScope::Reset(void)
{
	// Make inactive.
	m_iFlags &= ~HUD_ACTIVE;
}

int CHudScope::VidInit(void)
{
	for (int i = 0; i < SCOPE_HSPRITE_COUNT; i++)
		m_scopes[i] = 0;

	m_ScopeSize = 0;

	int sw = ScreenWidth, sh = ScreenHeight;

	//
	// Determine the ideal scope size to use.
	//

	// Widescreen support. 1:77
	if (HUD_IsWidescreen(sw, sh))
	{
		if (sh > 576)
		{
			if (sh > 800)
			{
				m_ScopeSize = SCOPE_LARGE;
			}
			else
			{
				m_ScopeSize = SCOPE_MEDIUM;
			}
		}
		else
		{
			m_ScopeSize = SCOPE_SMALL;
		}
	}
	// Normal display. 1:33
	else
	{
		if (sh > 600)
		{
			if (sh > 768)
			{
				m_ScopeSize = SCOPE_LARGE;
			}
			else
			{
				m_ScopeSize = SCOPE_MEDIUM;
			}
		}
		else
		{
			m_ScopeSize = SCOPE_SMALL;
		}
	}

	// Make inactive.
	m_iFlags &= ~HUD_ACTIVE;

	return 1;
};

int CHudScope::Draw(float flTime)
{
	if (!(m_iFlags & HUD_ACTIVE))
		return 1;

	int x, y, w, h;

	x = y = 0;

	if (!m_scopes[0])
	{
		switch (m_ScopeSize)
		{
		default:
		case SCOPE_SMALL:
			m_scopes[0]				= LoadSprite("sprites/scope/0.spr");
			break;

		case SCOPE_MEDIUM:
			m_scopes[SCOPE_U_L]		= LoadSprite("sprites/scope/800/U_L.spr");
			m_scopes[SCOPE_U_M_L]	= LoadSprite("sprites/scope/800/U_M_L.spr");
			m_scopes[SCOPE_U_M_R]	= LoadSprite("sprites/scope/800/U_M_R.spr");
			m_scopes[SCOPE_U_R]		= LoadSprite("sprites/scope/800/U_R.spr");
			m_scopes[SCOPE_M_L]		= LoadSprite("sprites/scope/800/M_L.spr");
			m_scopes[SCOPE_M_R]		= LoadSprite("sprites/scope/800/M_R.spr");
			m_scopes[SCOPE_L_L]		= LoadSprite("sprites/scope/800/L_L.spr");
			m_scopes[SCOPE_L_M_L]	= LoadSprite("sprites/scope/800/L_M_L.spr");
			m_scopes[SCOPE_L_M_R]	= LoadSprite("sprites/scope/800/L_M_R.spr");
			m_scopes[SCOPE_L_R]		= LoadSprite("sprites/scope/800/L_R.spr");
			break;

		case SCOPE_LARGE:
			m_scopes[SCOPE_U_L]		= LoadSprite("sprites/scope/1024/U_L.spr");
			m_scopes[SCOPE_U_M_L]	= LoadSprite("sprites/scope/1024/U_M_L.spr");
			m_scopes[SCOPE_U_M_R]	= LoadSprite("sprites/scope/1024/U_M_R.spr");
			m_scopes[SCOPE_U_R]		= LoadSprite("sprites/scope/1024/U_R.spr");
			m_scopes[SCOPE_M_L]		= LoadSprite("sprites/scope/1024/M_L.spr");
			m_scopes[SCOPE_M_R]		= LoadSprite("sprites/scope/1024/M_R.spr");
			m_scopes[SCOPE_L_L]		= LoadSprite("sprites/scope/1024/L_L.spr");
			m_scopes[SCOPE_L_M_L]	= LoadSprite("sprites/scope/1024/L_M_L.spr");
			m_scopes[SCOPE_L_M_R]	= LoadSprite("sprites/scope/1024/L_M_R.spr");
			m_scopes[SCOPE_L_R]		= LoadSprite("sprites/scope/1024/L_R.spr");
			break;
		}
	}

	w = SPR_Width(m_scopes[0], 0);
	h = SPR_Height(m_scopes[0], 0);

	int r, g, b, a;
	UnpackRGB(r, g, b, RGB_YELLOWISH);

	switch (m_ScopeSize)
	{
	default:

		//
		// Small scope.
		//
	case SCOPE_SMALL:
		//
		// This uses one sprite.
		//

		x = ScreenWidth / 2 - w;
		y = ScreenHeight / 2 - h;

		//-----------------------
		// Top left scope.
		//-----------------------
		SPR_Set(m_scopes[0], r, g, b);
		SPR_DrawHoles(0, x, y, NULL);

		//-----------------------
		// Top right scope.
		//-----------------------
		SPR_Set(m_scopes[0], r, g, b);
		SPR_DrawHoles(1, x + w, y, NULL);

		//-----------------------
		// Bottom right scope.
		//-----------------------
		SPR_Set(m_scopes[0], r, g, b);
		SPR_DrawHoles(2, x + w, y + h, NULL);

		//-----------------------
		// Bottom left scope.
		//-----------------------
		SPR_Set(m_scopes[0], r, g, b);
		SPR_DrawHoles(3, x, y + h, NULL);
		break;

		//
		// Medium & Large scopes.
		//
	case SCOPE_MEDIUM:
	case SCOPE_LARGE:
		x = ScreenWidth / 2 - w * 2;
		y = ScreenHeight / 2 - (h * 3) / 2;

		//-----------------------
		// Upper part.
		//-----------------------
		SPR_Set(m_scopes[SCOPE_U_L], r, g, b);
		SPR_DrawHoles(0, x, y, NULL);

		SPR_Set(m_scopes[SCOPE_U_M_L], r, g, b);
		SPR_DrawHoles(0, x + w, y, NULL);

		SPR_Set(m_scopes[SCOPE_U_M_R], r, g, b);
		SPR_DrawHoles(0, x + w * 2, y, NULL);

		SPR_Set(m_scopes[SCOPE_U_R], r, g, b);
		SPR_DrawHoles(0, x + w * 3, y, NULL);

		//-----------------------
		// Middle part.
		//-----------------------
		SPR_Set(m_scopes[SCOPE_M_L], r, g, b);
		SPR_DrawHoles(0, x, y + h, NULL);

		SPR_Set(m_scopes[SCOPE_M_R], r, g, b);
		SPR_DrawHoles(0, x + w * 3, y + h, NULL);

		//-----------------------
		// Lower part.
		//-----------------------
		SPR_Set(m_scopes[SCOPE_L_L], r, g, b);
		SPR_DrawHoles(0, x, y + h * 2, NULL);

		SPR_Set(m_scopes[SCOPE_L_M_L], r, g, b);
		SPR_DrawHoles(0, x + w, y + h * 2, NULL);

		SPR_Set(m_scopes[SCOPE_L_M_R], r, g, b);
		SPR_DrawHoles(0, x + w * 2, y + h * 2, NULL);

		SPR_Set(m_scopes[SCOPE_L_R], r, g, b);
		SPR_DrawHoles(0, x + w * 3, y + h * 2, NULL);
		break;
	}

	r = g = b = 0;
	a = 255;

	// Draw left bar.
	gEngfuncs.pfnFillRGBABlend(0, 0, x + 1, ScreenHeight, r, g, b, a);

	// Draw right bar.
	gEngfuncs.pfnFillRGBABlend(ScreenWidth - x - 1, 0, ScreenWidth, ScreenHeight, r, g, b, a);

	// Draw top bar.
	gEngfuncs.pfnFillRGBABlend(0, 0, ScreenWidth, y + 1, r, g, b, a);

	// Draw bottom bar.
	gEngfuncs.pfnFillRGBABlend(0, ScreenHeight - y - 1, ScreenWidth, ScreenHeight, r, g, b, a);
	
	return 1;
}

int CHudScope::MsgFunc_Scope(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ(pbuf, iSize);
	int fOn = READ_BYTE();

	if (fOn)
	{
		// Make active.
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		// Make inactive.
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}