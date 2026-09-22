#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSFlatTrail.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"
#include "r_efx.h"

CPSFlatTrail::CPSFlatTrail(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CPSFlatTrail::~CPSFlatTrail(void)
{
	KillSystem();
}

CPSFlatTrail::CPSFlatTrail(const Vector &start, const Vector &end, int sprindex, int frame, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float dist_delta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	VectorCopy(start, m_vecStart);
	VectorCopy(end, m_vecDirection);
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	m_iFrame = frame;

	VectorSubtract(end, m_vecStart, m_vecDelta);
	float fDist = Length(m_vecDelta);
	if (fDist < dist_delta)
		return;

	m_iMaxParticles = fDist/dist_delta;

	if (m_iMaxParticles <= 0)
		m_iMaxParticles = 1;// return?

	VectorAngles(m_vecDelta, m_vecAngles);
	m_vecAngles[0] *= -1.0f;

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

//	CON_PRINTF("CPSFlatTrail: fDist = %f, m_iMaxParticles = %d, Length(m_vecDelta) = %f\n", fDist, m_iMaxParticles, Length(m_vecDelta));
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSFlatTrail::ResetParameters(void)
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
void CPSFlatTrail::InitializeParticle(const int &index)
{
	VectorMA(m_vecStart, (float)index, m_vecDelta, m_pParticleList[index].m_vPos);
	VectorCopy(m_pParticleList[index].m_vPos, m_pParticleList[index].m_vPosPrev);

	VectorClear(m_pParticleList[index].m_vAccel);
	VectorClear(m_pParticleList[index].m_vVel);

	m_pParticleList[index].m_fEnergy = 1.0;
	m_pParticleList[index].m_fSizeX = 32.0f*m_fScale;
	m_pParticleList[index].m_fSizeY = 32.0f*m_fScale;
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].m_iFrame = m_iFrame;
	m_pParticleList[index].SetColor(m_color, m_fBrightness);
	m_pParticleList[index].m_fColorDelta[3] = m_fBrightnessDelta;
}

bool CPSFlatTrail::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;
	else if (m_iNumParticles <= 0)
		dying = true;

	m_fBrightness = Q_max(0.0f, m_fBrightness + m_fBrightnessDelta*elapsedTime);// update before checking!

	if (!dying && m_fBrightness <= 0.0f && m_fBrightnessDelta < 0.0f)
		dying = true;

	if (dying)
		return 1;

	CParticle *curPart = NULL;
	// special code for this system
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

//		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
//			curPart->FrameRandomize();
//		else
//			curPart->FrameIncrease();

		curPart->m_fColor[3] = m_fBrightness;
		curPart->UpdateSize(elapsedTime);
	}
	return 0;
}

void CPSFlatTrail::Render(void)
{
	if (m_pTexture == NULL)
		return;

	Vector up, rt;
	AngleVectors(m_vecAngles, NULL, rt, up);

	CParticle *p = NULL;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		p = &m_pParticleList[i];
		if (p->m_fEnergy <= 0.0f)
			continue;

		p->Render(rt, up, m_iRenderMode, true);
	}
}
