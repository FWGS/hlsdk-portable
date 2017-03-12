//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudplayerid.h"

DECLARE_MESSAGE(m_PlayerId, PlayerId )

extern cvar_t* g_phud_playerid;

int AgHudPlayerId::Init(void)
{
	HOOK_MESSAGE( PlayerId );

	m_iPlayer  = 0;
  m_bTeam    = false;
  m_iHealth  = 0;
  m_iArmour  = 0;
  m_flTurnoff = 0.0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int AgHudPlayerId::VidInit(void)
{

	return 1;
};

void AgHudPlayerId::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
  m_iPlayer = 0;
}


int AgHudPlayerId::Draw(float fTime)
{
  if (0 == g_phud_playerid->value || 0 >= m_iPlayer)
    return 1;

  if (m_flTurnoff < gHUD.m_flTime)
  {
    Reset();
    return 1;
  }

  if (g_PlayerInfoList[m_iPlayer].name)
  {
    char szText[64];
    if (m_bTeam)
      sprintf(szText,"%s %d/%d",g_PlayerInfoList[m_iPlayer].name,m_iHealth,m_iArmour);
    else
      sprintf(szText,"%s",g_PlayerInfoList[m_iPlayer].name);

    int r, g, b;

    if (m_bTeam)
  	  UnpackRGB(r,g,b, RGB_GREENISH);
    else
      UnpackRGB(r,g,b, RGB_REDISH);

    if (CVAR_GET_FLOAT("hud_centerid"))
      AgDrawHudStringCentered(ScreenWidth / 2, ScreenHeight - ScreenHeight/4,ScreenWidth,szText,r,g,b);
    else
      gHUD.DrawHudString(10, ScreenHeight - ScreenHeight/8,ScreenWidth,szText,r,g,b);
  }

  return 0;
}


int AgHudPlayerId::MsgFunc_PlayerId(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iPlayer  = READ_BYTE();
  if (1 == READ_BYTE())
    m_bTeam = true;
  else
    m_bTeam = false;
  m_iHealth  = READ_SHORT();
  m_iArmour  = READ_SHORT();

  if (0 == g_phud_playerid->value)
		m_iFlags &= ~HUD_ACTIVE;
	else
    m_iFlags |= HUD_ACTIVE;

  GetPlayerInfo(m_iPlayer, &g_PlayerInfoList[m_iPlayer]);

  m_flTurnoff = gHUD.m_flTime + 2; //Hold for 2 seconds.

	return 1;
}




//-- Martin Webrant
