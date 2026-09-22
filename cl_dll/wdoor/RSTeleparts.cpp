#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSTeleparts.h"
#include "r_efx.h"
#include "studio_util.h"// M_PI

CRSTeleparts::CRSTeleparts(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSTeleparts::~CRSTeleparts(void)
{
	for (int i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		if (m_pParticles[i] != NULL)
		{
			m_pParticles[i]->die = 0.0f;
			m_pParticles[i] = NULL;// remove all pointers
		}
	}
	if (m_pLight != NULL)
	{
		m_pLight->die = gEngfuncs.GetClientTime();
		m_pLight = NULL;
	}

	m_colorpal = 0;
	m_iType = 0;
	KillSystem();
}

CRSTeleparts::CRSTeleparts(const Vector &origin, float radius, byte color, byte type, float timetolive, int followentity, byte r, byte g, byte b)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	VectorCopy(origin, m_vecOrigin);
	m_colorpal = color;
	m_iType = type;
	m_iFollowEntity = followentity;

	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = 0.0f;
	m_fBrightnessDelta = 0.0f;
	m_fScale = radius;
	m_fScaleDelta = 0.0f;

	m_pTexture = NULL;
	m_iRenderMode = 0;
	texindex = 0;

	if (timetolive <= 0)
		m_fDieTime = 0;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
	if (m_pLight)
	{
		VectorCopy(m_vecOrigin, m_pLight->origin);
		m_pLight->radius = m_fScale*2.0f;
		m_pLight->color.r = r;
		m_pLight->color.g = g;
		m_pLight->color.b = b;
		m_pLight->decay = 0.0f;
		m_pLight->die = gEngfuncs.GetClientTime()+m_fDieTime;
		m_pLight->dark = true;
	}
#ifdef _DEBUG
	else
		gEngfuncs.Con_DPrintf("CRSTeleparts failed to allocate dynamic light!\n");
#endif

	int i = 0;
	for (i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		m_pParticles[i] = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);
		if (m_pParticles[i] != NULL)
		{
			m_pParticles[i]->die = m_fDieTime;
			m_pParticles[i]->color = m_colorpal;
			VectorCopy(m_vecOrigin, m_pParticles[i]->org);
//should be already 0 0 0			VectorClear(m_pParticles[i]->vel);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&m_pParticles[i]->packedColor, m_pParticles[i]->color);
		}
	}

	if (type > 0)
	{
		for (i = 0; i < NUMVERTEXNORMALS; ++i)// init array first!
		{
			m_vecAng[i][0] = gEngfuncs.pfnRandomFloat(-M_PI, M_PI);
			m_vecAng[i][1] = gEngfuncs.pfnRandomFloat(-M_PI, M_PI);
		}
	}
	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CRSTeleparts::ResetParameters(void)
{
	CRenderSystem::ResetParameters();

	m_pLight = NULL;
	m_colorpal = 0;
	m_iType = 0;
	for (int i = 0; i < NUMVERTEXNORMALS; ++i)
	{
		m_pParticles[i] = NULL;
//		m_vecAng[i][0] = 0.0f;
//		m_vecAng[i][1] = 0.0f;
//		m_vecAng[i][2] = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRSTeleparts::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	cl_entity_t *pFollowEnt = FollowEntity();

	if (m_pLight)
	{
		if (dying)
		{
			m_pLight->die = time;//gEngfuncs.GetClientTime();
		}
		else
		{
			if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)// no need to restore as it'll be updated automatically
			{
				m_pLight->radius = 1.0f;
			}// don't recreate if nodraw
			else if (!IEngineStudio.IsHardware())// software mode fixed lights fix?
			{
				m_pLight->die = 0.0f;// remove previous frame light
				m_pLight = gEngfuncs.pEfxAPI->CL_AllocDlight(0);
				// may return null
				if (m_pLight)
				{
					m_pLight->decay = 0.0f;
					m_pLight->die = time + 0.001f;
				}
			}
			if (m_pLight)
			{
				VectorCopy(m_vecOrigin, m_pLight->origin);
				m_pLight->radius = m_fScale*2.0f;
				m_pLight->color.r = m_color.r;
				m_pLight->color.g = m_color.g;
				m_pLight->color.b = m_color.b;
			}
		}
	}

	if (dying)
		return 1;

	int i = 0;
	float dist = 0.0f;

	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)// HACK?
	{
		for (i = 0; i < NUMVERTEXNORMALS; ++i)
		{
			if (m_pParticles[i])
				VectorCopy(m_vecOrigin, m_pParticles[i]->org);
		}
	}
	else
	{
		if (m_iType == 0)
		{
			for (i = 0; i < NUMVERTEXNORMALS; ++i)
			{
				if (m_pParticles[i])
				{
					dist = sin(time + i)*m_fScale;
					VectorMA(m_vecOrigin, dist, (float *)vdirs[i], m_pParticles[i]->org);
				}
			}
		}
		else
		{
			float s1, s2, c1, c2;
			Vector dir;
			for (i = 0; i < NUMVERTEXNORMALS; ++i)
			{
				if (m_pParticles[i])
				{
					SinCos(time*1.5f + m_vecAng[i][0], &s1, &c1);
	//				dir[0] = s*c;
					SinCos(time*1.5f + m_vecAng[i][1], &s2, &c2);
	//				dir[1] = s*c;
	//				SinCos(time*1.5f + m_vecAng[i][2], &s, &c);
	//				dir[2] = s*c;

					dir[0] = c1*c2;
					dir[1] = s1*c2;
					dir[2] = -s2;

	//				dist = sinf(time + i)*m_fScale;
					dist = m_fScale;// <-- COOL!
					VectorMA(m_vecOrigin, dist*0.6f, dir, m_pParticles[i]->org);
				}
			}
		}
	}
	return 0;
}

void CRSTeleparts::Render(void)
{
	// nothing here. The engine draws particles all by itself.
}
