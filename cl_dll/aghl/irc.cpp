// irc.cpp
// Made by Adi Degani - http://www.codeguru.com/network/irc.html
#include <stdio.h>
#include <stdlib.h>
#include <Tchar.h>
#include "agglobal.h"

#include "irc.h"

using namespace irc;

////////////////////////////////////////////////////////////////////

CIrcMessage::CIrcMessage(const char* lpszCmdLine, bool bIncoming)
	: m_bIncoming(bIncoming)
{
	ParseIrcCommand(lpszCmdLine);
}

CIrcMessage::CIrcMessage(const CIrcMessage& m)
	:	sCommand(m.sCommand),
		parameters(m.parameters),
		m_bIncoming(m.m_bIncoming)
{
	prefix.sNick = m.prefix.sNick;
	prefix.sUser = m.prefix.sUser;
	prefix.sHost = m.prefix.sHost;
}

void CIrcMessage::Reset()
{
	prefix.sNick = prefix.sUser = prefix.sHost = sCommand = "";
	m_bIncoming = false;
	parameters.clear();
}

CIrcMessage& CIrcMessage::operator = (const CIrcMessage& m)
{
	if( &m != this )
	{
		sCommand = m.sCommand;
		parameters = m.parameters;
		prefix.sNick = m.prefix.sNick;
		prefix.sUser = m.prefix.sUser;
		prefix.sHost = m.prefix.sHost;
		m_bIncoming = m.m_bIncoming;
	}
	return *this;
}

CIrcMessage& CIrcMessage::operator = (const char* lpszCmdLine)
{
	Reset();
	ParseIrcCommand(lpszCmdLine);
	return *this;
}

void CIrcMessage::ParseIrcCommand(const char* lpszCmdLine)
{
	const char* p1 = lpszCmdLine;
	const char* p2 = lpszCmdLine;

	assert(lpszCmdLine != NULL);
	assert(*lpszCmdLine);

	// prefix exists ?
	if( *p1 == ':' )
	{ // break prefix into its components (nick!user@host)
		p2 = ++p1;
		while( *p2 && !strchr(" !", *p2) )
			++p2;
		prefix.sNick.assign(p1, p2 - p1);
		if( *p2 != '!' )
			goto end_of_prefix;
		p1 = ++p2;
		while( *p2 && !strchr(" @", *p2) )
			++p2;
		prefix.sUser.assign(p1, p2 - p1);
		if( *p2 != '@' )
			goto end_of_prefix;
		p1 = ++p2;
		while( *p2 && !isspace(*p2) )
			++p2;
		prefix.sHost.assign(p1, p2 - p1);
end_of_prefix :
		while( *p2 && isspace(*p2) )
			++p2;
		p1 = p2;
	}

	// get command
	assert(*p1 != '\0');
	p2 = p1;
	while( *p2 && !isspace(*p2) )
		++p2;
	sCommand.assign(p1, p2 - p1);
	_strupr((char*)sCommand.c_str());
	while( *p2 && isspace(*p2) )
		++p2;
	p1 = p2;

	// get parameters
	while( *p1 )
	{
		if( *p1 == ':' )
		{
			++p1;
			// seek end-of-message
			while( *p2 )
				++p2;
			parameters.push_back(AgString(p1, p2 - p1));
			break;
		}
		else
		{
			// seek end of parameter
			while( *p2 && !isspace(*p2) )
				++p2;
			parameters.push_back(AgString(p1, p2 - p1));
			// see next parameter
			while( *p2 && isspace(*p2) )
				++p2;
			p1 = p2;
		}
	} // end parameters loop
}

AgString CIrcMessage::AsString() const
{
	AgString s;

	if( prefix.sNick.length() )
	{
		s += ":" + prefix.sNick;
		if( prefix.sUser.length() && prefix.sHost.length() )
			s += "!" + prefix.sUser + "@" + prefix.sHost;
		s += " ";
	}

	s += sCommand;

	for(unsigned int i=0; i < parameters.size(); i++)
	{
		s += " ";
		if( i == parameters.size() - 1 ) // is last parameter ?
			s += ":";
		s += parameters[i];
	}

	s += endl;

	return s;
}

////////////////////////////////////////////////////////////////////

CIrcSession::CIrcSession(IIrcSessionMonitor* pMonitor)
	:	m_hThread(NULL)
{
	InitializeCriticalSection(&m_cs);
}

CIrcSession::~CIrcSession()
{
	Disconnect();
	DeleteCriticalSection(&m_cs);
}

bool CIrcSession::Connect(const CIrcSessionInfo& info)
{
	assert(m_hThread==NULL && !(bool)m_socket);

	try
	{
    m_identServer.Start(info.sUserID.c_str());

		if( !m_socket.Create() )
			throw "Failed to create socket!";

		InetAddr addr(info.sServer.c_str(), (WORD)info.iPort);
		if( !m_socket.Connect(addr) )
		{
			m_socket.Close();
			throw "Failed to connect to host!";
		}

		m_info = info;

		// start receiving messages from host
    DWORD dwThreadId = 0;
		m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &dwThreadId);
		Sleep(100);

		if( info.sPassword.length() )
			m_socket.Send("PASS %s\r\n", info.sPassword.c_str());

		m_socket.Send("NICK %s\r\n", info.sNick.c_str());

		TCHAR szHostName[MAX_PATH];
		DWORD cbHostName = sizeof(szHostName);
		GetComputerName(szHostName, &cbHostName);

		m_socket.Send("USER %s %s %s :%s\r\n", 
			info.sUserID.c_str(), szHostName, "server", info.sFullName.c_str());
	}
	catch( const char* )
	{
		Disconnect();
	}
	catch( ... )
	{
		Disconnect();
	}

	return (bool)m_socket;
}

void CIrcSession::Disconnect(const char* lpszMessage)
{
	static const DWORD dwServerTimeout = 3 * 1000;

	if( !m_hThread )
		return;

	m_socket.Send("QUIT :%s\r\n", lpszMessage ? lpszMessage : "bye!");

	if( m_hThread && WaitForSingleObject(m_hThread, dwServerTimeout) != WAIT_OBJECT_0 )
	{
		m_socket.Close();
		Sleep(100);
		if( m_hThread && WaitForSingleObject(m_hThread, dwServerTimeout) != WAIT_OBJECT_0 )
		{
			TerminateThread(m_hThread, 1);
			CloseHandle(m_hThread);
			m_hThread = NULL;
			m_info.Reset();
		}
	}
}

void CIrcSession::Notify(const CIrcMessage* pmsg)
{
	// forward message to monitor objects
	EnterCriticalSection(&m_cs);
	for(CIrcSessionMonitors::iterator it = m_monitors.begin();
			it != m_monitors.end();
			it++
			)
	{
		(*it)->OnIrcMessage(pmsg);
	}
	LeaveCriticalSection(&m_cs);
}

void CIrcSession::DoReceive()
{
//	CIrcIdentServer m_identServer;
	char chBuf[1024*4+1];
	int cbInBuf = 0;

//	if( m_info.bIdentServer )
		//m_identServer.Start(m_info.sUserID.c_str());
  //Sleep(100);

	while( m_socket )
	{
		int cbRead;
		int nLinesProcessed = 0;

		cbRead = m_socket.Receive((unsigned char*)chBuf+cbInBuf, sizeof(chBuf)-cbInBuf-1);
		if( cbRead <= 0 )
			break;
		cbInBuf += cbRead;
		chBuf[cbInBuf] = '\0';

		char* pStart = chBuf;
		while( *pStart )
		{
			char* pEnd;

			// seek end-of-line
			for(pEnd=pStart; *pEnd && *pEnd != '\r' && *pEnd != '\n'; ++pEnd)
				;
			if( *pEnd == '\0' )
				break; // uncomplete message. stop parsing.

			++nLinesProcessed;

			// replace end-of-line with NULLs and skip
			while( *pEnd == '\r' || *pEnd == '\n' )
				*pEnd++ = '\0';

			if( *pStart )
			{
				// process single message by monitor objects
				CIrcMessage msg(pStart, true);
				Notify(&msg);
			}

			cbInBuf -= pEnd - pStart;
			assert(cbInBuf >= 0);

			pStart = pEnd;
		}

		// discard processed messages
		if( nLinesProcessed != 0 )
			memmove(chBuf, pStart, cbInBuf+1);
	}

	if( m_socket )
		m_socket.Close();

	if( m_info.bIdentServer )
		m_identServer.Stop();

	// notify monitor objects that the connection has been closed
	Notify(NULL);
}

DWORD WINAPI CIrcSession::ThreadProc(LPVOID pparam)
{
	CIrcSession* pThis = (CIrcSession*)pparam;
	try { pThis->DoReceive(); } catch( ... ) {}
	pThis->m_info.Reset();
	CloseHandle(pThis->m_hThread);
	pThis->m_hThread = NULL;
	return 0;
}

void CIrcSession::AddMonitor(IIrcSessionMonitor* pMonitor)
{
	assert(pMonitor != NULL);
	EnterCriticalSection(&m_cs);
	m_monitors.insert(pMonitor);
	LeaveCriticalSection(&m_cs);
}

void CIrcSession::RemoveMonitor(IIrcSessionMonitor* pMonitor)
{
	assert(pMonitor != NULL);
	EnterCriticalSection(&m_cs);
	m_monitors.erase(pMonitor);
	LeaveCriticalSection(&m_cs);
}

////////////////////////////////////////////////////////////////////

CIrcSessionInfo::CIrcSessionInfo()
	:	iPort(0), bIdentServer(false), iIdentServerPort(0)
{
}

CIrcSessionInfo::CIrcSessionInfo(const CIrcSessionInfo& si)
	:	sServer(si.sServer),
		sServerName(si.sServerName),
		iPort(si.iPort),
		sNick(si.sNick),
		sUserID(si.sUserID),
		sFullName(si.sFullName),
		sPassword(si.sPassword),
		bIdentServer(si.bIdentServer),
		sIdentServerType(si.sIdentServerType),
		iIdentServerPort(si.iIdentServerPort)
{
}

void CIrcSessionInfo::Reset()
{
	sServer = "";
	sServerName = "";
	iPort = 0;
	sNick = "";
	sUserID = "";
	sFullName = "";
	sPassword = "";
	bIdentServer = false;
	sIdentServerType = "";
	iIdentServerPort = 0;
}

////////////////////////////////////////////////////////////////////

CIrcIdentServer::CIrcIdentServer()
	: m_uiPort(0), m_hThread(NULL)
{
  m_hReady = CreateEvent(NULL,TRUE,FALSE,"IDENTEVENT");
}

CIrcIdentServer::~CIrcIdentServer()
{
  CloseHandle(m_hReady);
	Stop();
}

bool CIrcIdentServer::Start(
				const char* lpszUserID,
				unsigned int uiPort,
				const char* lpszResponseType
				)
{
	if( m_socket )
		return false;

	if( !m_socket.Create() )
		return false;

	if( !m_socket.Bind(InetAddr((WORD)uiPort)) )
	{
		m_socket.Close();
		return false;
	}

	m_sResponseType = lpszResponseType;
	m_sUserID = lpszUserID;
	m_uiPort = uiPort;

  DWORD dwThreadId = 0;
	m_hThread = CreateThread(NULL, 0, ListenProc, this, 0, &dwThreadId);
  WaitForSingleObject(m_hReady,INFINITE);

	return true;
}

void CIrcIdentServer::Stop()
{
	if( m_hThread )
	{
		m_socket.Close();
    Sleep(100);
		if( WaitForSingleObject(m_hThread, 2000) != WAIT_OBJECT_0 && m_hThread )
		{
			TerminateThread(m_hThread, 1);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
}

void CIrcIdentServer::DoThread()
{
	m_socket.Listen();

	while( (bool)m_socket )
	{
		InetAddr addr;

    SetEvent(m_hReady);
		Socket s = m_socket.Accept(addr);
    //int iErr = WSAGetLastError();
		if( !(bool)s )
			break;

		char szBuf[1024];
		int cbRead = s.Receive((unsigned char*)szBuf, sizeof(szBuf)-1);
		if( cbRead <= 0 )
			continue;
		szBuf[cbRead] = '\0';

		// strip CRLF from query
		for(char* p = szBuf; *p && *p != '\r' && *p != '\n'; ++p)
			;
		*p = '\0';

		s.Send("%s : USERID : %s : %s\r\n", 
			szBuf, m_sResponseType.c_str(), m_sUserID.c_str());
		Sleep(500);
		s.Close();
	}

  SetEvent(m_hReady);

	m_socket.Close();
}

DWORD WINAPI CIrcIdentServer::ListenProc(LPVOID pparam)
{
	CIrcIdentServer* pThis = (CIrcIdentServer*)pparam;

	try { pThis->DoThread(); } catch( ... ) {}

	pThis->m_sResponseType = "";
	pThis->m_sUserID = "";
	pThis->m_uiPort = 0;

	CloseHandle(pThis->m_hThread);
	pThis->m_hThread = NULL;

	return 0;
}

////////////////////////////////////////////////////////////////////

CIrcMonitor::HandlersMap CIrcMonitor::m_handlers;
CIrcMonitor::IrcCommandsMapsListEntry CIrcMonitor::m_handlersMapsListEntry
	= { &CIrcMonitor::m_handlers, NULL };


CIrcMonitor::CIrcMonitor(CIrcSession& session)
	: m_session(session)
{
	m_xPost.SetMonitor(this);
}

CIrcMonitor::~CIrcMonitor()
{
}

void CIrcMonitor::OnIrcMessage(const CIrcMessage* pmsg)
{
	CIrcMessage* pMsgCopy = NULL;
	if( pmsg )
		pMsgCopy = new CIrcMessage(*pmsg);
	m_xPost.Post(0, (LPARAM)pMsgCopy);
}

void CIrcMonitor::OnCrossThreadsMessage(WPARAM wParam, LPARAM lParam)
{
	CIrcMessage* pmsg = (CIrcMessage*)lParam;

	OnIrcAll(pmsg);

	if( pmsg )
	{
		PfnIrcMessageHandler pfn = FindMethod(pmsg->sCommand.c_str());
		if( pfn )
		{
			// call member function. if it returns 'false',
			// call the default handling
			if( !(this->*pfn)(pmsg) )
				OnIrcDefault(pmsg);
		}
		else // handler not found. call default handler
			OnIrcDefault(pmsg);
		delete pmsg;
	}
	else
		OnIrcDisconnected();
}

CIrcMonitor::PfnIrcMessageHandler CIrcMonitor::FindMethod(const char* lpszName)
{
	// call the recursive version with the most derived map
	return FindMethod(GetIrcCommandsMap(), lpszName);
}

CIrcMonitor::PfnIrcMessageHandler CIrcMonitor::FindMethod(IrcCommandsMapsListEntry* pMapsList, const char* lpszName)
{
	HandlersMap::iterator it = pMapsList->pHandlersMap->find(lpszName);
	if( it != pMapsList->pHandlersMap->end() )
		return (*it).second; // found !
	else if( pMapsList->pBaseHandlersMap )
		return FindMethod(pMapsList->pBaseHandlersMap, lpszName); // try at base class
	return NULL; // not found in any map
}

////////////////////////////////////////////////////////////////////

DECLARE_IRC_MAP(CIrcDefaultMonitor, CIrcMonitor)

CIrcDefaultMonitor::CIrcDefaultMonitor(CIrcSession& session)
	: CIrcMonitor(session)
{
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "NICK", OnIrc_NICK)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "PING", OnIrc_PING)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY(CIrcDefaultMonitor, "005", OnIrc_BOUNCE)
}

bool CIrcDefaultMonitor::OnIrc_NICK(const CIrcMessage* pmsg)
{
	if( (m_session.GetInfo().sNick == pmsg->prefix.sNick) && (pmsg->parameters.size() > 0) )
		m_session.m_info.sNick = pmsg->parameters[0];
	return false;
}

bool CIrcDefaultMonitor::OnIrc_PING(const CIrcMessage* pmsg)
{
	char szResponse[100];
	sprintf(szResponse, "PONG %s", pmsg->parameters[0].c_str());
	m_session << CIrcMessage(szResponse);
	return false;
}

bool CIrcDefaultMonitor::OnIrc_YOURHOST(const CIrcMessage* pmsg)
{
	static const char* lpszFmt = "Your host is %[^ \x5b,], running version %s";
	char szHostName[100], szVersion[100];
	if( sscanf(pmsg->parameters[1].c_str(), lpszFmt, &szHostName, &szVersion) > 0 )
		m_session.m_info.sServerName = szHostName;
	return false;
}

bool CIrcDefaultMonitor::OnIrc_BOUNCE(const CIrcMessage* pmsg)
{
	static const char* lpszFmt = "Try server %[^ ,], port %d";
	char szAltServer[100];
	int iAltPort = 0;
	if( sscanf(pmsg->parameters[1].c_str(), lpszFmt, &szAltServer, &iAltPort) == 2 )
	{
	}
	return false;
}
