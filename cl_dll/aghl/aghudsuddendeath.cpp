//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "aghudSuddenDeath.h"

DECLARE_MESSAGE(m_SuddenDeath, SuddenDeath )


int AgHudSuddenDeath::Init(void)
{
	HOOK_MESSAGE( SuddenDeath );

	m_bySuddenDeath = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
};

int AgHudSuddenDeath::VidInit(void)
{
	return 1;
};

void AgHudSuddenDeath::Reset(void)
{
  m_iFlags &= ~HUD_ACTIVE;
}

int AgHudSuddenDeath::Draw(float fTime)
{
  if (gHUD.m_iIntermission)
    Reset();

  int r, g, b;
  UnpackRGB(r,g,b, RGB_REDISH);
  
  AgDrawHudStringCentered(ScreenWidth / 2, gHUD.m_scrinfo.iCharHeight * 2,ScreenWidth,"Sudden death!",r,g,b);

  return 0;
}


int AgHudSuddenDeath::MsgFunc_SuddenDeath(const char *pszName,  int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );
	m_bySuddenDeath = READ_BYTE();
  
  if (m_bySuddenDeath)
    m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}


//-- Martin Webrant
