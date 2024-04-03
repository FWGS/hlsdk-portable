//=========== (C) Copyright 2008 Vyacheslav Dzhura. All rights reserved. ===========
//
// Purpose: Notification which is displayed for players which have connected 
//			to Decay game server, after there are two human players, also
//			displays count down before disconnecting player
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
#include "VGUI_loadtga.h"

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

// Windows' Dimensions
#define SNW_TITLE_X				XRES(40)
#define SNW_TITLE_Y				YRES(32)
#define SNW_TOPLEFT_BUTTON_X	XRES(251)//was 245
#define SNW_TOPLEFT_BUTTON_Y	YRES(400)
#define SNW_BUTTON_SIZE_X		XRES(100)
#define SNW_BUTTON_SIZE_Y		YRES(24)
#define SNW_BUTTON_SPACER_Y		YRES(8)
#define SNW_WINDOW_X			XRES(150)
#define SNW_WINDOW_Y			YRES(150)
#define SNW_WINDOW_SIZE_X		XRES(350)
#define SNW_WINDOW_SIZE_Y		YRES(220)
#define SNW_WINDOW_TITLE_X		XRES(16)
#define SNW_WINDOW_TITLE_Y		YRES(16)
#define SNW_WINDOW_TEXT_X		XRES(80) // was 16
#define SNW_WINDOW_TEXT_Y		YRES(32)
#define SNW_WINDOW_TEXT_SIZE_Y	YRES(168)
#define SNW_WINDOW_INFO_X		XRES(16)
#define SNW_WINDOW_INFO_Y		YRES(234)

// Creation
CSparePlayerWindow::CSparePlayerWindow(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hDecayFont = pSchemes->getSchemeHandle( "Briefing Text" ); //Decay

	// get the Font used for the Titles
	Font *pTitleFont = pSchemes->getFont( hTitleScheme );
	int r, g, b, a;

	// Create the Info Window itself
	m_pWindow  = new CTransparentPanel( 255, SNW_WINDOW_X, SNW_WINDOW_Y, SNW_WINDOW_SIZE_X, SNW_WINDOW_SIZE_Y );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );

	// Create the Title label
	m_pTitle = new Label( "", SNW_WINDOW_TEXT_X, YRES(5) );
	m_pTitle->setParent( m_pWindow );
	m_pTitle->setFont( pTitleFont );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setContentAlignment( vgui::Label::a_west );
	m_pTitle->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Decay_SparePlayerTitle"));


	// Create the Briefing panel
	m_pMemo = new TextPanel("", SNW_WINDOW_TEXT_X, SNW_WINDOW_TEXT_Y, YRES(230), SNW_WINDOW_TEXT_SIZE_Y );
	m_pMemo->setParent( m_pWindow );
	m_pMemo->setFont( pSchemes->getFont(hDecayFont) );
	pSchemes->getFgColor( hDecayFont, r, g, b, a );
	m_pMemo->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hDecayFont, r, g, b, a );
	m_pMemo->setBgColor( r, g, b, a );
	m_pMemo->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Decay_SparePlayerMessage"));

	// Create the Cancel button
	//m_pCancelButton = new CommandButton( "", SNW_TOPLEFT_BUTTON_X, SNW_TOPLEFT_BUTTON_Y, SNW_BUTTON_SIZE_X, SNW_BUTTON_SIZE_Y, true);
	//m_pCancelButton->setParent( this );
	//m_pCancelButton->setText( gHUD.m_TextMessage.BufferedLocaliseTextString("  CLOSE") );
	//m_pCancelButton->setVisible( true );
	//m_pCancelButton->addActionSignal(new CMenuHandler_TextWindow(HIDE_TEXTWINDOW));

	m_pImage = new CImageLabel( "gina", 0, 0, 128, 256 ); //	gfx/vgui/640_gina.tga
	//m_pImage->setText(25, "this is a test!!!");
	m_pImage->setParent( m_pWindow );
	m_pImage->setVisible( true );

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CSparePlayerWindow::Initialize( void )
{
	//m_pScrollPanel->setScrollValue( 0, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Called everytime the Team Menu is displayed
//-----------------------------------------------------------------------------
void CSparePlayerWindow::Update( void )
{
    // TODO: maybe update "Disconnect in ..." label here?

	//m_pMemo->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szText));
	//m_pTitle->setText(gHUD.m_TextMessage.BufferedLocaliseTextString(szTitle));

	int	 iYPos = SNW_TOPLEFT_BUTTON_Y;

	// Move the AutoAssign button into place
	//m_pCancelButton->setPos( SNW_TOPLEFT_BUTTON_X, iYPos );

	//m_pScrollPanel->validate();
	m_pImage->setBounds( XRES(8), YRES(16), 128, 256 );
}

//=====================================
// Key inputs
bool CSparePlayerWindow::SlotInput( int iSlot )
{
	if ( iSlot == 1)
	{
		//m_pCancelButton->fireActionSignal();
		return true;
	}
	return false;
}

//======================================
// Update the Team menu before opening it
void CSparePlayerWindow::Open( void )
{
	Update();
	CMenuPanel::Open();
}
