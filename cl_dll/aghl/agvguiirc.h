//++ BulliT

#if !defined(_AG_VGUI_IRC_)
#define _AG_VGUI_IRC_

#include<stdarg.h>
#include<VGUI_Panel.h>

namespace vgui
{
class TextEntry;
class TextPanel;
class EditPanel;
}

class AGVGuiIRC : public vgui::Panel
{
private:
	vgui::TextEntry*    m_pTextEntry;
  vgui::TextPanel*    m_pTextPanel;
  CommandButton*      m_pConnect;

  AgString            m_sText;

public:
	AGVGuiIRC(int x,int y,int wide,int tall);
public:
	virtual void doExecCommand();
	virtual void doConnectCommand();
	virtual void paintBackground();

  virtual int KeyInput(int down, int keynum, const char *pszCurrentBinding);
  void PrintMessage(const char* pszText);
};

#endif //_AG_VGUI_IRC_

//-- Martin Webrant
