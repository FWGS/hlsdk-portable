//++ BulliT

#include<VGUI_HeaderPanel.h>
#include<VGUI_TablePanel.h>
#include<VGUI_LineBorder.h>
#include<VGUI_Label.h>
#include<VGUI_Button.h>
#include<VGUI_ActionSignal.h>
#include<VGUI_TextGrid.h>
#include<VGUI_TextEntry.h>
#include<VGUI_EtchedBorder.h>
#include<VGUI_LoweredBorder.h>
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"
#include<VGUI_StackLayout.h>
#include<VGUI_EditPanel.h>

#include "hud.h"
#include "cl_util.h"
#include <keydefs.h>
#include "vgui_TeamFortressViewport.h"
#include "AGVGuiIRC.h"

extern /*irc::*/CIrcSession g_ircSession;

using namespace vgui;

namespace
{
  class TextHandler : public ActionSignal
  {
  private:
	  
	  AGVGuiIRC* _AGVGuiIRC;

  public:

	  TextHandler(AGVGuiIRC* AGVGuiIRC)
	  {
		  _AGVGuiIRC=AGVGuiIRC;
	  }

  public:

	  virtual void actionPerformed(Panel* panel)
	  {
		  _AGVGuiIRC->doExecCommand();
	  }
  };

  class ConnectHandler : public ActionSignal
  {
  private:
	  
	  AGVGuiIRC* _AGVGuiIRC;

  public:

	  ConnectHandler(AGVGuiIRC* AGVGuiIRC)
	  {
		  _AGVGuiIRC=AGVGuiIRC;
	  }

  public:

	  virtual void actionPerformed(Panel* panel)
	  {
		  _AGVGuiIRC->doConnectCommand();
	  }
  };

  class CloseHandler : public ActionSignal
  {
  public:
	  CloseHandler()
	  {
	  }

  public:
	  virtual void actionPerformed(Panel* panel)
	  {
   		gViewPort->ToggleIRC();
	  }
  };


  class TextInput : public vgui::TextEntry
  {
  public:
    TextInput(const char* text,int x,int y,int wide,int tall) : TextEntry(text,x,y,wide,tall)
    {
    };

    virtual void keyPressed(KeyCode code,Panel* panel)
    {
      if (gViewPort->m_pIRC->isVisible())
        TextEntry::keyPressed(code,panel);
    };
    virtual void keyTyped(KeyCode code,Panel* panel)
    {
      if (gViewPort->m_pIRC->isVisible())
        TextEntry::keyTyped(code,panel);
    };
    virtual void keyReleased(KeyCode code,Panel* panel)
    {
      if (gViewPort->m_pIRC->isVisible())
        TextEntry::keyReleased(code,panel);
    };
  };

}

#define IRC_TITLE_X		XRES(16)
#define IRC_TITLE_Y		YRES(16)
#define TEXT_SIZE_Y		YRES(16)

AGVGuiIRC::AGVGuiIRC(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
{
  setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );
	
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hIRCText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	Label *pLabel = new Label( "", IRC_TITLE_X, IRC_TITLE_Y );
	pLabel->setParent( this );
	pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	pLabel->setFont( Scheme::sf_primary1 );

	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pLabel->setFgColor( r, g, b, a );
	pLabel->setFgColor( Scheme::sc_primary1 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pLabel->setBgColor( r, g, b, a );
	pLabel->setContentAlignment( vgui::Label::a_west );
	pLabel->setText("AG IRC Client");

	int iXSize,iYSize;
	getSize( iXSize,iYSize );

  // Create the text panel
	m_pTextPanel = new TextPanel( "", XRES(16), IRC_TITLE_Y*2 + YRES(16), iXSize - XRES(32), iYSize - (YRES(48) + BUTTON_SIZE_Y*2 + TEXT_SIZE_Y*2));
  m_pTextPanel->setParent( this);

	// get the font and colors from the scheme
	m_pTextPanel->setFont( pSchemes->getFont(hIRCText) );
	pSchemes->getFgColor( hIRCText, r, g, b, a );
	m_pTextPanel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hIRCText, r, g, b, a );
	m_pTextPanel->setBgColor( r, g, b, a );

  int iTemp = iYSize - YRES(24) - TEXT_SIZE_Y - BUTTON_SIZE_Y; //Hack to get it to work with Visual 7.0 beta 2
	m_pTextEntry = new TextInput("",XRES(16), iTemp, iXSize - 2*XRES(16), TEXT_SIZE_Y);
	m_pTextEntry->setParent(this);
	m_pTextEntry->addActionSignal(new TextHandler(this));
  
  m_pConnect = new CommandButton("",XRES(16), iYSize - YRES(16) - BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
  m_pConnect->addActionSignal(new ConnectHandler(this));
	m_pConnect->setParent(this);

  CommandButton* pClose = new CommandButton("Close",iXSize - XRES(16) - CMENU_SIZE_X, iYSize - YRES(16) - BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
  pClose->addActionSignal(new CloseHandler());
	pClose->setParent(this);
}

void AGVGuiIRC::doExecCommand()
{
	char buf[2048];
  strcpy(buf,"irc ");
	m_pTextEntry->getText(0,buf+4,2040);
	m_pTextEntry->setText(null,0);

	gEngfuncs.pfnClientCmd(buf);
}

void AGVGuiIRC::doConnectCommand()
{
  m_sText = "";
  m_pTextPanel->setText(m_sText.c_str());
	gEngfuncs.pfnClientCmd(g_ircSession ? "ircdisconnect" : "ircconnect");
}

void AGVGuiIRC::paintBackground()
{
	// Transparent black background
	drawSetColor( 0,0,0, 100 );
	drawFilledRect(0,0,_size[0],_size[1]);
  if (g_ircSession)
    m_pConnect->setText("Disconnect");
  else
    m_pConnect->setText("Connect");
}

int	AGVGuiIRC::KeyInput(int down, int keynum, const char *pszCurrentBinding)
{
  if (!down)
    return 1;

  if (!isVisible())
    return 1;

  if (K_ESCAPE == keynum || pszCurrentBinding && 0 == _stricmp("toggleirc",pszCurrentBinding))
  {
		gViewPort->ToggleIRC();
    return 0;
  }

  if (m_pTextEntry->hasFocus())
    return 0;

  return 1;
}

void AGVGuiIRC::PrintMessage(const char* pszText)
{
  int iWide,iTall;
  m_pTextPanel->getSize(iWide,iTall);
  int iTextWide,iTextTall;
  m_pTextPanel->getTextImage()->getTextSizeWrapped(iTextWide,iTextTall);
  if ((iTextTall + 20) > iTall)
  {
    int iFind = m_sText.find("\n");
    if (NPOS != iFind)
    {
      m_sText = m_sText.substr(iFind+1);
    }
  }

  m_sText += pszText;
  m_sText += "\n";
  m_pTextPanel->setText(m_sText.c_str());
}

//-- Martin Webrant
