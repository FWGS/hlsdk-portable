/****************************************************************
*																*
*				clientfog.cpp									*
*																*
*				par Julien										*
*																*
****************************************************************/

// code de la partie client de l'effet de brouillard


//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "triangleapi.h"

#define	FOG_DISTANCE_INFINIE		4096


//------------------------------------
//
// d
// gmsgFog

DECLARE_MESSAGE(m_Fog, Fog );


//------------------------------------
//
// gestion des messages serveur


int CHudFog::MsgFunc_Fog( const char *pszName, int iSize, void *pbuf )
{

	BEGIN_READ( pbuf, iSize );

	int active	= READ_BYTE();
	maxfadetime	= READ_COORD();

	if ( maxfadetime > 0 )
		Fade = 1;

	bActive = active;
	fadetime = 0;

	mindist		= READ_COORD();
	maxdist		= READ_COORD();

	fogcolor.x	= READ_COORD();
	fogcolor.y	= READ_COORD();
	fogcolor.z	= READ_COORD();

	m_iFlags |= HUD_ACTIVE;
	return 1;
}


//------------------------------------
//
// rafraichissement de l'affichage


int CHudFog :: Draw	( float flTime )
{
	if ( Fade == 1 )
	{
		fadetime = Q_min ( maxfadetime, fadetime + gHUD.m_flTimeDelta );

		if ( fadetime >= maxfadetime )
			Fade = 0;
	}

	return 1;
}


void CHudFog :: DrawFog ( void )
{
	if ( Fade == 0 && m_iFlags & HUD_ACTIVE)
	{
		if ( bActive == 0 )
		{
			gEngfuncs.pTriAPI->Fog ( fogcolor, mindist, maxdist, 0 );
			m_iFlags &= ~HUD_ACTIVE;
		}
		else
		{
			gEngfuncs.pTriAPI->Fog ( fogcolor, mindist, maxdist, 1 );
		}
		return;
	}

	else if ( !(m_iFlags & HUD_ACTIVE) )
		return;

	float fldist;

	if ( Fade == 1 && bActive == 1 )
		fldist = FOG_DISTANCE_INFINIE * ( maxfadetime - fadetime ) / maxfadetime;

	else if ( Fade == 1 && bActive == 0 )
		fldist = FOG_DISTANCE_INFINIE * fadetime / maxfadetime;

	gEngfuncs.pTriAPI->Fog ( fogcolor, mindist + fldist, maxdist + fldist, 1 );
}



//------------------------------------
//
// initialisation au chargement de la dll

int CHudFog :: Init( void )
{
	bActive = 0;
	Fade = 0;

	HOOK_MESSAGE( Fog );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudFog :: VidInit( void )
{
	bActive = 0;
	Fade = 0;

	HOOK_MESSAGE( Fog );

	m_iFlags |= HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


