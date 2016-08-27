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

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>

#include "triangleapi.h"
#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"

extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle
extern vec3_t v_cl_angles;	// last client/mouse angle
extern vec3_t v_sim_org;	// last sim origin

DECLARE_MESSAGE(m_Scope, Scope)

int CHudScope::Init(void)
{
	HOOK_MESSAGE(Scope);

	m_iFlags = 0;

	gHUD.AddHudElem(this);
	return 1;
}

void CHudScope::Reset(void)
{

}

int CHudScope::VidInit(void)
{
	m_hSprite = SPR_Load("sprites/scopeborder.spr");

	return 1;
}

int CHudScope::MsgFunc_Scope(const char *pszName, int iSize, void *pbuf)
{
	// TODO: update local health data
	BEGIN_READ(pbuf, iSize);
	int fOn = READ_BYTE();

	if (fOn)
	{
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}


int CHudScope::Draw(float flTime)
{
	return 1;
}

int CHudScope::DrawScope(void)
{
	if (!(m_iFlags & HUD_ACTIVE))
		return 1;

	if (!m_hSprite)
		m_hSprite = SPR_Load("sprites/scopeborder.spr");

	int halfScopeHeight = ScreenHeight / 2;
	int halfScopeWidth = halfScopeHeight;
	int x, y;

	struct model_s* hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer(m_hSprite);

	gEngfuncs.pTriAPI->RenderMode(kRenderTransTexture); //additive


	//
	// Top left Half scope.
	//

	x = ScreenWidth / 2 - halfScopeWidth;
	y = 0;

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 0);
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.3f, 0.3f); // 0 0
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.3f, 1.0f); // 0 1
	gEngfuncs.pTriAPI->Vertex3f(x, y + halfScopeHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f); // 1 1
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y + halfScopeHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.3f); // 1 0
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y, 0);

	gEngfuncs.pTriAPI->End();


	//
	// Top right Half scope.
	//

	x = ScreenWidth / 2;
	y = 0;

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 1);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.3f);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(x, y + halfScopeHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(0.7f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y + halfScopeHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(0.7f, 0.3f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y, 0);

	gEngfuncs.pTriAPI->End();

	//
	// Bottom left Half scope.
	//

	x = ScreenWidth / 2 - halfScopeWidth;
	y = ScreenHeight / 2;

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 2);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.3f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.3f, 0.7f);
	gEngfuncs.pTriAPI->Vertex3f(x, y + halfScopeHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.7f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y + halfScopeHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y, 0);

	gEngfuncs.pTriAPI->End();


	//
	// Bottom right Half scope.
	//


	x = ScreenWidth / 2;
	y = ScreenHeight / 2;

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 3);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.7f);
	gEngfuncs.pTriAPI->Vertex3f(x, y + halfScopeHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(0.7f, 0.7f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y + halfScopeHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(0.7f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(x + halfScopeWidth, y, 0);

	gEngfuncs.pTriAPI->End();





	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); // normal
	gEngfuncs.pTriAPI->Color4f(1.0, 0.0, 0.0, 1.0);

	int w = ScreenWidth / 2 - halfScopeWidth + 5;

	//
	// Left black bar
	//

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 0);

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(0, 0, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(0, ScreenHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(w, ScreenHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(w, 0, 0);

	gEngfuncs.pTriAPI->End();


	x = ScreenWidth / 2 + halfScopeWidth - 5;

	//
	// Right black bar
	//

	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	gEngfuncs.pTriAPI->Color4f(0.0, 1.0, 0.0, 1.0);

	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(x, 0, 0);

	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(x, ScreenHeight, 0);

	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
	gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, ScreenHeight, 0);

	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
	gEngfuncs.pTriAPI->Vertex3f(ScreenWidth, 0, 0);

	gEngfuncs.pTriAPI->End();



	gEngfuncs.pTriAPI->RenderMode(kRenderNormal); //return to normal


	vec3_t forward;
	AngleVectors(v_angles, forward, NULL, NULL);
	VectorScale(forward, 1024, forward);
	VectorAdd(forward, v_origin, forward);
	pmtrace_t * trace = gEngfuncs.PM_TraceLine(v_origin, forward, PM_TRACELINE_PHYSENTSONLY, 2, -1);

	float dist = (trace->endpos - v_origin).Length();
	float meters = dist * 0.021527f; // Convert hammer units to meters.


	char szDistance[16];
	sprintf(szDistance, "%.2f m", meters);

	int len = strlen(szDistance);

	int x1, y1;
	x = ScreenWidth / 2 + ((float)halfScopeWidth * 0.7f);
	y = ScreenHeight / 2 - ((float)halfScopeHeight * 0.0625f);

	x1 = x + 1;
	y1 = y + 1;

	char c;
	int i;
	int r, g, b;

	r = g = b = 15;

	for (i = 0; i < len; i++)
	{
		TextMessageDrawChar(x1, y1, szDistance[i], r, g, b);

		c = szDistance[i];
		x1 += gHUD.m_scrinfo.charWidths[c];
	}

	r = 255;
	g = b = 0;

	for (i = 0; i < len; i++)
	{
		TextMessageDrawChar(x, y, szDistance[i], r, g, b);

		c = szDistance[i];
		x += gHUD.m_scrinfo.charWidths[c];
	}

	// gEngfuncs.Con_Printf("CMLWBR trace distance: %.3f\n", meters);

	return 1;
}