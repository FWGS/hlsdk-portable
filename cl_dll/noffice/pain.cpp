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

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Full Screen Damage Indicator. When the player receives damage,
//  the side of the screen will be white. If shot from the right,
//  the right side of the screen will flash white. etc.

void CHudHealth::DrawPainFront()
{
	if( !m_hPainFront )
	{
		m_hPainFront = SPR_Load( "sprites/pain_front.spr" );
		m_hPainFrontModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hPainFront );
	}

	HUD_DrawRectangle( m_hPainFrontModel, kRenderTransAdd );
}

void CHudHealth::DrawPainRight()
{
	if( !m_hPainRight )
	{
		m_hPainRight = SPR_Load( "sprites/pain_right.spr" );
		m_hPainRightModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hPainRight );
	}

	HUD_DrawRectangle( m_hPainRightModel, kRenderTransAdd );
}

void CHudHealth::DrawPainLeft()
{
	if( !m_hPainLeft )
	{
		m_hPainLeft = SPR_Load( "sprites/pain_front.spr" );
		m_hPainLeftModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hPainLeft );
	}

	HUD_DrawRectangle( m_hPainLeftModel, kRenderTransAdd );
}

void CHudHealth::DrawPainRear()
{
	if( !m_hPainRear )
	{
		m_hPainRear = SPR_Load( "sprites/pain_rear.spr" );
		m_hPainRearModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hPainRear );
	}

	HUD_DrawRectangle( m_hPainRearModel, kRenderTransAdd );
}

