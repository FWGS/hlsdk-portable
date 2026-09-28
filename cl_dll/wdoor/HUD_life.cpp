#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_loadlife, FLoadLife)
extern int iMouseInUse;

int CHudLoadLife::Init(void)
{
	m_iOn = 0;
	m_flHealth = 0;

	HOOK_MESSAGE(FLoadLife);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

int CHudLoadLife:: MsgFunc_FLoadLife(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	m_flHealth = READ_SHORT();
	m_type = READ_BYTE();
	m_flHealth2 = READ_LONG();

	if(m_flHealth < 0)
		m_flHealth = 0;

	if(m_flHealth2 < 0)
		m_flHealth2 = 0;

	return 1;
}

int CHudLoadLife::VidInit(void)
{
	int HUD_flash_d = gHUD.GetSpriteIndex( "boss_hp_bar" );
	m_hSprite1 = gHUD.GetSprite(HUD_flash_d);
	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_d);

    m_hSprite3 = LoadSprite("sprites/boss_hpbar2.spr");
	return 1;
};

int CHudLoadLife::Draw(float flTime)
{
	if ( iMouseInUse || gHUD.m_iHideHUDDisplay & ( HIDEHUD_HEALTH ) || m_iOn == 0 )
		return 1;

	int r, g, b, x, y, a,x2,y2;

	x = ScreenWidth / 2 -256;
	y = ScreenHeight * 0.01;

	x2 = x + 208;
	y2 = y + 4;

	UnpackRGB(r,g,b, RGB_WHITE);

	m_prc1->right = m_flHealth;

	a = 255;

    SPR_Set(m_hSprite3, r, g, b );
	SPR_Draw( 0,  x, y, NULL);

	ScaleColors(r, g, b, a );
	SPR_Set(m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0,  x, y, m_prc1);

	gHUD.DrawHudNumberLarge(x2, y2, DHN_3DIGITS | DHN_DRAWZERO, m_flHealth2, 255, 255, 255);

	return 1;
}