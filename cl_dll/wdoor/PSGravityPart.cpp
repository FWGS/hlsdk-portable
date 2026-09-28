//=======================================================================//
//Purpose: generates gravity-dependant particles, which removes in water!//
//=======================================================================//
#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSGravityPart.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "studio_util.h"

PSGravityPart::PSGravityPart(void)
{
	ResetParameters();
}

PSGravityPart::~PSGravityPart(void)
{
	KillSystem();
}

PSGravityPart::PSGravityPart(int maxParticles, float PartLife, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float scale, float scaledelta, byte r, byte g, byte b, float a, float adelta, int sprindex, bool animate, int frame, int r_mode, float PartEmitterLife)
{
	index = 0;
	removenow = false;
	ResetParameters();
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	VectorCopy(origin, m_vecOrigin);
	if (VectorCompare(origin, direction))
	{
		VectorClear(m_vecDirection);
		m_flRandomDir = true;
	}
	else
	{
		VectorCopy(direction, m_vecDirection);
		m_flRandomDir = false;
	}
	VectorCopy(spread, m_vecSpread);
	m_fParticleVelocity = partvelocity;
	m_iRenderMode = r_mode;
	m_fAnimatedSpr = animate;
	m_iFrame = frame;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_fLife = PartLife;

	if (PartEmitterLife <= 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + PartEmitterLife;

	InitializeSystem();
}

void PSGravityPart::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	VectorClear(m_vecSpread);
	m_flRandomDir = true;
	m_fParticleVelocity = 100.0f;
}

bool PSGravityPart::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	if( m_fBrightness <= 0.0f)
		m_fBrightness = 0.0f;

	if (dying && m_iNumParticles == 0)
		return 1;

	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
		cl_entity_t *pFollow = FollowEntity();

	if (m_iFlags & RENDERSYSTEM_FLAG_SIMULTANEOUS)
		Emit(m_iMaxParticles);

	else if (m_fNextEmitTime <= time)
	{
		Emit(1);
		m_fNextEmitTime = time + 1.0f/(float)m_iMaxParticles;
	}

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
		gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, PM_WORLD_ONLY, -1, &pmtrace);

		if (gEngfuncs.PM_PointContents(curPart->m_vPos, NULL ) == CONTENTS_WATER)
			curPart->m_fEnergy -= 1.0f;

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
				curPart->m_vVel = curPart->m_vVel * 0.9f;
			}
		}

		if (m_fAnimatedSpr == FALSE)
		{
			curPart->m_iFrame = m_iFrame;
		}
		if (m_fAnimatedSpr == TRUE)
		{
			if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
				curPart->FrameRandomize();
			else
				curPart->FrameIncrease();
		}
		curPart->m_fEnergy -= 1.5f * elapsedTime;
		curPart->m_fColor[3] = m_fBrightness;
		curPart->UpdateSize(elapsedTime);
	}
	m_fBrightness += m_fBrightnessDelta*(float)elapsedTime;
	return 0;
}

void PSGravityPart::InitializeParticle(const int &index)
{
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPos);
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPosPrev);
	VectorClear(m_pParticleList[index].m_vAccel);
	VectorRandom(m_pParticleList[index].m_vVel);
	Vector rnd2;
	VectorRandom(rnd2);
	VectorAdd(m_pParticleList[index].m_vVel, rnd2, m_pParticleList[index].m_vVel);

	if (m_flRandomDir)
	{
		VectorNormalize(m_pParticleList[index].m_vVel);
	}
	else
	{
		m_pParticleList[index].m_vVel[0] *= m_vecSpread[0];
		m_pParticleList[index].m_vVel[1] *= m_vecSpread[1];
		m_pParticleList[index].m_vVel[2] *= m_vecSpread[2];
		VectorAdd(m_pParticleList[index].m_vVel, m_vecDirection, m_pParticleList[index].m_vVel);
	}
	m_pParticleList[index].m_vAccel.z = -g_cl_gravity/2;
	m_pParticleList[index].m_vVel = m_pParticleList[index].m_vVel * m_fParticleVelocity;
	m_pParticleList[index].m_fEnergy = m_fLife;
	m_pParticleList[index].m_fSizeX = m_fScale;
	m_pParticleList[index].m_fSizeY = m_fScale;
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].SetColor(m_color, m_fBrightness);
	m_pParticleList[index].SetColorDelta(m_fColorDelta, m_fBrightnessDelta);
}
