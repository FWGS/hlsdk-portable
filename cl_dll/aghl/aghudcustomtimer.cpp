//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudCustomTimer.h"

DECLARE_COMMAND(m_CustomTimer, CustomTimer);

int AgHudCustomTimer::Init(void)
{
  m_flTurnoff = 0;
	m_iFlags = 0;

	gHUD.AddHudElem(this);

  HOOK_COMMAND("customtimer",CustomTimer);

	return 1;
};

int AgHudCustomTimer::VidInit(void)
{
	return 1;
};

void AgHudCustomTimer::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudCustomTimer::Draw(float fTime)
{
  if (m_flTurnoff < gHUD.m_flTime || gHUD.m_iIntermission)
  {
    gEngfuncs.pfnPlaySoundByName("fvox/bell.wav",1);
    Reset();
    return 1;
  }

  char szText[32];
  int r, g, b;
  UnpackRGB(r,g,b, RGB_GREENISH);
  
  sprintf(szText,"Timer %d",(int)(m_flTurnoff - gHUD.m_flTime));
  AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight*4 ,ScreenWidth,szText,r,g,b);

  return 0;
}

void AgHudCustomTimer::UserCmd_CustomTimer()
{
  if (2 == gEngfuncs.Cmd_Argc())
  {
    m_flTurnoff = gHUD.m_flTime + atof(gEngfuncs.Cmd_Argv(1));
    m_iFlags |= HUD_ACTIVE;
  }
}




//-- Martin Webrant
