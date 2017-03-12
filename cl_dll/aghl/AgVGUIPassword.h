//++ BulliT

#if !defined(_AG_VGUI_Password_)
#define _AG_VGUI_Password_

#include<stdarg.h>
#include<VGUI_Panel.h>

namespace vgui
{
class TextEntry;
class TextPanel;
class EditPanel;
}

class AgVGuiPassword : public vgui::Panel
{
private:
  vgui::Label*        m_pLabel;
	vgui::TextEntry*    m_pTextEntry;
  CommandButton*      m_pConnect;

  AgString            m_sAddress;
  typedef map<AgString, AgString, less<AgString> > AgAddressToPasswordMap;
  AgAddressToPasswordMap m_mapPasswords;

  void ReadPasswords();
  void SavePasswords();

public:
	AgVGuiPassword(int x,int y,int wide,int tall);
public:
	virtual void doConnect();
	virtual void paintBackground();

  virtual int KeyInput(int down, int keynum, const char *pszCurrentBinding);

  void Connect(const char* pszHostname, const char* pszAddress, bool bPassworded);
};

#endif //_AG_VGUI_Password_

//-- Martin Webrant
