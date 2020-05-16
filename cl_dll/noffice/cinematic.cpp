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
// cinematic.cpp
//
// implementation of CHudCinematic class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "event_api.h"
#include "event_args.h"
#include "triangleapi.h"

void CHudMessage::DrawCinematic()
{
	if( !m_hSprite )
	{
		m_hSprite = SPR_Load( "sprites/wide_bar.spr" );
		m_hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite );
	}

	HUD_DrawRectangle( m_hSpriteModel, kRenderTransAlpha );
}

