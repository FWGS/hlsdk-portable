#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBeam.h"
#include "pm_defs.h"
#include "event_api.h"

CPSBeam::CPSBeam(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CPSBeam::~CPSBeam(void)
{
	KillSystem();
}

CPSBeam::CPSBeam(int maxParticles, const Vector &origin, const Vector &end, int sprindex, int r_mode, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	// Calling constructors directly is forbidden!
	ResetParameters();
//	m_pTexture = NULL;
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	VectorCopy(origin, m_vecOrigin);
	VectorCopy(end, m_vecEnd);
//	m_pParticleList = NULL;
//	m_iFollowEntity = -1;

	m_iRenderMode = r_mode;
	if (timetolive < 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSBeam::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	VectorClear(m_vecEnd);
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSBeam::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0 && m_fDieTime <= time)
		dying = true;

	if (dying && m_iNumParticles == 0)
		return 1;

//	UpdateFrame();
	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
		FollowEntity();

	Emit(225 * elapsedTime);

	pmtrace_t pmtrace;
	CParticle *curPart = NULL;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();
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

		gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, PM_STUDIO_BOX, -1, &pmtrace);
		if (pmtrace.fraction != 1)
		{
			//traceFraction = pmtrace.fraction;
			//VectorCopy( pmtrace.plane.normal, traceNormal );
			curPart->m_fEnergy = -1.0;
		}

		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			curPart->FrameRandomize();
		else
			curPart->FrameIncrease();

		curPart->m_fEnergy -= 1.5f * elapsedTime;
		curPart->UpdateColor(elapsedTime);
		curPart->UpdateSize(elapsedTime);
		curPart->m_fColor[3] = curPart->m_fEnergy;
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CPSBeam::InitializeParticle(const int &index)
{
	float dist = gEngfuncs.pfnRandomFloat(0,1);
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPos);
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPosPrev);
	VectorClear(m_pParticleList[index].m_vAccel);
	VectorSubtract(m_vecEnd, m_vecOrigin, m_pParticleList[index].m_vVel);// vel = end - org
	VectorMA(m_pParticleList[index].m_vPos, dist, m_pParticleList[index].m_vVel, m_pParticleList[index].m_vPos);// pos += vel+dist
	VectorNormalize(m_pParticleList[index].m_vVel);
//???	m_pParticleList[index].m_vVel = m_pParticleList[index].m_vVel * 0; //Scalar velocity component
	m_pParticleList[index].m_fEnergy = 1.0f;
	m_pParticleList[index].m_fSizeX = 1.0f;
	m_pParticleList[index].m_fSizeY = 1.0f;
	m_pParticleList[index].m_fSizeDelta = 0.0f;
//	m_pParticleList[index].m_texture = texindex;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].FrameRandomize();
	m_pParticleList[index].SetDefaultColor();
//	m_pParticleList[index].m_weight = 0.0;
//	m_pParticleList[index].m_weightDelta = 0.0;
}
