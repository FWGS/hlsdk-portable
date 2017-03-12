
#include<VGUI_HeaderPanel.h>
#include<VGUI_TablePanel.h>
#include<VGUI_LineBorder.h>
#include<VGUI_Label.h>
#include<VGUI_Button.h>
#include<VGUI_ActionSignal.h>
#include "VGUI_ScrollPanel.h"
#include "VGUI_TextImage.h"

#include "hud.h"
#include "cl_util.h"

#include "vgui_TeamFortressViewport.h"
#include "AgVGuiMapBrowser.h"
#include "parsemsg.h"
#include <keydefs.h>

static int g_iStartIndex = 0;
static AgString    s_sMapList;
static bool        s_bHaveAllMaps = true;
static AgStringSet s_setMaps;
static AgStringSet s_setLocalMaps;
extern cvar_t* g_pcl_show_local_maps;

const char* GetMap(unsigned int iRow)
{
  if (iRow < s_setMaps.size())
  {
    AgStringSet::iterator itrMaps = s_setMaps.begin();
    for (unsigned int i = 0; i < iRow && itrMaps != s_setMaps.end(); i++, itrMaps++)
    {}
    
    if (itrMaps != s_setMaps.end())
        return (*itrMaps).c_str();
  }
  return NULL;
}


using namespace vgui;

namespace
{
  class MapBrowserTablePanel;

  class MapBrowserTablePanel_InputSignal : public InputSignal
  {
    MapBrowserTablePanel* m_pMapBrowser;
  public:
    MapBrowserTablePanel_InputSignal(MapBrowserTablePanel* pMapBrowser)
    {
      m_pMapBrowser = pMapBrowser;
    }
	  virtual void cursorMoved(int x,int y,Panel* panel) {};
	  virtual void cursorEntered(Panel* panel){};
	  virtual void cursorExited(Panel* Panel) {};
    virtual void mousePressed(MouseCode code,Panel* panel);
    virtual void mouseDoublePressed(MouseCode code,Panel* panel);
 	  virtual void mouseReleased(MouseCode code,Panel* panel) {};
	  virtual void mouseWheeled(int delta,Panel* panel) {};
	  virtual void keyPressed(KeyCode code,Panel* panel) {};
	  virtual void keyTyped(KeyCode code,Panel* panel) {};
	  virtual void keyReleased(KeyCode code,Panel* panel) {};
	  virtual void keyFocusTicked(Panel* panel) {};
  };

  #define CELL_HEIGHT				YRES(15)

  class MapBrowserTablePanel : public TablePanel
  {
  private:
	  Label				*m_pLabel;
	  int					m_nMouseOverRow;

  public:
	  
	  MapBrowserTablePanel( int x,int y,int wide,int tall,int columnCount) : TablePanel( x,y,wide,tall,columnCount)
	  {
		  m_pLabel = new Label( "", 0, 0 );
		  
		  m_nMouseOverRow = 0;
	    setCellEditingEnabled(false);
	  }

  public:
	  void setMouseOverRow( int row )
    {
		  m_nMouseOverRow	= row;

      DoUpdateMap();
	  }

	  void DoChangeMap( void )
	  {
      stopCellEditing();
      DoCancel();
      const char* pszMap = GetMap(m_nMouseOverRow + g_iStartIndex);

      if (pszMap && strlen(pszMap))
      {
        char szCommand[256];
        sprintf(szCommand,"agmap %s",pszMap);
        ServerCmd(szCommand);
      }
	  }

	  void DoChangeNextMap( void )
	  {
      DoCancel();
      const char* pszMap = GetMap(m_nMouseOverRow + g_iStartIndex);

      if (pszMap && strlen(pszMap))
      {
        char szCommand[256];
        sprintf(szCommand,"agnextmap %s",pszMap);
        ServerCmd(szCommand);
      }
	  }
	  
	  void DoCancel( void )
	  {
		  ClientCmd( "togglemapbrowser\n" );
	  }


    void DoPrev( void )
    {
      g_iStartIndex -= getRowCount();
      if (g_iStartIndex < 0)
        g_iStartIndex = 0;

      DoUpdateMap();
    }


    void DoNext( void )
    {
      g_iStartIndex += getRowCount();
      if (g_iStartIndex > (int)s_setMaps.size())
        g_iStartIndex = 0;

      DoUpdateMap();
    }

    void DoUpdateMap( void )
    {
      AgVGuiMapBrowser* pVGUI = (AgVGuiMapBrowser*)getParent();
      pVGUI->UpdateMap(GetMap(m_nMouseOverRow + g_iStartIndex));
    }

	  virtual int getRowCount()
	  {
		  int rowcount;
		  int height, width;

		  getSize( width, height );
		  height = max( 0, height );
		  rowcount = height / CELL_HEIGHT;

		  return rowcount;
	  }

	  virtual int getCellTall(int row)
	  {
		  return CELL_HEIGHT - 2;
	  }
	  
	  virtual Panel* getCellRenderer(int column,int row,bool columnSelected,bool rowSelected,bool cellSelected)
	  {
      const char* pszMap = GetMap(row + g_iStartIndex);
		  if ( row == m_nMouseOverRow )
		  {
        m_pLabel->setFgColor( 255, 255, 255, 0 );
			  
		  }
		  else
		  {
        m_pLabel->setFgColor( 200, 240, 63, 100 );
        if (pszMap && strlen(pszMap))
        {
          AgStringSet::iterator itrLocalMaps = s_setLocalMaps.find(pszMap); 
          if (itrLocalMaps == s_setLocalMaps.end())
              m_pLabel->setFgColor( 200, 0, 0, 100 );
        }
		  }
		  m_pLabel->setBgColor( 0, 0, 0, 200 );
		  m_pLabel->setContentAlignment( vgui::Label::a_west );
		  m_pLabel->setFont( Scheme::sf_primary2 );

		  if ( pszMap )
		  {
			  // Fill out with the correct data
			  switch ( column )
			  {
			  case 0:
          {
					    m_pLabel->setText( pszMap );
          }
			    break;
			  default:
				  break;
			  }
		  }
		  else
		  {
			  if ( !row && !column )
			  {
				  if ( !s_bHaveAllMaps )
				  {
					  m_pLabel->setText( "Please wait..." );
				  }
				  else
				  {
  //					m_pLabel->setText( "Press 'Refresh' to search for servers..." );
				  }
			  }
			  else
			  {
				  m_pLabel->setText( "" );
			  }
		  }
		  
		  return m_pLabel;
	  }

	  virtual Panel* startCellEditing(int column,int row)
	  {
		  return null;
	  }
  };

  enum Action
  {
    Close, More, Previous, Change, ChangeNext,
  };

  class MapBrowserHandler : public ActionSignal
  {
    Action m_act;
    MapBrowserTablePanel* m_pMapBrowser;
  public:
	  MapBrowserHandler(Action act, MapBrowserTablePanel* pMapBrowser)
	  {
      m_act = act;
      m_pMapBrowser = pMapBrowser;
	  }
  public:
	  virtual void actionPerformed(Panel* panel)
	  {
      switch (m_act)
      {
      case Close:
   		  gViewPort->ToggleMapBrowser();
        break;
      case More:
        m_pMapBrowser->DoNext();
        break;
      case Change:
        m_pMapBrowser->DoChangeMap();
        break;
      case ChangeNext:
        m_pMapBrowser->DoChangeNextMap();
        break;
      default:
        break;
      }
	  }
  };

  void MapBrowserTablePanel_InputSignal::mousePressed(MouseCode code,Panel* panel)
  {
	  int x, y;
	  int therow = 2;

	  if ( code != MOUSE_LEFT )
		  return;

	  panel->getApp()->getCursorPos(x,y);
	  panel->screenToLocal( x, y );

	  therow = y / (CELL_HEIGHT);
	  
	  // Figure out which row it's on
	  m_pMapBrowser->setMouseOverRow( therow );
  }

  void MapBrowserTablePanel_InputSignal::mouseDoublePressed(MouseCode code,Panel* panel)
  {
	  int x, y;
	  int therow = 2;

	  if ( code != MOUSE_LEFT )
		  return;

	  panel->getApp()->getCursorPos(x,y);
	  panel->screenToLocal( x, y );

	  therow = y / (CELL_HEIGHT-1);
	  
	  // Figure out which row it's on
	  m_pMapBrowser->setMouseOverRow( therow );
	  m_pMapBrowser->DoChangeMap();
  }
}

#define MAPBROWSER_TITLE_X		XRES(16)
#define MAPBROWSER_TITLE_Y		YRES(16)

#define MAPBROWSER_BUTTON_SIZE_X			XRES(100)
#define MAPBROWSER_BUTTON_SIZE_Y			YRES(24)
#define MAPBROWSER_BUTTON_SPACER_Y		YRES(8)
#define MAPBROWSER_BUTTON_SPACER_X		XRES(8)

#define TABLE_X			    XRES(8)
#define TABLE_Y			    YRES(60)
#define HEADER_SIZE_X			XRES(100)
#define HEADER_SIZE_Y			0 //YRES(18) 

#define NUM_COLUMNS       1

AgVGuiMapBrowser::AgVGuiMapBrowser(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
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
	m_pLabel = new Label( "", MAPBROWSER_TITLE_X, MAPBROWSER_TITLE_Y );
	m_pLabel->setParent( this );
	m_pLabel->setFont( pSchemes->getFont(hTitleScheme) );
	m_pLabel->setFont( Scheme::sf_primary1 );

	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setFgColor( r, g, b, a );
//	m_pLabel->setFgColor( Scheme::sc_primary1 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pLabel->setBgColor( r, g, b, a );
	m_pLabel->setContentAlignment( vgui::Label::a_west );
	m_pLabel->setText("AG Map Browser");

	int iXSize,iYSize;
	getSize( iXSize,iYSize );

  Label* pHeaderLabel = new Label("Maps");
	pHeaderLabel->setContentAlignment( vgui::Label::a_west );
	pHeaderLabel->setFgColor( Scheme::sc_primary1 );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pHeaderLabel->setBgColor( r, g, b, a );
	pHeaderLabel->setFont( Scheme::sf_primary2 );

	m_pHeaderPanel= new HeaderPanel(TABLE_X,TABLE_Y,HEADER_SIZE_X,HEADER_SIZE_Y);
	m_pHeaderPanel->setParent(this);
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pHeaderPanel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pHeaderPanel->setBgColor( r, g, b, a );
	m_pHeaderPanel->addSectionPanel(pHeaderLabel);
	m_pHeaderPanel->setSliderPos( 0, HEADER_SIZE_X );

  m_pTablePanel = new MapBrowserTablePanel( TABLE_X, TABLE_Y + HEADER_SIZE_Y, HEADER_SIZE_X, iYSize - (TABLE_Y + HEADER_SIZE_Y + MAPBROWSER_BUTTON_SIZE_Y*2 + MAPBROWSER_BUTTON_SPACER_Y), NUM_COLUMNS );
	m_pTablePanel->setParent(this);
	m_pTablePanel->setHeaderPanel(m_pHeaderPanel);
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTablePanel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTablePanel->setBgColor( r, g, b, a );
	m_pTablePanel->addInputSignal(new MapBrowserTablePanel_InputSignal((MapBrowserTablePanel*)m_pTablePanel));

	int bw = MAPBROWSER_BUTTON_SIZE_X;
  int bh = MAPBROWSER_BUTTON_SIZE_Y;
	int btny = iYSize - YRES(16) - BUTTON_SIZE_Y;//tall - MAPBROWSER_BUTTON_SIZE_Y - MAPBROWSER_BUTTON_SPACER_Y; 
	int btnx = TABLE_X;

  CommandButton* pMore = new CommandButton("More",btnx, btny, bw, bh);
  pMore->addActionSignal(new MapBrowserHandler(More,(MapBrowserTablePanel*)m_pTablePanel));
  pMore->setParent(this);

  btnx += MAPBROWSER_BUTTON_SPACER_X + MAPBROWSER_BUTTON_SIZE_X;

  CommandButton* pChange = new CommandButton("Change Now",btnx, btny, bw, bh);
  pChange->addActionSignal(new MapBrowserHandler(Change,(MapBrowserTablePanel*)m_pTablePanel));
  pChange->setParent(this);

  btnx += MAPBROWSER_BUTTON_SPACER_X + MAPBROWSER_BUTTON_SIZE_X;

  CommandButton* pChangeNext = new CommandButton("Change Next",btnx, btny, bw, bh);
  pChangeNext->addActionSignal(new MapBrowserHandler(ChangeNext,(MapBrowserTablePanel*)m_pTablePanel));
  pChangeNext->setParent(this);

  btnx += MAPBROWSER_BUTTON_SPACER_X + MAPBROWSER_BUTTON_SIZE_X;

  CommandButton* pClose = new CommandButton("Close",btnx, btny, bw, bh);
  pClose->addActionSignal(new MapBrowserHandler(Close,(MapBrowserTablePanel*)m_pTablePanel));
  pClose->setParent(this);

  // Create the Scroll panel
	m_pTextScrollPanel = new CTFScrollPanel( TABLE_X + HEADER_SIZE_X + XRES(16), TABLE_Y + HEADER_SIZE_Y, iXSize - (TABLE_X + HEADER_SIZE_X + XRES(16)), iYSize - (TABLE_Y + HEADER_SIZE_Y + MAPBROWSER_BUTTON_SIZE_Y + MAPBROWSER_BUTTON_SPACER_Y + MAPBROWSER_TITLE_Y + YRES(8)));
	m_pTextScrollPanel->setParent(this);
	m_pTextScrollPanel->setScrollBarVisible(false, false);
  m_pTextScrollPanel->setScrollBarAutoVisible(true, true);


	// Create the text panel
	m_pTextPanel = new TextPanel( "", 0,0, 64,64);
	m_pTextPanel->setParent( m_pTextScrollPanel->getClient() );

	// get the font and colors from the scheme
	m_pTextPanel->setFont( pSchemes->getFont(hIRCText) );
	pSchemes->getFgColor( hIRCText, r, g, b, a );
	m_pTextPanel->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hIRCText, r, g, b, a );
	m_pTextPanel->setBgColor( r, g, b, a );
	m_pTextPanel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Map_Description_not_available"));

	m_pTextScrollPanel->setScrollValue( 0, 0 );
}


void AgVGuiMapBrowser::paintBackground()
{
	// Transparent black background
	drawSetColor( 0,0,0, 100 );
	drawFilledRect(0,0,_size[0],_size[1]);
}

void AgVGuiMapBrowser::UpdateMap(const char* pszMap)
{
  char* pText = NULL;

  if (pszMap)
  {
    char szMapFile[MAX_PATH];
    sprintf(szMapFile,"maps/%s.txt",pszMap);
	  pText = (char*)gEngfuncs.COM_LoadFile(szMapFile, 5, NULL);
  }

	//force the scrollbars on so clientClip will take them in account after the validate
	m_pTextScrollPanel->setScrollBarAutoVisible(false, false);
	m_pTextScrollPanel->setScrollBarVisible(true, true);
	m_pTextScrollPanel->validate();

  if (pText)
    m_pTextPanel->setText(pText);
  else
    m_pTextPanel->setText(gHUD.m_TextMessage.BufferedLocaliseTextString("#Map_Description_not_available"));


	// Get the total size of the MOTD text and resize the text panel
	int iScrollSizeX, iScrollSizeY;

	// First, set the size so that the client's wdith is correct at least because the
	//  width is critical for getting the "wrapped" size right.
	// You'll see a horizontal scroll bar if there is a single word that won't wrap in the
	//  specified width.
	m_pTextPanel->getTextImage()->setSize(m_pTextScrollPanel->getClientClip()->getWide(), m_pTextScrollPanel->getClientClip()->getTall());
	m_pTextPanel->getTextImage()->getTextSizeWrapped( iScrollSizeX, iScrollSizeY );
	
	// Now resize the textpanel to fit the scrolled size
	m_pTextPanel->setSize( iScrollSizeX , iScrollSizeY );

	//turn the scrollbars back into automode
	m_pTextScrollPanel->setScrollBarAutoVisible(true, true);
	m_pTextScrollPanel->setScrollBarVisible(false, false);
	m_pTextScrollPanel->setScrollValue( 0, 0 );

	m_pTextScrollPanel->validate();

  if (pText)
		gEngfuncs.COM_FreeFile(pText);
}

int AgVGuiMapBrowser::MsgFunc_MapList( const char *pszName, int iSize, void *pbuf )
{
	if (s_bHaveAllMaps)
    s_sMapList = "";

	BEGIN_READ( pbuf, iSize );
	s_bHaveAllMaps = 0 == READ_BYTE();
  s_sMapList += READ_STRING();

  if (s_bHaveAllMaps)
  {
    s_setMaps.empty();
    AgToLower(s_sMapList);

    int iStart = 0;
    int iEnd = 0;
    do
    {
      iEnd = s_sMapList.find("#",iStart);

      if (-1 != iEnd)
      {
        AgString sMap = s_sMapList.substr(iStart,iEnd-iStart);
        AgTrim(sMap);
        if (sMap.length())
        {
          s_setMaps.insert(sMap);
        }
        iStart = iEnd + 1;
      }
    }
    while (-1 != iEnd);

    s_setMaps.insert("boot_camp");
    s_setMaps.insert("bounce");
    s_setMaps.insert("datacore");
    s_setMaps.insert("lambda_bunker");
    s_setMaps.insert("snark_pit");
    s_setMaps.insert("stalkyard");
    s_setMaps.insert("subtransit");
    s_setMaps.insert("undertow");

    ((MapBrowserTablePanel*)m_pTablePanel)->DoUpdateMap();

    s_sMapList = "";
  }

	return 1;
}

void AgVGuiMapBrowser::GetMaps()
{
  if (s_bHaveAllMaps && 0 == s_setMaps.size())
  {
    if (0 == s_setLocalMaps.size())
      GetLocalMaps();
    ServerCmd("maplist\n");
  }
}

void AgVGuiMapBrowser::GetLocalMaps()
{
  char szDirAG[MAX_PATH];
  char szDirVALVE[MAX_PATH];
  
  strcpy(szDirAG,AgGetDirectory());
  strcat(szDirAG,"/maps");
  strcpy(szDirVALVE,AgGetDirectoryValve());
  strcat(szDirVALVE,"/maps");
  
  AgStringSet setFiles;
  AgStringSet::iterator itrFiles;
  
  AgDirList(szDirAG,setFiles);
  AgDirList(szDirVALVE,setFiles);
  
  for (itrFiles = setFiles.begin() ;itrFiles != setFiles.end();++itrFiles)
  {
    AgString sFile = *itrFiles;
    AgToLower(sFile);
    if (!strstr(sFile.c_str(),".bsp"))
      continue;
    sFile = sFile.substr(0,sFile.length()-4);
    AgTrim(sFile);
    s_setLocalMaps.insert(sFile);
  }

  s_setLocalMaps.insert("boot_camp");
  s_setLocalMaps.insert("bounce");
  s_setLocalMaps.insert("datacore");
  s_setLocalMaps.insert("lambda_bunker");
  s_setLocalMaps.insert("snark_pit");
  s_setLocalMaps.insert("stalkyard");
  s_setLocalMaps.insert("subtransit");
  s_setLocalMaps.insert("undertow");
}


int	AgVGuiMapBrowser::KeyInput(int down, int keynum, const char *pszCurrentBinding)
{
  if (!down)
    return 1;

  if (!isVisible())
    return 1;

  if (K_ESCAPE == keynum || pszCurrentBinding && 0 == _stricmp("togglemapbrowser",pszCurrentBinding))
  {
		gViewPort->ToggleMapBrowser();
    return 0;
  }

  if (K_MWHEELUP == keynum)
  {
    ((MapBrowserTablePanel*)m_pTablePanel)->DoPrev();
    return 0;
  }

  if (K_MWHEELDOWN == keynum)
  {
    ((MapBrowserTablePanel*)m_pTablePanel)->DoNext();
    return 0;
  }

  return 1;
}