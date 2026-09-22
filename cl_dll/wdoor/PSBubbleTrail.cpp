//==================================================================//
//Purpose: generates up-movig particle trail. Lives ONLY under water//
//==================================================================//
#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBubbleTrail.h"
#include "event_api.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "pm_defs.h"

CPSBubbleTrail::CPSBubbleTrail(void)
{
	ResetParameters();
}

CPSBubbleTrail::~CPSBubbleTrail(void)
{
	KillSystem();
}

CPSBubbleTrail::CPSBubbleTrail(const Vector &start, const Vector &end, int sprindex, int frame, float scale, float dist_delta, float timetolive)
{
	index = 0;
	removenow = false;
	ResetParameters();
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	VectorCopy(start, m_vecStart);
	VectorCopy(end, m_vecDirection);
	m_iRenderMode = kRenderTransAdd;
	m_fScale = scale;
	m_iFrame = frame;

	VectorSubtract(end, m_vecStart, m_vecDelta);
	float fDist = Length(m_vecDelta);
	if (fDist < dist_delta)
		return;

	m_iMaxParticles = fDist/dist_delta;

	if (m_iMaxParticles <= 0)
		m_iMaxParticles = 1;// return?

	m_vecDelta[0] /= m_iMaxParticles;
	m_vecDelta[1] /= m_iMaxParticles;
	m_vecDelta[2] /= m_iMaxParticles;

	if (timetolive <= 0.0f)// if 0, just fade
		m_fDieTime = timetolive;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
	m_iNumParticles = m_iMaxParticles;

	for (int i = 0; i < m_iNumParticles; ++i)
		InitializeParticle(i);
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSBubbleTrail::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	VectorClear(m_vecStart);
	VectorClear(m_vecDelta);
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CPSBubbleTrail::InitializeParticle(const int &index)
{
	VectorMA(m_vecStart, (float)index, m_vecDelta, m_pParticleList[index].m_vPos);
	VectorCopy(m_pParticleList[index].m_vPos, m_pParticleList[index].m_vPosPrev);

	VectorClear(m_pParticleList[index].m_vAccel);
	VectorClear(m_pParticleList[index].m_vVel);

	m_pParticleList[index].m_vAccel = Vector(0.0f, 0.0f, 10.0f);
	m_pParticleList[index].m_vVel[2] = gEngfuncs.pfnRandomFloat(60,90);
	m_pParticleList[index].m_fEnergy = 10.0f;
	m_pParticleList[index].m_fSizeX = m_fScale;
	m_pParticleList[index].m_fSizeY = m_fScale;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].m_iFrame = m_iFrame;
	m_pParticleList[index].SetDefaultColor();
}

bool CPSBubbleTrail::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	else if (m_iNumParticles <= 0)
		dying = true;

	if (dying)
		return 1;

	CParticle *curPart = NULL;
	pmtrace_t pmtrace;
//	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

		VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);
		VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);
		VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVel, curPart->m_vPos);
		gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, PM_STUDIO_IGNORE, -1, &pmtrace);

		if ( gEngfuncs.PM_PointContents( pmtrace.endpos, NULL ) != CONTENTS_WATER)
			curPart->m_fEnergy = -1.0f;
		else
			curPart->m_fEnergy -= 1.5f * elapsedTime;
	}
	return 0;
}

void CPSBubbleTrail::Render(void)
{
	if (m_pTexture == NULL)
		return;

	Vector up, rt;
	CParticle *p = NULL;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		p = &m_pParticleList[i];
		if (p->m_fEnergy <= 0.0f)
			continue;

		p->Render(rt, up, m_iRenderMode, true);
	}
}