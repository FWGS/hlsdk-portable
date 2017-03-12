//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudnextmap.h"

DECLARE_MESSAGE(m_Nextmap, Nextmap )

int AgHudNextmap::Init(void)
{
	HOOK_MESSAGE( Nextmap );

	m_iFlags = 0;
	m_szNextmap[0] = '\0';
  m_flTurnoff = 0;

	gHUD.AddHudElem(this);

	return 1;
};

int AgHudNextmap::VidInit(void)
{
	return 1;
};

void AgHudNextmap::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudNextmap::Draw(float fTime)
{
  if (m_flTurnoff < gHUD.m_flTime)
  {
    Reset();
    return 1;
  }

  char szText[32];
  int r, g, b;
  UnpackRGB(r,g,b, RGB_YELLOWISH);
  sprintf(szText,"Nextmap is %s",m_szNextmap);
  AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight*5 ,ScreenWidth,szText,r,g,b);

  return 0;
}


int AgHudNextmap::MsgFunc_Nextmap(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
  strcpy(m_szNextmap,READ_STRING());
  
  m_flTurnoff = gHUD.m_flTime + 10; //Display for 10 seconds.
  m_iFlags |= HUD_ACTIVE;

	return 1;
}


//-- Martin Webrant
