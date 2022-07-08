/************************************************************************
*																		*
*			vgui_OrdiMenu.cpp , par Julien								*
*																		*
************************************************************************/
// menu vgui pour le contr




#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
//#include "vgui_OrdiMenu.h"
#include "crutches.h" //Load some code crutches for HLINVASION, modif de Roy

#define ORDI_STRING_LEN		5000


void GetOrdiText ( const char *textname, char *messagename, Font *pRadioFont, int sizeX );

#define SCANNE_CHAR if ( fscanf ( myfile, "%s", cscan ) == EOF ) break

#define TEXT_FILE_PATH			"invasion/texts.txt" //Backward slash replaced with forward slash modif de Roy


/*
	m_pGButton[0]->setText( BOUTON_ENTREE );
	m_pGButton[1]->setText( BOUTON_APPLICATIONS );
	m_pGButton[2]->setText( BOUTON_MESSAGERIE );
	m_pGButton[3]->setText( BOUTON_INFORMATIONS );
	m_pGButton[4]->setText( BOUTON_DONNEESSYSTEME );
	m_pGButton[5]->setVisible ( false );
	m_pGButton[6]->setText( BOUTON_SECURITE );
*/

/*
#define BOUTON_ENTREE			"Entrer"
#define BOUTON_APPLICATIONS		"Applications"
#define BOUTON_MESSAGERIE		"Messagerie"
#define BOUTON_INFORMATIONS		"Informations"
#define BOUTON_DONNEESSYSTEME	"Donn
#define BOUTON_SECURITE			"S

#define ENTREE_TEXT		"Entrer"
#define TITLE_TEXT		"R
#define INTRO_TEXT		"Terminal n
#define MESSAGE_TEXTE5	"============================================================\n================   MESSAGERIE   ===================\n\nMessage de :\n        ROSENBERG, Docteur\n\nNumro de message :\n        08042002P-194122M\n\n\n	Cher collgue, je m'inquite de l'activit des militaires\nces jours ci. Ils nous ont permi de poursuivre nos travaux\ncar je crois que la technologie de la tlportation les intressait.\nMais j'ai l'impression que cela ne va pas durer.\nIls se dbarasseront de nous quand nous\nne leur feront plus besoin.\n"
#define MESSAGE_TEXTE2	"============================================================\n================   MESSAGERIE   ===================\n\nMessage de :\n        VAN BUREN, Docteur\n\nNum
#define MESSAGE_TEXTE3	"============================================================\n================   MESSAGERIE   ===================\n\nMessage de :\n        HARRINGTON, Professeur\n\nNum
#define MESSAGE_TEXTE4	"============================================================\n================   MESSAGERIE   ===================\n\nMessage de :\n        NEWEL, Professeur\n\nNum

//#define MESSAGE_TEXTE1	"============================================================\n================   MESSAGERIE   ===================\n\nMessage de :\n        JOHNSON, Docteur\n\nNum
#define MESSAGE_TEXTE1	GetOrdiText ( "MESSAGE_TEXTE1" )

#define INFO_TEXTE1		"============================================================\n================   INFORMATIONS   ===================\n\n Destin
#define INFO_TEXTE2		"============================================================\n================   INFORMATIONS   ===================\n\n Destin

#define REFUSE_TEXTE	"============================================================\n================   AVERTSSEMENT   ===================\n\nVOUS N'ETES PAS AUTORISE\n A ACCEDER A CES INFORMATIONS"
#define KEYPAD_TEXTE	"============================================================\n================     SECURITE     ===================\n\nLa modification de tout param
#define KEYPAD_TEXTE2	"============================================================\n================     SECURITE     ===================\n\n\n\n\nCameras de securit
*/


#define HANDLER_KEYPAD 2
#define SECURITY_CODE		7602

COrdiMenuPanel :: COrdiMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	//constructeur du vgui - initialise les elements sans les afficher
	//fonction appel
	//aucun texte pour l instant - celui ci est cree lors de l affichage des menus

	Initialize();

	//===================================
	// chargement des polices
	//===================================

	pSchemes = gViewPort->GetSchemeManager();

	hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	hInfoText = pSchemes->getSchemeHandle( "Briefing Text" );

	pTitleFont = pSchemes->getFont( hTitleScheme );
	pTextFont = pSchemes->getFont( hInfoText );

	pSchemes->getFgColor( hTitleScheme, r[0], g[0], b[0], a[0] );
	pSchemes->getBgColor( hTitleScheme, r[1], g[1], b[1], a[1] );

	pSchemes->getFgColor( hInfoText, r[2], g[2], b[2], a[2] );
	pSchemes->getBgColor( hInfoText, r[3], g[3], b[3], a[3] );



	//====================================
	// 
	//====================================

	// fenetre

	m_pWindow = new CTransparentPanel( 255, ORDIMENU_WINDOW_X, ORDIMENU_WINDOW_Y, ORDIMENU_WINDOW_SIZE_X, ORDIMENU_WINDOW_SIZE_Y );
    m_pWindow->setParent( this );
    m_pWindow->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );

	// boutton quitter

    m_pCancelButton = new CommandButton( "", ORDIMENU_CANCEL_BUTTON_X, ORDIMENU_CANCEL_BUTTON_Y, ORDIMENU_STANDART_BUTTON_SIZE_X, ORDIMENU_STANDART_BUTTON_SIZE_Y);
    m_pCancelButton->setParent( m_pWindow );
    m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );

	char txt [ORDI_STRING_LEN];
	GetOrdiText ( "BOUTON_QUITER", txt, pTitleFont, 9999 );
	m_pCancelButton->setText( txt );

	// titre

	m_pTitle = new Label ( ""/*TITLE_TEXT*/, ORDIMENU_TITLE_X, ORDIMENU_TITLE_Y );
	m_pTitle->setFont( pTitleFont ); 
	m_pTitle->setParent( m_pWindow );
	m_pTitle->setFgColor( r[0], g[0], b[0], a[0] );
	m_pTitle->setBgColor( r[1], g[1], b[1], a[1] );
	m_pTitle->setContentAlignment( vgui::Label::a_west );

	GetOrdiText ( "TITLE_TEXT", txt, pTitleFont, 9999 );
	m_pTitle->setText( txt );


	// texte

	m_pText[0] = new TextPanel( " ", ORDIMENU_TEXT_X + 15 ,ORDIMENU_TEXT_Y + 15, ORDIMENU_TEXT_SIZE_X -30, ORDIMENU_TEXT_SIZE_Y -30 );
	m_pText[0]->setParent( m_pWindow );
	m_pText[0]->setFont( pTextFont );
	m_pText[0]->setFgColor( r[2], g[2], b[2], a[2] );
	m_pText[0]->setBgColor( r[3], g[3], b[3], a[3] );


	// bordure du texte

	m_pBorder[0] = new TextPanel( " ",   ORDIMENU_TEXT_X,ORDIMENU_TEXT_Y, ORDIMENU_TEXT_SIZE_X, ORDIMENU_TEXT_SIZE_Y );
	m_pBorder[0]->setParent( m_pWindow );
	m_pBorder[0]->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );
	m_pBorder[0]->setFgColor( r[2], g[2], b[2], a[2] );
	m_pBorder[0]->setBgColor( r[3], g[3], b[3], a[3] );

	// deuxieme bordure
	

	m_pBorder[1] = new TextPanel( "",   ORDIMENU_TEXT_X,ORDIMENU_TEXT_Y, ORDIMENU_TEXT_SIZE_X, ORDIMENU_TEXT_SIZE_Y/3 );
	m_pBorder[1]->setParent( m_pWindow );
	m_pBorder[1]->setVisible ( false );
	m_pBorder[1]->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );
	m_pBorder[1]->setFgColor( r[2], g[2], b[2], a[2] );
	m_pBorder[1]->setBgColor( r[3], g[3], b[3], a[3] );



	// boutons

	int posY = ORDIMENU_GBUTTONS_Y;

	for ( int i = 0; i < MAX_GBUTTONS ; i++ )
	{
		m_pGButton[i] = new CommandButton( " ", ORDIMENU_GBUTTONS_X, posY, ORDIMENU_STANDART_BUTTON_SIZE_X, ORDIMENU_STANDART_BUTTON_SIZE_Y);
		m_pGButton[i]->setParent( m_pWindow );
		m_pGButton[i]->setVisible ( false );
		posY += ORDIMENU_STANDART_BUTTON_SIZE_Y + 20;
	}


	GetOrdiText ( "BOUTON_ENTREE", txt, pTextFont, 9999);
	m_pGButton[0]->setText( txt );

	GetOrdiText ( "BOUTON_APPLICATIONS", txt, pTextFont, 9999 );
	m_pGButton[1]->setText( txt );

	GetOrdiText ( "BOUTON_MESSAGERIE", txt, pTextFont, 9999 );
	m_pGButton[2]->setText( txt );

	GetOrdiText ( "BOUTON_INFORMATIONS", txt, pTextFont, 9999 );
	m_pGButton[3]->setText( txt );

	GetOrdiText ( "BOUTON_DONNEESSYSTEME", txt, pTextFont, 9999 );
	m_pGButton[4]->setText( txt );
	m_pGButton[5]->setVisible ( false );

	GetOrdiText ( "BOUTON_SECURITE", txt, pTextFont, 9999 );
	m_pGButton[6]->setText( txt );

	m_pGButton[0]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_PRINCIPAL, this ) );
	m_pGButton[1]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_REFUSE, this ) );
	m_pGButton[2]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_MESSAGE, this ) );
	m_pGButton[3]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_INFO, this ) );
	m_pGButton[4]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_REFUSE, this ) );
	m_pGButton[6]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_REFRESH, MENU_KEYPAD_CAM, this ) );


	// keypad

	m_pText[1] = new TextPanel( "1", XRES(412),YRES(238), XRES(30), YRES(30));
	m_pText[2] = new TextPanel( "2", XRES(457),YRES(238), XRES(30), YRES(30));
	m_pText[3] = new TextPanel( "3", XRES(502),YRES(238), XRES(30), YRES(30));
	
	m_pText[4] = new TextPanel( "4", XRES(412),YRES(283), XRES(30), YRES(30));
	m_pText[5] = new TextPanel( "5", XRES(457),YRES(283), XRES(30), YRES(30));
	m_pText[6] = new TextPanel( "6", XRES(502),YRES(283), XRES(30), YRES(30));

	m_pText[7] = new TextPanel( "7", XRES(412),YRES(328), XRES(30), YRES(30));
	m_pText[8] = new TextPanel( "8", XRES(457),YRES(328), XRES(30), YRES(30));
	m_pText[9] = new TextPanel( "9", XRES(502),YRES(328), XRES(30), YRES(30));

	m_pText[10] = new TextPanel( "0", XRES(457),YRES(373), XRES(30), YRES(30));

	m_pKeypad[1] = new CommandButton( " ", XRES(400), YRES(230), XRES(30), YRES(30) );
	m_pKeypad[2] = new CommandButton( " ", XRES(445), YRES(230), XRES(30), YRES(30) );
	m_pKeypad[3] = new CommandButton( " ", XRES(490), YRES(230), XRES(30), YRES(30) );

	m_pKeypad[4] = new CommandButton( " ", XRES(400), YRES(275), XRES(30), YRES(30) );
	m_pKeypad[5] = new CommandButton( " ", XRES(445), YRES(275), XRES(30), YRES(30) );
	m_pKeypad[6] = new CommandButton( " ", XRES(490), YRES(275), XRES(30), YRES(30) );

	m_pKeypad[7] = new CommandButton( " ", XRES(400), YRES(320), XRES(30), YRES(30) );
	m_pKeypad[8] = new CommandButton( " ", XRES(445), YRES(320), XRES(30), YRES(30) );
	m_pKeypad[9] = new CommandButton( " ", XRES(490), YRES(320), XRES(30), YRES(30) );

	m_pKeypad[0] = new CommandButton( " ", XRES(445), YRES(365), XRES(30), YRES(30) );

	for ( int j = 1; j < 11 ; j++ )
	{
		m_pText[j]->setParent( m_pWindow );
		m_pText[j]->setFgColor( r[2], g[2], b[2], a[2] );
		m_pText[j]->setBgColor( r[3], g[3], b[3], a[3] );
	}

	for ( int k = 0; k < 10 ; k++ )
	{
		m_pKeypad[k]->setParent( m_pWindow );

		m_pKeypad[k]->addActionSignal( new CMenuHandler_OrdiMenu( HANDLER_KEYPAD, k, this ) );

	}

	// affichage du code

	m_pBorder[2] = new TextPanel( "coucou", XRES(440) ,YRES(400), XRES(70), YRES(30) );
	m_pBorder[2]->setParent( m_pWindow );
	m_pBorder[2]->setFont( pTitleFont );
	m_pBorder[2]->setFgColor( r[2], g[2], b[2], a[2] );
	m_pBorder[2]->setBgColor( r[3], g[3], b[3], a[3] );





}





//=================================================
// changements de menus
//=================================================


void COrdiMenuPanel :: Refresh ( void )
{
	// efface tous les elements

	for ( int i = 0; i < MAX_GBUTTONS ; i++ )
	{
		m_pGButton[i]->setVisible ( false );
	}

	for ( int j = 0; j < MAX_TEXTS; j++ )
	{
		m_pText[j]->setText (" ");
	}

	for ( int k = 0; k < 10; k++ )
	{
		m_pKeypad[k]->setVisible ( false );
	}

	m_pBorder[0]->setVisible ( false );
	m_pBorder[1]->setVisible ( false );
	m_pBorder[2]->setVisible ( false );


	// redessine un seul bouton pour le menu d'accueil

	if ( m_iCurrentMenu == MENU_ACCUEIL )
	{
			m_pGButton[0]->setVisible ( true );
			m_pBorder[0]->setVisible ( true );
		
			char txt [ORDI_STRING_LEN];
			GetOrdiText ( "INTRO_TEXT", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
			m_pText[0]->setText ( txt );
	}

	// redessine tous les boutons pour les autres menus
	else
	{
		for ( int i = 1; i < MAX_GBUTTONS ; i++ )
		{
			m_pGButton[i]->setVisible ( true );
		}
		m_pGButton[5]->setVisible ( false );

		if ( m_iID != 1 )
			m_pGButton[6]->setVisible ( false );
		else
		{
			m_pGButton[1]->setVisible ( false );
	//		m_pGButton[3]->setVisible ( false );
			m_pGButton[4]->setVisible ( false );
		}



	//redessine la partie de droite selon le menu


		switch ( m_iCurrentMenu )
		{
		case MENU_PRINCIPAL :
			{
				break;
			}
		case MENU_REFUSE :
			{
				m_pBorder[1]->setVisible ( true );
		
				char txt [ORDI_STRING_LEN];
				GetOrdiText ( "REFUSE_TEXTE", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
				m_pText[0]->setText ( txt );
				break;
			}
		case MENU_MESSAGE :
			{
				m_pBorder[0]->setVisible ( true );

				switch ( m_iID )
				{
				case 1:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "MESSAGE_TEXTE1", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				case 2:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "MESSAGE_TEXTE2", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				case 3:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "MESSAGE_TEXTE3", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				case 4:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "MESSAGE_TEXTE4", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				case 5:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "MESSAGE_TEXTE5", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				}
				break;
			}

		case MENU_INFO :
			{
				m_pBorder[0]->setVisible ( true );
				switch ( m_iID )
				{
				case 1:
				case 2:
				case 3:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "INFO_TEXTE1", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				case 4:
				case 5:
					{
						char txt [ORDI_STRING_LEN];
						GetOrdiText ( "INFO_TEXTE2", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
						m_pText[0]->setText ( txt );
						break;
					}
				}
				break;
			}

		case MENU_KEYPAD_CAM :
			{
				m_pBorder[1]->setVisible ( true );
				char txt [ORDI_STRING_LEN];
				GetOrdiText ( "KEYPAD_TEXTE", txt, pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
				m_pText[0]->setText ( txt );

				for ( int i = 1; i < MAX_TEXTS; i++ )
				{
					int j = i == 10 ? 0 : i;
					char szBuf[16];
					sprintf( szBuf, "%i", j );

					m_pText[i]->setText (szBuf);
				}

				for ( int k = 0; k < 10; k++ )
				{
					m_pKeypad[k]->setVisible ( true );
				}

				for ( int l = 0; l < 4; l++ )
				{
					keytab [l] = 0;
				}

				break;
			}
		}
	}
}



void CMenuHandler_OrdiMenu :: actionPerformed(Panel* panel)		// fonction declenchee lors de l'appui sur un boutton
{
#ifdef DOUBLECLICKFIXPATH_INVASION_VGUI
	if(skipTime>0){
		skipTime--;
		return;
	}
	skipTime = 1; //Skip every second press modif de Roy
#endif
	// demande de changement de menu

	if ( m_iType == HANDLER_REFRESH )
	{
		m_pPanel->m_iCurrentMenu = m_iMenu;
		m_pPanel->Refresh ();
	}
	else if ( m_iType == HANDLER_KEYPAD )
	{
		// blip
		gEngfuncs.pfnClientCmd("ordimenu 1" );


		// d

		for ( int l = 0; l < 3; l++ )
		{
			m_pPanel->keytab [l] = m_pPanel->keytab [l+1];
		}
		m_pPanel->keytab [3] = m_iMenu;

		int code = m_pPanel->keytab [0]*1000 + m_pPanel->keytab [1]*100 + m_pPanel->keytab [2]*10 + m_pPanel->keytab [3];

		m_pPanel->m_pBorder[2]->setVisible ( true );
		char sz[16];
		sprintf( sz, "%i", code );
		m_pPanel->m_pBorder[2]->setText(sz);

		if ( code == SECURITY_CODE )
		{
			gEngfuncs.pfnClientCmd("ordimenu 2" );
			char txt [ORDI_STRING_LEN];
			GetOrdiText ( "KEYPAD_TEXTE2", txt, m_pPanel->pTextFont, ORDIMENU_TEXT_SIZE_X -30 );
			m_pPanel->m_pText[0]->setText(txt);
		}
	}
}


//===============
//====================================================


void COrdiMenuPanel::Initialize( void )
{
	// il ne se passe rien
}

void COrdiMenuPanel::Reset( void )
{
	m_iCurrentMenu = MENU_ACCUEIL;
	Refresh ();

    CMenuPanel::Reset();
}



void GetOrdiText ( const char *textname, char *messagename, Font *pRadioFont, int sizeX )
{

	// ouverture du fichier texte

	char filename [128];
	sprintf ( filename, TEXT_FILE_PATH );

//	char messagename [62];
	sprintf ( messagename, textname );


	FILE *myfile = fopen ( filename, "r" );

	if ( myfile == NULL )
	{
		CONPRINTF ("\\\nORDITXT : impossible d'ouvrir %s\n\\\n", filename );
		return;
	}


	char	cscan [128];
	char	messagetext [ORDI_STRING_LEN];
	int		startoffset = 0;
	int		stopoffset = 0;


	while ( 1 )
	{
		// titre

		SCANNE_CHAR;		
		if ( strcmp ( cscan, messagename ) != 0 ) continue;

		// point d'entr

		SCANNE_CHAR;
		if ( strcmp ( cscan, "{" ) != 0 ) continue;

		// offsets de d

		int startoffset = (int)ftell ( myfile ) + 2;
		
		while ( 1 )
		{	
			SCANNE_CHAR;
			if ( strcmp ( cscan, "}" ) != 0 ) continue;
			break;
		}

		int stopoffset = (int)ftell ( myfile );

		// r

		fseek ( myfile, startoffset, SEEK_SET );

		int i = 0; //Loop iterator fix. Must be outside of the loop to still be accessible afterwards.
		for ( i=0; i<(int)(stopoffset-startoffset); i++ )
		{
			char copie = getc ( myfile );

			if ( copie == '}' )
			{
				messagetext [i-1] = '\0';
				break;
			}

			messagetext [i] = copie;
		}

		messagetext [i] = '\0';
	}

	// fermeture du fichier texte

	fclose ( myfile );




	// mise en forme

		
	
	int curseurX = 0;
	int curseurY = 0;
	int rien;

	char source [ORDI_STRING_LEN];
	char dest	[ORDI_STRING_LEN];

	sprintf ( source, messagetext );
	sprintf ( dest, "" );

	int i;

	for ( i=0; i<(int)strlen(source); i++ )
	{
		if ( source[i] != '\n' )
			break;
	}


	for ( i=i; i<(int)strlen(source); i++ )
	{
		if (source[i] == '\0' )
			continue;

		if ( i == 0 &&  source[i] == '\n' )
			continue;

		// peut-on caser le mot dans l'espace qu'il nous reste

		int wordlen = 0;
		int lettres = 0;

		if (source[i] != ' ' && source[i] != '\n' )
		{
			while ( source[i+lettres] != ' ' )
			{
				if (source[i+lettres] == '\n' )
					break;
				if (source[i+lettres] == '\0' )
					break;

				//wordlen += gHUD.m_scrinfo.charWidths[ source[i+lettres] ];

				char caractere [2];
				sprintf ( caractere, "%c", source[i+lettres] );

				int len = 0;
				pRadioFont->getTextSize ( caractere, len, rien );

				wordlen += len;
				lettres ++;
			}


		}
		else if ( source[i] == '\n' )
		{
			curseurX = 0;
			strcat ( dest, "\n" );
			continue;
		}
		else
		{
//			wordlen = gHUD.m_scrinfo.charWidths[' '];

			char caractere [2];
			sprintf ( caractere, "%c", ' ' );

			int len = 0;
			pRadioFont->getTextSize ( caractere, len, rien );

			wordlen = len;
			lettres = 1;
		}


		if ( curseurX + wordlen > sizeX )
		{
//			curseurY += gHUD.m_scrinfo.iCharHeight;

			
			// retour 
			curseurX = 0;

			strcat ( dest, "\n" );
		}

		// dessin des lettres


		for ( int j=0; j<lettres; j++ )
		{

			char cat [2];
			sprintf ( cat, "" );

			sprintf ( cat, "%c", source[i+j] );

			strcat ( dest, cat );


			char caractere [2];
			sprintf ( caractere, "%c", source[i+j] );
			int len = 0;
			pRadioFont->getTextSize ( caractere, len, rien );

			curseurX += len;
		}


		i += lettres - 1;
	}

	sprintf ( messagename, dest );

	return;

}




