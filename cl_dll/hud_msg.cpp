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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
//#include "r_efx.h"

#define MAX_TELES 256
Vector g_vecTeleMins[MAX_TELES];
Vector g_vecTeleMaxs[MAX_TELES];
int g_iTeleNum;
bool g_bLoadedTeles;
        
float g_iFogColor[3];
float g_iStartDist;
float g_iEndDist;

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

void CHud::MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
	g_iTeleNum = 0;
	g_bLoadedTeles = false;
	int i;

	//Clear all the teleporters
	for( i = 0; i < MAX_TELES; i++ )
	{
		g_vecTeleMins[i].x = 0.0;
		g_vecTeleMins[i].y = 0.0;
		g_vecTeleMins[i].z = 0.0;

		g_vecTeleMaxs[i].x = 0.0;
		g_vecTeleMaxs[i].y = 0.0;
		g_vecTeleMaxs[i].z = 0.0;
	}
	/***** FOG CLEARING JIBBA JABBA *****/
	for( i = 0; i < 3; i++ )
		g_iFogColor[i] = 0.0;

	g_iStartDist = 0.0f;
	g_iEndDist = 0.0f;
	/***** FOG CLEARING JIBBA JABBA *****/

	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while( pList )
	{
		if( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	BEGIN_READ( pbuf, iSize );
	g_iTeleNum = READ_BYTE();

	for( i = 0; i < g_iTeleNum; i++ )
	{
		g_vecTeleMins[i].x = READ_COORD();
		g_vecTeleMins[i].y = READ_COORD();
		g_vecTeleMins[i].z = READ_COORD();
		g_vecTeleMaxs[i].x = READ_COORD();
		g_vecTeleMaxs[i].y = READ_COORD();
		g_vecTeleMaxs[i].z = READ_COORD();
	}

	for( i = 0; i < 3; i++ )
		g_iFogColor[i] = READ_SHORT(); // Should just get a byte.

	//If they both are 0, it means no fog for this level.
	g_iStartDist = READ_SHORT();
	g_iEndDist = READ_SHORT();
}

int CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	if( m_Teamplay )
		ClientCmd( "richpresence_gamemode Teamplay\n" );
	else
		ClientCmd( "richpresence_gamemode\n" );
	ClientCmd( "richpresence_update\n" );
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

// QUAKECLASSIC
int CHud::MsgFunc_QItems( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iQuakeItems = READ_LONG();

	return 1;
}
