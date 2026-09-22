#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_ModeShow, FModeShow)

int CHudModeShow::Init(void)
{
	m_ifucker1 = 0;
	m_icons = 0;
	HOOK_MESSAGE(FModeShow);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

int CHudModeShow::VidInit(void)
{
	return 1;
};

int CHudModeShow:: MsgFunc_FModeShow(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_ifucker1 = READ_BYTE();
	m_icons = READ_BYTE();
	return 1;
}

int CHudModeShow::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL )  )
		return 1;

	if (m_ifucker1 == 0 && m_icons <= 0)
		return 1;

	int r, g, b, x, y, a,y2;
	int icons;

	r = 255;
	g = 255;
	b = 255;
	a = 255;
	x = 8;
	y = ScreenHeight - ScreenHeight * 0.45 - 48;
	y2 = ScreenHeight - ScreenHeight * 0.45 - 80;

	if (m_ifucker1 == 1)
	{
		m_hSprite1 = LoadSprite("sprites/guardbackup.spr");

		SPR_Set(m_hSprite1, r, g, b );
		SPR_DrawHoles( 0,  x, y, NULL);
	}

	if (m_icons >= 1)
	{
		m_hSprite2 = LoadSprite("sprites/wdoor_menutext_icons.spr");
		m_hSprite3 = LoadSprite("sprites/wdoor_menutext_icons_drk.spr");

		if(m_icons == 1 || m_icons == 2 || m_icons == 14 || m_icons == 23)
		{
			icons = 11;
		}
		else if(m_icons >= 3 && m_icons <= 8)
		{
			icons = 4;
		}
		else if(m_icons >= 9 && m_icons <= 12)
		{
			icons = 9;
		}
		else if(m_icons == 13)
		{
			icons = 6;
		}
		else if(m_icons == 15)
		{
			icons = 2;
		}
		else if(m_icons == 16)
		{
			icons = 7;
		}
		else if(m_icons == 17)
		{
			icons = 7;
		}
		else if(m_icons == 18)
		{
			icons = 29;
		}
		else if(m_icons == 19)
		{
			icons = 8;
		}
		else if(m_icons == 20)
		{
			icons = 15;
		}
		else if(m_icons == 21)
		{
			icons = 16;
		}
		else if(m_icons == 22)
		{
			icons = 14;
		}
		else
		{
			icons = 32;
		}

		SPR_Set(m_hSprite3, r, g, b );
		SPR_DrawHoles(icons,  x, y2, NULL);

		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive(icons,  x, y2, NULL);
	}

	return 1;
}