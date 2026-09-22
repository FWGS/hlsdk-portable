#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSSpawnEffect.h"
#include "triangleapi.h"

CPSSpawnEffect::CPSSpawnEffect(void)
{
	ResetParameters();
}

CPSSpawnEffect::~CPSSpawnEffect(void)
{
	KillSystem();
}

CPSSpawnEffect::CPSSpawnEffect(int maxParticles, const Vector &origin, float scale, float scaledelta, float radius, float radiusdelta, float velocity, int sprindex, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	VectorCopy(origin, m_vecOrigin);
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_fRadius = radius;
	m_fRadiusDelta = radiusdelta;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_iRenderMode = r_mode;

	if (timetolive <= 0.0f)
		m_fDieTime = -1.0f;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
	m_iNumParticles = m_iMaxParticles;// all particles are present at start

	for (int i = 0; i < m_iNumParticles; ++i)
		InitializeParticle(i);
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CPSSpawnEffect::InitializeParticle(const int &index)
{
	m_pParticleList[index].m_vPos[0] = m_vecOrigin[0] + m_fRadius*sinf((float)(index*2));
	m_pParticleList[index].m_vPos[1] = m_vecOrigin[1] + m_fRadius*cosf((float)(index*2));
	m_pParticleList[index].m_vPos[2] = m_vecOrigin[2] + ((float)index)*(48.0f/(float)m_iMaxParticles) - 24.0f;// 48 means player height, 24 is half.
	VectorCopy(m_pParticleList[index].m_vPos, m_pParticleList[index].m_vPosPrev);

	VectorClear(m_pParticleList[index].m_vAccel);
	VectorClear(m_pParticleList[index].m_vVel);
	m_pParticleList[index].m_vAccel[2] = 4.0f;
	m_pParticleList[index].m_vVel[2] = 16.0f;

	m_pParticleList[index].m_fEnergy = 1.0f;
	m_pParticleList[index].m_fSizeX = m_fScale;
	m_pParticleList[index].m_fSizeY = m_fScale;
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;
	m_pParticleList[index].m_pTexture = m_pTexture;

	m_pParticleList[index].SetColor(m_color, m_fBrightness);
	m_pParticleList[index].m_fColorDelta[0] = 0.1f;
	m_pParticleList[index].m_fColorDelta[1] = 0.1f;
	m_pParticleList[index].m_fColorDelta[2] = 0.1f;
	m_pParticleList[index].m_fColorDelta[3] = m_fBrightnessDelta;
}

bool CPSSpawnEffect::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;
	else if (m_iNumParticles == 0)
		dying = true;

	if (dying)
		return 1;

	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
		FollowEntity();

	CParticle *curPart = NULL;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

		VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);

		curPart->m_vPos[0] = m_vecOrigin[0] + m_fRadius*sinf(-6.0f*time + (float)(i*2));
		curPart->m_vPos[1] = m_vecOrigin[1] + m_fRadius*cosf(-6.0f*time + (float)(i*2));
		curPart->m_vPos[2] = curPart->m_vPos[2] + curPart->m_vVel[2]*elapsedTime;

		VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);

		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			curPart->FrameRandomize();
		else
			curPart->FrameIncrease();

//		curPart->m_fEnergy += m_fBrightnessDelta*elapsedTime;
		curPart->UpdateColor(elapsedTime);
		curPart->UpdateSize(elapsedTime);
		curPart->UpdateEnergyByBrightness();
	}
	m_fRadius += m_fRadiusDelta*elapsedTime;// effect radius is increasing
	return 0;
}

void CPSSpawnEffect::Render(void)
{
	if (PointIsVisible(m_vecOrigin))
	{
		Vector v_up, v_right;
		AngleVectors(m_vecAngles, NULL, v_right, v_up);
		CParticle *p = NULL;
		for (int i = 0; i < m_iNumParticles; ++i)
		{
			p = &m_pParticleList[i];
			if (p->m_fEnergy <= 0.0f)
				continue;

			p->Render(v_right, v_up, m_iRenderMode);
		}
		gEngfuncs.pTriAPI->RenderMode(kRenderNormal);// ?
	}
}
