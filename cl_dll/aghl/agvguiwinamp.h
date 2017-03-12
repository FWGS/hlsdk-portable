//++ BulliT

#if !defined(_AG_VGUI_WINAMP_)
#define _AG_VGUI_WINAMP_

#include<stdarg.h>
#include<VGUI_Panel.h>

class AGVGuiWinamp : public vgui::Panel
{
private:
  Label*              m_pLabel;
public:
	AGVGuiWinamp(int x,int y,int wide,int tall);
public:
          void UserCmd_Winamp();
	virtual void paintBackground();
  virtual int KeyInput(int down, int keynum, const char *pszCurrentBinding);
};

#endif //_AG_VGUI_WINAMP_

//-- Martin Webrant
