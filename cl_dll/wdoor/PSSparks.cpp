#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSSparks.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"

CPSSparks::CPSSparks(void)
{
	ResetParameters();
}

CPSSparks::~CPSSparks(void)
{
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: The one and only constructor
//  Use negative scale values to get real sprite dimensions multiplied by fabs(scale)
//  So many parameters here because this system ignores customization outside of constructor (all particles get initialized inside)
// Input  : maxParticles - 
//			origin - 
//			scalex - 
//			scaley - 
//			velocity - 
//			startenergy - constantly decreases (particles are removed when their energy reaches zero), was always 2.0f
//			sprindex - 
//			r_mode - render mode
//			timetolive - 
//-----------------------------------------------------------------------------

CPSSparks::CPSSparks(int maxParticles, const Vector &origin, float scalex, float scaley, float velocity, float partlife, int sprindex, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	m_pParticleList = NULL;
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_iRenderMode = kRenderTransAdd;
	m_fEnergyStart = partlife;
	m_fSizeX = scalex*0.1f;
	m_fSizeY = scaley*0.1f;

	if (velocity < 0.0f)
	{
		m_fVelocity = -velocity;
		m_bReversed = true;
	}
	else
	{
		m_fVelocity = velocity;
		m_bReversed = false;
	}
	if (timetolive <= 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	m_iFollowEntity = -1;
	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSSparks::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	m_fSizeX = 0.0f;
	m_fSizeY = 0.0f;
	m_fVelocity = 0.0f;
	m_bReversed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPSSparks::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	if (dying && m_iNumParticles == 0)
		return 1;

	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
		FollowEntity();

	if (m_iFlags & RENDERSYSTEM_FLAG_SIMULTANEOUS)
	{
		Emit(m_iMaxParticles);
		dying = true;// XDM3035b: don't repeat after this shot
	}
	else if (m_fNextEmitTime <= time)
	{
		Emit(1);
		m_fNextEmitTime = time + 10.0f/(float)m_iMaxParticles;
	}

	CParticle *curPart = NULL;

	pmtrace_t pmtrace;
	if (!(m_iFlags & RENDERSYSTEM_FLAG_NOCLIP))
	{
//		gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	}
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

		VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);
		VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);
		VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVel, curPart->m_vPos);

		if (!(m_iFlags & RENDERSYSTEM_FLAG_NOCLIP))
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, PM_WORLD_ONLY, -1, &pmtrace);
			if (pmtrace.fraction != 1.0f)
			{
				if (m_iFlags & RENDERSYSTEM_FLAG_CLIPREMOVE)
				{
					curPart->m_fEnergy = -1.0f;
				}
				else
				{
					float p = DotProduct(curPart->m_vVel, pmtrace.plane.normal);
					VectorMA(curPart->m_vVel, -2.0f*p, pmtrace.plane.normal, curPart->m_vVel);
					curPart->m_vVel = curPart->m_vVel * 0.8f;
				}
			}
		}
		curPart->m_fEnergy -= elapsedTime;
		curPart->UpdateColor(elapsedTime);
		curPart->m_fColor[3] = curPart->m_fEnergy;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CPSSparks::InitializeParticle(const int &index)
{
	Vector rnd;// = VectorRandom() + VectorRandom();
	Vector rnd2;
	VectorRandom(rnd);
	VectorRandom(rnd2);
	VectorAdd(rnd, rnd2, rnd);

	if (m_bReversed)
	{
		VectorMA(m_vecOrigin, m_fVelocity, rnd, m_pParticleList[index].m_vPos);
		VectorInverse(rnd);
	}
	else
		VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPos);

	VectorScale(rnd, m_fVelocity, m_pParticleList[index].m_vVel);
	VectorCopy(m_pParticleList[index].m_vPos, m_pParticleList[index].m_vPosPrev);

	if (m_iFlags & RENDERSYSTEM_FLAG_ADDGRAVITY)
	{
		m_pParticleList[index].m_vAccel.x = 0.0f;
		m_pParticleList[index].m_vAccel.y = 0.0f;
		m_pParticleList[index].m_vAccel.z = -g_cl_gravity;
	}
	else
		VectorClear(m_pParticleList[index].m_vAccel);

	m_pParticleList[index].m_fEnergy = m_fEnergyStart;
	m_pParticleList[index].m_fSizeX = fabs(m_fSizeX);
	m_pParticleList[index].m_fSizeY = fabs(m_fSizeY);

	if (m_pTexture)
	{
		if (m_fSizeX <= 0.0f)// use sprite dimensions. FIXME: for now, absolute scale is 1:32
			m_pParticleList[index].m_fSizeX *= (m_pTexture->maxs[1] - m_pTexture->mins[1])/3.20f;

		if (m_fSizeY <= 0.0f)
			m_pParticleList[index].m_fSizeY *= (m_pTexture->maxs[1] - m_pTexture->mins[1])/3.20f;
	}
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].SetDefaultColor();
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CPSSparks::Render(void)
{
	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	Vector v_fwd;
	AngleVectors(m_vecAngles, v_fwd, NULL, NULL);

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	CParticle *curPart = NULL;
	Vector velocity, crossvel, backpoint;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			continue;

		if (!PointIsVisible(curPart->m_vPos))// faster?
			continue;

		VectorCopy(curPart->m_vVel, velocity);
		CrossProduct(velocity, v_fwd, crossvel);

		Vector vx = velocity*curPart->m_fSizeX;
		Vector cy = crossvel*curPart->m_fSizeY;
		gEngfuncs.pTriAPI->Color4f(curPart->m_fColor[0], curPart->m_fColor[1], curPart->m_fColor[2], curPart->m_fColor[3] * curPart->m_fEnergy);
		gEngfuncs.pTriAPI->Brightness(curPart->m_fEnergy);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos - vx + cy);
		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos + cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos - cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos - vx - cy);
	}
	gEngfuncs.pTriAPI->End();
}
