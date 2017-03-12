//++ BulliT

#include <time.h>
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudtimer.h"

DECLARE_MESSAGE(m_Timer, Timer )

extern cvar_t* g_phud_timer;
extern cvar_t* g_pcl_liveupdate;

int AgHudTimer::Init(void)
{
	HOOK_MESSAGE( Timer );

	m_lTimelimit = 0;
  m_lEffectiveTime = 0;
  m_flTurnoff = 0.0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

  m_szTime[0] = '\0';
	return 1;
};

int AgHudTimer::VidInit(void)
{
  m_szTime[0] = '\0';
	return 1;
};

void AgHudTimer::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
  m_flTurnoff = 0;
}

int AgHudTimer::Draw(float fTime)
{
  if (0 == g_phud_timer->value || m_flTurnoff < gHUD.m_flTime)
  {
    Reset();
    return 1;
  }

  int r, g, b;
  UnpackRGB(r,g,b, RGB_GREENISH);
  
  long lTime = 0;
  if (3 == g_phud_timer->value)
  {
    _strtime(m_szTime);
  }
  else
  {
    if (2 == g_phud_timer->value || 0 == m_lTimelimit)
    {
      //Effective time.
      lTime = m_lEffectiveTime;
    }
    else 
    {
      //Time remaining.
      lTime = m_lTimelimit - m_lEffectiveTime;
    }

    if (86400 < lTime)
    {
      //More than one day. Print days, hour, minutes and seconds
      ldiv_t d1 = ldiv(lTime, 86400);
      ldiv_t d2 = ldiv(d1.rem, 3600);
      ldiv_t d3 = ldiv(d2.rem, 60);
      sprintf(m_szTime,"%ldd %ldh %02ldm %02lds",d1.quot,d2.quot,d3.quot,d3.rem);
    }
    else if (3600 < lTime)
    {
      //More than one hour. Print hour, minutes and seconds
      ldiv_t d1 = ldiv(lTime, 3600);
      ldiv_t d2 = ldiv(d1.rem, 60);
      sprintf(m_szTime,"%ldh %02ldm %02lds",d1.quot,d2.quot,d2.rem);
    }
    else if (60 < lTime)
    {
      //More than one minute. Print minutes and seconds.
      ldiv_t d = ldiv(lTime, 60);
      sprintf(m_szTime,"%ld:%02ld",d.quot,d.rem);
    }
    else
    {
      //Less than a minute left. Print seconds.
      sprintf(m_szTime,"%ld",lTime);
    }

    if (0 == m_lTimelimit || 60 < (m_lTimelimit - m_lEffectiveTime))
  	  UnpackRGB(r,g,b, RGB_YELLOWISH);
    else
      UnpackRGB(r,g,b, RGB_REDISH);
  }
  
//  if (0 == g_iUser1)
    AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight ,ScreenWidth,m_szTime,r,g,b);

  return 0;
}


int AgHudTimer::MsgFunc_Timer(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_lTimelimit      = READ_LONG();
  m_lEffectiveTime  = READ_LONG();
  m_flTurnoff = gHUD.m_flTime + 5;

  if (0 == g_phud_timer->value)
		m_iFlags &= ~HUD_ACTIVE;
	else
    m_iFlags |= HUD_ACTIVE;

  if (1 == g_pcl_liveupdate->value)
    LiveUpdate();

	return 1;
}

void AgHudTimer::LiveUpdate()
{
  //Special code for Mr. T-rex for his LCD display. I made it because his efforts with website and ingame menu.
  static char szLiveUpdate[] = "FRGS: %02d DTHS: %02d\r\n%ld:%02ld %s";

  cl_entity_t* pPlayer = gEngfuncs.GetLocalPlayer();
  ldiv_t d = ldiv(m_lTimelimit - m_lEffectiveTime, 60);
  
  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/liveup.txt",AgGetDirectory());
  FILE* pFile = fopen(szFile,"w");
  if (!pFile)
    return;

  fprintf(pFile,szLiveUpdate,g_PlayerExtraInfo[pPlayer->index].frags,g_PlayerExtraInfo[pPlayer->index].deaths,d.quot,d.rem,AgMapname().c_str());

  //Close it up.
  fflush(pFile);
  fclose(pFile);
}



//-- Martin Webrant
