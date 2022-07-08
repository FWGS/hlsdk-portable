/************************************************************************
*																		*
*			vgui_OrdiControl.cpp , par Julien							*
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
#include "crutches.h" //Load some code crutches for HLINVASION, modif de Roy



#ifndef CRASHFIXPATH_INVASION_VGUI
class CHandler_Conveyor : public InputSignal
#else
class CHandler_Conveyor : public ActionSignal
#endif
{
private:
	int m_iConveyor;	
	COrdiControlPanel *p;


public:
	CHandler_Conveyor( COrdiControlPanel *pPanel, int i )
	{
		p = pPanel;
		m_iConveyor = i;
	}
#ifdef DOUBLECLICKFIXPATH_INVASION_VGUI
	int skipTime = 0; //Just an integer to store double-press fix, modif de Roy
#endif

	void cursorEntered(Panel *panel) {};
	void mouseReleased(MouseCode code,Panel* panel) {};
	void mouseDoublePressed(MouseCode code,Panel* panel) {};
	void cursorExited(Panel* panel) {};
	void mouseWheeled(int delta,Panel* panel) {};
	void keyPressed(KeyCode code,Panel* panel) {};
	void keyTyped(KeyCode code,Panel* panel) {};
	void keyReleased(KeyCode code,Panel* panel) {};
	void keyFocusTicked(Panel* panel) {};

	void cursorMoved(int x,int y,Panel* panel) {};
#ifndef CRASHFIXPATH_INVASION_VGUI
	void mousePressed(MouseCode code,Panel* panel);
#else
	void actionPerformed(Panel* panel);
#endif

};

#ifndef CRASHFIXPATH_INVASION_VGUI
void CHandler_Conveyor :: mousePressed(MouseCode code,Panel* panel)
#else
void CHandler_Conveyor :: actionPerformed(Panel* panel)		// fonction declenchee lors de l'appui sur un boutton
#endif
{
#ifdef DOUBLECLICKFIXPATH_INVASION_VGUI
	if(skipTime>0){
		skipTime--;
		return;
	}
	skipTime = 1; //Skip every second press modif de Roy
#endif
	if ( p->m_ibitConveyor & ( 1 << m_iConveyor ) )
	{
		p->m_ibitConveyor &= ~( 1 << m_iConveyor );
	}
	else
	{
		p->m_ibitConveyor |= ( 1 << m_iConveyor );
	}


	p->Initialize ();

	gEngfuncs.pfnClientCmd("ordimenu 1" );
}




COrdiControlPanel :: COrdiControlPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	//constructeur du vgui - initialise les elements sans les afficher
	//fonction appel
	//return; //CRASHFIXPATH trying to diagnose the crash.


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


	//return; //CRASHFIXPATH
	//====================================
	// 
	//====================================

	// fenetre

	m_pWindow = new CTransparentPanel( 255, XRES(10), YRES(10), XRES(620), YRES(460) );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder(Color(178,119,0,0)) ); //Precalculated. Hates floats.
	//m_pWindow->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );

	// boutton quitter

	m_pCancelButton = new CommandButton( "Appliquer", XRES(20), YRES(400), XRES(150), YRES(20) );
	m_pCancelButton->setParent( m_pWindow );
//	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );
	m_pCancelButton->addActionSignal( new CMenuHandler_OrdiControl( this ) );

	// image

	m_pImage = new 	CImageLabel( "conveyor",XRES(0), YRES(0) );
	m_pImage->setParent( m_pWindow );

	// boutons

	int posY = XRES(60);
	for ( int i = 0; i < 4 ; i++ )
	{
		char sz[16];
		sprintf( sz, "Tapis %i", i+1 );
		m_pButton[i] = new TextPanel( sz, XRES ( 450 ), posY, XRES(120), YRES(20) );
		m_pButton[i]->setParent( m_pWindow );

		m_pButton[i]->setFont( pTextFont );
		m_pButton[i]->setFgColor( r[2], g[2], b[2], a[2] );
		m_pButton[i]->setBgColor( r[3], g[3], b[3], a[3] );

		posY += 70;
	}


	posY = XRES(55);

	for ( int i = 0; i < 4 ; i++ )
	{
#ifndef CRASHFIXPATH_INVASION_VGUI
		m_pFleche[i] = new CImageLabel( "boutR",  XRES ( 500 ), posY );
#else
		m_pFleche[i] = new CommandButton( "R", XRES( 500 ), posY, XRES(20), YRES(20) );
#endif
		m_pFleche[i]->setParent( m_pWindow );
#ifndef CRASHFIXPATH_INVASION_VGUI
		m_pFleche[i]->addInputSignal ( new CHandler_Conveyor ( this, i ) );
#else
		m_pFleche[i]->addActionSignal ( new CHandler_Conveyor ( this, i ) );
#endif
		posY += 70;
	}


	// texte
	m_pText = new TextPanel( "PANNEAU DE CONTROLE\nGENERAL\n\nVeuillez avertir le responsable du personnel avant de changer la direction des tapis menant au compresseur.", XRES ( 380 ),YRES ( 320 ), XRES ( 200 ), YRES ( 80 ) );
	m_pText->setParent( m_pWindow );
	m_pText->setFont( pTextFont );
	m_pText->setFgColor( r[2], g[2], b[2], a[2] );
	m_pText->setBgColor( r[3], g[3], b[3], a[3] );
	//m_pText->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );
	m_pText->setBorder( new LineBorder(Color(178,119,0,0)) ); //Precalculated. Hates floats.


	// bordure du texte
/*
	m_pBorder = new TextPanel( " ", XRES ( 370 ),YRES ( 310 ), XRES ( 200 ), YRES ( 95 ) );
	m_pBorder->setParent( m_pWindow );
	m_pBorder->setBorder( new LineBorder(Color(178,119,0,0)) ); //Precalculated. Hates floats.
	//m_pBorder->setBorder( new LineBorder( Color(255*0.7,170*0.7,0,0 )) );
	m_pBorder->setFgColor( r[2], g[2], b[2], a[2] );
	m_pBorder->setBgColor( r[3], g[3], b[3], a[3] );
*/

	Initialize();	// ?


}


void CMenuHandler_OrdiControl :: actionPerformed(Panel* panel)		// fonction declenchee lors de l'appui sur un boutton
{
	char sz[16];
	sprintf( sz, "ordicontrol %i", m_pPanel->m_ibitConveyor );

	gEngfuncs.pfnClientCmd( sz );			// conveyor
	gEngfuncs.pfnClientCmd("ordimenu 2" );	// blip

	gViewPort->HideTopMenu();

}


//===============
//====================================================


void COrdiControlPanel::Initialize( void )
{

	for ( int i=0; i<4; i++ )
	{
		if(m_pFleche[i] == NULL) break; //If they aren't initialized yet, break the loop. We're probably debugging, modif de Roy.
		if (m_ibitConveyor & ( 1 << i ) )
		{
#ifndef CRASHFIXPATH_INVASION_VGUI
			m_pFleche[i]->m_pTGA = LoadTGA("boutR");
			m_pFleche[i]->setImage( m_pFleche[i]->m_pTGA );
#else
			m_pFleche[i]->setText("R");
#endif
		}
		else
		{
#ifndef CRASHFIXPATH_INVASION_VGUI
			m_pFleche[i]->m_pTGA = LoadTGA("boutL");
			m_pFleche[i]->setImage( m_pFleche[i]->m_pTGA );
#else
			m_pFleche[i]->setText("L");
#endif
		}
	}
}

void COrdiControlPanel::Reset( void )
{
    CMenuPanel::Reset();
}










