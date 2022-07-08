//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_soin.h							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- header du soin s
//---------------------------------------------------------
//---------------------------------------------------------



class CSoin : public CMenuPanel
{
public:
	CTransparentPanel *m_pWindow;		// cadre


	CImageLabel		*m_pImage;
	CImageLabel		*m_pOK;

	CImageLabel		*m_pDigit	[7];
	int				m_iDigit	[7];


	CSoin(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);	//constructeur
	virtual void Initialize( void );	//aucun int
	virtual void Reset( void );			//idem
};

