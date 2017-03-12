// AgIRC.h: interface for the AgIRC class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AGIRC_H__B955D0B4_C876_4C79_BB48_522114B04B53__INCLUDED_)
#define AFX_AGIRC_H__B955D0B4_C876_4C79_BB48_522114B04B53__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

using namespace irc;
class AgIRC : public CIrcDefaultMonitor 
{
protected:
  bool m_bSilent;
  AgString m_sChannel;
  bool OnIrc_YOURHOST(const CIrcMessage* pmsg);
	bool OnIrc_NICK(const CIrcMessage* pmsg);
	bool OnIrc_PRIVMSG(const CIrcMessage* pmsg);
	bool OnIrc_JOIN(const CIrcMessage* pmsg);
	bool OnIrc_PART(const CIrcMessage* pmsg);
	bool OnIrc_KICK(const CIrcMessage* pmsg);
  bool OnIrc_MODE(const CIrcMessage* pmsg);
	bool OnIrc_QUIT(const CIrcMessage* pmsg);

	virtual void OnIrcDefault(const CIrcMessage* pmsg);
	virtual void OnIrcDisconnected();

  void OnMessage(const CIrcMessage* pmsg);
  void PrintMessage(AgString sMessage);

	DEFINE_IRC_MAP()

public:
	AgIRC();
	virtual ~AgIRC();

public:
  void  SilentMode();
  bool  Connect(const CIrcSessionInfo* psi);
  bool  Command(AgString sCommand);
  bool  Disconnect(const AgString& sMessage);
};

#endif // !defined(AFX_AGIRC_H__B955D0B4_C876_4C79_BB48_522114B04B53__INCLUDED_)
