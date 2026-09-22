#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_GameOver, FGameOver)

int CHudGameOver::Init(void)
{
	m_ifloat1 = 0;
	m_ifloat2 = 0;
	HOOK_MESSAGE(FGameOver);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

int CHudGameOver::VidInit(void)
{

	m_hSprite1 = LoadSprite("sprites/game_over1.spr");
	m_hSprite2 = LoadSprite("sprites/game_over2.spr");
	m_hSprite3 = LoadSprite("sprites/3days.spr");

	return 1;
};

int CHudGameOver:: MsgFunc_FGameOver(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_ifloat1 = READ_BYTE();
	m_ifloat2 = READ_BYTE();

	return 1;
}

int CHudGameOver::Draw(float flTime)
{
	if ( m_ifloat2 == 0  ){
	return 1;
	}

	int r, g, b, x, y, a;

	x = (ScreenWidth / 2) - 256;
	y = (ScreenHeight / 2) - 64;

	if ( m_ifloat2 == 1  )
	{
		UnpackRGB(r,g,b, RGB_WHITE);	
		a = m_ifloat1;
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite1, r, g, b );
		SPR_DrawAdditive( gEngfuncs.pfnRandomLong ( 0 , 1 ),  x, y, NULL);

		UnpackRGB(r,g,b, RGB_WHITE);
		x = ScreenWidth / 2;
		a = m_ifloat1;
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive( gEngfuncs.pfnRandomLong ( 0 , 1 ),  x, y, NULL);
	}
	else
	{
		x = (ScreenWidth / 2) - 128;

		UnpackRGB(r,g,b, RGB_WHITE);	
		a = m_ifloat1;
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite3, r, g, b );
	
		SPR_DrawAdditive( 0,  x, y, NULL);
	}
	

	return 1;
}