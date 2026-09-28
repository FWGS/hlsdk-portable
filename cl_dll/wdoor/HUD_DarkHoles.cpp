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

extern int iMouseInUse;

#include "triangleapi.h"

DECLARE_MESSAGE(m_DarkHoles, FDarkHoles )

void DrawQuad2(float xmin, float ymin, float xmax, float ymax)
{
	//top left
	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3f(xmin, ymin, 0); 
	//bottom left
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3f(xmin, ymax, 0);
	//bottom right
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3f(xmax, ymax, 0);
	//top right
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3f(xmax, ymin, 0);
}


int CHudDarkHoles::Init(void)
{
	HOOK_MESSAGE( FDarkHoles );

	m_iOn = 0;
	m_alpha = 0;
	m_ddf = 0;

	m_iFlags = 0;

	gHUD.AddHudElem(this);

	return 1;
}


int CHudDarkHoles::VidInit(void)
{
	m_hSprite1 = SPR_Load("sprites/fdm_nohp.spr");

	return 1;
}
int CHudDarkHoles::MsgFunc_FDarkHoles(const char *pszName, int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	m_iOn = READ_BYTE();
	m_alpha = READ_BYTE();
	m_ddf = READ_BYTE();

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
void CHudDarkHoles::Reset(void)
{
	m_iOn = 0;
	m_ddf = 0;
}

int CHudDarkHoles::Draw(float fTime)
{
	if ( iMouseInUse )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	if(m_ddf == 1)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Brightness(1.0);
		gEngfuncs.pTriAPI->Color4ub(255, 0, 0, 255);
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		float left = (ScreenWidth - ScreenHeight)/2;
		float right = left + ScreenHeight;
		float centerx = ScreenWidth;
		float centery = ScreenHeight;
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite1 ), 0);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad2(0, 0, centerx, centery);
		DrawQuad2(0, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();
	}
	else if(m_ddf == 2)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Brightness(1.0);
		gEngfuncs.pTriAPI->Color4ub(0, 0, 0, 255);
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		float left = (ScreenWidth - ScreenHeight)/2;
		float right = left + ScreenHeight;
		float centerx = ScreenWidth;
		float centery = ScreenHeight;
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite1 ), 0);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad2(0, 0, centerx, centery);
		DrawQuad2(0, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();
	}
	else if(m_ddf == 3)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Brightness(1.0);
		gEngfuncs.pTriAPI->Color4ub(0, 255, 0, 255);
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		float left = (ScreenWidth - ScreenHeight)/2;
		float right = left + ScreenHeight;
		float centerx = ScreenWidth;
		float centery = ScreenHeight;
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite1 ), 0);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad2(0, 0, centerx, centery);
		DrawQuad2(0, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();
	}
	else if(m_ddf == 4)
	{
		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Brightness(1.0);
		gEngfuncs.pTriAPI->Color4ub(255, 255, 128, 255);
		gEngfuncs.pTriAPI->CullFace(TRI_NONE);
		float left = (ScreenWidth - ScreenHeight)/2;
		float right = left + ScreenHeight;
		float centerx = ScreenWidth;
		float centery = ScreenHeight;
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hSprite1 ), 0);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad2(0, 0, centerx, centery);
		DrawQuad2(0, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();
	}

	return 1;
}