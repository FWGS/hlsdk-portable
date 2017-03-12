
#ifndef AGVGUIMAPBROWSER_H
#define AGVGUIMAPBROWSER_H

#include<VGUI_Panel.h>


class AgVGuiMapBrowser : public vgui::Panel
{
private:
  vgui::HeaderPanel * m_pHeaderPanel;
  vgui::TablePanel*   m_pTablePanel;
	CTransparentPanel*  m_pBackgroundPanel;
  Label*              m_pLabel;
  vgui::ScrollPanel*  m_pTextScrollPanel;
  vgui::TextPanel*    m_pTextPanel;
public:
	AgVGuiMapBrowser(int x,int y,int wide,int tall);
public:
	virtual void paintBackground();
  virtual int KeyInput(int down, int keynum, const char *pszCurrentBinding);

  int MsgFunc_MapList(const char *pszName, int iSize, void *pbuf);
  void GetMaps();
  void GetLocalMaps();

  void UpdateMap(const char* pszMap);
};

/*
namespace vgui
{
class Button;
class TablePanel;
class HeaderPanel;
}

class CTransparentPanel;
class CommandButton;

// Scoreboard positions
#define SB_X_INDENT				(20 * ((float)ScreenHeight / 640))
#define SB_Y_INDENT				(20 * ((float)ScreenHeight / 480))

class AgVGuiMapBrowser : public CTransparentPanel
{
private:
	HeaderPanel * _headerPanel;
	TablePanel*  _tablePanel;
	ScrollPanel*  _scrollPanel;

	CommandButton*	   _ChangeMapButton;
	CommandButton*	   _ChangeNextMapButton;
  CommandButton*	   _InfoButton;
	CommandButton*	   _CancelButton;
  CommandButton*		 _NextPageButton;

  void DoNext();

public:
	AgVGuiMapBrowser(int x,int y,int wide,int tall);
public:
	virtual void setSize(int wide,int tall);
  virtual int KeyInput(int down, int keynum, const char *pszCurrentBinding);

  int MsgFunc_MapList(const char *pszName, int iSize, void *pbuf);
  void GetMaps();
};
*/
#endif