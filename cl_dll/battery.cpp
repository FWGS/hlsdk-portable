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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE( m_Battery, Battery )

int CHudBattery::Init( void )
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE( Battery );

	gHUD.AddHudElem( this );

	// modif de Julien
	m_sprCorps = SPR_Load("sprites/hud_health_body.spr");
	m_wrcCorps = CreateWrect ( 0, 0, 144, 152 );

	for ( int i=0; i<MAX_ARMOR_GROUP; i++ )
	{
		m_flArmor [i] [0] = 0;
		m_flArmor [i] [1] = 0;
		m_flArmor [i] [2] = 0;
	}


	return 1;
}

int CHudBattery::VidInit( void )
{
	int HUD_suit_empty = gHUD.GetSpriteIndex( "suit_empty" );
	int HUD_suit_full = gHUD.GetSpriteIndex( "suit_full" );

	m_hSprite1 = m_hSprite2 = 0;  // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect( HUD_suit_empty );
	m_prc2 = &gHUD.GetSpriteRect( HUD_suit_full );
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;

	// modif de Julien
	m_sprCorps = SPR_Load("sprites/hud_health_body.spr");
	m_wrcCorps = CreateWrect ( 0, 0, 144, 152 );

	for ( int i=0; i<MAX_ARMOR_GROUP; i++ )
	{
		m_flArmor [i] [0] = 0;
		m_flArmor [i] [1] = 0;
		m_flArmor [i] [2] = 0;
	}

	return 1;
}

int CHudBattery::MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	int x = READ_SHORT();

	if( x != m_iBat )
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	// modif de Julien

	float unTiers =		( (float)MAX_MEMBER_ARMOR		/ 3 );
	float deuxTiers =	( (float)MAX_MEMBER_ARMOR * 2	/ 3 );

	for ( int i=0; i<MAX_ARMOR_GROUP; i++ )
	{
		// lecture
		float value = READ_COORD();

		// dommages

		if ( value > m_flArmorvalue[i] )
			m_flPain[i] = ARMOR_PAIN_TIME;

		else if ( value < m_flArmorvalue[i] )
			m_flPain[i] = -ARMOR_PAIN_TIME;

		m_flArmorvalue [i] = value;

		// couleurs

		/* horreur pour simplifier les couleurs :
			blanc	255	255	255
			jaune	255	255	0
			orange	255	128	0
			rouge	255	0	0
		*/

		m_flArmor [i] [0] = (int)( value > unTiers ? 255 : ( value / unTiers ) * ( 255 - 150 ) + 150 );
		m_flArmor [i] [1] = (int)( value > deuxTiers ? 255 : ( value / deuxTiers ) * 255 );
		m_flArmor [i] [2] = (int)( value < deuxTiers ? 0 : ( (value - deuxTiers) / unTiers ) * 255 );

	}

	return 1;
}

int CHudBattery::Draw( float flTime )
{
	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;

	// modif de jUlien
	if ( !(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)))  )
		return 1;

/*
	int r, g, b, x, y, a;
	wrect_t rc;

	rc = *m_prc2;
	rc.top  += m_iHeight * ( (float)( 100 - ( Q_min( 100, m_iBat ) ) ) * 0.01f );	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	UnpackRGB( r, g, b, RGB_YELLOWISH );

	if( !( gHUD.m_iWeaponBits & ( 1 << ( WEAPON_SUIT ) ) ) )
		return 1;

	// Has health changed? Flash the health #
	if( m_fFade )
	{
		if( m_fFade > FADE_TIME )
			m_fFade = FADE_TIME;

		m_fFade -= ( (float)gHUD.m_flTimeDelta * 20.0f );
		if( m_fFade <= 0 )
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim
		a = MIN_ALPHA + ( m_fFade / FADE_TIME ) * 128;
	}
	else
		a = MIN_ALPHA;

	ScaleColors( r, g, b, a );

	int iOffset = ( m_prc1->bottom - m_prc1->top ) / 6;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = ScreenWidth / 5;

	// make sure we have the right sprite handles
	if( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );

	SPR_Set( m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0,  x, y - iOffset, m_prc1 );

	if( rc.bottom > rc.top )
	{
		SPR_Set( m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x, y - iOffset + ( rc.top - m_prc2->top ), &rc );
	}

	x += ( m_prc1->right - m_prc1->left );
	x = gHUD.DrawHudNumber( x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b );
*/


	// modif de Julien
	// affichage du nouveau hud
	
	for ( int i = 0; i < 7; i++ )
	{
		// couleurs pr
		int r = m_flArmor[i][0], g = m_flArmor[i][1], b = m_flArmor[i][2];

		// brillance pour les dommages
		if ( m_flPain[i] > 0 )
		{
			// blanc
			r = r + ( m_flPain[i] / ARMOR_PAIN_TIME ) * ( 255 - r );
			g = g + ( m_flPain[i] / ARMOR_PAIN_TIME ) * ( 255 - g );
			b = b + ( m_flPain[i] / ARMOR_PAIN_TIME ) * ( 255 - b );

			m_flPain[i] = Q_max ( 0, m_flPain[i] - gHUD.m_flTimeDelta );
		}

		if ( m_flPain[i] < 0 )
		{
			// rouge
			r = r + ( -m_flPain[i] / ARMOR_PAIN_TIME ) * ( 255 - r );
			g = g + ( -m_flPain[i] / ARMOR_PAIN_TIME ) * ( 0 - g );
			b = b + ( -m_flPain[i] / ARMOR_PAIN_TIME ) * ( 0 - b );

			m_flPain[i] = Q_min ( 0, m_flPain[i] + gHUD.m_flTimeDelta );
		}

		ScaleColors ( r,g,b, 200 );

		SPR_Set( m_sprCorps, r, g, b );
		SPR_DrawAdditive(i, 0, 0, &m_wrcCorps);
	}

	//-----------------------

	return 1;
}
