#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
extern int iMouseInUse;
DECLARE_MESSAGE(m_AirBar, CheckAirbar)

int CHudAirBar::Init(void)
{
	m_iOn = 0;
	m_flHealth = 0;

	HOOK_MESSAGE(CheckAirbar);

	m_iFlags |= HUD_ACTIVE;
 
	gHUD.AddHudElem(this);

	return 1;
};


int CHudAirBar::VidInit(void)
{
	int HUD_flash_d = gHUD.GetSpriteIndex( "health_check_new2" );
	int HUD_flash_j = gHUD.GetSpriteIndex( "health_check_new1" );

	m_hSprite3 = LoadSprite("sprites/air_icon.spr");

	m_hSprite1 = gHUD.GetSprite(HUD_flash_d);
	m_hSprite2 = gHUD.GetSprite(HUD_flash_j);

	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc2 = &gHUD.GetSpriteRect(HUD_flash_j);

	return 1;
};


int CHudAirBar:: MsgFunc_CheckAirbar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	int x = READ_BYTE();
	m_iMaxHealth = READ_BYTE();

	if(m_iMaxHealth >= 250)
	{
		m_flHealth = x * 200 / m_iMaxHealth;
	}
	else
	{
		m_flHealth = x * 160 / m_iMaxHealth;
	}

	if(m_flHealth < 1)
		m_flHealth = 1;

	return 1;
}

int CHudAirBar::Draw(float flTime)
{
	if ( iMouseInUse || gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) || m_iOn == 0 )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int r, g, b, x, y, a;

	a = 240;
	x = ScreenWidth * 0.005 + 35;
	y = ScreenHeight - (64 + ScreenHeight * 0.005);

	if(m_iMaxHealth >= 250)
	{
		m_prc1->right = 200;
	}
	else
	{
		m_prc1->right = 160;
	}

	SPR_Set(m_hSprite1, 80, 80, 80 );
	SPR_DrawHoles( 0,  x, y, m_prc1);
	ScaleColors(r, g, b, a );

	if(m_iOn == 2)
	{
		UnpackRGB(r,g,b, 0x00FFFFFF);
	}
	else
	{
		UnpackRGB(r,g,b, 0x00FFFF00);
	}

	SPR_Set(m_hSprite3, 255, 255, 255 );
	SPR_DrawHoles( 0,  x - 35, y - 7, NULL);

	if(m_flHealth >= 1)
	{
		a = 120;
		m_prc2->right = m_flHealth;
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0,  x, y, m_prc2);
	}

	return 1;
}