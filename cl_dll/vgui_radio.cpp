//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_radio.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code de la radio en vgui							---
//---------------------------------------------------------
//---------------------------------------------------------



//---------------------------------------------------------
//inclusions

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"
#include "crutches.h" //Load some code crutches for HLINVASION, modif de Roy


#define RADIO_X						0
#define RADIO_Y						0

#define TEXT_MARGE_G				15
#define TEXT_MARGE_H				20
#define TEXT_SIZE_X					(477 - 2*TEXT_MARGE_G )	
#define TEXT_SIZE_Y					66	

#define	GLASSES		0
#define EINSTEIN	1
#define LUTHER		2
#define SLICK		3
#define BARNEY		4
#define GMAN		5
#define VOCAL		6
#define INFOCOMBI	7
#define INCONNU		8

//---------------------------------------------------------

void TeamFortressViewport::CreateRadio()
{
	m_pRadio = new CRadio(0, false, 0, ScreenHeight - 100, ScreenWidth, 100 );
	m_pRadio->setParent(this);
	m_pRadio->setVisible(false);
}

CMenuPanel* TeamFortressViewport :: ShowRadio()
{
    m_pRadio->Reset();
    return m_pRadio;
}


//-----------------------------------------------------------

CRadio :: CRadio(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	//===================================
	// chargement des polices
	//===================================

	pSchemes = gViewPort->GetSchemeManager();

	hRadioFont = pSchemes->getSchemeHandle( "Invasion Text" );
	pRadioFont = pSchemes->getFont( hRadioFont );

	int r,g,b,a;
	int r2,g2,b2,a2;

	pSchemes->getFgColor( hRadioFont, r,g,b,a );
	pSchemes->getBgColor( hRadioFont, r2,g2,b2,a2 );

	//============================


	m_pTextBox = new CImageLabel ( "radio_textbox", RADIO_X, RADIO_Y );
	m_pTextBox->setParent( this );


	m_pText = new TextPanel ( "", TEXT_MARGE_G, TEXT_MARGE_H, TEXT_SIZE_X, TEXT_SIZE_Y );
	m_pText->setParent( m_pTextBox );
	m_pText->setFont( pRadioFont ); 
	m_pText->setFgColor( r, g, b, a );
	m_pText->setBgColor( r2,g2,b2,a2 );

	//--------------

	m_pHeadTextBox = new CImageLabel ( "radio_textbox", RADIO_X + 72, RADIO_Y );
	m_pHeadTextBox->setParent( this );

	m_pHead = new CImageLabel ( "", 0, 0 );
	m_pHead->setParent( this );

	m_pRadioheadText = new TextPanel ( "", TEXT_MARGE_G, TEXT_MARGE_H, TEXT_SIZE_X, TEXT_SIZE_Y );
	m_pRadioheadText->setFont( pRadioFont ); 
	m_pRadioheadText->setParent( m_pHeadTextBox );
	m_pRadioheadText->setFgColor( r, g, b, a );
	m_pRadioheadText->setBgColor( r2,g2,b2,a2 );


	Initialize();

}

//====================================================


void CRadio :: CreateRadioText ( char source [], char dest [] )
{
	int cursorX = 0;
	int cursorY = 0;
	int nil;


	sprintf ( dest, "" );

	for ( int i=0; i<(int)strlen(source); i++ )
	{
		if (source[i] == '\n' ) //if we encounter a line break, we skip it.
			continue;
		if (source[i] == '\0' ) //if we encounter null termination, we break out of the loop.
			break;

		// can we fit the word in the space we have left

		int wordlen = 0;
		int letters = 0;

		if (source[i] != ' ' ) //not an empty character
		{

			while ( source[i+letters] != ' ' ) //while an i+letters character is not empty, letters starts at 0, so we basically start at i
			{
				if (source[i+letters] == '\n' ) //if we encounter line break or null, we have a word, so let's quit
					break;
				if (source[i+letters] == '\0' )
					break;

				//wordlen += gHUD.m_scrinfo.charWidths[ source[i+letters] ];

				char character [2]; //temporary buffer
				sprintf ( character, "%c", source[i+letters] ); //put character in buffer

				int len = 0; //get text length as it's drawn
				pRadioFont->getTextSize ( character, len, nil );

				wordlen += len; //append to wordlen
				letters ++; //move to next character
			}
		}
		else //do the same for the empty character and quit
		{
//			wordlen = gHUD.m_scrinfo.charWidths[' '];

			char character [2];
			sprintf ( character, "%c", ' ' );

			int len = 0;
			pRadioFont->getTextSize ( character, len, nil );

			wordlen = len;
			letters = 1;
		}//so, now we have wordlen being the drawn length of the word (or, rather, the entire printed string) so far


		if ( cursorX + wordlen > TEXT_SIZE_X ) //now, if current X position plus that supposed lenght exceed allowed X size
		{
//			cursorY += gHUD.m_scrinfo.iCharHeight;

			
			// return
			cursorX = 0; //we return carriage and insert a line break.

			strcat ( dest, "\n" );
		}

		// drawing letters

		for ( int j=0; j<letters; j++ )
		{
			if ( i+j >= 180 )
				break;

			char cat [2];
			sprintf ( cat, "%c", source[i+j] );

			strcat ( dest, cat );


			char character [2];
			sprintf ( character, "%c", source[i+j] );
			int len = 0;
			pRadioFont->getTextSize ( character, len, nil );

			cursorX += len;

		}


		i += letters - 1; //move to next word
	}

}

void CRadio::Initialize( void )
{
	if ( m_iHead != VOCAL )
	{
		m_pTextBox->		setVisible ( false );
		m_pHeadTextBox->	setVisible ( true );
		m_pHead->			setVisible ( true );


		char formetext [256];
		CreateRadioText ( m_cText, formetext );

		m_pText->setText ( "" );
		m_pRadioheadText->setText ( formetext );


		switch ( m_iHead )
		{
		default:
		case GLASSES:
			m_pHead-> m_pTGA = LoadTGA("radiohead_glasses"); break;
		case EINSTEIN:
			m_pHead-> m_pTGA = LoadTGA("radiohead_einstein"); break;
		case LUTHER:
			m_pHead-> m_pTGA = LoadTGA("radiohead_luther"); break;
		case SLICK:
			m_pHead-> m_pTGA = LoadTGA("radiohead_slick"); break;
		case BARNEY:
			m_pHead-> m_pTGA = LoadTGA("radiohead_barney"); break;
		case INFOCOMBI:
			m_pHead-> m_pTGA = LoadTGA("radiohead_info"); break;
		}

		m_pHead->setImage( m_pHead->m_pTGA );
	}
	else
	{

		char formetext [256];
		CreateRadioText ( m_cText, formetext );

		m_pText->setText ( formetext );
		//m_pText->setText ( m_cText );
		m_pRadioheadText->setText ( "" );

		m_pTextBox->		setVisible ( true );
		m_pHeadTextBox->	setVisible ( false );
		m_pHead->			setVisible ( false );

	}
}



void CRadio::Reset( void )
{
	Initialize ();

    CMenuPanel::Reset();
	
}
