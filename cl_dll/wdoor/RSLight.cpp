#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSLight.h"
#include "r_efx.h"
#include "game_fx.h"
#include "event_api.h"

CRSLight::CRSLight(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSLight::~CRSLight(void)
{
	if (m_pLight != NULL)
	{
		m_pLight->die = gEngfuncs.GetClientTime();
		m_pLight = NULL;
	}
	KillSystem();
	FX_Type = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Advanced dynamic light
// Input  : origin - 
//			r,g,b - light color
//-----------------------------------------------------------------------------
CRSLight::CRSLight(const Vector &origin, byte r, byte g, byte b, float radius, float timetolive, int type, int followentity)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(LIGHT_INDEX_TE_RSLIGHT);// if key != 0, the engine will overwrite existing dlight with the same key
	if (m_pLight == NULL)
	{
		gEngfuncs.Con_DPrintf("CRSLight failed to allocate dynamic light!\n");
		removenow = true;
		return;// light is vital to this system
	}

	VectorCopy(origin, m_vecOrigin);
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fScale = radius;
	m_iFollowEntity = followentity;

	m_pTexture = NULL;
	m_iRenderMode = 0;

	VectorCopy(m_vecOrigin, m_pLight->origin);
	m_pLight->radius = radius;
	m_pLight->color.r = r;
	m_pLight->color.g = g;
	m_pLight->color.b = b;
	m_pLight->decay = 0.0f;

	m_fBaseOrigin = origin;//for additional Effects
	FX_Type = type;


	if (timetolive <= 0)
	{
		m_pLight->die = gEngfuncs.GetClientTime() + 1000.0f;
		m_fDieTime = 0;
	}
	else
	{
		m_pLight->die = gEngfuncs.GetClientTime() + timetolive;
		m_fDieTime = m_pLight->die;
	}

	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CRSLight::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	m_pLight = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update light position and parameters. NOTE: light needs to be
//   recreated every frame in software mode.
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSLight::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	if (dying)
		return 1;

	if (FX_Type > 0)
	switch (FX_Type)
	{
		case PROJ_GUTS:
			gEngfuncs.pEfxAPI->R_RocketTrail(m_fBaseOrigin, m_pLight->origin, 2);
		break;

		case PROJ_CLUSTERBOMB:
				gEngfuncs.pEfxAPI->R_RocketTrail(m_fBaseOrigin, m_pLight->origin, 1);
		break;

		case PROJ_MMISSILE:
				gEngfuncs.pEfxAPI->R_SparkStreaks(m_pLight->origin,25,-300,300);
				gEngfuncs.pEfxAPI->R_RocketTrail(m_fBaseOrigin, m_pLight->origin, 0);
		break;

		case PROJ_30MMGREN:
				gEngfuncs.pEfxAPI->R_RocketTrail(m_fBaseOrigin, m_pLight->origin, 0);
		break;

		case BEAM_PULSERIFLE:
			if ( gEngfuncs.GetClientTime() >= m_flTimeUpdate )
			{
				dir.x=gEngfuncs.pfnRandomFloat(-360,360);
				dir.y=gEngfuncs.pfnRandomFloat(-360,360);
				dir.z=gEngfuncs.pfnRandomFloat(-360,360);
				dir=dir.Normalize();

				gEngfuncs.pEfxAPI->R_BeamPoints( m_pLight->origin, m_pLight->origin+(dir*gEngfuncs.pfnRandomFloat(50,100)), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.3, 0.9, 2.5, 40, 20, BLAST_SKIN_PULSE, 0, 255, 255, 255);
			m_flTimeUpdate = gEngfuncs.GetClientTime() + 0.1;
			}
			gEngfuncs.pEfxAPI->R_StreakSplash ( m_pLight->origin+(dir*gEngfuncs.pfnRandomFloat(10,100)), vec3_origin, 5, gEngfuncs.pfnRandomFloat(2,5), 50, -50, 50);
		break;

		case PROJ_DISPLACER:
			if ( gEngfuncs.GetClientTime() >= m_flTimeUpdate )
			{
				dir.x=gEngfuncs.pfnRandomFloat(-360,360);
				dir.y=gEngfuncs.pfnRandomFloat(-360,360);
				dir.z=gEngfuncs.pfnRandomFloat(-360,360);
				dir=dir.Normalize();

				switch (gEngfuncs.pfnRandomLong(0,1))
				{
				       case 0:gEngfuncs.pEfxAPI->R_BeamPoints(m_pLight->origin, m_pLight->origin+(dir*300), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.2, 2, 4, 255, 20, BLAST_SKIN_PLASMA, 0, 0, 255, 0);break;
				       case 1:gEngfuncs.pEfxAPI->R_BeamPoints(m_pLight->origin, m_pLight->origin+(dir*400), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.2, 1, 3, 255, 20, BLAST_SKIN_PLASMA, 0, 255, 200, 0);break;
				}
				m_flTimeUpdate = gEngfuncs.GetClientTime() + 0.05;
			}
		break;

		case PROJ_TELEENTER:
			if ( gEngfuncs.GetClientTime() >= m_flTimeUpdate)
			{
				dir.x=gEngfuncs.pfnRandomFloat(-360,360);
				dir.y=gEngfuncs.pfnRandomFloat(-360,360);
				dir.z=gEngfuncs.pfnRandomFloat(-360,360);
				dir=dir.Normalize();

				switch (gEngfuncs.pfnRandomLong(0,1))
				{
				       case 0:gEngfuncs.pEfxAPI->R_BeamPoints(m_pLight->origin, m_pLight->origin+(dir*100), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.2, 2, 4, 255, 10, BLAST_SKIN_ENERGYBEAM, 0, 255, 255, 255);break;
				       case 1:gEngfuncs.pEfxAPI->R_BeamPoints(m_pLight->origin, m_pLight->origin+(dir*100), gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/rings_all.spr"), 0.2, 1, 3, 255, 10, BLAST_SKIN_ENERGYBOLT, 0, 255, 255, 255);break;
				}
				m_flTimeUpdate = gEngfuncs.GetClientTime() + 0.1;
			}
		break;

		case PROJ_WARHEAD:
			gEngfuncs.pEfxAPI->R_StreakSplash (m_pLight->origin, vec3_origin, 4, gEngfuncs.pfnRandomFloat(90, 125), 500, -200, 200);
		break;


	}

	if (m_pLight)
	{
		if (dying)
		{
			m_pLight->die = time;
		}
		else
		{
			if (m_iFollowEntity > 0)
			{
				if (FollowEntity())
				{
					m_pLight->radius = m_fScale;
				}// light can't find it's entity (yet!)
			}

			if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)// no need to restore as it'll be updated automatically
			{
				m_pLight->radius = 1.0f;
			}// don't recreate if nodraw
			else if (!IEngineStudio.IsHardware())// software mode fixed lights fix?
			{
				m_pLight->die = 0.0f;// remove previous frame light
				m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(LIGHT_INDEX_TE_RSLIGHT);
				if (m_pLight)
				{
					m_pLight->decay = 0.0f;
					m_pLight->die = time + 0.001f;
				}
				else// unable to allocate the light
				{
					dying = true;
					return 1;
				}
			}

			if (m_pLight)
			{
				m_pLight->radius = m_fScale;
				m_pLight->color.r = m_color.r;
				m_pLight->color.g = m_color.g;
				m_pLight->color.b = m_color.b;
				VectorCopy(m_vecOrigin, m_pLight->origin);

				if (m_fDieTime <= 0.0f)// don't let the light die
					m_pLight->die = time + 1.0f;
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: nothing to render here
//-----------------------------------------------------------------------------
void CRSLight::Render(void)
{
}
