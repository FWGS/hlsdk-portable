//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_soin.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code du soin s
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
#ifdef NOATTACKFIXPATH_INVASION_VGUI
extern bool hlinv_isAttackSuspended; //We need to prevent attacks while VGUI is active, modif de Roy
#endif


//----------------------------------------------------------


#define TARGA_TOP		50
#define TARGA_LEFT		100

#define VIEWPORT_LEFT	(ScreenWidth - 288 - 100 - 30 - 15)
#define VIEWPORT_TOP	(ScreenHeight - 296 - 100 - 30 )

#define CADRE_TOP		15
#define CADRE_LEFT		15

#define BOUT_WIDTH		24

#define LIGNE1			( TARGA_TOP + 7 - BOUT_WIDTH/2 )
#define LIGNE2			( TARGA_TOP + 44 - BOUT_WIDTH/2 )
#define LIGNE3			( TARGA_TOP + 84 - BOUT_WIDTH/2 )
#define LIGNE4			( TARGA_TOP + 127 - BOUT_WIDTH/2 )
#define LIGNE5			( TARGA_TOP + 165 - BOUT_WIDTH/2 )

#define COLONE2			( TARGA_LEFT - 10 - BOUT_WIDTH )							
#define COLONE1			( COLONE2 - 10 - BOUT_WIDTH )	

#define OK_LEFT			( TARGA_LEFT  - 75 )	
#define OK_TOP			( TARGA_TOP + 296 - 50 )					



//---------------------------------------------------------


void TeamFortressViewport::CreateSoin()
{
	m_pSoin = new CSoin(100, false, VIEWPORT_LEFT, VIEWPORT_TOP, ScreenWidth - VIEWPORT_LEFT, ScreenHeight - VIEWPORT_TOP);
	m_pSoin->setParent(this);
	m_pSoin->setVisible(false);
}

CMenuPanel* TeamFortressViewport :: ShowSoin()
{
#ifdef NOATTACKFIXPATH_INVASION_VGUI
		hlinv_isAttackSuspended = true; //We need to prevent attacks while VGUI is active, modif de Roy
		gEngfuncs.Con_Printf ( "client.dll : hlinv_isAttackSuspended SETTING TRUE :  > %i\n", hlinv_isAttackSuspended );	//alertatconsole
#endif
    m_pSoin->Reset();
    return m_pSoin;
}



//---------------------------------------------------------



class CHandler_Soin : public InputSignal
{
private:
	int m_iBodyPart;
	
	CSoin *p;


public:
	CHandler_Soin( CSoin *pSoin, int i )
	{
		p = pSoin;
		m_iBodyPart = i;
	}
		
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
	void mousePressed(MouseCode code,Panel* panel);

};


void CHandler_Soin :: mousePressed(MouseCode code,Panel* panel)
{
	if ( m_iBodyPart == -1 )
	{
		int commande = 0;

		for ( int i=0; i<7; i++ )
		{
			if ( p->m_iDigit [i] == 1 )
				commande |= (1<<i);
		}

		char buf [64];
		sprintf ( buf, "soin %i", commande );

		gEngfuncs.pfnClientCmd(buf);

		return;
	}

	if ( p->m_iDigit [m_iBodyPart] == 0 )
	{
		p->m_iDigit [m_iBodyPart] = 1;

		p->m_pDigit [m_iBodyPart]->m_pTGA = LoadTGA("bout1");
		p->m_pDigit [m_iBodyPart]->setImage( p->m_pDigit [m_iBodyPart]->m_pTGA );
	}
	else
	{
		p->m_iDigit [m_iBodyPart] = 0;

		p->m_pDigit [m_iBodyPart]->m_pTGA = LoadTGA("bout0");
		p->m_pDigit [m_iBodyPart]->setImage( p->m_pDigit [m_iBodyPart]->m_pTGA );
	}

	gEngfuncs.pfnClientCmd("ordimenu 1" );
}


//-----------------------------------------------------------

CSoin :: CSoin(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// fenetre

	m_pWindow = new CTransparentPanel(
		0,
		CADRE_LEFT,
		CADRE_TOP,
		403,
		396
		);

    m_pWindow->setParent( this );
    m_pWindow->setBorder( new LineBorder( Color(255,255,255,100) ) );

	// image

	m_pImage = new 	CImageLabel( "soin", TARGA_LEFT, TARGA_TOP );
	m_pImage->setParent( m_pWindow );

	m_pOK = new CImageLabel( "bout2", OK_LEFT, OK_TOP );
	m_pOK->setParent( m_pWindow );

	m_pOK->addInputSignal ( new CHandler_Soin ( this, -1 ) );


	m_pDigit [0] = new CImageLabel( "bout0", COLONE2, LIGNE1 );
	m_pDigit [1] = new CImageLabel( "bout0", COLONE2, LIGNE2 );
	m_pDigit [2] = new CImageLabel( "bout0", COLONE2, LIGNE4 );
	m_pDigit [3] = new CImageLabel( "bout0", COLONE1, LIGNE3 );
	m_pDigit [4] = new CImageLabel( "bout0", COLONE2, LIGNE3 );
	m_pDigit [5] = new CImageLabel( "bout0", COLONE1, LIGNE5 );
	m_pDigit [6] = new CImageLabel( "bout0", COLONE2, LIGNE5 );

	for ( int i=0; i<7; i++ )
	{
		m_pDigit [i]->setParent ( m_pWindow );
		m_pDigit [i]->addInputSignal ( new CHandler_Soin ( this, i ) );
	}

	for ( int i=0; i<7; i++ )
		m_iDigit [i] = 0;



}

void CSoin::Initialize( void )
{

/*	for ( int i=0; i<7; i++ )
	{			
		m_pDigit [i]->m_pTGA = LoadTGA("bout0");
		m_pDigit [i]->setImage( m_pDigit [i]->m_pTGA );
	}

	for ( int i=0; i<7; i++ )
		m_iDigit [i] = 0;
*/

}


/*
ARMOR_HEAD 0;
ARMOR_CHEST 1;
ARMOR_STOMACH 2;
ARMOR_ARM_R 3;
ARMOR_ARM_L 4;
ARMOR_LEG_R 5;
ARMOR_LEG_L 6;
*/


void CSoin::Reset( void )
{
	Initialize ();
    CMenuPanel::Reset();	
}

