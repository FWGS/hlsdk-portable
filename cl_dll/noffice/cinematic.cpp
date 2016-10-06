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
// Geiger.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "event_api.h"
#include "event_args.h"
#include "triangleapi.h"

#include "parsemsg.h"


DECLARE_MESSAGE(m_Cinematic, Cinematic)

int CHudCinematic::Init(void)
{
	HOOK_MESSAGE(Cinematic);

	m_iFlags = 0;

	m_hSprite = 0;
	m_flCineTime = 0;

	gHUD.AddHudElem(this);

	return 1;
};

int CHudCinematic::VidInit(void)
{
	m_flCineTime = 0;
	m_iFlags = 0;

	m_hSprite = SPR_Load( "sprites/wide_bar.spr" );

	return 1;
};

int CHudCinematic::MsgFunc_Cinematic(const char *pszName, int iSize, void *pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	m_flCineTime = READ_BYTE();
	m_flCineTime = m_flCineTime + gEngfuncs.GetClientTime();

	return 1;
}

int CHudCinematic::Draw(float flTime)
{
	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return 1;

	if (!(m_iFlags & HUD_ACTIVE))
		return 1;

#if 0
	// Height for a single bar.
	int barHeight = ScreenHeight / 6;

	// Draw upper dark bar.
	gEngfuncs.pfnFillRGBA(0, 0, ScreenWidth, barHeight, 40, 40, 40, 255);

	// Draw lower dark bar.
	gEngfuncs.pfnFillRGBABlend(0, ScreenHeight - barHeight, ScreenWidth, barHeight, 0, 0, 0, 225);
#endif

	return 1;
}

void CHudCinematic::DrawCinematic(void)
{
	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return;

	if (!(m_iFlags & HUD_ACTIVE))
		return;

	if (m_flCineTime <= gEngfuncs.GetClientTime())
	{
		m_iFlags &= ~HUD_ACTIVE;
		m_flCineTime = 0;

		return;
	}

	if (!m_hSprite)
		m_hSprite = SPR_Load("sprites/wide_bar.spr");

	struct model_s * hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer(m_hSprite);

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

		//top left
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3f(0, 0, 0);

		//bottom left
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3f(0, ScreenHeight, 0);

		//bottom right
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, ScreenHeight, 0);

		//top right
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, 0, 0);

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal

	return;
}
