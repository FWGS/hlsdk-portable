// AgIRC.cpp: implementation of the AgIRC class.
//
//////////////////////////////////////////////////////////////////////

#include "hud.h"
#include "cl_util.h"
#include "irc.h"
#include "AgIRC.h"
#include "vgui_TeamFortressViewport.h"
#include "AGVGuiIRC.h"
#include "aghudsettings.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern irc::CIrcSession g_ircSession;

DECLARE_IRC_MAP(AgIRC, CIrcDefaultMonitor)

AgIRC::AgIRC() 
 : irc::CIrcDefaultMonitor(g_ircSession)
{
  m_bSilent = false;
	IRC_MAP_ENTRY(AgIRC, "JOIN", OnIrc_JOIN)
	IRC_MAP_ENTRY(AgIRC, "KICK", OnIrc_KICK)
	IRC_MAP_ENTRY(AgIRC, "MODE", OnIrc_MODE)
	IRC_MAP_ENTRY(AgIRC, "NICK", OnIrc_NICK)
	IRC_MAP_ENTRY(AgIRC, "PART", OnIrc_PART)
	IRC_MAP_ENTRY(AgIRC, "PRIVMSG", OnIrc_PRIVMSG)
	IRC_MAP_ENTRY(AgIRC, "002", OnIrc_YOURHOST)
	IRC_MAP_ENTRY(AgIRC, "QUIT", OnIrc_QUIT)
	IRC_MAP_ENTRY(AgIRC, "MODE", OnIrc_MODE)
}

AgIRC::~AgIRC()
{

}


bool AgIRC::Connect(const CIrcSessionInfo* psi)
{
	g_ircSession.AddMonitor(this);
  return g_ircSession.Connect(*psi);
  
}

bool AgIRC::Disconnect(const AgString& sMessage)
{
	g_ircSession.Disconnect(sMessage.c_str());
	g_ircSession.RemoveMonitor(this);
  return true;
}

bool AgIRC::Command(AgString sCommand)
{
	if( sCommand[0] != '/' )
		sCommand = "PRIVMSG " + m_sChannel + " :" + sCommand;
  else if (0 == strnicmp(sCommand.c_str(),"/msg", 4))
  {
    //Need to fix up privmsg command so that it adds : for last command.
    AgString sUser,sText;
    char* pszCommand = (char*)(const char*)&sCommand[4];
    pszCommand++;
    
		while ('\0' != *pszCommand && !isspace(*pszCommand))
    {
      sUser += *pszCommand;
      pszCommand++;
    }

    if ('\0' != *pszCommand)
    {
      pszCommand++;
      sText = pszCommand;
    }

    sCommand = "PRIVMSG " +  sUser + " :" + sText;
  }
	else
		sCommand = sCommand.substr(1);

  g_ircSession << irc::CIrcMessage(sCommand.c_str());
  return true;
}

void AgIRC::PrintMessage(AgString sMessage)
{
  if (m_bSilent)
    return;

  AgTrim(sMessage);
  if (sMessage.size())
  {
    ConsolePrint(("IRC: " + sMessage + "\n").c_str());
    if (gViewPort && gViewPort->m_pIRC)
      gViewPort->m_pIRC->PrintMessage(sMessage.c_str());
  }
}


void AgIRC::OnMessage(const CIrcMessage* pmsg)
{
  AgString sMessage;
  sMessage = pmsg->prefix.sNick + " ";

  //sMessage = pmsg->prefix.sNick + " " + pmsg->sCommand + " ";

	for (unsigned int i = 1; i < pmsg->parameters.size(); i++)
	{
    sMessage += pmsg->parameters[i] + " ";
  }

  PrintMessage(sMessage.c_str());
}


bool AgIRC::OnIrc_YOURHOST(const CIrcMessage* pmsg)
{
	CIrcDefaultMonitor::OnIrc_YOURHOST(pmsg);

  AgString sMessage;
  sMessage = "Your host is ";
  sMessage += pmsg->parameters[1];

  PrintMessage(sMessage.c_str());

  AgString sChannel = gEngfuncs.pfnGetCvarString("irc_autojoin");
  if (sChannel.length())
  {
      AgString sJoin;
      sJoin = "/JOIN " + sChannel;
      Command(sJoin);
  }

  AgString sCommand = gEngfuncs.pfnGetCvarString("irc_autocommand");
  if (sCommand.length())
      Command(sCommand);

  AgString sCommand2 = gEngfuncs.pfnGetCvarString("irc_autocommand2");
  if (sCommand2.length())
      Command(sCommand2);

  AgString sCommand3 = gEngfuncs.pfnGetCvarString("irc_autocommand3");
  if (sCommand3.length())
      Command(sCommand3);

	return true;
}

bool AgIRC::OnIrc_NICK(const CIrcMessage* pmsg)
{
	CIrcDefaultMonitor::OnIrc_NICK(pmsg);

	if( pmsg->prefix.sNick == m_session.GetInfo().sNick && (pmsg->parameters.size() > 0) )
	{
	}
  else if (pmsg->prefix.sNick.size())
  {
    AgString sMessage;
    sMessage = pmsg->prefix.sNick + " is now known as " + pmsg->parameters[0];
    PrintMessage(sMessage.c_str());
  }

	return true;
}

bool AgIRC::OnIrc_PRIVMSG(const CIrcMessage* pmsg)
{
  if (0 == pmsg->prefix.sNick.size() && pmsg->m_bIncoming || AgIsMatch() && 0 == g_iUser1)
  {
    return true;
  } 

	AgString sName;
	if (!pmsg->m_bIncoming)
		sName = m_session.GetInfo().sNick;
	else
		sName = pmsg->prefix.sNick;
		

  AgString sMessage;

  sMessage = sName + " ";

	for (unsigned int i = 1; i < pmsg->parameters.size(); i++)
	{
    sMessage += pmsg->parameters[i] + " ";
  }

  AgTrim(sMessage);

  if (gViewPort && gViewPort->m_pIRC)
    gViewPort->m_pIRC->PrintMessage(sMessage.c_str());

  sMessage = "IRC: " + sMessage;
  sMessage += "\n";

  if (!m_bSilent)
	gHUD.m_SayText.SayTextPrint( sMessage.c_str(), sMessage.size());

	return true;
}

bool AgIRC::OnIrc_JOIN(const CIrcMessage* pmsg)
{
  if (!pmsg->prefix.sNick.size())
  {
    if (0 != m_sChannel.size() && m_sChannel != pmsg->parameters[0].c_str())
    {
      AgString sPart;
      sPart = "/PART " + m_sChannel;
      Command(sPart);
    }
    m_sChannel = pmsg->parameters[0].c_str();
  }
  else
  {
    AgString sMessage;

    sMessage = pmsg->prefix.sNick + " has joined " + pmsg->parameters[0];
    PrintMessage(sMessage.c_str());
  }
   

	return true;
}

bool AgIRC::OnIrc_PART(const CIrcMessage* pmsg)
{
	if( !pmsg->prefix.sNick.length())
    return false;

  AgString sMessage;
  sMessage = pmsg->prefix.sNick + " has left " + pmsg->parameters[0];
  PrintMessage(sMessage.c_str());
	return true;
}

bool AgIRC::OnIrc_KICK(const CIrcMessage* pmsg)
{
	if( !pmsg->prefix.sNick.length() )
		return false;

  AgString sMessage;
  sMessage = pmsg->prefix.sNick + " was kicked by " + pmsg->parameters[0];
  PrintMessage(sMessage.c_str());
	return true;
}

bool AgIRC::OnIrc_MODE(const CIrcMessage* pmsg)
{
	if( !pmsg->prefix.sNick.length() )
		return false;
	if( pmsg->prefix.sNick == m_session.GetInfo().sNick )
		return false;

  AgString sMessage;
  sMessage = pmsg->prefix.sNick + " sets mode: ";
	for (unsigned int i = 1; i < pmsg->parameters.size(); i++)
	{
    sMessage += pmsg->parameters[i] + " ";
  }

  PrintMessage(sMessage.c_str());
	return true;
}

bool AgIRC::OnIrc_QUIT(const CIrcMessage* pmsg)
{
	if( !pmsg->prefix.sNick.length() )
		return false;

  AgString sMessage;
  sMessage = pmsg->prefix.sNick + " has quit IRC ";
  if (pmsg->parameters[0].size())
    sMessage += "(" + pmsg->parameters[0] + ")";
  PrintMessage(sMessage.c_str());
	return true;
}


void AgIRC::OnIrcDefault(const CIrcMessage* pmsg)
{
	CIrcDefaultMonitor::OnIrcDefault(pmsg);

  OnMessage(pmsg);
}

void AgIRC::OnIrcDisconnected()
{
  AgString sMessage;
  sMessage = "Disconnected from " + m_session.GetInfo().sServerName;
  PrintMessage(sMessage.c_str());
}



void AgIRC::SilentMode()
{
#ifndef AG_TESTCHEAT  
  m_bSilent = true;
#endif
}
