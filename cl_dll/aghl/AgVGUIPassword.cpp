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
#include "AgVGuiPassword.h"


using namespace vgui;

namespace
{
  class TextHandler : public ActionSignal
  {
  private:
	  
	  AgVGuiPassword* _AgVGuiPassword;

  public:

	  TextHandler(AgVGuiPassword* AgVGuiPassword)
	  {
		  _AgVGuiPassword=AgVGuiPassword;
	  }

  public:

	  virtual void actionPerformed(Panel* panel)
	  {
		  _AgVGuiPassword->doConnect();
	  }
  };

  class ConnectHandler : public ActionSignal
  {
  private:
	  
	  AgVGuiPassword* _AgVGuiPassword;

  public:

	  ConnectHandler(AgVGuiPassword* AgVGuiPassword)
	  {
		  _AgVGuiPassword=AgVGuiPassword;
	  }

  public:

	  virtual void actionPerformed(Panel* panel)
	  {
		  _AgVGuiPassword->doConnect();
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
      if (gViewPort->m_pPassword->isVisible())
        TextEntry::keyPressed(code,panel);
    };
    virtual void keyTyped(KeyCode code,Panel* panel)
    {
      if (gViewPort->m_pPassword->isVisible())
        TextEntry::keyTyped(code,panel);
    };
    virtual void keyReleased(KeyCode code,Panel* panel)
    {
      if (gViewPort->m_pPassword->isVisible())
        TextEntry::keyReleased(code,panel);
    };
  };

}

#define VGUIPASSWORD_TITLE_X		XRES(16)
#define VGUIPASSWORD_TITLE_Y		YRES(16)
#define TEXT_SIZE_Y		YRES(16)

AgVGuiPassword::AgVGuiPassword(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
{
  setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );
	
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
//	SchemeHandle_t hVGUIPasswordText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	m_pLabel = new Label( "", VGUIPASSWORD_TITLE_X, VGUIPASSWORD_TITLE_Y );
	m_pLabel->setParent( this );
	m_pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	m_pLabel->setFont( Scheme::sf_primary1 );

	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setFgColor( r, g, b, a );
	m_pLabel->setFgColor( Scheme::sc_primary1 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setBgColor( r, g, b, a );
	m_pLabel->setContentAlignment( vgui::Label::a_west );
	m_pLabel->setText("Enter Password");

	int iXSize,iYSize;
	getSize( iXSize,iYSize );

  int iTemp = iYSize - YRES(24) - VGUIPASSWORD_TITLE_Y - BUTTON_SIZE_Y; //Hack to get it to work with Visual 7.0 beta 2
	m_pTextEntry = new TextInput("",XRES(16), iTemp, iXSize - 2*XRES(16), TEXT_SIZE_Y);
	m_pTextEntry->setParent(this);
	m_pTextEntry->addActionSignal(new TextHandler(this));
  
  m_pConnect = new CommandButton("Connect",XRES(16), iYSize - YRES(16) - BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
  m_pConnect->addActionSignal(new ConnectHandler(this));
	m_pConnect->setParent(this);

  ReadPasswords();
}


void AgVGuiPassword::doConnect()
{
  char szPassword[256];
  szPassword[0] = '\0';
  m_pTextEntry->getText(0,szPassword,sizeof(szPassword));

  AgAddressToPasswordMap::iterator itrPasswords = m_mapPasswords.find(m_sAddress.c_str());
  if (itrPasswords != m_mapPasswords.end())
    (*itrPasswords).second = szPassword;
  else
    m_mapPasswords.insert(AgAddressToPasswordMap::value_type(m_sAddress,szPassword));
  SavePasswords();

	char szCMD[256];
	sprintf(szCMD,"password %s\n", szPassword);
	ClientCmd(szCMD);

  sprintf(szCMD, "connect %s\n", m_sAddress.c_str() );
	ClientCmd(szCMD);

  gViewPort->HidePassword();
}

void AgVGuiPassword::paintBackground()
{
	// Transparent black background
	drawSetColor( 0,0,0, 100 );
	drawFilledRect(0,0,_size[0],_size[1]);
}

int	AgVGuiPassword::KeyInput(int down, int keynum, const char *pszCurrentBinding)
{
  if (!down)
    return 1;

  if (!isVisible())
    return 1;

  if (K_ESCAPE == keynum)
  {
    gViewPort->HidePassword();
    return 0;
  }

  if (m_pTextEntry->hasFocus())
    return 0;

  return 1;
}


void AgVGuiPassword::Connect(const char* pszHostname, const char* pszAddress, bool bPassworded)
{
  m_sAddress = pszAddress;

  if (!bPassworded)
  {
    doConnect();
    return;
  }
  else
  {
    char szMessage[256];
    sprintf(szMessage,"Enter password for %s",pszHostname);
    m_pLabel->setText(szMessage,strlen(szMessage));

    AgString sPassword;
    AgAddressToPasswordMap::iterator itrPasswords = m_mapPasswords.find(pszAddress);
    if (itrPasswords != m_mapPasswords.end())
      sPassword = (*itrPasswords).second;

    m_pTextEntry->setText(sPassword.c_str(),sPassword.length());
    gViewPort->ShowPassword();
  }
}

void AgVGuiPassword::ReadPasswords()
{
  char	szData[4096];
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/passwords.txt",AgGetDirectory());
  FILE* pFile = fopen(szFile,"r");
  if (!pFile)
    return;

  int iRead = fread(szData,sizeof(char),sizeof(szData)-2,pFile);
  fclose(pFile);
  if (0 >= iRead)
    return;
  szData[iRead] = '\0';

  char* pszPasswordString = strtok( szData, "\n");
  while (pszPasswordString != NULL)
  {
    char szAddress[64],szPassword[64];
    szAddress[0] = '\0';
    szPassword[0] = '\0';
    sscanf(pszPasswordString,"%s %s\n",szAddress,szPassword);
    
    AgString sAddress(szAddress);
    AgString sPassword(szPassword);
    AgTrim(sAddress);
    AgTrim(sPassword);
    m_mapPasswords.insert(AgAddressToPasswordMap::value_type(sAddress,sPassword));
    pszPasswordString = strtok( NULL, "\n");
  }
}

void AgVGuiPassword::SavePasswords()
{
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/passwords.txt",AgGetDirectory());
  FILE* pFile = fopen(szFile,"wb");
  if (!pFile)
  {
    // file error
    char szMsg[128];
    sprintf(szMsg,"Couldn't create/save password file %s.\n",szFile);
    ConsolePrint(szMsg);
    return;
  }
  
  //Loop and write the file.
  for (AgAddressToPasswordMap::iterator itrPasswords = m_mapPasswords.begin() ;itrPasswords != m_mapPasswords.end(); ++itrPasswords)
    fprintf(pFile,"%s %s\n",(*itrPasswords).first.c_str(),(*itrPasswords).second.c_str());
  
  fflush(pFile);
  fclose(pFile);
}

//-- Martin Webrant
