#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSStreaks.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"

// UNDONE this particle system is untested. Maybe this should be merged with CPSSparks

CPSStreaks::CPSStreaks(void)
{
	ResetParameters();
}

CPSStreaks::~CPSStreaks(void)
{
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: The one and only constructor
// Input  : maxParticles - 
//			timetolive - 
//-----------------------------------------------------------------------------
CPSStreaks::CPSStreaks(int maxParticles, const Vector &origin, const Vector &direction, float speed, float velocitymin, float velocitymax, float scalex, float scaley, float scaledelta, color24 color, float a, float adelta, int sprindex, int r_mode, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
//	m_pTexture = NULL;
	m_pParticleList = NULL;
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iMaxParticles = maxParticles;
	m_vecOrigin = origin;
	m_vecVelocity = direction;
	m_fSpeed = speed;
	m_fVelocityMin = velocitymin;
	m_fVelocityMax = velocitymax;
//	m_fEnergyStart = startenergy;// 2.0f?

	m_color = color;
	m_fBrightness = a;// 1.0f
	m_fBrightnessDelta = adelta;// 0.0f
	m_fScale = 1.0f;// not used anyway
	m_fScaleDelta = scaledelta;// default was 0.5f
	m_fSizeX = scalex*0.1f;
	m_fSizeY = scaley*0.1f;

	if (m_fSizeX <= 0.0f)
		m_fSizeX = 1.0f;

	if (m_fSizeY <= 0.0f)
		m_fSizeY = 0.05f;

	m_iRenderMode = r_mode;
	InitializeSystem();
	Emit(maxParticles);

	if (timetolive > 0)
		dying = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSStreaks::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	m_fSizeX = 1.0f;
	m_fSizeY = 0.05f;
	m_fVelocityMin = 0.0f;
	m_fVelocityMax = 0.0f;
	m_fSpeed = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPSStreaks::Update(const float &time, const double &elapsedTime)
{
	if (dying && m_iNumParticles == 0)
		return 1;

//	UpdateFrame();
//	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
//		FollowEntity();

//	if (m_iFlags & RENDERSYSTEM_FLAG_SIMULTANEOUS)
	{
		Emit(m_iMaxParticles);
	}
/*	else if (m_fNextEmitTime <= time)
	{
		Emit(1);
		m_fNextEmitTime = time + 10.0f/(float)m_iMaxParticles;
	}*/


	CParticle *curPart = NULL;
	float length;

	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

//		length = (curPart->m_vPos - curPart->m_vPosPrev).Length();
		VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);
		VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);
		VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVel, curPart->m_vPos);

		length = curPart->m_vVel.Length() *0.06f;

/*
		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			curPart->FrameRandomize();
		else
			curPart->FrameIncrease();
*/
//		curPart->m_fEnergy -= elapsedTime;
		curPart->UpdateColor(elapsedTime);
//		curPart->UpdateSize(elapsedTime);
		curPart->UpdateEnergyByBrightness();
		curPart->m_fSizeY = length;
//		curPart->m_fColor[3] = curPart->m_fEnergy;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: A new particle has been created, initialize system-specific start
//			values for it.
// Input  : index - particle index in array
//-----------------------------------------------------------------------------
void CPSStreaks::InitializeParticle(const int &index)
{
	m_pParticleList[index].m_vPos = m_vecOrigin;
	m_pParticleList[index].m_vPosPrev = m_vecOrigin;
	m_pParticleList[index].m_vVel = m_vecVelocity * m_fSpeed;
	m_pParticleList[index].m_vVel.x += gEngfuncs.pfnRandomFloat(m_fVelocityMin, m_fVelocityMax);
	m_pParticleList[index].m_vVel.y += gEngfuncs.pfnRandomFloat(m_fVelocityMin, m_fVelocityMax);
	m_pParticleList[index].m_vVel.z += gEngfuncs.pfnRandomFloat(m_fVelocityMin, m_fVelocityMax);
	m_pParticleList[index].m_vAccel = Vector(0.0f, 0.0f, -1.0f/* *g_cl_gravity*/);
	m_pParticleList[index].m_fEnergy = m_fEnergyStart;
	m_pParticleList[index].m_fSizeX = fabs(m_fSizeX);
	m_pParticleList[index].m_fSizeY = fabs(m_fSizeY);
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;// 0.5f
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].SetColor(m_color, m_fBrightness);
	m_pParticleList[index].SetColorDelta(m_fColorDelta, m_fBrightnessDelta);

	if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
		m_pParticleList[index].FrameRandomize();
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen.
//-----------------------------------------------------------------------------
void CPSStreaks::Render(void)
{
//	if (!PointIsVisible(m_vecOrigin))// dangerous
//		return;

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
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos/*+ vx*/+ cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos/*+ vx*/- cy);
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(curPart->m_vPos - vx - cy);
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
