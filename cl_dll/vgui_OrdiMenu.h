/************************************************************************
*																		*
*			vgui_OrdiMenu.h , par Julien									*
*																		*
************************************************************************/
// menu vgui pour le contr




#define ORDIMENU_WINDOW_X                   XRES(10)
#define ORDIMENU_WINDOW_Y                   YRES(10)
#define ORDIMENU_WINDOW_SIZE_X				XRES(620)
#define ORDIMENU_WINDOW_SIZE_Y				YRES(460)

#define ORDIMENU_CANCEL_BUTTON_X			XRES(20)
#define ORDIMENU_CANCEL_BUTTON_Y			YRES(400)

#define ORDIMENU_STANDART_BUTTON_SIZE_X       XRES(150)
#define ORDIMENU_STANDART_BUTTON_SIZE_Y       YRES(20)

#define ORDIMENU_TITLE_X					XRES(20)
#define ORDIMENU_TITLE_Y					YRES(20)

#define ORDIMENU_TEXT_X						XRES(300)
#define ORDIMENU_TEXT_Y						YRES(70)
#define ORDIMENU_TEXT_SIZE_X				XRES(300)
#define ORDIMENU_TEXT_SIZE_Y				YRES(370)

#define ORDIMENU_GBUTTONS_X					ORDIMENU_CANCEL_BUTTON_X
#define ORDIMENU_GBUTTONS_Y					YRES(70)

#define MAX_GBUTTONS						7
#define MAX_DBUTTONS						3
#define MAX_TEXTS							11


#define MENU_ACCUEIL						0
#define MENU_PRINCIPAL						1
#define MENU_REFUSE							2
#define MENU_MESSAGE						3
#define MENU_INFO							4
#define MENU_KEYPAD_CAM						5

#define HANDLER_REFRESH						1

class COrdiMenuPanel : public CMenuPanel
{
public:
	CTransparentPanel *m_pWindow;		// cadre
	Label *m_pTitle;					//titre

	CommandButton *m_pCancelButton;					//boutton d' annulation
	CommandButton *m_pGButton [ MAX_GBUTTONS ];		//autres bouttons de gauche
	CommandButton *m_pKeypad [ 10 ];					//pav



	TextPanel *m_pText[MAX_TEXTS];				//texte
	TextPanel *m_pBorder[7];			//cadre contenant le txt ( menu deroulant en option )

	CSchemeManager *pSchemes;			//polices de caract
	SchemeHandle_t hTitleScheme;
	SchemeHandle_t hInfoText;
	Font *pTitleFont;
	Font *pTextFont;
	int r[4], g[4], b[4], a[4];

	int keytab [4];						// code
	int m_iCurrentMenu;					// menu affich
	int m_iID;							// numero de l ordinateur




	COrdiMenuPanel(int iTrans, int iRemoveMe, int x,int y,int wide,int tall);	//constructeur
	virtual void Initialize( void );
	virtual void Reset( void );
	void Refresh ( void );
};

