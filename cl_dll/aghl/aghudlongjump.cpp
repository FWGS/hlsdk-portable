//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudLongjump.h"

DECLARE_MESSAGE(m_Longjump, Longjump )

int AgHudLongjump::Init(void)
{
	HOOK_MESSAGE( Longjump );

  m_flTurnoff = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int AgHudLongjump::VidInit(void)
{
	return 1;
};

void AgHudLongjump::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudLongjump::Draw(float fTime)
{
  if (m_flTurnoff < gHUD.m_flTime || gHUD.m_iIntermission)
  {
    Reset();
    return 1;
  }

  char szText[32];
  int r, g, b;
  UnpackRGB(r,g,b, RGB_GREENISH);
  sprintf(szText,"Longjump %d",(int)(m_flTurnoff - gHUD.m_flTime));
  AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight*2 ,ScreenWidth,szText,r,g,b);

  return 0;
}


int AgHudLongjump::MsgFunc_Longjump(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
  int iTime = READ_BYTE();

  m_flTurnoff = gHUD.m_flTime + iTime;
  m_iFlags |= HUD_ACTIVE;

	return 1;
}




//-- Martin Webrant
