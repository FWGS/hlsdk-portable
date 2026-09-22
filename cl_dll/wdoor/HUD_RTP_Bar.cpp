#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
extern int iMouseInUse;
DECLARE_MESSAGE(m_RTPbar, CheckRTPbar)

int CHudRTPbar::Init(void)
{
	m_iOn = 0;
	m_flHealth = 0;

	HOOK_MESSAGE(CheckRTPbar);

	m_iFlags |= HUD_ACTIVE;
 
	gHUD.AddHudElem(this);

	return 1;
};


int CHudRTPbar::VidInit(void)
{
	int HUD_flash_d = gHUD.GetSpriteIndex( "healthcheck_l" );
	int HUD_flash_j = gHUD.GetSpriteIndex( "healthcheck_l" );
	m_hSprite3 = LoadSprite("sprites/button_push.spr");

	m_hSprite1 = gHUD.GetSprite(HUD_flash_d);
	m_hSprite2 = gHUD.GetSprite(HUD_flash_j);

	m_prc1 = &gHUD.GetSpriteRect(HUD_flash_d);
	m_prc2 = &gHUD.GetSpriteRect(HUD_flash_j);

	return 1;
};


int CHudRTPbar:: MsgFunc_CheckRTPbar(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iOn = READ_BYTE();
	int x = READ_BYTE();
	m_iMaxHealth = READ_BYTE();

	m_flHealth = x * 256 / m_iMaxHealth;

	if(m_flHealth < 0)
		m_flHealth = 0;

	return 1;
}

int CHudRTPbar::Draw(float flTime)
{
	if ( iMouseInUse || gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) || m_iOn == 0 )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int y = ScreenHeight / 2;
	int x = (ScreenWidth - 256) / 2;

	m_prc1->right = 256;
	SPR_Set(m_hSprite1, 255, 255, 255 );
	SPR_DrawHoles( 0, x, y, m_prc1);

	m_prc2->right = m_flHealth;
	SPR_Set(m_hSprite2, 255, 255, 0 );
	SPR_DrawAdditive( 0,  x, y, m_prc2);

	m_prc2->right = m_flHealth;
	SPR_Set(m_hSprite2, 255, 255, 0 );
	SPR_DrawAdditive( 0,  x, y, m_prc2);

	int frame;
	SPR_Set(m_hSprite3, 255, 255, 255 );
	frame = (int)(flTime * 20) % SPR_Frames(m_hSprite3);
	SPR_DrawHoles( frame,  x + 280, y - 50, NULL);

	return 1;
}