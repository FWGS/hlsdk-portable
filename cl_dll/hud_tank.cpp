//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					hud_tank.cpp						---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//- code du hud et de la cam
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
#include "com_model.h"
#include "ref_params.h"

extern void CAM_ToThirdPerson(void);
extern void CAM_ToFirstPerson(void);

extern vec3_t v_origin;
extern vec3_t v_angles;

#define TANK_HEALTH		1200
#define TANK_PAIN_TIME	0.3

bool m_iPlayerInTankExternal = false;

//------------------------------------
//
// d
// gmsgTankView

DECLARE_MESSAGE(m_HudTank, TankView );


//------------------------------------
//
// gestion des messages serveur


int CHudTank::MsgFunc_TankView( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	if ( READ_BYTE() == 1 )
	{
		m_iFlags |= HUD_ACTIVE;

		m_iPlayerInTank	= true;
		m_iPlayerInTankExternal = true; //For client weapons (glock, specifically) modif de Roy
		m_iCamEnt		= READ_COORD();

		// brillance du compteur de vie

		float value = READ_LONG();

		if ( value > m_flTankHealth )
			m_flPain = ARMOR_PAIN_TIME;

		else if ( value < m_flTankHealth )
			m_flPain = -ARMOR_PAIN_TIME;

		m_flTankHealth	= value;

	}
	else
	{
		m_iFlags &= ~HUD_ACTIVE;

		m_iPlayerInTank	= false;
		m_iPlayerInTankExternal = false; //For client weapons (glock, specifically) modif de Roy
		m_iCamEnt		= 0;

	}

	return 1;
}




//------------------------------------
//
// rafraichissement de l'affichage


int CHudTank :: Draw	( float flTime )
{

	int iJauge = (int)(256 - (256 * m_flTankHealth/TANK_HEALTH)	);	// dimensions du sprite : 256 large 112 haut
	int r, g, b;
	wrect_t rec1;


	if ( m_flTankHealth > 0 )
	{
		// sprite jaune

		UnpackRGB(r,g,b, RGB_YELLOWISH);

		
		// brillance pour les dommages
		if ( m_flPain > 0 )
		{
			// blanc
			r = r + ( m_flPain / TANK_PAIN_TIME ) * ( 255 - r );
			g = g + ( m_flPain / TANK_PAIN_TIME ) * ( 255 - g );
			b = b + ( m_flPain / TANK_PAIN_TIME ) * ( 255 - b );

			m_flPain = Q_max ( 0, m_flPain - gHUD.m_flTimeDelta );
		}

		if ( m_flPain < 0 )
		{
			// rouge
			r = r + ( -m_flPain / TANK_PAIN_TIME ) * ( 255 - r );
			g = g + ( -m_flPain / TANK_PAIN_TIME ) * ( 0 - g );
			b = b + ( -m_flPain / TANK_PAIN_TIME ) * ( 0 - b );

			m_flPain = Q_min ( 0, m_flPain + gHUD.m_flTimeDelta );
		}

		ScaleColors(r, g, b, 200 );

		rec1 = CreateWrect(iJauge,0,256,112);

		SPR_Set( m_sprTank, r, g, b);
		SPR_DrawAdditive( 0, ScreenWidth - 256 -32 + iJauge, ScreenHeight - 112 - 16, &rec1 );
	}

	if ( m_flTankHealth != TANK_HEALTH )
	{
		// sprite rouge

		r = 255; g = 0; b = 0;
		ScaleColors(r, g, b, 170 );

		rec1 = CreateWrect(0,0,iJauge,112);

		SPR_Set( m_sprTank, r, g, b);
		SPR_DrawAdditive( 0, ScreenWidth - 256 -32, ScreenHeight - 112 - 16, &rec1 );
	}

		
	return 1;
}


void CHudTank :: SetViewPos ( struct ref_params_s *pparams )
{
	cl_entity_t *p = gEngfuncs.GetEntityByIndex( m_iCamEnt );

	pparams->vieworg[0] = p->origin.x;
	pparams->vieworg[1] = p->origin.y;
	pparams->vieworg[2] = p->origin.z;

	pparams->viewangles[0] = p->curstate.angles.x;
	pparams->viewangles[1] = p->curstate.angles.y;
	pparams->viewangles[2] = p->curstate.angles.z;



}

//------------------------------------
//
// initialisation au chargement de la dll

int CHudTank :: Init( void )
{
	HOOK_MESSAGE( TankView );

	m_sprTank	= SPR_Load("sprites/tank.spr");

	m_iPlayerInTank = false;
	m_iPlayerInTankExternal = false; //For client weapons (glock, specifically) modif de Roy
	m_iCamEnt = 0;
	m_iFlags &= ~HUD_ACTIVE;
	m_iFlags |= HUD_ALWAYSDRAW;

	m_CamPos = m_CamAng = m_CamVelocity = m_CamAVelocity = Vector(0,0,0);
	m_flTankHealth	= 0;
	m_flPain		= 0;


	gHUD.AddHudElem(this);
	return 1;
}


//------------------------------------
//
// initialisation apr


int CHudTank :: VidInit( void )
{
	m_sprTank	= SPR_Load("sprites/tank.spr");

	m_iPlayerInTank = false;
	m_iPlayerInTankExternal = false; //For client weapons (glock, specifically) modif de Roy
	m_iCamEnt = 0;
	m_iFlags &= ~HUD_ACTIVE;
	m_iFlags |= HUD_ALWAYSDRAW;

	m_CamPos = m_CamAng = m_CamVelocity = m_CamAVelocity = Vector(0,0,0);
	m_flTankHealth	= 0;
	m_flPain		= 0;


	return 1;
}


