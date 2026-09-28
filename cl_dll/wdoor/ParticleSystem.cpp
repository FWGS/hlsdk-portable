#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"


//-----------------------------------------------------------------------------
// Purpose: Default constructor. Should never be used.
//-----------------------------------------------------------------------------
CParticleSystem::CParticleSystem(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. Used for memory cleaning. Destroy all data here.
//-----------------------------------------------------------------------------
CParticleSystem::~CParticleSystem(void)
{
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Default particle system
// Input  : maxParticles - 
//			origin - 
//			direction - 
//			sprindex - 
//			r_mode - 
//			timetolive - 
//-----------------------------------------------------------------------------
CParticleSystem::CParticleSystem(int maxParticles, const Vector &origin, const Vector &direction, int sprindex, int r_mode, float timetolive)
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
//	m_pParticleList = NULL;
	m_iMaxParticles = maxParticles;
	VectorCopy(origin, m_vecOrigin);
	VectorCopy(direction, m_vecDirection);
	m_iRenderMode = r_mode;
//	m_iFollowEntity = -1;
//	m_fBrightness = 1.0f;
//	m_fBrightnessDelta = 0.0f;
	m_fScale = 4.0f;
//	m_fScaleDelta = 0.0f;

	if (timetolive < 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
//DON'T! We need flags set BEFORE we call this	Emit(maxParticles);
//	dying = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
//-----------------------------------------------------------------------------
void CParticleSystem::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	m_pParticleList = NULL;// NEW UPD 2007.10.11
	VectorClear(m_vecDirection);
	m_fNextEmitTime = 0.0f;
	m_iMaxParticles = 0;
	m_iNumParticles = 0;
	m_iAccumulatedEmit = 0;
	m_fEnergyStart = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Must be called from class constructor.
//-----------------------------------------------------------------------------
void CParticleSystem::InitializeSystem(void)
{
	if (m_pParticleList != NULL)// 0x00000001
	{
		delete [] m_pParticleList;// Access Violation.
		m_pParticleList = NULL;
	}

	assert(m_iMaxParticles > 0);
	m_pParticleList = new CParticle[m_iMaxParticles];

	m_iNumParticles = 0;
	m_iAccumulatedEmit = 0;
	m_fNextEmitTime = 0.0f;

	CRenderSystem::InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
void CParticleSystem::KillSystem(void)
{
	if (m_pParticleList != NULL)
	{
		delete [] m_pParticleList;
		m_pParticleList = NULL;
	}
	m_iMaxParticles = 0;
	m_iNumParticles = 0;
	m_iAccumulatedEmit = 0;

	CRenderSystem::KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Reset or recreate particles. It's the tricky part to not to release
//			all particles at the same time.
// Warning: Must NOT be called from constructor because it depends on flags!
// Input  : numParticles - 
// Output : int
//-----------------------------------------------------------------------------
int CParticleSystem::Emit(const int &numParticles)
{
	if (dying)
		return 0;
/*	numParticles += m_iAccumulatedEmit;

	// TODO: revisit
	while ((numParticles >= 1) && (m_iNumParticles < m_iMaxParticles))
	{
		InitializeParticle(m_iNumParticles);
		++m_iNumParticles;
		--numParticles;
	}

	m_iAccumulatedEmit = numParticles;
	return numParticles;
*/
	m_iAccumulatedEmit += numParticles;// XDM3035: optimizations

	while ((m_iAccumulatedEmit >= 1) && (m_iNumParticles < m_iMaxParticles))
	{
		InitializeParticle(m_iNumParticles);
		++m_iNumParticles;
		--m_iAccumulatedEmit;
	}

//	m_iAccumulatedEmit = numParticles;

	return m_iAccumulatedEmit;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CParticleSystem::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	if (dying && m_iNumParticles == 0)
		return 1;

	FollowEntity();
	Emit(1);

	pmtrace_t pmtrace;
	if (m_iFlags & RENDERSYSTEM_FLAG_ADDPHYSICS)// XDM3035
	{
		gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
		gEngfuncs.pEventAPI->EV_PushPMStates();
	//	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
		gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	}
	CParticle *curPart;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			continue;
//			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

//		curPart->m_texture = m_pTexture;
		VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);
		VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);
		VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVel, curPart->m_vPos);

		if (m_iFlags & RENDERSYSTEM_FLAG_ADDPHYSICS)
		{
			gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, (m_iFlags & RENDERSYSTEM_FLAG_NOCLIP)?PM_WORLD_ONLY:PM_STUDIO_BOX, -1, &pmtrace);
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
/*already included in acceleration			else
			{
				curPart->m_vPos.z -= elapsedTime * g_cl_gravity;
			}*/
		}

		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			curPart->FrameRandomize();
		else
			curPart->FrameIncrease();

		curPart->m_fEnergy -= 0.5 * elapsedTime;
		curPart->UpdateColor(elapsedTime);
		curPart->UpdateSize(elapsedTime);
		curPart->m_fColor[3] = curPart->m_fEnergy;
	}
	if (m_iFlags & RENDERSYSTEM_FLAG_ADDPHYSICS)
	{
		gEngfuncs.pEventAPI->EV_PopPMStates();
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CParticleSystem::InitializeParticle(const int &index)
{
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPos);
	VectorCopy(m_vecOrigin, m_pParticleList[index].m_vPosPrev);
	VectorCopy(m_vecDirection, m_pParticleList[index].m_vVel);

	if (m_iFlags & RENDERSYSTEM_FLAG_ADDGRAVITY)//RENDERSYSTEM_FLAG_ADDPHYSICS)// XDM3035
	{
		m_pParticleList[index].m_vAccel.x = 0.0f;
		m_pParticleList[index].m_vAccel.y = 0.0f;
		m_pParticleList[index].m_vAccel.z = -g_cl_gravity;
	}
	else
		VectorClear(m_pParticleList[index].m_vAccel);

//	m_pParticleList[index].m_vecAngles
	m_pParticleList[index].m_fEnergy = m_fEnergyStart;
	m_pParticleList[index].m_fSizeX = m_fScale;
	m_pParticleList[index].m_fSizeY = m_fScale;
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;
	m_pParticleList[index].m_pTexture = m_pTexture;

	m_pParticleList[index].SetColor(m_color, m_fBrightness);
	m_pParticleList[index].SetColorDelta(m_fColorDelta, m_fBrightnessDelta);

/*	m_pParticleList[index].SetDefaultColor();
	m_pParticleList[index].m_fColor[3] = m_fBrightness;
	m_pParticleList[index].m_fColorDelta[3] = m_fBrightnessDelta;
	*/
//	m_pParticleList[index].m_weight = 0.0;
//	m_pParticleList[index].m_weightDelta = 0.0;
	m_pParticleList[index].m_iFlags = 0;
	m_pParticleList[index].m_iFrame = 0;
	
}

//-----------------------------------------------------------------------------
// Purpose: extremely useful for explosion and wind effects
// Input  : origin - 
//			force - 
//			radius - 
//			point - 
//-----------------------------------------------------------------------------
void CParticleSystem::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
/*UNDONE	for (int i = 0; i < m_iNumParticles; ++i)
	{
		float dist = Length(m_pParticleList[i].m_vPos - origin);
		if (dist < radius)
		{
			if (point)
				m_pParticleList[i].m_vAccel += force*((radius-dist)/radius);
			else
				m_pParticleList[i].m_vAccel += force;
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CParticleSystem::Render(void)
{
	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)
		return;

	if (!PointIsVisible(m_vecOrigin))
		return;

	if (m_iFlags & RENDERSYSTEM_FLAG_ZROTATION)
	{
		m_vecAngles[0] = 0.0f;
		m_vecAngles[2] = 0.0f;
	}

	gEngfuncs.GetViewAngles(m_vecAngles);
	VectorAdd(m_vecAngles, g_ev_punchangle, m_vecAngles);

	Vector v_up, v_right;
	AngleVectors(m_vecAngles, NULL, v_right, v_up);

	CParticle *p = NULL;
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		p = &m_pParticleList[i];
		if (p->m_fEnergy <= 0.0f)
			continue;

		if (!PointIsVisible(p->m_vPos))// faster?
			continue;

		p->Render(v_right, v_up, m_iRenderMode);
	}
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);// ?
}
