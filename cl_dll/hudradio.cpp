//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					hudradio.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code serveur de la radio							---
//---------------------------------------------------------
//---------------------------------------------------------



//---------------------------------------------------------
//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "cl_entity.h"
#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif


#define	GLASSES		0
#define EINSTEIN	1
#define LUTHER		2
#define SLICK		3
#define BARNEY		4
#define GMAN		5
#define VOCAL		6
#define INFOCOMBI	7
#define INCONNU		8

#define TEMPS_AFFICHAGE		7
#define ZONE_TXT_LEN		400


//------------------------------------
//
// d
// gmsgRadioMsg

DECLARE_MESSAGE(m_HudRadio, RadioMsg );


//------------------------------------
//
// gestion des messages serveur


int CHudRadio :: MsgFunc_RadioMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_flStartTime	= READ_COORD ();
	m_iHead			= READ_LONG ();
	int len			= READ_LONG ();

	// initialisation de la chaine de caracteres

	for ( int i=0; i<256; i++ )
		m_cText [i] = '\0';

	for ( int i=0; i<len; i++ )
		m_cText [i] = READ_BYTE ();

	if ( m_iFlags & HUD_ACTIVE )
		m_bOpen = false;
	else
		m_bOpen = true;


	m_iFlags |= HUD_ACTIVE;

	// activation du vgui
	#if USE_VGUI
	if ( gViewPort )
	{
		CMenuPanel *pNewMenu = NULL;

		pNewMenu = gViewPort->OpenRadioMenu();

		CRadio *pRadio = (CRadio*)pNewMenu;

		pRadio->m_iHead = m_iHead;
		sprintf ( pRadio->m_cText, m_cText );
		pRadio->Initialize ();

		gViewPort->HideCommandMenu();

		pNewMenu->SetMenuID( MENU_RADIO );
		pNewMenu->SetActive( true );
		
		gViewPort->m_pCurrentMenu = pNewMenu;
		gViewPort->m_pCurrentMenu->Open();
	//	gViewPort->UpdateCursorState();	

	}
	#endif


	
	return 1;
}

/*		WRITE_COORD ( gpGlobals->time );
		WRITE_LONG	( m_iHead );
		WRITE_LONG	( len );

		WRITE_LONG	( (int)&txt );

*/

//------------------------------------
//
// rafraichissement de l'affichage


int CHudRadio :: Draw	( float flTime )
{

	// fermeture

	if ( gHUD.m_flTime > m_flStartTime + TEMPS_AFFICHAGE )
	{
		m_iFlags &= ~HUD_ACTIVE;

		// vgui
		#if USE_VGUI
		if ( gViewPort )
		{
			gViewPort->HideTopMenu();
		}
		#endif


		return 1;
	}

	//
	/* affichage radio */
	//

/*
	int bordG = 0;
	wrect_t rec;
	int r,g,b;

	if ( m_iHead != VOCAL )
	{
		// affichage t

		r=g=b=255;

		switch ( m_iHead )
		{
		default:
		case GLASSES:
			rec = CreateWrect(0,0,72,96);
			SPR_Set( m_sprHead1, r, g, b);
			break;
		case EINSTEIN:
			rec = CreateWrect(72,0,144,96);
			SPR_Set( m_sprHead1, r, g, b);
			break;
		case LUTHER:
			rec = CreateWrect(144,0,216,96);
			SPR_Set( m_sprHead1, r, g, b);
			break;
		case SLICK:
			rec = CreateWrect(0,0,72,96);
			SPR_Set( m_sprHead2, r, g, b);
			break;
		case BARNEY:
			rec = CreateWrect(72,0,144,96);
			SPR_Set( m_sprHead2, r, g, b);
			break;
		case INFOCOMBI:
			rec = CreateWrect(144,0,216,96);
			SPR_Set( m_sprHead2, r, g, b);
			break;

		}

		SPR_Draw( 0, bordG, ScreenHeight - 96, &rec );

		bordG += 72;
	}

	// affichage fen

	r=g=b=255;
	ScaleColors ( r, g, b, 255 );

	rec = CreateWrect(0,0,256,96);

	SPR_Set( m_sprText1, r, g, b);
	SPR_Draw( 0, bordG, ScreenHeight - 96, &rec );

	rec = CreateWrect(0,0,224,96);

	SPR_Set( m_sprText2, r, g, b);
	SPR_Draw( 0, bordG + 256, ScreenHeight - 96, &rec );



	// affichage du texte

	int curseurX = 0;
	int curseurY = 0;

	r=g=b=255;

	for ( int i=0; i<(int)strlen(m_cText); i++ )
	{
		if (m_cText[i] == '\n' )
			continue;
		if (m_cText[i] == '\0' )
			break;


		// peut-on caser le mot dans l'espace qu'il nous reste

		int wordlen = 0;
		int lettres = 0;

		if (m_cText[i] != ' ' )
		{

			while ( m_cText[i+lettres] != ' ' )
			{
				if (m_cText[i+lettres] == '\n' )
					break;
				if (m_cText[i+lettres] == '\0' )
					break;

				wordlen += gHUD.m_scrinfo.charWidths[ m_cText[i+lettres] ];
				lettres ++;
			}
		}
		else
		{
			wordlen = gHUD.m_scrinfo.charWidths[' '];
			lettres = 1;
		}


		if ( curseurX + wordlen > ZONE_TXT_LEN )
		{
			// retour 
			curseurX = 0;
			curseurY += gHUD.m_scrinfo.iCharHeight;
		}

		// dessin des lettres

		for ( int j=0; j<lettres; j++ )
		{
			if ( i+j >= 180/)
				break;

			TextMessageDrawChar( bordG + 40 + curseurX, ScreenHeight - 96 + 20 + curseurY, m_cText[i+j], r, g, b );

			curseurX += gHUD.m_scrinfo.charWidths[ m_cText[i+j] > 0 ? m_cText[i+j] : m_cText[i+j] + 0xFF ];
		}


		i += lettres - 1;
	}

*/

		
	return 1;
}



//------------------------------------
//
// initialisation au chargement de la dll

int CHudRadio :: Init( void )
{
	HOOK_MESSAGE( RadioMsg );

	m_sprHead1	= SPR_Load("sprites/hud_texthead1.spr");
	m_sprHead2	= SPR_Load("sprites/hud_texthead2.spr");
	m_sprText1	= SPR_Load("sprites/hud_textbox1.spr");
	m_sprText2	= SPR_Load("sprites/hud_textbox2.spr");

	
	m_flStartTime	= 0;
	m_bOpen			= false;

	m_iHead			= INCONNU;
	sprintf ( m_cText, "" );

	m_iFlags &= ~HUD_ACTIVE;
	m_iFlags |= HUD_ALWAYSDRAW;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudRadio :: VidInit( void )
{
	m_sprHead1	= SPR_Load("sprites/hud_texthead1.spr");
	m_sprHead2	= SPR_Load("sprites/hud_texthead2.spr");
	m_sprText1	= SPR_Load("sprites/hud_textbox1.spr");
	m_sprText2	= SPR_Load("sprites/hud_textbox2.spr");

	
	m_flStartTime	= 0;
	m_bOpen			= false;

	m_iHead			= INCONNU;
	sprintf ( m_cText, "" );

	m_iFlags &= ~HUD_ACTIVE;
	m_iFlags |= HUD_ALWAYSDRAW;

	gHUD.AddHudElem(this);
	return 1;
}


