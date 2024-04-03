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
/***
*
*	(C) 2008 Vyacheslav Dzhura
*
****/
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;
extern TEMPENTITY *pFlare;	// Vit_amiN

extern float g_lastFOV;			// Vit_amiN

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud::MsgFunc_ResetHUD( const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	// Vit_amiN: reset the FOV
	m_iFOV = 0;	// default_fov
	g_lastFOV = 0.0f;

	return 1;
}

void CAM_ToFirstPerson( void );

void CHud::MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL;	// Vit_amiN: clear egon's beam flare
}

void CHud::MsgFunc_Camera( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iCamMode = READ_BYTE();
	m_vecCamPos.x = READ_COORD();
	m_vecCamPos.y = READ_COORD();
	m_vecCamPos.z = READ_COORD();

	gHUD.DrawHudString( 100, 100, ScreenWidth, "Camera message received!", 255, 180, 0 );
}

int CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	gEngfuncs.pfnConsolePrint("--- MsgFunc_GameMode ---\n");

	m_Teamplay = 0; //READ_BYTE();

	BEGIN_READ( pbuf, iSize );
	int bMode = READ_BYTE();

	if ( bMode == 2 )
	{
		m_bAlienMode = true;
		m_Health.m_iFlags |= HUD_ALIEN;
		//m_TextMessage.m_iFlags |= HUD_ALIEN;
		gEngfuncs.pfnConsolePrint("--- AlIEN SLAVE MODE ---\n");
	}
	else
		if ( bMode == 3 )
		{
			m_bAlienMode = false;
			m_Health.m_iFlags &= ~HUD_ALIEN;
			//m_TextMessage.m_iFlags &= ~HUD_ALIEN;
			gEngfuncs.pfnConsolePrint("--- NORMAL MODE ---\n");
		}

	return 1;
}

int CHud::MsgFunc_DecayName( const char *spzName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int iPlayerDecayId;

	iPlayerDecayId = READ_BYTE();

	char name[32];
	sprintf( name, "Spare player" );

	if ( iPlayerDecayId == 1 )
	{
		if ( m_bAlienMode )
			sprintf( name, "X-8973" );
		else
			sprintf( name, "Gina" );
	}
	if ( iPlayerDecayId == 2 )
	{
		if ( m_bAlienMode )
			sprintf( name, "R-4913" );
		else
			sprintf( name, "Colette" );
	}

	char buf[256];
	sprintf(buf, "name \"%s\"\n", name);
	gEngfuncs.pfnClientCmd(buf);

	return 1;
}

int CHud::MsgFunc_Damage( const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;

	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for( i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = ( blood * 0.5 ) + ( armor * 0.5 );

	if( count < 10 )
		count = 10;

	// TODO: kick viewangles,  show damage visually
	return 1;
}

int CHud::MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	int r, g, b;
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if( m_iConcussionEffect )
	{
		UnpackRGB( r, g, b, RGB_YELLOWISH );	// Vit_amiN: fixed
		this->m_StatusIcons.EnableIcon( "dmg_concuss", r, g, b );
	}
	else
		this->m_StatusIcons.DisableIcon( "dmg_concuss" );
	return 1;
}
