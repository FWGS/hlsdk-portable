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
#include "r_efx.h"

//LRC - the fogging fog
float g_fFogColor[3];
float g_fStartDist;
float g_fEndDist;
//int g_iFinalStartDist; //for fading
int g_iFinalEndDist;   //for fading
float g_fFadeDuration; //negative = fading out

#define MAX_CLIENTS 32

extern BEAM *pBeam;
extern BEAM *pBeam2;

/// USER-DEFINED SERVER MESSAGE HANDLERS

int CHud :: MsgFunc_ResetHUD(const char *pszName, int iSize, void *pbuf )
{
	ASSERT( iSize == 0 );

	// clear all hud data
	HUDLIST *pList = m_pHudList;

	while ( pList )
	{
		if ( pList->p )
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	//LRC - reset fog
	g_fStartDist = 0;
	g_fEndDist = 0;

	return 1;
}

void CAM_ToFirstPerson(void);

void CHud :: MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf )
{
	CAM_ToFirstPerson();
}

void CHud :: MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:InitHUD");
	//LRC - clear the fog
	g_fStartDist = 0;
	g_fEndDist = 0;

	//LRC - clear all shiny surfaces
	if (m_pShinySurface)
	{
		delete m_pShinySurface;
		m_pShinySurface = NULL;
	}

	m_iSkyMode = SKY_OFF; //LRC

	// prepare all hud data
	HUDLIST *pList = m_pHudList;

	while (pList)
	{
		if ( pList->p )
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
}

//LRC
void CHud :: MsgFunc_SetFog( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:SetFog");
	BEGIN_READ( pbuf, iSize );

	for ( int i = 0; i < 3; i++ )
		 g_fFogColor[ i ] = READ_BYTE();

	g_fFadeDuration = READ_SHORT();
	g_fStartDist = READ_SHORT();

	if (g_fFadeDuration > 0)
	{
//		// fading in
//		g_fStartDist = READ_SHORT();
		g_iFinalEndDist = READ_SHORT();
//		g_fStartDist = FOG_LIMIT;
		g_fEndDist = FOG_LIMIT;
	}
	else if (g_fFadeDuration < 0)
	{
//		// fading out
//		g_iFinalStartDist = 
		g_iFinalEndDist = g_fEndDist = READ_SHORT();
	}
	else
	{
//		g_fStartDist = READ_SHORT();
		g_fEndDist = READ_SHORT();
	}
}

//LRC
void CHud :: MsgFunc_KeyedDLight( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:KeyedDLight");
	BEGIN_READ( pbuf, iSize );

// as-yet unused:
//	float	decay;				// drop this each second
//	float	minlight;			// don't add when contributing less
//	qboolean	dark;			// subtracts light instead of adding (doesn't seem to do anything?)

	int iKey = READ_BYTE();
	dlight_t *dl = gEngfuncs.pEfxAPI->CL_AllocDlight( iKey );

	int bActive = READ_BYTE();
	if (!bActive)
	{
		// die instantly
		dl->die = gEngfuncs.GetClientTime();
	}
	else
	{
		// never die
		dl->die = gEngfuncs.GetClientTime() + 1E6;

		dl->origin[0] = READ_COORD();
		dl->origin[1] = READ_COORD();
		dl->origin[2] = READ_COORD();
		dl->radius = READ_BYTE();
		dl->color.r = READ_BYTE();
		dl->color.g = READ_BYTE();
		dl->color.b = READ_BYTE();
	}
}

//LRC
void CHud :: MsgFunc_AddShine( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:AddShine");
	BEGIN_READ( pbuf, iSize );

	float fScale = READ_BYTE();
	float fAlpha = READ_BYTE()/255.0;
	float fMinX = READ_COORD();
	float fMaxX = READ_COORD();
	float fMinY = READ_COORD();
	float fMaxY = READ_COORD();
	float fZ = READ_COORD();
	char *szSprite = READ_STRING();

//	gEngfuncs.Con_Printf("minx %f, maxx %f, miny %f, maxy %f\n", fMinX, fMaxX, fMinY, fMaxY);

	CShinySurface *pSurface = new CShinySurface(fScale, fAlpha, fMinX, fMaxX, fMinY, fMaxY, fZ, szSprite);
	pSurface->m_pNext = m_pShinySurface;
	m_pShinySurface = pSurface;
}

//LRC
void CHud :: MsgFunc_SetSky( const char *pszName, int iSize, void *pbuf )
{
//	CONPRINT("MSG:SetSky");
	BEGIN_READ( pbuf, iSize );

	m_iSkyMode = READ_BYTE();
	m_vecSkyPos.x = READ_COORD();
	m_vecSkyPos.y = READ_COORD();
	m_vecSkyPos.z = READ_COORD();
}

int CHud :: MsgFunc_GameMode(const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_Teamplay = READ_BYTE();

	return 1;
}


int CHud :: MsgFunc_Damage(const char *pszName, int iSize, void *pbuf )
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;
	
	BEGIN_READ( pbuf, iSize );
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i=0 ; i<3 ; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud :: MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
		this->m_StatusIcons.EnableIcon("dmg_concuss",255,160,0);
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}
