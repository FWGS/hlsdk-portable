//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Notepad
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "vgui_int.h"
#include "VGUI_Font.h"
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include "vgui_loadtga.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

// Notepad Dimensions
#define NOTEPAD_TITLE_X				XRES(40)
#define NOTEPAD_TITLE_Y				YRES(32)
#define NOTEPAD_TOPLEFT_BUTTON_X	XRES(251)//was 245
#define NOTEPAD_TOPLEFT_BUTTON_Y	YRES(400)
#define NOTEPAD_BUTTON_SIZE_X		XRES(100)
#define NOTEPAD_BUTTON_SIZE_Y		YRES(24)
#define NOTEPAD_BUTTON_SPACER_Y		YRES(8)
#define NOTEPAD_WINDOW_X			XRES(150)
#define NOTEPAD_WINDOW_Y			YRES(150)
#define NOTEPAD_WINDOW_SIZE_X		XRES(300)
#define NOTEPAD_WINDOW_SIZE_Y		YRES(220)
#define NOTEPAD_WINDOW_TITLE_X		XRES(16)
#define NOTEPAD_WINDOW_TITLE_Y		YRES(16)
#define NOTEPAD_WINDOW_TEXT_X		XRES(16)
#define NOTEPAD_WINDOW_TEXT_Y		YRES(32)
#define NOTEPAD_WINDOW_TEXT_SIZE_Y	YRES(168)
#define NOTEPAD_WINDOW_INFO_X		XRES(16)
#define NOTEPAD_WINDOW_INFO_Y		YRES(234)

// Creation
CNotepad::CNotepad(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hDecayFont = pSchemes->getSchemeHandle( "Briefing Text" ); //Decay

	// get the Font used for the Titles
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );
	int r, g, b, a;

	// Create the title
	m_pTitle = new Label( "", NOTEPAD_WINDOW_X+10, NOTEPAD_WINDOW_Y+5 );
	m_pTitle->setParent( this );
	m_pTitle->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setContentAlignment( vgui::Label::a_west );
	m_pTitle->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szTitle));

	// Create the Info Window
	m_pNotepadWindow  = new CTransparentPanel( 255, NOTEPAD_WINDOW_X, NOTEPAD_WINDOW_Y, NOTEPAD_WINDOW_SIZE_X, NOTEPAD_WINDOW_SIZE_Y );
	m_pNotepadWindow->setParent( this );
	m_pNotepadWindow->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );

	// Create the Scroll panel
	m_pScrollPanel = new CTFScrollPanel( NOTEPAD_WINDOW_TEXT_X, NOTEPAD_WINDOW_TEXT_Y, NOTEPAD_WINDOW_SIZE_X - (NOTEPAD_WINDOW_TEXT_X * 2), NOTEPAD_WINDOW_TEXT_SIZE_Y );
	m_pScrollPanel->setParent(m_pNotepadWindow);
	m_pScrollPanel->setScrollBarVisible(false, false);

	// Create the Map Briefing panel
	m_pBriefing = new TextPanel("", 0,0, NOTEPAD_WINDOW_SIZE_X - NOTEPAD_WINDOW_TEXT_X, NOTEPAD_WINDOW_TEXT_SIZE_Y );
	m_pBriefing->setParent( m_pScrollPanel->getClient() );
	m_pBriefing->setFont( pSchemes->getFont(hDecayFont) );
	pSchemes->getFgColor( hDecayFont, r, g, b, a );
	m_pBriefing->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDecayFont, r, g, b, a );
	m_pBriefing->setBgColor( r, g, b, a );
    m_pBriefing->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szText));

	// Create the Cancel button
	m_pCancelButton = new CommandButton( "", NOTEPAD_TOPLEFT_BUTTON_X, NOTEPAD_TOPLEFT_BUTTON_Y, NOTEPAD_BUTTON_SIZE_X, NOTEPAD_BUTTON_SIZE_Y, true);
	m_pCancelButton->setParent( this );
	m_pCancelButton->setText( gHUD.m_TextMessage.BufferedLocaliseTextString("  CLOSE") );
	m_pCancelButton->setVisible( true );
	m_pCancelButton->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));

	/*
	m_pImage = new CImageLabel( "gina", 0, 0, 128, 256 ); //	gfx/vgui/640_gina.tga
	//m_pImage->setText(25, "this is a test!!!");
	m_pImage->setParent( this );
	m_pImage->setVisible( true );
	*/

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CNotepad::Initialize( void )
{
	m_pScrollPanel->setScrollValue( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the Team Menu is displayed
//-----------------------------------------------------------------------------
void CNotepad::Update( void )
{
	m_pBriefing->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szText));
	m_pTitle->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szTitle));

	int	 iYPos = NOTEPAD_TOPLEFT_BUTTON_Y;

	// Move the AutoAssign button into place
	m_pCancelButton->setPos( NOTEPAD_TOPLEFT_BUTTON_X, iYPos );

	m_pScrollPanel->validate();
	//m_pImage->setBounds( NOTEPAD_TOPLEFT_BUTTON_X, NOTEPAD_TOPLEFT_BUTTON_Y-256-32, 128, 256 );
}

//=====================================
// Key inputs
bool CNotepad::SlotInput( int iSlot )
{
	if ( iSlot == 1)
	{
		m_pCancelButton->fireActionSignal();
		return true;
	}
	return false;
}

//======================================
// Update the Team menu before opening it
void CNotepad::Open( void )
{
	Update();
	CMenuPanel::Open();
}
