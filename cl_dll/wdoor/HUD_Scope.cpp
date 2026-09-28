#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>


#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"

#include "eventscripts.h"

#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"

#include <string.h>

extern int iMouseInUse;

#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;

DECLARE_MESSAGE(m_GunScope, FGunScope)

int CHudGunScope::Init(void)
{
	m_iHudMode = 0;
	m_ifucktime = 0;
	HOOK_MESSAGE(FGunScope);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
};

int CHudGunScope::VidInit(void)
{
	//int HUD_scope;
	//HUD_scope = gHUD.GetSpriteIndex( "hud_scope" );
	m_hCrosshair = SPR_Load("sprites/hud_scope.spr");
	m_hCrosshair2 = SPR_Load("sprites/super_cine.spr");
//	m_hCrosshair3 = SPR_Load("sprites/the_end.spr");
//	m_hCrosshair4 = SPR_Load("sprites/yjsp.spr");
//	m_hCrosshair5 = SPR_Load("sprites/horror_cg.spr");

	return 1;
};

int CHudGunScope:: MsgFunc_FGunScope(const char *pszName,  int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iHudMode = READ_BYTE();
	return 1;
}

void DrawQuad(float xmin, float ymin, float xmax, float ymax)
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

int CHudGunScope::Draw(float flTime)
{
	if ( !IEngineStudio.IsHardware())//�������٣������¼�
	{
		gEngfuncs.pfnClientCmd("echo !IEngineStudio.IsHardware()\n");
		gEngfuncs.pfnClientCmd("echo ������������ģʽ!\n");
		gEngfuncs.pfnClientCmd("disconnect\n");
	}
	else if (ScreenHeight < 600 || ScreenWidth < 800 )//�ֱ��ʹ��;���
	{
		gEngfuncs.pfnClientCmd("echo ScreenHeight < 600 || ScreenWidth < 800\n");
		gEngfuncs.pfnClientCmd("echo �ֱ��ʹ���!\n");
		gEngfuncs.pfnClientCmd("disconnect\n");
	}

	/*
	else if(CVAR_GET_FLOAT( "r_detailtextures" ) != 1){//��OPenGLģʽ
			m_ifucktime++;
			gEngfuncs.Cvar_SetValue( "r_detailtextures", 1.0 );
			if(m_ifucktime >= 10){
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			gEngfuncs.pTriAPI->Brightness(1.0);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			float left = (ScreenWidth - ScreenHeight)/2;
			float right = left + ScreenHeight;
			float centerx = ScreenWidth;
			float centery = ScreenHeight;
			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair4 ), 1);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, centerx, centery);

			gEngfuncs.pTriAPI->End();
			}
	}*/
	//��������
	/*
	if(IEngineStudio.GetCvar( "r_fullbright" )->value != 0){
	gEngfuncs.Cvar_SetValue( "r_fullbright", 0 );
	}
	if(IEngineStudio.GetCvar( "r_drawentities" )->value != 1){
	gEngfuncs.Cvar_SetValue( "r_drawentities", 1 );
	}
	if(IEngineStudio.GetCvar( "hud_draw" )->value != 1){
	gEngfuncs.Cvar_SetValue( "hud_draw", 1 );
	}
	if(IEngineStudio.GetCvar( "r_dynamic" )->value != 1){
	gEngfuncs.Cvar_SetValue( "r_dynamic", 1 );
	}
	if(IEngineStudio.GetCvar( "suitvolume" )->value != 0){
	gEngfuncs.Cvar_SetValue( "suitvolume", 0 );
	}
	if(IEngineStudio.GetCvar( "host_limitlocal" )->value != 0){
	gEngfuncs.Cvar_SetValue( "host_limitlocal", 0 );
	}
	*/

	if(iMouseInUse){
		if(m_iHudMode == 4)//draw the dark
		{
			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			gEngfuncs.pTriAPI->Brightness(1.0);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			float left = (ScreenWidth - ScreenHeight)/2;
			float right = left + ScreenHeight;
			float centerx = ScreenWidth;
			float centery = ScreenHeight;

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, centerx, centery);

			gEngfuncs.pTriAPI->End();
		}
		else if(m_iHudMode == 3)//draw the end
		{
	
		}
		else if(m_iHudMode == 2)//draw alternative scope 2
		{
			gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
			gEngfuncs.pTriAPI->Brightness(1.0);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			float left = (ScreenWidth - ScreenHeight)/2;
			float right = left + ScreenHeight;
			float centerx = ScreenWidth/2;
			float centery = ScreenHeight/2;

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 9);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(left, 0, centerx, centery);
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 10);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(centerx, 0, right, centery);
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 11);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(centerx, centery, right, ScreenHeight);
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 12);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(left, centery, centerx, ScreenHeight);
			gEngfuncs.pTriAPI->End();

			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer(m_hCrosshair), 0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, left+1, ScreenHeight);
			DrawQuad(right-1, 0, ScreenWidth, ScreenHeight);
			gEngfuncs.pTriAPI->End();
		}
		else{
			gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
			gEngfuncs.pTriAPI->Brightness(1.0);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			float left = (ScreenWidth - ScreenHeight)/2;
			float right = left + ScreenHeight;
			float centerx = ScreenWidth;
			float centery = ScreenHeight;

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair2 ), 0);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, centerx, centery);

			gEngfuncs.pTriAPI->End();
		}

		return 1;
	}
	
	if(!m_hCrosshair){
	return 0;
	}

	if(!m_iHudMode){
	return 0;//draw scope
	}
	/*
		if(m_iHudMode == 5)//draw the end
		{
			int frame = (int)(flTime * 10) % SPR_Frames(m_hCrosshair5);

			gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			gEngfuncs.pTriAPI->Brightness(1.0);
			gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
			gEngfuncs.pTriAPI->CullFace(TRI_NONE);
			float left = (ScreenWidth - ScreenHeight)/2;
			float right = left + ScreenHeight;
			float centerx = ScreenWidth;
			float centery = ScreenHeight;

			if(frame <= 14)
			frame += 1;

			gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair5 ), frame);
			gEngfuncs.pTriAPI->Begin(TRI_QUADS);
			DrawQuad(0, 0, centerx, centery);

			gEngfuncs.pTriAPI->End();
			return 1;
		}
	*/

	gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Color4ub(255, 255, 255, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	float left = (ScreenWidth - ScreenHeight)/2;
	float right = left + ScreenHeight;
	float centerx = ScreenWidth/2;
	float centery = ScreenHeight/2;

	if(m_iHudMode == 1)//draw alternative scope
	{
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 5);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 6);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, 0, right, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 7);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, centery, right, ScreenHeight);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 8);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, centery, centerx, ScreenHeight);
		gEngfuncs.pTriAPI->End();
	}
	else if(m_iHudMode == 2)//draw alternative scope 2
	{
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 9);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 10);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, 0, right, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 11);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, centery, right, ScreenHeight);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 12);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, centery, centerx, ScreenHeight);
		gEngfuncs.pTriAPI->End();
	}
	else if(m_iHudMode == 3)
	{
		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 1);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, 0, centerx, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 2);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, 0, right, centery);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 3);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(centerx, centery, right, ScreenHeight);
		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer( m_hCrosshair ), 4);
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		DrawQuad(left, centery, centerx, ScreenHeight);
		gEngfuncs.pTriAPI->End();
	}

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
	gEngfuncs.pTriAPI->SpriteTexture((struct model_s *)gEngfuncs.GetSpritePointer(m_hCrosshair), 0);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);
	DrawQuad(0, 0, left+1, ScreenHeight);
	DrawQuad(right-1, 0, ScreenWidth, ScreenHeight);
	gEngfuncs.pTriAPI->End();
	return 1;
}