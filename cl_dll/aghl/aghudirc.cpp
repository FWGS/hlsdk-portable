//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <lmcons.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudirc.h"
#include "vgui_TeamFortressViewport.h"

DECLARE_COMMAND(m_IRC, IRCConnect);
DECLARE_COMMAND(m_IRC, IRCConnect2);
DECLARE_COMMAND(m_IRC, IRCDisconnect);
DECLARE_COMMAND(m_IRC, IRCCommand);
DECLARE_COMMAND(m_IRC, IRCToggle);

extern irc::CIrcSession g_ircSession;
extern cvar_t* g_pirc_server;
extern cvar_t* g_pirc_nick;
extern cvar_t* g_pirc_port;
extern cvar_t* g_pirc_userid;
extern cvar_t* g_pirc_password;
extern cvar_t* g_pirc_fullname;

AgIRC* g_pIRC = NULL;

int AgHudIRC::Init(void)
{
	m_iFlags = 0;

	gHUD.AddHudElem(this);

  HOOK_COMMAND("IRCConnect",IRCConnect);
  HOOK_COMMAND("IRCConnect2",IRCConnect2);
  HOOK_COMMAND("IRCDisconnect",IRCDisconnect);
  HOOK_COMMAND("IRC",IRCCommand);
  HOOK_COMMAND("toggleirc",IRCToggle);

	return 1;
};

int AgHudIRC::VidInit(void)
{
	return 1;
};

void AgHudIRC::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudIRC::Draw(float fTime)
{
  return 0;
}

void AgHudIRC::UserCmd_IRCConnect()
{
  IRCConnect(false);
}

void AgHudIRC::UserCmd_IRCConnect2()
{
  IRCConnect(true);
}


void AgHudIRC::IRCConnect(bool bSilent)
{
  if (!gViewPort)
    return;
 	
  char szUser[UNLEN],szComputer[UNLEN];   
  DWORD dwSize = UNLEN;
  GetComputerName(szComputer,&dwSize);
  szComputer[dwSize] = '\0';
  dwSize = UNLEN;
  GetUserName(szUser,&dwSize);
  szUser[dwSize] = '\0';

  CIrcSessionInfo si;
  
	si.sServer = gEngfuncs.pfnGetCvarString("irc_server");
	si.iPort = gEngfuncs.pfnGetCvarFloat("irc_port");
  si.sNick = gEngfuncs.pfnGetCvarString("irc_nick");
  si.sUserID = gEngfuncs.pfnGetCvarString("irc_userid");
  si.sFullName = gEngfuncs.pfnGetCvarString("irc_fullname");
  si.sPassword = gEngfuncs.pfnGetCvarString("irc_password");
	si.bIdentServer = true;
	si.iIdentServerPort = 113;
	si.sIdentServerType = "UNIX";

  if (0 == si.sNick.size())
  {
	  int iPlayer = gEngfuncs.GetLocalPlayer()->index;	// Get the local player's index
	  si.sNick = gEngfuncs.PlayerInfo_ValueForKey(iPlayer,"name");
    si.sNick += "|AG";
  }

  if (0 == si.sUserID.size())
  {
    si.sUserID = szComputer;
  }

  if (0 == si.sFullName.size())
  {
    si.sFullName = szUser;
  }

  if (!bSilent)
  {
    if (gEngfuncs.Cmd_Argc() > 1)
      si.sServer = gEngfuncs.Cmd_Argv(1);

    if (gEngfuncs.Cmd_Argc() > 2)
      si.iPort = atoi(gEngfuncs.Cmd_Argv(2));

    if (gEngfuncs.Cmd_Argc() > 3)
      si.sNick = gEngfuncs.Cmd_Argv(3);

    if (gEngfuncs.Cmd_Argc() > 4)
      si.sUserID = gEngfuncs.Cmd_Argv(4);

    if (gEngfuncs.Cmd_Argc() > 5)
      si.sFullName = gEngfuncs.Cmd_Argv(5);

    if (gEngfuncs.Cmd_Argc() > 6)
      si.sPassword = gEngfuncs.Cmd_Argv(6);
  }

  if (g_pIRC)
  {
    g_pIRC->Disconnect("http://www.planethalflife.com/agmod");
  }
  else
    g_pIRC = new AgIRC();

  if (bSilent)
	  g_pIRC->SilentMode();

  g_pIRC->Connect(&si);
}

void AgHudIRC::UserCmd_IRCDisconnect()
{
  if (!gViewPort)
    return;
  if (!g_pIRC)
    return;
  g_pIRC->Disconnect("http://www.planethalflife.com/agmod");
  delete g_pIRC; 
  g_pIRC = NULL;
}

void AgHudIRC::UserCmd_IRCCommand()
{
  if (!gViewPort)
    return;
  if (!g_pIRC)
    return;

  AgString sCommand;

	for (int i = 1; i < gEngfuncs.Cmd_Argc(); i++ )
	{
		const char *param = gEngfuncs.Cmd_Argv( i );
		if ( param )
		{
      if (0 != sCommand.size())
        sCommand += " ";
      sCommand += param;
		}
	}


  g_pIRC->Command(sCommand);

}

void AgHudIRC::UserCmd_IRCToggle()
{
	if ( gViewPort )
	{
		gViewPort->ToggleIRC();
	}
}




//-- Martin Webrant
