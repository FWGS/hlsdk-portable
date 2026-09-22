/****
*
*  Over Ground (2002-2003) - Mod pour Half-Life - SDK
*
*  Code source de Tigerknee (tigerknee@voila.fr)
*  Plus d'infos sur le site internet du mod :
*  http://og.portailmods.com
*  
****/

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Tbutton, Tbutton )

int CHudTbutton::Init(void)
{
	HOOK_MESSAGE( Tbutton );
	gHUD.AddHudElem(this);

	f_starttime	= 0.0;
	f_endtime	= 0.0;

	return 1;
};

int CHudTbutton::VidInit(void)
{
	return 1;
};

void CHudTbutton::DrawDefaultProgressBar( float var )
{
	int		width, height, x, y;
	int		bwidth, bheight;

	// Les valeurs pour la taille...
	width	= XRES( 340 );
	height	= YRES( 10 );
	bwidth	= XRES( 1 );
	bheight = bwidth;

	// On s'arrange pour le centr� horizontalement
	x		= ( ScreenWidth - width ) / 2;
	y		= ( ScreenHeight - height ) / 2 + YRES( 50 );

	DrawProgressBar( width, height, x, y, bwidth, bheight, var );
}

void CHudTbutton::DrawProgressBar( int width, int height, int x, int y, int bwidth, int bheight, float var )
{
	// Ligne du haut
	FillRGBA( x, y, width, bheight, 222, 222, 222, 150);
	// Ligne de gauche
	FillRGBA( x, y + bheight, bwidth, height - bheight*2, 222, 222, 222, 150 );
	// Ligne de droite
	FillRGBA( x + width - bwidth, y + bheight, bwidth, height - bheight*2, 222, 222, 222, 150 );
	// Ligne du bas
	FillRGBA( x, y + height - bheight, width, bheight, 222, 222, 222, 150 );

	if (var > 0)
	FillRGBA( x + bwidth*2, y + bheight*2, (int)(var * (width-4*bwidth)), height - 4*bheight, 255, 255, 255, 255 );//central
}

int CHudTbutton::Draw(float fTime)
{
	float	currenttime, var;

	if ( gEngfuncs.IsSpectateOnly() != 0 || g_iUser1 || f_endtime <= 0 )
		return 1;

	currenttime = gEngfuncs.GetClientTime();

	// On calcule le pourcentage
	var	= ( currenttime - f_starttime ) / f_endtime;

//	gEngfuncs.Con_Printf( "var: %f\n", var);

	// On dessinne la barre de progression
	DrawDefaultProgressBar( var );

	// On regarde si on doit la faire disparaitre
	if ( ( f_starttime + f_endtime ) <= currenttime )
	{
		// On affiche plus rien l'action est finie
		f_starttime	= 0.0;
		f_endtime	= 0.0;
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}


int CHudTbutton::MsgFunc_Tbutton(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	f_endtime = READ_SHORT();

	f_starttime = gEngfuncs.GetClientTime();

	if ( f_endtime > 0 )
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}
