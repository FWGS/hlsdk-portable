//++ BulliT

#if !defined(_AG_IRC_HUD_)
#define _AG_IRC_HUD_

#include "irc.h"
#include "AgIRC.h"

class AgHudIRC: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);

  void IRCConnect(bool bSilent);

public:
  void UserCmd_IRCConnect();
  void UserCmd_IRCConnect2();
  void UserCmd_IRCDisconnect();
  void UserCmd_IRCCommand();
  void UserCmd_IRCToggle();
};

#endif //_AG_IRC_HUD_

//-- Martin Webrant
