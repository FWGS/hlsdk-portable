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

#include "parsemsg.h"

void CHudMessage::DrawCinematic()
{
	if( !m_hSprite )
	{
		m_hSprite = SPR_Load( "sprites/wide_bar.spr" );
		m_hSpriteModel = (struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite );
	}

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );
	gEngfuncs.pTriAPI->SpriteTexture( m_hSpriteModel, 0 );
	gEngfuncs.pTriAPI->Color4f( 1.0f, 1.0f, 1.0f, 1.0f );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );

		// top right
		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 1.0f );
		gEngfuncs.pTriAPI->Vertex3f( 0, 0, 0 );

		// top left
		gEngfuncs.pTriAPI->TexCoord2f( 0.0f, 0.0f );
		gEngfuncs.pTriAPI->Vertex3f( 0, ScreenHeight, 0 );

		// bottom left
		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 0.0f );
		gEngfuncs.pTriAPI->Vertex3f( ScreenWidth, ScreenHeight, 0 );

		// bottom right
		gEngfuncs.pTriAPI->TexCoord2f( 1.0f, 1.0f );
		gEngfuncs.pTriAPI->Vertex3f( ScreenWidth, 0, 0 );

	gEngfuncs.pTriAPI->End(); //end our list of vertexes
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal ); //return to normal
}

