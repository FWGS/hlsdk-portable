//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_keypad.h						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code du keypad										---
//---------------------------------------------------------
//---------------------------------------------------------



class CKeypad : public CMenuPanel
{
public:
	CTransparentPanel *m_pWindow;		// cadre

	CommandButton *m_pCancelButton;					//boutton d' annulation

	int				m_iCode;
	int				m_iEnt;
	CImageLabel		*m_pImage;

	CImageLabel		*m_pDigit	[4];
	int				m_iDigit	[4];


	CKeypad(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);	//constructeur
	virtual void Initialize( void );	//aucun int
	virtual void Reset( void );			//idem
};

