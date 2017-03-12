//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudvote.h"

DECLARE_MESSAGE(m_Vote, Vote )


int AgHudVote::Init(void)
{
	HOOK_MESSAGE( Vote );

	m_iFlags = 0;
  m_flTurnoff = 0.0;
	m_iVoteStatus = 0;
  m_iFor = 0;
  m_iAgainst = 0;
  m_iUndecided = 0;
	m_szVote[0] = '\0';
  m_szValue[0] = '\0';
  m_szCalled[0] = '\0';

	gHUD.AddHudElem(this);

	return 1;
};

int AgHudVote::VidInit(void)
{
	return 1;
};

void AgHudVote::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudVote::Draw(float fTime)
{
  if (m_flTurnoff < gHUD.m_flTime || gHUD.m_iIntermission)
  {
    Reset();
    return 1;
  }

  int r, g, b;
  UnpackRGB(r,g,b, RGB_YELLOWISH);

  char szText[128];
  sprintf(szText,"Vote: %s %s",m_szVote,m_szValue);
  gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 ,ScreenWidth,szText,r,g,b);
  sprintf(szText,"Called by: %s",m_szCalled);
  gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight,ScreenWidth,szText,r,g,b);
  if (Called == m_iVoteStatus)
  {
    sprintf(szText,"For: %d",m_iFor);
    gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*2 ,ScreenWidth,szText,r,g,b);
    sprintf(szText,"Against: %d ",m_iAgainst);
    gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*3,ScreenWidth,szText,r,g,b);
    sprintf(szText,"Undecided: %d",m_iUndecided);
    gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*4,ScreenWidth,szText,r,g,b);
  }
  else if (Accepted == m_iVoteStatus)
  {
    strcpy(szText,"Accepted!");
    gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*2,ScreenWidth,szText,r,g,b);
  }
  else if (Denied == m_iVoteStatus)
  {
    strcpy(szText,"Denied!");
    gHUD.DrawHudString(ScreenWidth/20, ScreenHeight/8 + gHUD.m_scrinfo.iCharHeight*2,ScreenWidth,szText,r,g,b);
  }

  return 0;
}


int AgHudVote::MsgFunc_Vote(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
 	m_iVoteStatus = READ_BYTE();
  m_iFor = READ_BYTE();
  m_iAgainst = READ_BYTE();
  m_iUndecided = READ_BYTE();
  strcpy(m_szVote,READ_STRING());
  strcpy(m_szValue,READ_STRING());
  strcpy(m_szCalled,READ_STRING());
  
  m_flTurnoff = gHUD.m_flTime + 4; 

  if (m_iVoteStatus)
    m_iFlags |= HUD_ACTIVE;
	else
    m_iFlags &= ~HUD_ACTIVE;
    

	return 1;
}


//-- Martin Webrant
