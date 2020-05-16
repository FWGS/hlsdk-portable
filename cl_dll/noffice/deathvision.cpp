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
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "event_api.h"
#include "event_args.h"
#include "triangleapi.h"

void CHudHealth::DrawDeathVision()
{
	if( !m_hDeathVision )
	{
		m_hDeathVision = SPR_Load( "sprites/death_vision.spr" );
		m_hDeathVisionModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hDeathVision );
	}

	HUD_DrawRectangle( m_hDeathVisionModel, kRenderTransAdd );
}
