//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_keypad.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code duy keypad										---
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



//---------------------------------------------------------

#define TARGA_LEFT		(int)(ScreenWidth / 2) - 100
#define TARGA_TOP		(int)(ScreenHeight / 2) - 148

#define BUTTON_X		39
#define BUTTON_Y		34

#define LIGNE_1			84
#define LIGNE_2			132
#define LIGNE_3			180
#define LIGNE_4			228

#define COLONE_1		22
#define COLONE_2		80
#define COLONE_3		139

#define QUIT_BUTTON		-1
#define ERASE_BUTTON	-2

#define DIGIT_LEFT		( TARGA_LEFT + 50 )
#define DIGIT_TOP		( TARGA_TOP + 28 )

#define DIGIT_WIDTH		24

//---------------------------------------------------------

class CHandler_Keypad : public InputSignal
{
private:
	CKeypad	*p;
	int m_iX;
	int m_iY;
#ifdef DOUBLECLICKFIXPATH_INVASION_VGUI
	int skipTime = 0; //Just an integer to store double-press fix, modif de Roy
#endif

public:
	CHandler_Keypad( CKeypad *pKeypad )
	{
		p = pKeypad;
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

	void cursorMoved(int x,int y,Panel* panel) { m_iX = x ; m_iY = y; };
	void mousePressed(MouseCode code,Panel* panel);

};


void	CHandler_Keypad :: mousePressed(MouseCode code,Panel* panel)
{ 
#ifdef DOUBLECLICKFIXPATH_INVASION_VGUI
	if(skipTime>0){
		skipTime--;
		return;
	}
	skipTime = 1; //Skip every second press modif de Roy
#endif
	int colone = 0;
	int ligne = 0;

	if ( m_iX > COLONE_3 && m_iX < COLONE_3 + BUTTON_X )
		colone = 3;
	else if ( m_iX > COLONE_2 && m_iX < COLONE_2 + BUTTON_X )
		colone = 2;
	else if ( m_iX > COLONE_1 && m_iX < COLONE_1 + BUTTON_X )
		colone = 1;

	if ( m_iY > LIGNE_4 &&  m_iY < LIGNE_4 + BUTTON_Y )
		ligne = 4;
	else if ( m_iY > LIGNE_3 &&  m_iY < LIGNE_3 + BUTTON_Y )
		ligne = 3;
	else if ( m_iY > LIGNE_2 &&  m_iY < LIGNE_2 + BUTTON_Y )
		ligne = 2;
	else if ( m_iY > LIGNE_1 &&  m_iY < LIGNE_1 + BUTTON_Y )
		ligne = 1;


	if ( colone != 0 && ligne != 0 )
	{
		int pad [4] [3] = { { 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }, { QUIT_BUTTON, 0, ERASE_BUTTON } };

		int number = pad [ligne-1] [colone-1];


		// commandes sp

		if ( number == QUIT_BUTTON )
		{
	/*		// blip
			gEngfuncs.pfnClientCmd("ordimenu 1" );
			gViewPort->HideTopMenu();
			return;
	*/
/*		 CMenuHandler_TextWindow *pClose = new CMenuHandler_TextWindow(HIDE_TEXTWINDOW);
		 pClose->actionPerformed ( p );
*/
			
			
//			p->m_pCancelButton->doClick();
			return;
		
		}
		else if ( number == ERASE_BUTTON )
		{
			// blip
			gEngfuncs.pfnClientCmd("ordimenu 1" );
			p->Initialize ();
			return;
		}

		// chiffres

		int pos = 0;

		for ( int i=0; i<4; i++ )
		{
			if ( p->m_iDigit[i] != -1 )
				pos ++;
			else break;
		}

		// complet

		if ( pos == 4 )
			return;

		// nouveau chiffre

		p->m_iDigit [pos] = number;

		// nouvelle image

		char buf [32];
		sprintf ( buf, "%i", number );


		p->m_pDigit [pos]-> m_pTGA = LoadTGA(buf);
		p->m_pDigit [pos]->setImage( p->m_pDigit [pos]->m_pTGA );


		// contr

		if ( pos == 3 )
		{
			int combi = p->m_iDigit[0]*1000 + p->m_iDigit[1]*100 + p->m_iDigit[2]*10 + p->m_iDigit[3];

			if ( combi == p->m_iCode && combi != 0 )
			{
				// bonne combi
				// blip
				gEngfuncs.pfnClientCmd("ordimenu 1" );

				char buf2[32];
				sprintf ( buf2, "keypad %i", p->m_iEnt );
				gEngfuncs.pfnClientCmd( buf2 );
			}
			else
			{
				gEngfuncs.pfnClientCmd("keypad 0" );
			}

			return;
		}

		else
		{
			// blip
			gEngfuncs.pfnClientCmd("ordimenu 1" );
		}


	}
}



//-----------------------------------------------------------

CKeypad :: CKeypad(int iTrans, int iRemoveMe, int x,int y,int wide,int tall) : CMenuPanel(iTrans, iRemoveMe, x,y,wide,tall)
{
	// image

	m_pImage = new 	CImageLabel( "keypad", TARGA_LEFT, TARGA_TOP );
	m_pImage->setParent( this );
	m_pImage->addInputSignal ( new CHandler_Keypad ( this ) );

	// d

	m_pCancelButton = new CommandButton( "", COLONE_1, LIGNE_4, BUTTON_X, BUTTON_Y, true );
	m_pCancelButton->setParent ( m_pImage );
	m_pCancelButton->setFgColor(0,0,0,255);
	m_pCancelButton->setBgColor(0,0,0,255);

	m_pCancelButton->setButtonBorderEnabled( false );
	m_pCancelButton->setPaintBorderEnabled ( false );
	m_pCancelButton->setPaintBackgroundEnabled( false );

	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow(HIDE_TEXTWINDOW) );


	Initialize();	// ?


}

//====================================================


void CKeypad::Initialize( void )
{
	for (int i=0; i<4; i++ )
	{
		if ( m_pDigit[i] != NULL )
		{
			m_pDigit [i]-> m_pTGA = LoadTGA("");
			m_pDigit [i]->setImage( 	m_pDigit [i]->m_pTGA );
		}

		else
		{
			m_pDigit [i] = new CImageLabel( "", DIGIT_LEFT + i * DIGIT_WIDTH, DIGIT_TOP );
			m_pDigit [i]->setParent ( this );
		}
	}

	for ( int i=0; i<4; i++ )
		m_iDigit [i] = -1;
}



void CKeypad::Reset( void )
{
	Initialize ();

    CMenuPanel::Reset();
	
}
