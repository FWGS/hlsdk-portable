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

//#include "mp3.h" // AJH - Killar MP3
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#define MAX_CLIENTS 32

//extern BEAM *pBeam;
//extern BEAM *pBeam2;

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
	//pBeam = pBeam2 = NULL;
}

int CHud::MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

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

int CHud::MsgFunc_PlayMP3( const char *pszName, int iSize, void *pbuf ) //AJH -Killar MP3
{
	const char *pszSound;
	char cmd[64];

	BEGIN_READ( pbuf, iSize );

	//gMP3.PlayMP3( READ_STRING() );
	pszSound = READ_STRING();

	if( !IsXashFWGS( ))
	{
		sprintf( cmd, "mp3 loop %s\n", pszSound );
		gEngfuncs.pfnClientCmd( cmd );
	}
	else
		gEngfuncs.pfnPrimeMusicStream( pszSound, 1 );

	return 1;
}

int CHud::MsgFunc_PlayBGM( const char *pszName, int iSize, void *pbuf ) //AJH -Killar MP3
{
	const char *pszSound;
	char cmd[64];

	BEGIN_READ( pbuf, iSize );

	if( CVAR_GET_FLOAT( "hud_bgm" ) > 0 )
	{
		//gMP3.PlayMP3( READ_STRING() );
		pszSound = READ_STRING();

		if( !IsXashFWGS( ))
		{
			sprintf( cmd, "mp3 loop %s\n", pszSound );
			gEngfuncs.pfnClientCmd( cmd );
		}
		else
			gEngfuncs.pfnPrimeMusicStream( pszSound, 1 );
	}

	return 1;
}

int CHud::MsgFunc_StopMP3( const char *pszName, int iSize, void *pbuf ) //AJH -Killar MP3
{
	//gMP3.StopMP3();
	if( !IsXashFWGS( ))
		gEngfuncs.pfnClientCmd( "mp3 stop\n" );
	else
		gEngfuncs.pfnPrimeMusicStream( 0, 0 );

	return 1;
}

int CHud::MsgFunc_AddELight( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocElight( READ_SHORT() );

	int bELightActive = READ_BYTE();
	if( !bELightActive )
	{
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		dl->die = gEngfuncs.GetClientTime() + (float)1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_COORD();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
	return 1;
}
