//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					lensflare.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code de la lampe des grunts 						---
//---------------------------------------------------------
//---------------------------------------------------------



//---------------------------------------------------------
//inclusions

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "parsemsg.h"
#include "triangleapi.h"
#include "cl_entity.h"
#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif
#include "pmtrace.h"
#include "pm_defs.h"
#include "event_api.h"

extern void VectorAngles( const float *forward, float *angles );
extern vec3_t v_origin, v_angles;			// angles de la vue du joueur

#define FLASH_SPR			"sprites/spot01.spr"
#define	MAX_FLARE			12
#define FLASH_BLANC_SIZE	35


int lensflare_spritevars [MAX_FLARE] [3] =
{
/*distance - diametre - brightness*/

	{ 15,	6,		25 },		// grands du bout
//	{ 0,	50,		220 },		// flash

	{ 35,	7,		33 },		// marron au milieu
	{ 50,	9,		33 },		// marron au milieu
	{ 65,	7,		30 },		// marron au milieu

	{ 75,	3,		40 },		// petits jaunes
	{ 79,	8,		35 },
	{ 83,	3,		50 },

	{ 95,	2,		40 },		// petits bleus
	{ 97,	4,		120 },
	{ 100,	2,		40 },

	{ 110,	20,		60 },		// grands du bout
	{ 120,	30,		50 },
};

char lensflare_spritename [MAX_FLARE] [128] =
{
	"sprites/lensflare01.spr",
//	"sprites/lensflare05.spr",

	"sprites/lensflare01.spr",
	"sprites/lensflare01.spr",
	"sprites/lensflare01.spr",

	"sprites/lensflare04.spr",
	"sprites/lensflare04.spr",
	"sprites/lensflare04.spr",

	"sprites/lensflare02.spr",
	"sprites/lensflare03.spr",
	"sprites/lensflare02.spr",

	"sprites/lensflare01.spr",
	"sprites/lensflare01.spr",
};

//------------------------------------
//
// d
// gmsgLFlammes

DECLARE_MESSAGE(m_LensFlare, LensFlare );


//------------------------------------
//
// gestion des messages serveur


int CHudLensFlare::MsgFunc_LensFlare( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int mode	= READ_BYTE();

	if ( mode == 1 )	// allume
	{
		gruntlight_t *p = NULL;
		p = new gruntlight_t;

		p->index = READ_BYTE();

		p->pNext = m_pLight;
		m_pLight = p;

		m_iFlags |= HUD_ACTIVE;

	}

	else if ( mode  == 0 )
	{
		int idx		= READ_BYTE();

		// destruction d'une flamme

		gruntlight_t *p = NULL;
		gruntlight_t *q = NULL;
		p = m_pLight;

		while ( p != NULL )
		{
			if ( p->index == idx )
			{
				if ( q == NULL )
				{
					// premi

					m_pLight = p->pNext,

					delete p;
					break;
				}
				else
				{
					q->pNext = p->pNext;
					delete p;

					break;
				}
			}

			q = p;
			p = p->pNext;
		}
	}

	return 1;
}

//------------------------------------
//
// destruction du registre des lampes

void CHudLensFlare :: ClearLights ( void )
{
	while ( m_pLight != NULL )
	{
		gruntlight_t *p = NULL;
		p = m_pLight;

		m_pLight = m_pLight->pNext;
		delete p;
	}
}



//------------------------------------
//
// rafraichissement de l'affichage


int CHudLensFlare :: Draw	( float flTime )
{
	return 1;
}


void CHudLensFlare :: DrawLight ( void )
{
	// hud 
	if ( ! (m_iFlags & HUD_ACTIVE))
		return;

	// initialisation triapi

	int modelindex;
	struct model_s *mod = gEngfuncs.CL_LoadModel( FLASH_SPR , &modelindex );
	gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, 0 );	

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling
	gEngfuncs.pTriAPI->Brightness( 0.15 );						
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 0.75, 0.5 );

	vec3_t v_forward, v_right, v_up, vertex;
	AngleVectors ( v_angles, v_forward, v_right, v_up );


	gruntlight_t *p = m_pLight;

	while ( p != NULL )
	{
		
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex( p->index );

		// vecteurs

		// attach 0 : muzzleflash
		// attach 1 : right hand

		vec3_t vecOrigin = ent->attachment[2];
		vec3_t vecDirFlash = ( ent->attachment[3] - ent->attachment[2] ).Normalize();

		pmtrace_t tr = *( gEngfuncs.PM_TraceLine( vecOrigin, vecOrigin+vecDirFlash*256, PM_WORLD_ONLY, 2, 1 ) );


/*		pmtrace_t tr;
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		gEngfuncs.pEventAPI->EV_PlayerTrace( vecOrigin, vecOrigin+vecDirFlash*256, PM_WORLD_ONLY, -1, &tr );					
*/
	
		gEngfuncs.pTriAPI->Begin( TRI_TRIANGLES );							//d

			gEngfuncs.pTriAPI->TexCoord2f( 0.5, 1 );
			vertex = vecOrigin;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1-tr.fraction );
			vertex = vecOrigin + vecDirFlash*256*tr.fraction + v_up * 12*tr.fraction;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1-tr.fraction );
			vertex = vecOrigin + vecDirFlash*256*tr.fraction - v_up * 12*tr.fraction;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );


		gEngfuncs.pTriAPI->End();									//fin du trac

		p = p->pNext;

	}

	// ----------- lens flare-----------

	p = m_pLight;

	while ( p != NULL )
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex( p->index );

		vec3_t vecOrigin = ent->attachment[3];
		vec3_t vecDirFlash = ( ent->attachment[3] - ent->attachment[2] ).Normalize();
		vec3_t vecScrPos;

		// test de pr

		if ( gEngfuncs.pTriAPI->WorldToScreen ( vecOrigin, vecScrPos ) )
		{
			p = p->pNext;
			continue;
		}

		vecScrPos [0] = XPROJECT ( vecScrPos [0] );
		vecScrPos [1] = YPROJECT ( vecScrPos [1] );

		if ( vecScrPos[0] < XRES(-10) || vecScrPos[0] > XRES(650) || vecScrPos[1] < YRES(-10) || vecScrPos[1] > YRES(490) )
		{
			p = p->pNext;
			continue;
		}

		// test de direction

		vec3_t vecAngFlash, vecAngDelta;
		VectorAngles ( -vecDirFlash, vecAngFlash );
		VectorAngles ( ((vecOrigin-v_origin).Normalize()), vecAngDelta );

		float flDifX = (float)fabs( (int)(vecAngFlash.x-vecAngDelta.x)%360 );
		flDifX = flDifX > 180 ? flDifX - 360 : flDifX;
		flDifX = flDifX < -180 ? flDifX + 360 : flDifX;

		float flDifY = (float)fabs( (int)(vecAngDelta.y-vecAngFlash.y)%360 );
		flDifY = flDifY > 180 ? flDifY - 360 : flDifY;
		flDifY = flDifY < -180 ? flDifY + 360 : flDifY;

		if ( fabs( flDifX ) > 45 ||	fabs( flDifY ) > 45 )
		{
			p = p->pNext;
			continue;
		}


		// test de visibilit

		pmtrace_t tr = *( gEngfuncs.PM_TraceLine( v_origin, vecOrigin, PM_TRACELINE_ANYVISIBLE, 2, 1 ) );

		if ( tr.fraction != 1.0 )
		{
			p = p->pNext;
			continue;
		}

		// brillance - position sur l'

		float brightnessratio = 1;

		float pixelDist = sqrt ( (vecScrPos[0]-XRES(320))*(vecScrPos[0]-XRES(320))
								+(vecScrPos[1]-YRES(240))*(vecScrPos[1]-YRES(240)) );

		float maxdist = sqrt ( XRES(330)*XRES(330) + YRES(250)*YRES(250) );

		brightnessratio *= pixelDist < XRES(80) ? 1.2 : 1.2 * ( 1 - (pixelDist-XRES(80)) / (maxdist-XRES(80)));


		// brillance - angle

		brightnessratio *= fabs( flDifY ) < 25 ? 1 : 1 - ( (fabs(flDifY)-25)/20 );
		brightnessratio *= fabs( flDifX ) < 25 ? 1 : 1 - ( (fabs(flDifX)-25)/20 );

		
		
		// affichage

		vec3_t	vecFlareOrg = v_origin + (vecOrigin - v_origin).Normalize()*10;
		vec3_t	vecFlareEnd = v_origin + v_forward*10;
		vec3_t	flUnit		= (vecFlareEnd - vecFlareOrg) / 100;

		for ( int i=0; i<MAX_FLARE; i++ )
		{
			// initialisation triapi

			mod = gEngfuncs.CL_LoadModel( lensflare_spritename[i] , &modelindex );
			gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, 0 );	

			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling
			gEngfuncs.pTriAPI->Brightness( 200 );						
			gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0/*0.75*/, (float)((float)lensflare_spritevars[i][2] / 256.0f) * brightnessratio );

			vec3_t pos = vecFlareOrg + flUnit * lensflare_spritevars[i][0];
			vec3_t vertex;

			gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//d

				gEngfuncs.pTriAPI->TexCoord2f( 0,0 );
				vertex = pos + v_up * lensflare_spritevars[i][1] / 20 - v_right * lensflare_spritevars[i][1] / 20;
				gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

				gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
				vertex = pos + v_up * lensflare_spritevars[i][1] / 20 + v_right * lensflare_spritevars[i][1] / 20;
				gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

				gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
				vertex = pos - v_up * lensflare_spritevars[i][1] / 20 + v_right * lensflare_spritevars[i][1] / 20;
				gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

				gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
				vertex = pos - v_up * lensflare_spritevars[i][1] / 20 - v_right * lensflare_spritevars[i][1] / 20;
				gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );


			gEngfuncs.pTriAPI->End();									//fin du trac

		}

		// flash blanc

		// initialisation triapi

		mod = gEngfuncs.CL_LoadModel( "sprites/lensflare05.spr" , &modelindex );
		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)mod, 0 );	

		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );					//mode de transparence
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );						//(des)activer le face culling
		gEngfuncs.pTriAPI->Brightness( 200 );						
		gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, (float)(220.0f / 256.0f) * brightnessratio );

		vec3_t pos = vecOrigin + vecDirFlash * 2;
		vec3_t vertex;

		gEngfuncs.pTriAPI->Begin( TRI_QUADS );							//d

			gEngfuncs.pTriAPI->TexCoord2f( 0,0 );
			vertex = pos + v_up * FLASH_BLANC_SIZE - v_right * FLASH_BLANC_SIZE;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
			vertex = pos + v_up * FLASH_BLANC_SIZE + v_right * FLASH_BLANC_SIZE;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
			vertex = pos - v_up * FLASH_BLANC_SIZE + v_right * FLASH_BLANC_SIZE;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );

			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
			vertex = pos - v_up * FLASH_BLANC_SIZE - v_right * FLASH_BLANC_SIZE;
			gEngfuncs.pTriAPI->Vertex3f( vertex.x, vertex.y, vertex.z );


		gEngfuncs.pTriAPI->End();									//fin du trac






		p = p->pNext;
	}


}



//------------------------------------
//
// initialisation au chargement de la dll

int CHudLensFlare :: Init( void )
{
	HOOK_MESSAGE( LensFlare );
	ClearLights ();

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudLensFlare :: VidInit( void )
{
	HOOK_MESSAGE( LensFlare );
	ClearLights();

	m_iFlags &= ~HUD_ACTIVE;

	gHUD.AddHudElem(this);
	return 1;
}
