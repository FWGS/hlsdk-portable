/***
*
*	(C) 2008 Vyacheslav Dzhura
*
****/
//
// mode.cpp
//
// implementation of CHudModeIcon class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_AlienCrosshair, AlienState)

// gHUD.m_AlienCrosshair.m_iState

/*
class CHudAlienCrosshair: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset( void );
	int MsgFunc_AlienState(const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hCrosshair[4];
	wrect_t *m_prcCrosshair[4]; 
	int	  m_iState;
};
*/

int CHudAlienCrosshair::Init(void)
{
	m_iState = 0;

	HOOK_MESSAGE(AlienState);

	m_iFlags |= HUD_ACTIVE;
	m_iFlags |= HUD_ALIEN;

	gHUD.AddHudElem(this);

	return 1;
};

void CHudAlienCrosshair::Reset(void)
{
	m_iState = 0;
}

int CHudAlienCrosshair::VidInit(void)
{
	int sCenter = gHUD.GetSpriteIndex( "islave_center" );
	int sCharged = gHUD.GetSpriteIndex( "islave_charged" );
	int sInner = gHUD.GetSpriteIndex( "islave_inner" );
    int sOuter = gHUD.GetSpriteIndex( "islave_outer" );

	if ( (sCenter == -1) || (sCharged == -1) || (sInner == -1) || (sOuter == -1) )
		return 0;

	m_hCrosshair[0] = gHUD.GetSprite(sOuter);
	m_hCrosshair[1] = gHUD.GetSprite(sInner);
	m_hCrosshair[2] = gHUD.GetSprite(sCenter);
	m_hCrosshair[3] = gHUD.GetSprite(sCharged);

	m_prcCrosshair[0] = &gHUD.GetSpriteRect( sOuter );
	m_prcCrosshair[1] = &gHUD.GetSpriteRect( sInner );
	m_prcCrosshair[2] = &gHUD.GetSpriteRect( sCenter );
	m_prcCrosshair[3] = &gHUD.GetSpriteRect( sCharged );

	return 1;
};

int CHudAlienCrosshair:: MsgFunc_AlienState(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iState = READ_BYTE();

	return 1;
}

int CHudAlienCrosshair::Draw(float flTime)
{
    if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL ) )
		return 1;

	if ( !gHUD.m_bAlienMode )
		return 1;

	if (m_iState > 3)
	  m_iState = 3;

	int chR, chG, chB;
	chR = chG = chB = 0;

	for (int i = 0; i < 4; i++ )
	{
		if ( i > m_iState )
			break;

		m_hActiveSprite = m_hCrosshair[i];
		m_prcActiveRect = m_prcCrosshair[i];

		if ( i < 3 )
		{  // 180, 255, 96
			chR = 180;
			chG = 255;
			chB = 96;
		} else
		{
			chR = 255;
			chG = 0;
			chB = 0;
		}

		if (( i == 2 ) && ( m_iState == 3 ))
			continue;

		int x,y, SWidth, SHeight;
		SWidth = m_prcActiveRect->right - m_prcActiveRect->left;
		SHeight = m_prcActiveRect->bottom - m_prcActiveRect->top; // SPR_Height(m_hActiveSprite,0);
		x = ScreenWidth / 2 - ( SWidth / 2 );
		y =	ScreenHeight / 2 - ( SHeight / 2 );

		SPR_Set(m_hActiveSprite, chR, chG, chB );
		SPR_DrawAdditive(0, x, y, m_prcActiveRect);
	}	
	
	//char szMes[20];
    //sprintf(szMes,"%d %d/%d", m_fMode, SWidth, SHeight);
	//gHUD.DrawHudString( 5, 5, ScreenWidth, szMes, r, g, b); 
	return 1;
}