//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <demo_api.h>
#include "parsemsg.h"

DECLARE_MESSAGE(m_Splash, Splash )


int AgHudSplash::Init(void)
{
	HOOK_MESSAGE( Splash );

	m_flTurnoff = 0.0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int AgHudSplash::VidInit(void)
{
  m_hSprite = SPR_Load("sprites/ag_splash.spr");
	return 1;
}

void AgHudSplash::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

#define RGB_WHITEISH 0x00FFFFFF //255,255,255

int AgHudSplash::Draw(float fTime)
{
  if (m_flTurnoff < gHUD.m_flTime || 0 == m_hSprite)
  {
    Reset();
    return 1;
  }

	int r, g, b, x, y;

	UnpackRGB(r,g,b, RGB_WHITEISH);
  ScaleColors(r, g, b, 255);	
	SPR_Set(m_hSprite, r, g, b );
	// This should show up to the top right corner
	y = SPR_Height(m_hSprite,0);
	x = ScreenWidth - SPR_Width(m_hSprite,0);
	SPR_DrawHoles( 0,  x, y, NULL);

	return 0;
}


int AgHudSplash::MsgFunc_Splash(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	// update data
	int iTime = READ_BYTE();

  m_flTurnoff = gHUD.m_flTime + iTime; 
  m_iFlags |= HUD_ACTIVE;

	if (iTime > 0)
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}



//-- Martin Webrant
