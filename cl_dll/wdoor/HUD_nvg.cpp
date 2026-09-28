#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"
#include "vgui_TeamFortressViewport.h"

#include "triangleapi.h"

DECLARE_MESSAGE(m_NVG, NVGActivate )

int CHudNVG::Init(void)
{
	HOOK_MESSAGE( NVGActivate );
	m_iOn = 0;
	m_color = 0;

	m_iFlags = 0;

	gHUD.AddHudElem(this);
	return 1;
}


int CHudNVG::VidInit(void)
{
	m_hFlicker = LoadSprite("sprites/nvg.spr");
	m_hFlicker2 = LoadSprite("sprites/nvg_noise.spr");
	m_hFlicker3 = LoadSprite("sprites/dark_noisy.spr");
	return 1;
}

int CHudNVG::Draw(float fTime)
{
	if (m_iOn)
	{
		int x, y, w, h;
		int frame;
		int x2, y2, w2, h2;
		int frame2;

		if(m_color == 2)
		{
			SPR_Set(m_hFlicker, 48, 48, 48 );// COLOR WHITE
			frame = (int)(fTime * 15) % SPR_Frames(m_hFlicker);
			w = SPR_Width(m_hFlicker,0);
			h = SPR_Height(m_hFlicker,0);
			for(y = -(rand() % h); y < ScreenHeight; y += h) 
			{
				for(x = -(rand() % w); x < ScreenWidth; x += w) 
				{
					SPR_DrawAdditive( frame, x, y, NULL );
				}
			}

			SPR_Set(m_hFlicker2, 64, 64, 64 );// COLOR WHITE
				
			frame2 = (int)(fTime * 5) % SPR_Frames(m_hFlicker2);//15
			w2 = SPR_Width(m_hFlicker2,0);
			h2 = SPR_Height(m_hFlicker2,0);
			for(y2 = -(rand() % h2); y2 < ScreenHeight; y2 += h2) 
			{
				for(x2 = -(rand() % w2); x2 < ScreenWidth; x2 += w2) 
				{
					SPR_DrawAdditive( frame2, x2, y2, NULL );
				}
			}
		}
		else if(m_color == 1)
		{
			SPR_Set(m_hFlicker3, 0, 0, 0 );// DARK
			frame = (int)(fTime * 20) % SPR_Frames(m_hFlicker3);
			w = SPR_Width(m_hFlicker3,0);
			h = SPR_Height(m_hFlicker3,0);
			for(y = -(rand() % h); y < ScreenHeight; y += h) 
			{
				for(x = -(rand() % w); x < ScreenWidth; x += w) 
				{
					SPR_DrawHoles( frame, x, y, NULL );
					SPR_DrawHoles( frame, x, y, NULL );
				}
			}
		}
		else
		{
			SPR_Set(m_hFlicker, 128, 16, 16 );// COLOR RED
			frame = (int)(fTime * 15) % SPR_Frames(m_hFlicker);
			w = SPR_Width(m_hFlicker,0);
			h = SPR_Height(m_hFlicker,0);
			for(y = -(rand() % h); y < ScreenHeight; y += h) 
			{
				for(x = -(rand() % w); x < ScreenWidth; x += w) 
				{
					SPR_DrawAdditive( frame, x, y, NULL );
				}
			}

			SPR_Set(m_hFlicker2, 255, 32, 32 );// COLOR RED
				
			frame2 = (int)(fTime * 5) % SPR_Frames(m_hFlicker2);//15
			w2 = SPR_Width(m_hFlicker2,0);
			h2 = SPR_Height(m_hFlicker2,0);
			for(y2 = -(rand() % h2); y2 < ScreenHeight; y2 += h2) 
			{
				for(x2 = -(rand() % w2); x2 < ScreenWidth; x2 += w2) 
				{
					SPR_DrawAdditive( frame2, x2, y2, NULL );
				}
			}
		}		
	}
	return 1;
}

int CHudNVG::MsgFunc_NVGActivate(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iOn = READ_BYTE();
	m_color = READ_BYTE();
	
	if (m_iOn==1)
	{
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;
	}

	return 1;
}
void CHudNVG::Reset(void)
{
	m_iOn = 0;
}
