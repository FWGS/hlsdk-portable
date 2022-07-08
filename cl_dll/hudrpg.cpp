/****************************************************************
*																*
*				hudrpg.cpp										*
*																*
*				par Julien										*
*																*
****************************************************************/

// interface rpg sur le hud

//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "eventscripts.h"



//------------------------------------
//
// rafraichissement de l'affichage

int CHudRPG :: Draw	( float flTime )
{
	//--------------------------------
	// menu

	int iTopleftX = ScreenWidth - 144 - 15;
	int iTopleftY = ScreenHeight - 65;

	int iYellow [3] = {255, 255, 0};
	int iRed	[3] = {255,0,0};

	ScaleColors( iYellow[0],	iYellow[1],	iYellow[2],	m_iMenuState & RPG_MENU_ACTIVE ? 200 : 100 );
	ScaleColors( iRed[0],		iRed[1],	iRed[2],	m_iMenuState & RPG_MENU_ACTIVE ? 200 : 100 );

	// roquette

	if ( m_iMenuState & RPG_MENU_ROCKET_EMPTY )
		SPR_Set( m_sprMenu, iRed[0], iRed[1], iRed[2] );
	else
		SPR_Set( m_sprMenu, iYellow[0], iYellow[1], iYellow[2] );

	SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &m_rcRocket );

	if ( m_iMenuState & RPG_MENU_ROCKET_SELECTED )
		SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &m_rcSelect );
	else
		SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &m_rcEmpty );


	// electro

	if ( m_iMenuState & RPG_MENU_ELECTRO_EMPTY )
		SPR_Set( m_sprMenu, iRed[0], iRed[1], iRed[2] );
	else
		SPR_Set( m_sprMenu, iYellow[0], iYellow[1], iYellow[2] );

	SPR_DrawAdditive( 0, iTopleftX + 48, iTopleftY, &m_rcElectro );	

	if ( m_iMenuState & RPG_MENU_ELECTRO_SELECTED )
		SPR_DrawAdditive( 0, iTopleftX + 48, iTopleftY, &m_rcSelect );
	else
		SPR_DrawAdditive( 0, iTopleftX + 48, iTopleftY, &m_rcEmpty );

	// nuclear
	
	if ( m_iMenuState & RPG_MENU_NUCLEAR_EMPTY )
		SPR_Set( m_sprMenu, iRed[0], iRed[1], iRed[2] );
	else
		SPR_Set( m_sprMenu, iYellow[0], iYellow[1], iYellow[2] );

	SPR_DrawAdditive( 0, iTopleftX + 96, iTopleftY, &m_rcNuclear );

	if ( m_iMenuState & RPG_MENU_NUCLEAR_SELECTED )
		SPR_DrawAdditive( 0, iTopleftX + 96, iTopleftY, &m_rcSelect );
	else
		SPR_DrawAdditive( 0, iTopleftX + 96, iTopleftY, &m_rcEmpty );

	gHUD.DrawHudNumberString
		( iTopleftX + 10, iTopleftY - 15, iTopleftX - 100, m_iAmmo1,
			m_iAmmo1 ? iYellow[0] : iRed[0], m_iAmmo1 ? iYellow[1] : iRed[1], m_iAmmo1 ? iYellow[2] : iRed[2] );

	gHUD.DrawHudNumberString
		( iTopleftX + 48 + 10, iTopleftY - 15, iTopleftX - 100, m_iAmmo2,
			m_iAmmo2 ? iYellow[0] : iRed[0], m_iAmmo2 ? iYellow[1] : iRed[1], m_iAmmo2 ? iYellow[2] : iRed[2] );

	gHUD.DrawHudNumberString
		( iTopleftX + 48 + 48 + 10, iTopleftY - 15, iTopleftX - 100, m_iAmmo3,
			m_iAmmo3 ? iYellow[0] : iRed[0], m_iAmmo3 ? iYellow[1] : iRed[1], m_iAmmo3 ? iYellow[2] : iRed[2] );


	//--------------------------------
	// viseur


	iTopleftX = ScreenWidth / 2 - 32;
	iTopleftY = ScreenHeight / 2 - 32;


	SPR_Set( m_sprCrosshair, 255, 255, 255 );

	switch ( m_iViseurState )
	{
	case RPG_CROSSHAIR_NORMAL:
		SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &m_rcCrosshair );
		break;

	case RPG_CROSSHAIR_EMPTY:
		SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &m_rcCrosshair );
		SPR_DrawAdditive( 3, iTopleftX, iTopleftY, &m_rcCrosshair );
		break;

	case RPG_CROSSHAIR_LOCKED:
		SPR_DrawAdditive( 2, iTopleftX, iTopleftY, &m_rcCrosshair );
		SPR_DrawAdditive( 4, iTopleftX, iTopleftY, &m_rcCrosshair );
		break;

	case RPG_CROSSHAIR_PROCESS:
		{
			int iTaux = ( int ) ( 64 * ( m_flSelectTime - flTime ) );
			iTaux = iTaux < 0 ? 0 : iTaux;

			wrect_t rec1 = CreateWrect ( 0, 0, 64, iTaux );
			wrect_t rec2 = CreateWrect ( 0, iTaux, 64, 64 );

			SPR_DrawAdditive( 1, iTopleftX, iTopleftY, &m_rcCrosshair );
			SPR_DrawAdditive( 3, iTopleftX, iTopleftY, &rec1  );
			SPR_DrawAdditive( 4, iTopleftX, iTopleftY + iTaux, &rec2  );
			break;
		}
	}


	//--------------------------------
	// textes

	cl_entity_t *pRpg = GetViewEntity();

	if ( m_iViseurState != RPG_CROSSHAIR_NORMAL )
	{
		iTopleftX = 30;
		iTopleftY = ScreenHeight  * 2/3;

		// premier texte

		wrect_t rec1;

		switch ( m_iViseurState )
		{
		case RPG_CROSSHAIR_EMPTY:
			rec1 = CreateWrect ( 3, 1, 198, 16 );
			pRpg->curstate.skin = 0;
			break;
		case RPG_CROSSHAIR_PROCESS:
			rec1 = CreateWrect ( 3, 17, 198, 32 );
			pRpg->curstate.skin = 1;
			break;
		case RPG_CROSSHAIR_LOCKED:
			rec1 = CreateWrect ( 3, 33, 198, 48 );
			pRpg->curstate.skin = 2;
			break;
		}

		SPR_Set( m_sprText, 255, 255, 255 );
		SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &rec1  );

		// deuxieme texte

		if ( m_iTextState != 0 )
		{
			if ( m_iTextState == RPG_TEXT_TOUCHE )
				rec1 = CreateWrect ( 3, 49, 198, 64 );
			else
				rec1 = CreateWrect ( 3, 65, 198, 78 );

			float flFade = ( m_flTextTime - flTime ) * 255 / 2;

			if ( flFade < 0 )
				m_iTextState = 0;

			else
			{
				int r = 255, g = 255, b = 255;
				ScaleColors(r, g, b, flFade );
				iTopleftY += 18;

				SPR_Set( m_sprText, r, g, b );
				SPR_DrawAdditive( 0, iTopleftX, iTopleftY, &rec1  );
			}
		}
	}
	else
	{
		pRpg->curstate.skin = 0;
	}


	return 1;


}


//------------------------------------
//
// d

DECLARE_MESSAGE(m_RPG, RpgViseur);
DECLARE_MESSAGE(m_RPG, MenuRpg);



//------------------------------------
//
// initialisation au chargement de la dll

int CHudRPG :: Init( void )
{
	m_sprMenu		= SPR_Load("sprites/rpg_menu.spr");
	m_sprCrosshair	= SPR_Load("sprites/rpg_viseur.spr");
	m_sprText		= SPR_Load("sprites/rpg_txt.spr");

	m_rcRocket	= CreateWrect ( 0,	0, 48, 48 );
	m_rcElectro = CreateWrect ( 48,	0, 96, 48 );
	m_rcNuclear = CreateWrect ( 96, 0, 144,48 );
	m_rcSelect	= CreateWrect ( 144,0, 192,48 );
	m_rcEmpty	= CreateWrect ( 192,0, 240,48 );

	m_rcCrosshair = CreateWrect ( 0, 0, 64, 64 );

	HOOK_MESSAGE(RpgViseur);
	HOOK_MESSAGE(MenuRpg);

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudRPG :: VidInit( void )
{
	m_sprMenu	= SPR_Load("sprites/rpg_menu.spr");
	m_sprCrosshair	= SPR_Load("sprites/rpg_viseur.spr");
	m_sprText		= SPR_Load("sprites/rpg_txt.spr");

	m_rcRocket	= CreateWrect ( 0,	0, 48, 48 );
	m_rcElectro = CreateWrect ( 48,	0, 96, 48 );
	m_rcNuclear = CreateWrect ( 96, 0, 144,48 );
	m_rcSelect	= CreateWrect ( 144,0, 192,48 );
	m_rcEmpty	= CreateWrect ( 192,0, 240,48 );

	m_rcCrosshair = CreateWrect ( 0, 0, 64, 64 );

	m_iFlags &= ~HUD_ACTIVE;
	return 1;
}


//------------------------------------
//
// gestion des messages serveur


int CHudRPG::MsgFunc_RpgViseur( const char *pszName, int iSize, void *pbuf )
{

	BEGIN_READ( pbuf, iSize );

	int iRead = READ_BYTE();

	switch ( iRead )
	{
	default:
	case RPG_CROSSHAIR_NORMAL:
	case RPG_CROSSHAIR_EMPTY:
	case RPG_CROSSHAIR_LOCKED:
		m_iViseurState = iRead;
		break;

	case RPG_CROSSHAIR_PROCESS:
		m_iViseurState = iRead;
		m_flSelectTime = gHUD.m_flTime + 1.0;
		break;

	case RPG_TEXT_TOUCHE:
	case RPG_TEXT_MANQUE:
		m_iTextState = iRead;
		m_flTextTime = gHUD.m_flTime + 2.0;
		break;
	}

	return 1;
}

int CHudRPG::MsgFunc_MenuRpg ( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	
	m_iMenuState = READ_BYTE();

	m_iAmmo1 = READ_BYTE();
	m_iAmmo2 = READ_BYTE();
	m_iAmmo3 = READ_BYTE();


	if ( (m_iMenuState & RPG_MENU_CLOSE) && ( m_iFlags & HUD_ACTIVE ) )
		m_iFlags &= ~HUD_ACTIVE;

	else if ( !(m_iMenuState & RPG_MENU_CLOSE) && !( m_iFlags & HUD_ACTIVE ) )
		m_iFlags |= HUD_ACTIVE;

	return 1;
}


//------------------------------------
//
// outil pour le remplissage des structures wrect

wrect_t CreateWrect ( int left, int top, int right, int bottom )
{
	wrect_t rec;
	rec.left = left;
	rec.top = top;
	rec.right = right;
	rec.bottom = bottom;

	return rec;
};
