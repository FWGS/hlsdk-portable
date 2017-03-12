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
#include "AGVGuiWinamp.h"

static HWND hwnd = NULL;

struct WINAMP
{
  char szCommand[16];
  UINT uiMessage;
  char szCommandButton[16];
};

static WINAMP s_Commands[]=
{
  "play"     ,40045, "Play",
  "pause"    ,40046, "Pause",
  "stop"     ,40047, "Stop",
  "next"     ,40048, "Next Track",
  "prev"     ,40044, "Previous Track",
  "playcd"   ,40323, "Play CD",
  "increase" ,40058, "Increase Vol.",
  "decrease" ,40059, "Decrease Vol.",
  "repeat"   ,40022, "Toggle Repeat",
  "shuffle"  ,40023, "Toggle Shuffle",
  "forward"  ,40148, "Fast Forward",
  "rewind"   ,40144, "Fast Rewind",
};

using namespace vgui;

namespace
{
  class WinampCommandHandler : public ActionSignal
  {
  private:
    char m_szCommand[256];
  public:
	  WinampCommandHandler(const char* szCommand)
	  {
		  strcpy(m_szCommand,szCommand);
	  }
  public:
	  virtual void actionPerformed(Panel* panel)
	  {
	    gEngfuncs.pfnClientCmd(m_szCommand);
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
   		gViewPort->ToggleWinamp();
	  }
  };
}

#define WINAMP_TITLE_X		XRES(16)
#define WINAMP_TITLE_Y		YRES(16)

#define WINAMP_TOPLEFT_BUTTON_X		XRES(80)
#define WINAMP_TOPLEFT_BUTTON_Y		YRES(60)
#define WINAMP_BUTTON_SIZE_X			XRES(124)
#define WINAMP_BUTTON_SIZE_Y			YRES(24)
#define WINAMP_BUTTON_SPACER_Y		YRES(8)
#define WINAMP_BUTTON_SPACER_X		XRES(8)

AGVGuiWinamp::AGVGuiWinamp(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
{
  setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0)) );
	
	// Get the scheme used for the Titles
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();

	// schemes
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
//	SchemeHandle_t hIRCText = pSchemes->getSchemeHandle( "Briefing Text" );

	// color schemes
	int r, g, b, a;

	// Create the title
	m_pLabel = new Label( "AG Winamp", WINAMP_TITLE_X, WINAMP_TITLE_Y );
	m_pLabel->setParent( this );
	m_pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	m_pLabel->setFont( Scheme::sf_primary1 );

	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setFgColor( r, g, b, a );
	m_pLabel->setFgColor( Scheme::sc_primary1 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setBgColor( r, g, b, a );
	m_pLabel->setContentAlignment( vgui::Label::a_west );
	m_pLabel->setText("AG Winamp");

	int iXSize,iYSize;
	getSize( iXSize,iYSize );

  for (int i = 0; i < sizeof(s_Commands)/sizeof(s_Commands[0]); i++)
  {
    char szCommand[256];
    sprintf(szCommand,"winamp %s",s_Commands[i].szCommand);

    int iXPos = WINAMP_TOPLEFT_BUTTON_X;
	  int iYPos = WINAMP_TOPLEFT_BUTTON_Y + ( (WINAMP_BUTTON_SIZE_Y + WINAMP_BUTTON_SPACER_Y) * i );
    if (i > 5)
    {
      iXPos += WINAMP_BUTTON_SIZE_X + WINAMP_BUTTON_SPACER_X;
      iYPos = WINAMP_TOPLEFT_BUTTON_Y + ( (WINAMP_BUTTON_SIZE_Y + WINAMP_BUTTON_SPACER_Y) * (i-6) );
    }

    CommandButton* pPlay = new CommandButton(s_Commands[i].szCommandButton,iXPos, iYPos, WINAMP_BUTTON_SIZE_X, WINAMP_BUTTON_SIZE_Y);
    pPlay->addActionSignal(new WinampCommandHandler(szCommand));
	  pPlay->setParent(this);
  }

  CommandButton* pClose = new CommandButton("Close",iXSize - XRES(16) - CMENU_SIZE_X, iYSize - YRES(16) - BUTTON_SIZE_Y, CMENU_SIZE_X, BUTTON_SIZE_Y);
  pClose->addActionSignal(new CloseHandler());
	pClose->setParent(this);
}

void GetSongTitle(LPSTR pszSong, int iSize)
{
  GetWindowText(hwnd,pszSong,iSize);
  char* p = pszSong + strlen(pszSong)-8;
  while (p >= pszSong)
  {
    if (!strnicmp(p,"- Winamp",8)) 
      break;
    p--;
  }
  if (p >= pszSong)
    p--;
  while (p >= pszSong && *p == ' ') 
    p--; 
  *++p = 0;
}

static DWORD dwTime = 0;
void AGVGuiWinamp::paintBackground()
{
	// Transparent black background
	drawSetColor( 0,0,0, 100 );
	drawFilledRect(0,0,_size[0],_size[1]);

  if (NULL == hwnd)
    return;

  if (dwTime > GetTickCount())
    return;

  char szSong[2048];
  GetSongTitle(szSong,sizeof(szSong));
  if (0 == strlen(szSong))
  {
    dwTime = GetTickCount() + 10000;
  }
  else
  {
    m_pLabel->setText(szSong);
    dwTime = GetTickCount() + 1000;
  }
}

void AGVGuiWinamp::UserCmd_Winamp()
{
  if (NULL == hwnd)
    hwnd = FindWindow("Winamp v1.x",NULL); 
  
  if (!::IsWindow(hwnd)) 
  {
    ConsolePrint("Could not find Winamp window.\n");
    hwnd = NULL;
    return;
  }
 
  if (gEngfuncs.Cmd_Argc() == 1)
  {
    char szSong[2048];
    GetSongTitle(szSong,sizeof(szSong));
    strcat(szSong,"\n");
    ConsolePrint(szSong);
    return;
  }

  if (gEngfuncs.Cmd_Argc() < 2)
    return;

  if (3 == gEngfuncs.Cmd_Argc() && 0 == strcmp("volume",gEngfuncs.Cmd_Argv(1)))
  {
    PostMessage(hwnd,WM_USER,atoi(gEngfuncs.Cmd_Argv(2)),122);
  }
  else
  {
    for (int i = 0; i < sizeof(s_Commands)/sizeof(s_Commands[0]); i++)
    {
      if (0 == strcmp(s_Commands[i].szCommand,gEngfuncs.Cmd_Argv(1)))
      {
        PostMessage(hwnd,WM_COMMAND,s_Commands[i].uiMessage,0);
        break;
      }
    }
  }
}


int	AGVGuiWinamp::KeyInput(int down, int keynum, const char *pszCurrentBinding)
{
  if (!down)
    return 1;

  if (!isVisible())
    return 1;

  if (K_ESCAPE == keynum || pszCurrentBinding && 0 == _stricmp("togglewinamp",pszCurrentBinding))
  {
		gViewPort->ToggleWinamp();
    return 0;
  }

  if (K_MWHEELUP == keynum || K_MWHEELDOWN == keynum)
  {
    if (NULL == hwnd)
      hwnd = FindWindow("Winamp v1.x",NULL); 

    if (!::IsWindow(hwnd)) 
    {
      ConsolePrint("Could not find Winamp window.\n");
      hwnd = NULL;
      return 1;
    }

    if (K_MWHEELUP == keynum)
      PostMessage(hwnd,WM_COMMAND,40058,0);
    else
      PostMessage(hwnd,WM_COMMAND,40059,0);

    return 0;
  }

  return 1;
}


//-- Martin Webrant
