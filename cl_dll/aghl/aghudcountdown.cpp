//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "agglobal.h"

DECLARE_MESSAGE(m_Countdown, Countdown )


int AgHudCountdown::Init(void)
{
	HOOK_MESSAGE( Countdown );

	m_btCountdown = -1;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int AgHudCountdown::VidInit(void)
{

	return 1;
};

void AgHudCountdown::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
	m_btCountdown = -1;
}

#define RGB_WHITEISH 0x00FFFFFF //255,255,255

int AgHudCountdown::Draw(float fTime)
{
  if (gHUD.m_iIntermission)
    return 0;

  char szText[128];
  szText[0] = '\0';
	int r, g, b;
	UnpackRGB(r,g,b, RGB_YELLOWISH);
  if (50 != m_btCountdown)
  {
    int iWidth  = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).right - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).left;
    //int iHeight = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).bottom - gHUD.GetSpriteRect(gHUD.m_HUD_number_0).top;

	  gHUD.DrawHudNumber( ScreenWidth / 2 - iWidth/2, gHUD.m_scrinfo.iCharHeight*10, DHN_DRAWZERO , m_btCountdown, r, g, b);
    if (0 != strlen(m_szPlayer1) && 0 != strlen(m_szPlayer2))
    {
      sprintf(szText,"%s vs %s",m_szPlayer1,m_szPlayer2);
      //Write arena text.
      AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight*7,ScreenWidth,szText,r,g,b);
    }
    else
    {
      //Write match text.
      strcpy(szText,"Match about to start");
      AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight*7,ScreenWidth,szText,r,g,b);
    }
  }
  else
  {
    if (0 != strlen(m_szPlayer1))
    {
      sprintf(szText,"Last round won by %s",m_szPlayer1);
      AgDrawHudStringCentered(ScreenWidth / 2 , gHUD.m_scrinfo.iCharHeight*7 ,ScreenWidth,szText,r,g,b);
    }
    else
    {
      strcpy(szText,"Waiting for players to get ready");
      AgDrawHudStringCentered(ScreenWidth / 2 , gHUD.m_scrinfo.iCharHeight*7 ,ScreenWidth,szText,r,g,b);
    }
    
  }

  return 0;
}


int AgHudCountdown::MsgFunc_Countdown(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	//Update data
	m_btCountdown = READ_BYTE();
  char btSound = READ_BYTE();
  strcpy(m_szPlayer1,READ_STRING());
  strcpy(m_szPlayer2,READ_STRING());

	if (0 <= m_btCountdown)
  {
		m_iFlags |= HUD_ACTIVE;

    if (btSound)
    {
      //Play countdown sound
      if (0 == m_btCountdown)
      {
        gEngfuncs.pfnPlaySoundByName("barney/ba_bring.wav",1);
      }
      else if (1 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/one.wav",1);
      else if (2 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/two.wav",1);
      else if (3 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/three.wav",1);
      else if (4 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/four.wav",1);
      else if (5 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/five.wav",1);
      else if (6 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/six.wav",1);
      else if (7 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/seven.wav",1);
      else if (8 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/eight.wav",1);
      else if (9 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/nine.wav",1);
      else if (10 == m_btCountdown)
        gEngfuncs.pfnPlaySoundByName("fvox/ten.wav",1);
    }
  }
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}




//-- Martin Webrant
