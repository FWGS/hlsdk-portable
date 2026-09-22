/**********************************************************
*														  *
*		The Code was created by the Go-Mod 3 Team		  *
*		 2009-2010 Ranger366 & FITAMOD (HLMODER)		  *
*														  *
**********************************************************/

#include "hud.h"
#include "cl_util.h"
#include "vgui_TeamFortressViewport.h"

extern int g_iSelectMenuType;

CSelectMenu :: CSelectMenu(int iTrans, int iRemoveMe, int x, int y, int wide, int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
    // Start 
    CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
    SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
    // End 

    m_pPanel = new CTransparentPanel( 300, ScreenWidth * 0.5 - 320 , ScreenHeight * 0.5 - 240, 640, 480);
    m_pPanel->setParent( this );

	if(g_iSelectMenuType == 6)
    {
        // Imagen Body 6
        m_pMyPicture = new CImageLabel( "select_menu6_e", 1, 1);

        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );
	}
	else if(g_iSelectMenuType == 5)
    {
		m_pMyPicture = new CImageLabel( "select_menu5_e", 1, 1);

        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 590, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 590, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );

        m_pSpeak = new CommandButton( "", 40, 315, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button3" ) );

        m_pSpeak = new CommandButton( "", 40, 415, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button4" ) );
	}
	else if(g_iSelectMenuType == 4)
    {
		m_pMyPicture = new CImageLabel( "select_menu4_e", 1, 1);

        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );

        m_pSpeak = new CommandButton( "", 40, 315, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button3" ) );

        m_pSpeak = new CommandButton( "", 40, 415, 480, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button4" ) );
	}
	else if(g_iSelectMenuType == 3)
    {
		m_pMyPicture = new CImageLabel( "select_menu3_e", 1, 1);
   
        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );

        m_pSpeak = new CommandButton( "", 40, 315, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button3" ) );

        m_pSpeak = new CommandButton( "", 40, 415, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button4" ) );
    }
	else if(g_iSelectMenuType == 2){

	    // Imagen Body 2
		m_pMyPicture = new CImageLabel( "select_menu2_e", 1, 1);
    
        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );
	}
	else
    {
	    // Imagen Body 1
		m_pMyPicture = new CImageLabel( "select_menu1_e", 1, 1);

        m_pMyPicture->setVisible( true );
        m_pMyPicture->setParent( m_pPanel );

        m_pSpeak = new CommandButton( "", 40, 115, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button1" ) );

        m_pSpeak = new CommandButton( "", 40, 215, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button2" ) );

        m_pSpeak = new CommandButton( "", 40, 315, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button3" ) );

        m_pSpeak = new CommandButton( "", 40, 415, 300, 60);
        m_pSpeak->setContentAlignment( vgui::Label::a_center );
        m_pSpeak->setParent( m_pPanel );
        m_pSpeak->addActionSignal( new CMenuHandler_StringCommand( "select_button4" ) );
	}
	////////////// START CUARTA FILA \\\\\\\\\\\\\\\\\\\\\\\\\\

}
