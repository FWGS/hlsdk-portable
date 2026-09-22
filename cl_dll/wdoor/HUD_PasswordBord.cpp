#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
extern int iMouseInUse;
DECLARE_MESSAGE(m_PWBord, WPWBord)

int CHudPWBord::Init(void)
{
	m_iOn = 0;
	m_select = 0;
	m_light1 = 0;
	m_light2 = 0;
	m_light3 = 0;
	m_light4 = 0;
	m_light5 = 0;
	m_light6 = 0;
	m_light7 = 0;
	m_light8 = 0;
	m_light9 = 0;

	HOOK_MESSAGE(WPWBord);

	m_iFlags |= HUD_ACTIVE;
 
	gHUD.AddHudElem(this);

	return 1;
};


int CHudPWBord::VidInit(void)
{
	m_hSprite1 = LoadSprite("sprites/nbpwb_1.spr");
	m_hSprite2 = LoadSprite("sprites/nbpwb_2.spr");
	m_hSprite3 = LoadSprite("sprites/nbpwb_3.spr");

	return 1;
};


int CHudPWBord:: MsgFunc_WPWBord(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	m_select = READ_BYTE();
	m_light1 = READ_BYTE();
	m_light2 = READ_BYTE();
	m_light3 = READ_BYTE();
	m_light4 = READ_BYTE();
	m_light5 = READ_BYTE();
	m_light6 = READ_BYTE();
	m_light7 = READ_BYTE();
	m_light8 = READ_BYTE();
	m_light9 = READ_BYTE();

	return 1;
}

int CHudPWBord::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) || m_iOn == 0 )
		return 1;

	int y = (ScreenHeight -320) / 2;
	int x = (ScreenWidth - 256) / 2;
	int x2,y2;

	SPR_Set(m_hSprite1, 255, 255, 255 );
	SPR_DrawHoles( 0, x, y, NULL);

	x += 30;
	y += 59;
	if(m_light1 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 1)
	{
	x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 74;
	if(m_light2 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 2)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 76;
	if(m_light3 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 3)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x -= 150;
	y += 82;
	if(m_light4 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 4)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 74;
	if(m_light5 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
		}
	if(m_select == 5)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 76;
	if(m_light6 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 6)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x -= 150;
	y += 80;
	if(m_light7 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 7)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 74;
	if(m_light8 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 8)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	x += 76;
	if(m_light9 == 1)
	{
		SPR_Set(m_hSprite2, 255, 255, 255 );
		SPR_DrawAdditive( 0, x, y, NULL);
	}
	if(m_select == 9)
	{
		x2 = x - 10;
		y2 = y - 10;
		SPR_Set(m_hSprite3, 255, 255, 255 );
		SPR_DrawHoles( 0, x2, y2, NULL);
	}
	return 1;
}