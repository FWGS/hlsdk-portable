// irc.h

#ifndef _IRC_H_
#define	_IRC_H_

/*
	IRC (RFC #1459) Client Implementation
*/

#pragma warning (disable: 4786)

#include "socket.h"
#include "CrossThreadsMessagingDevice.h"

////////////////////////////////////////////////////////////////////
namespace irc {
////////////////////////////////////////////////////////////////////

static const char* endl = "\r\n";

// RFC's Identity Server (RFC #1413)
class CIrcIdentServer
{
public :
	CIrcIdentServer();
	virtual ~CIrcIdentServer();

	bool Start(
			const char* lpszUserID,
			unsigned int uiPort = 113,
			const char* lpszResponseType = "UNIX"
			);
	void Stop();

  HANDLE m_hReady;

protected :
	AgString m_sResponseType;
	unsigned int m_uiPort;
	AgString m_sUserID;

	void DoThread();

private :
	Socket m_socket;
	HANDLE m_hThread;

	static DWORD WINAPI ListenProc(LPVOID pparam);
};


////////////////////////////////////////////////////////////////////

class CIrcMessage
{
public :
	struct Prefix
	{
		AgString sNick, sUser, sHost;
	} prefix;
	AgString sCommand;
	vector<AgString> parameters;
	bool m_bIncoming;

	CIrcMessage() : m_bIncoming(false) {} // default constructor
	CIrcMessage(const char* lpszCmdLine, bool bIncoming=false); // parser constructor
	CIrcMessage(const CIrcMessage& m); // copy constructor

	void Reset();

	CIrcMessage& operator = (const CIrcMessage& m);
	CIrcMessage& operator = (const char* lpszCmdLine);

	AgString AsString() const;

private :
	void ParseIrcCommand(const char* lpszCmdLine);
};

////////////////////////////////////////////////////////////////////

struct IIrcSessionMonitor
{
	virtual void OnIrcMessage(const CIrcMessage* pmsg) = 0;
};

////////////////////////////////////////////////////////////////////

struct CIrcSessionInfo
{
	AgString sServer;
	AgString sServerName;
	unsigned int iPort;
	AgString sNick;
	AgString sUserID;
	AgString sFullName;
	AgString sPassword;
	bool bIdentServer;
	AgString sIdentServerType;
	unsigned int iIdentServerPort;

	CIrcSessionInfo();
	CIrcSessionInfo(const CIrcSessionInfo& si);

	void Reset();
};

////////////////////////////////////////////////////////////////////

class CIrcDefaultMonitor; // foreward

class CIrcSession
{
public :
	friend class CIrcDefaultMonitor;

	CIrcSession(IIrcSessionMonitor* pMonitor = NULL);
	virtual ~CIrcSession();

	void AddMonitor(IIrcSessionMonitor* pMonitor);
	void RemoveMonitor(IIrcSessionMonitor* pMonitor);
		
	bool Connect(const CIrcSessionInfo& info);
	void Disconnect(const char* lpszMessage = "Bye!");

	const CIrcSessionInfo& GetInfo() const
				{ return (const CIrcSessionInfo&)m_info; }

	operator bool() const { return (bool)m_socket; }

	// send-to-stream operators
	friend CIrcSession& operator << (CIrcSession& os, const CIrcMessage& m);

protected :
	Socket m_socket;
	CIrcSessionInfo m_info;

	void DoReceive();

private :
  typedef set<IIrcSessionMonitor*, less<IIrcSessionMonitor*> >  CIrcSessionMonitors;
  CIrcSessionMonitors m_monitors;
	HANDLE m_hThread;
	CRITICAL_SECTION m_cs; // protect m_monitors

	void Notify(const CIrcMessage* pmsg);
	static DWORD WINAPI ThreadProc(LPVOID pparam);

	CIrcIdentServer m_identServer;
};


__inline CIrcSession& operator << (CIrcSession& os, const CIrcMessage& m)
{
	if( os )
	{
		os.m_socket.Send(m.AsString().c_str());
		os.Notify(&m);
	}
	return os;
}

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

class CIrcMonitor :
	public IIrcSessionMonitor,
	private CCrossThreadsMessagingDevice::ICrossThreadsMessagingDeviceMonitor
{
public :
	typedef bool (CIrcMonitor::*PfnIrcMessageHandler)(const CIrcMessage* pmsg);
	struct LessString
	{
		bool operator()(const char* s1, const char* s2) const
			{ return stricmp(s1, s2) < 0; }
	};
	typedef map<const char*, PfnIrcMessageHandler, LessString > HandlersMap;
	struct IrcCommandsMapsListEntry
	{
		HandlersMap* pHandlersMap;
		IrcCommandsMapsListEntry* pBaseHandlersMap;
	};

	CIrcMonitor(CIrcSession& session);
	virtual ~CIrcMonitor();

	virtual void OnIrcMessage(const CIrcMessage* pmsg);

protected :
	CIrcSession& m_session;

	virtual IrcCommandsMapsListEntry* GetIrcCommandsMap() 
				{ return &m_handlersMapsListEntry; }

	virtual void OnIrcAll(const CIrcMessage* pmsg) {}
	virtual void OnIrcDefault(const CIrcMessage* pmsg) {}
	virtual void OnIrcDisconnected() {}

protected :
	CCrossThreadsMessagingDevice m_xPost;
	static IrcCommandsMapsListEntry m_handlersMapsListEntry;
	static HandlersMap m_handlers;

	PfnIrcMessageHandler FindMethod(const char* lpszName);
	PfnIrcMessageHandler FindMethod(IrcCommandsMapsListEntry* pMapsList, const char* lpszName);

	virtual void OnCrossThreadsMessage(WPARAM wParam, LPARAM lParam);
};

// define an IRC command-to-member map.
// put that macro inside the class definition (.H file)
#define	DEFINE_IRC_MAP()	\
protected :	\
	virtual IrcCommandsMapsListEntry* GetIrcCommandsMap()	\
				{ return &m_handlersMapsListEntry; }	\
protected : \
	static CIrcMonitor::IrcCommandsMapsListEntry m_handlersMapsListEntry;	\
	static CIrcMonitor::HandlersMap m_handlers;	\
protected :

// IRC command-to-member map's declaration. 
// add this macro to the class's .CPP file
#define	DECLARE_IRC_MAP(this_class, base_class)	\
	CIrcMonitor::HandlersMap this_class##::m_handlers;	\
	CIrcMonitor::IrcCommandsMapsListEntry this_class##::m_handlersMapsListEntry	\
		= { &this_class##::m_handlers, &base_class##::m_handlersMapsListEntry };

// map actual member functions to their associated IRC command.
// put any number of this macro in the class's constructor.
#define	IRC_MAP_ENTRY(class_name, name, member)	\
	m_handlers[(name)] = (PfnIrcMessageHandler)&class_name##::member;

  
////////////////////////////////////////////////////////////////////

class CIrcDefaultMonitor : public CIrcMonitor
{
public :
	CIrcDefaultMonitor(CIrcSession& session);

	DEFINE_IRC_MAP()

protected :
	bool OnIrc_NICK(const CIrcMessage* pmsg);
	bool OnIrc_PING(const CIrcMessage* pmsg);
	bool OnIrc_YOURHOST(const CIrcMessage* pmsg);
	bool OnIrc_BOUNCE(const CIrcMessage* pmsg);
};

////////////////////////////////////////////////////////////////////
}; // end of namespace irc
////////////////////////////////////////////////////////////////////

#endif // _IRC_H_


