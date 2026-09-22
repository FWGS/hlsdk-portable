#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

extern int iMouseInUse;

DECLARE_MESSAGE(m_ArmorBar, CheckAPbar)

int CHudArmorBar::Init(void)
{
	m_iOn = 0;
	m_flHealth = 0;

	HOOK_MESSAGE(CheckAPbar);

	m_iFlags |= HUD_ACTIVE;
 
	gHUD.AddHudElem(this);

	return 1;
};


int CHudArmorBar::VidInit(void)
{
	int HUD_flash_d = gHUD.GetSpriteIndex( "health_check_new2" );
	int HUD_flash_j = gHUD.GetSpriteIndex( "health_check_new1" );

	m_hSprite1 = gHUD.GetSprite(HUD_flash_d);
	m_hSprite2 = gHUD.GetSprite(HUD_flash_j);
	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc2 = &gHUD.GetSpriteRect(HUD_flash_j);

	m_hSprite3 = LoadSprite("sprites/fdm_apicon.spr");
	m_hSprite4 = LoadSprite("sprites/fdm_darkicon.spr");

	return 1;
};


int CHudArmorBar:: MsgFunc_CheckAPbar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	m_flHealth = READ_BYTE();
	m_iMaxHealth = READ_BYTE();
	m_inumber = READ_SHORT();

	if(m_flHealth < 0)
	{
		m_flHealth = 0;
	}
	if(m_inumber > 999)
	{
		m_inumber = 999;
	}

	return 1;
}

int CHudArmorBar::Draw(float flTime)
{
	if ( iMouseInUse || gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) || m_iOn == 0 )
		return 1;

	int r, g, b, x, y, a;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	x = 300 + ScreenWidth * 0.005;
	y = ScreenHeight - (32 + ScreenHeight * 0.005);

	SPR_Set(m_hSprite4, 255, 255, 255 );
	SPR_DrawHoles( 0,  x + 1, y, NULL);
	SPR_DrawHoles( 0,  x - 1, y, NULL);
	SPR_DrawHoles( 0,  x    , y - 1, NULL);

    SPR_Set(m_hSprite3, 255, 255, 255 );
	SPR_DrawAdditive( 0,  x, y, NULL);
	
	a = 240;
	x = 300 + ScreenWidth * 0.005 + 36;
	y = ScreenHeight - (27 + ScreenHeight * 0.005);
	SPR_Set(m_hSprite1, 80, 80, 80 );
	m_prc1->right = m_iMaxHealth;
	SPR_DrawHoles( 0,  x, y, m_prc1);
	ScaleColors(r, g, b, a );

	UnpackRGB(r,g,b, 0x004040C0);

	if(m_flHealth > 0)
	{
		a = 180;
		m_prc2->right = m_flHealth;
		ScaleColors(r, g, b, a );
		SPR_Set(m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0,  x, y, m_prc2);
	}

	UnpackRGB(r,g,b, 0x00FFFFFF);
	int z = y - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	z = gHUD.DrawHudNumber(x+6, y, DHN_3DIGITS | DHN_DRAWZERO, m_inumber, r, g, b);

	return 1;
}