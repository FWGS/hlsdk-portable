//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					vgui_radio.h						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- header de la radio									---
//---------------------------------------------------------
//---------------------------------------------------------



class CRadio : public CMenuPanel
{
public:
	CTransparentPanel *m_pWindow;		// cadre

	int				m_iHead;
	char			m_cText [256];

	CImageLabel		*m_pTextBox;
	CImageLabel		*m_pHeadTextBox;
	CImageLabel		*m_pHead;

	TextPanel *m_pText;
	TextPanel *m_pRadioheadText;

//	Label *m_pText;
//	Label *m_pRadioheadText;


	// polices

	CSchemeManager *pSchemes;			//polices de caract
	SchemeHandle_t hRadioFont;
	Font *pRadioFont;


	void CreateRadioText ( char source [], char dest [] );



	CRadio(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);	//constructeur
	virtual void Initialize( void );	//aucun int
	virtual void Reset( void );			//idem

};

