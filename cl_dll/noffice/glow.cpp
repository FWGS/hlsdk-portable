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


#include "pm_shared.h"
#include "pm_defs.h"
#include "pmtrace.h"

#include "parsemsg.h"

extern vec3_t v_origin;		// last view origin
extern vec3_t v_angles;		// last view angle
extern vec3_t v_cl_angles;	// last client/mouse angle
extern vec3_t v_sim_org;	// last sim origin

DECLARE_MESSAGE(m_Glow, Glow)

int CHudGlow::Init(void)
{
	HOOK_MESSAGE(Glow);

	m_iFlags = 0;

	gHUD.AddHudElem(this);

	srand((unsigned)time(NULL));

	return 1;
};

int CHudGlow::VidInit(void)
{
	m_iFlags = 0;

	m_hSprite = SPR_Load("sprites/glow01.spr");

	return 1;
};

int CHudGlow::MsgFunc_Glow(const char *pszName, int iSize, void *pbuf)
{
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

int CHudGlow::Draw(float flTime)
{
	return 1;
}

#define GLOW_TRACE_DISTANCE	80.0f
#define GLOW_MAX_ALPHA		1.0f

void CHudGlow::DrawGlow(void)
{
	if (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL)
		return;

	if (!(m_iFlags & HUD_ACTIVE))
		return;

	cl_entity_t* ent = gEngfuncs.GetViewModel();
	if (!ent)
		return;

	float r, g, b, a;
	vec3_t origin, forward, right, up, screen, point;
	struct model_s *hSpriteModel;
	int size = 10;

	r = 1.0f; g = b = 0.95f;

	VectorCopy(ent->attachment[0], origin);
	AngleVectors(v_angles, forward, right, up);

	vec3_t tracedir;
	VectorScale(forward, GLOW_TRACE_DISTANCE, tracedir);
	VectorAdd(tracedir, origin, tracedir);
	pmtrace_t * trace = gEngfuncs.PM_TraceLine(origin, tracedir, PM_TRACELINE_PHYSENTSONLY, 2, -1);

	if (trace->fraction != 1.0)
	{
		a = ((trace->fraction * GLOW_MAX_ALPHA) / GLOW_TRACE_DISTANCE) * 100;
	}
	else
	{
		a = GLOW_MAX_ALPHA;
	}

	a = Q_min(a, GLOW_MAX_ALPHA);

	// gEngfuncs.Con_Printf("Glow opacity: %f.\n", a);

	gEngfuncs.pTriAPI->WorldToScreen(origin, point);

	hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer(m_hSprite);

	gEngfuncs.pTriAPI->SpriteTexture(hSpriteModel, 0);
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Color4f( r, g, b, a );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		VectorMA(origin, size, up, point);
		VectorMA(point, size, right, point);
		gEngfuncs.pTriAPI->Vertex3fv(point);

		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		VectorMA(origin, size, up, point);
		VectorMA(point, -size, right, point);
		gEngfuncs.pTriAPI->Vertex3fv(point);

		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		VectorMA(origin, -size, up, point);
		VectorMA(point, -size, right, point);
		gEngfuncs.pTriAPI->Vertex3fv(point);

		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		VectorMA(origin, -size, up, point);
		VectorMA(point, size, right, point);
		gEngfuncs.pTriAPI->Vertex3fv(point);

	gEngfuncs.pTriAPI->End();

	return;
}
