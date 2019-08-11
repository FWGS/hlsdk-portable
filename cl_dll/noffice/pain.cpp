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
#include "com_model.h"
#include "studio_util.h"

#include "parsemsg.h"

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Full Screen Damage Indicator. When the player receives damage,
//  the side of the screen will be white. If shot from the right,
//  the right side of the screen will flash white. etc.

void CHudHealth::DrawPain2(void)
{
	if (!(m_fAttackFront || m_fAttackRear || m_fAttackLeft || m_fAttackRight))
		return;

	int a, shade;

	// TODO:  get the shift value of the health
	a = 255;	// max brightness until then

	float fFade = gHUD.m_flTimeDelta * 2;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd); //additive
	gEngfuncs.pTriAPI->CullFace(TRI_NONE); //no culling

	// SPR_Draw top
	if (m_fAttackFront > 0.4)
	{
		if (!m_hPainFront)
			m_hPainFront = SPR_Load("sprites/pain_front.spr");

		shade = a * Q_max(m_fAttackFront, 0.5);

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *) gEngfuncs.GetSpritePointer(m_hPainFront), 0);
		gEngfuncs.pTriAPI->Color4f(1, 1, 1, shade);
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

		gEngfuncs.pTriAPI->End();

		m_fAttackFront = Q_max(0, m_fAttackFront - fFade);
	}
	else
		m_fAttackFront = 0;

	if (m_fAttackRight > 0.4)
	{
		if (!m_hPainRight)
			m_hPainRight = SPR_Load("sprites/pain_right.spr");

		shade = a * Q_max(m_fAttackRight, 0.5);

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *) gEngfuncs.GetSpritePointer(m_hPainRight), 0);
		gEngfuncs.pTriAPI->Color4f(1, 1, 1, shade);
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

		gEngfuncs.pTriAPI->End();

		m_fAttackRight = Q_max(0, m_fAttackRight - fFade);
	}
	else
		m_fAttackRight = 0;

	if (m_fAttackRear > 0.4)
	{
		if (!m_hPainRear)
			m_hPainRear = SPR_Load("sprites/pain_rear.spr");

		shade = a * Q_max(m_fAttackRear, 0.5);

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *) gEngfuncs.GetSpritePointer(m_hPainRear), 0);
		gEngfuncs.pTriAPI->Color4f(1, 1, 1, shade);
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

		gEngfuncs.pTriAPI->End();

		m_fAttackRear = Q_max(0, m_fAttackRear - fFade);
	}
	else
		m_fAttackRear = 0;

	if (m_fAttackLeft > 0.4)
	{
		if (!m_hPainLeft)
			m_hPainLeft = SPR_Load("sprites/pain_left.spr");

		shade = a * Q_max(m_fAttackLeft, 0.5);

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *) gEngfuncs.GetSpritePointer(m_hPainLeft), 0);
		gEngfuncs.pTriAPI->Color4f(1, 1, 1, shade);
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

		gEngfuncs.pTriAPI->End();

		m_fAttackLeft = Q_max(0, m_fAttackLeft - fFade);
	}
	else
		m_fAttackLeft = 0;

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal
}
