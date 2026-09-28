#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSDrips.h"
#include "pm_defs.h"
#include "event_api.h"
#include "triangleapi.h"
#include "studio_util.h"

CPSDrips::CPSDrips(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CPSDrips::~CPSDrips(void)
{
	KillSystem();
}

// TODO: fix particle rotation!!
CPSDrips::CPSDrips(int maxParticles, const Vector &origin, const Vector &mins, const Vector &maxs, const Vector &dir, int sprindex, int sprindex_splash, int r_mode, float sizex, float sizey, float scaledelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
//	m_pTexture = NULL;
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
//	m_iSplashTexture = sprindex_splash;
	if (sprindex_splash > 0)
	{
		m_pTexture2 = IEngineStudio.GetModelByIndex(sprindex_splash);
		if (!m_pTexture2 || m_pTexture2->type != mod_sprite)
			m_pTexture2 = NULL;
	}
	else
		m_pTexture2 = NULL;

	m_iMaxParticles = maxParticles;
	VectorCopy(mins, m_vecMinS);
	VectorCopy(maxs, m_vecMaxS);
	m_vecMinS[2] += 1.0f;

//	m_vecDirection = dir.Normalize();
//	m_fSpeed = dir.Length();
	VectorCopy(origin, m_vecOrigin);
	VectorCopy(dir, m_vecDirection);
	m_fSpeed = VectorNormalize(m_vecDirection);

	m_fScale = -0.1f;// XDM3035: this prevents CRenderSystem::InitializeSystem() from modifying sizes
	m_fScaleDelta = scaledelta;
	m_fSizeX = m_pTexture->maxs[1] - m_pTexture->mins[1];
	m_fSizeY = m_pTexture->maxs[2] - m_pTexture->mins[2];
	if (sizex > 0.0f)
		m_fSizeX *= sizex;
	if (sizey > 0.0f)
		m_fSizeY *= sizey;

//	m_pParticleList = NULL;
//	m_iFollowEntity = -1;
	m_iRenderMode = r_mode;

	if (timetolive <= 0.0f)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

//	CON_PRINTF("CPSDrips: size: %f %f\n", m_fSizeX, m_fSizeY);
	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CPSDrips::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	m_fSpeed = 0.0f;
//	m_fSizeX = 1.0f;
//	m_fSizeY = 1.0f;
//	m_iSplashTexture = 0;
	m_pTexture2 = NULL;
	VectorClear(m_vecMinS);
	VectorClear(m_vecMaxS);
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CPSDrips::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0 && m_fDieTime <= time)
		dying = true;

	if (dying && m_iNumParticles == 0)
		return 1;

//	if (!(m_iFlags & RENDERSYSTEM_FLAG_DONTFOLLOW))
//	if (FollowEntity() == false)// entity not visible
//		return 0;

//	CON_DPRINTF(" .. CPSDrips::Update() (e %d)\n", m_iFollowEntity);
//	Emit(ceil((float)m_iMaxParticles*0.5f));
	Emit(Q_max(1, m_iMaxParticles>>2));// 1/4

	int c = 0;
	pmtrace_t pmtrace;
	CParticle *curPart = NULL;

	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction(false, true);
	gEngfuncs.pEventAPI->EV_PushPMStates();// BUGBUG: do not call this during map loading process
//	gEngfuncs.pEventAPI->EV_SetSolidPlayers(-1);
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);

	for (int i = 0; i < m_iNumParticles; ++i)
	{
		curPart = &m_pParticleList[i];

		if (curPart->m_fEnergy <= 0.0f)
			m_pParticleList[i] = m_pParticleList[--m_iNumParticles];

		if (curPart->m_iFlags & PARTICLE_FLAG1)// splash
		{
			curPart->m_fEnergy -= (float)(1.5 * elapsedTime);
		}
		else
		{
			VectorCopy(curPart->m_vPos, curPart->m_vPosPrev);

			// BUGBUG TODO FIXME T_T  do something with this!!!
			VectorMA(curPart->m_vVel, elapsedTime, curPart->m_vAccel, curPart->m_vVel);
			VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVel, curPart->m_vPos);
//works LAGLAG			VectorMA(curPart->m_vPos, elapsedTime, curPart->m_vVelAdd, curPart->m_vPos);
//works			VectorScale(curPart->m_vVelAdd, 1.0f-elapsedTime*2.0f, curPart->m_vVelAdd);

//			VectorMA(curPart->m_vAccel, elapsedTime*-0.1, curPart->m_vAccel, curPart->m_vAccel);

			gEngfuncs.pEventAPI->EV_PlayerTrace(curPart->m_vPosPrev, curPart->m_vPos, PM_STUDIO_BOX, -1, &pmtrace);
			if (!pmtrace.inwater)
				c = gEngfuncs.PM_PointContents(curPart->m_vPos, NULL);

			if (pmtrace.inwater || (c < CONTENTS_SOLID && c > CONTENTS_SKY))
			{
//				CON_DPRINTF("in water\n");
				VectorClear(curPart->m_vVel);
				if (/*m_iSplashTexture > 0*/m_pTexture2 && PointIsVisible(curPart->m_vPos))// FIXME too slow?
				{
					curPart->m_pTexture = m_pTexture2;//IEngineStudio.GetModelByIndex(m_iSplashTexture);
/*					if (!curPart->m_pTexture || curPart->m_pTexture->type != mod_sprite)
					{
						curPart->m_fEnergy = -1.0f;
						continue;
					}
					else*/
					{
//						CON_DPRINTF("texture loaded\n");
						curPart->m_fEnergy = 1.0f;
//						curPart->m_fSizeX = 4.0f;
//						curPart->m_fSizeY = 4.0f;
						curPart->SetSizeFromTexture(m_fScale, m_fScale);// slow?
						curPart->m_fSizeDelta = SPLASH_SIZE_DELTA;
						curPart->m_iFlags |= PARTICLE_FLAG1;
						curPart->m_iFrame = 0;
#ifdef DRIPSPARALLELTEST// testing: circles parallel to surface
						Vector normal;//, angles;
						VectorCopy(pmtrace.plane.normal, normal);
						normal[0] *= -1.0f;
						normal[1] *= -1.0f;
						VectorAngles(normal, curPart->m_vVel);// angles
#endif// unused since there is only horizontal water in Half-Life

					}
				}
				else
				{
					curPart->m_fEnergy = -1.0f;
					continue;
				}
			}
			else if (pmtrace.fraction != 1.0f)
			{
//				if (m_iFlags & RENDERSYSTEM_FLAG_CLIPREMOVE || c == CONTENTS_SOLID || !PointIsVisible(curPart->m_vPos))
				curPart->m_fEnergy = -1.0f;
				curPart->m_fSizeDelta = 0.0f;
				VectorClear(curPart->m_vVel);
				continue;
			}
		}
		if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
			curPart->FrameRandomize();
		else
			curPart->FrameIncrease();

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
void CPSDrips::InitializeParticle(const int &index)
{
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].m_vPos[0] = m_vecOrigin[0] + gEngfuncs.pfnRandomFloat(m_vecMinS[0], m_vecMaxS[0]);
	m_pParticleList[index].m_vPos[1] = m_vecOrigin[1] + gEngfuncs.pfnRandomFloat(m_vecMinS[1], m_vecMaxS[1]);
	m_pParticleList[index].m_vPos[2] = m_vecOrigin[2] + gEngfuncs.pfnRandomFloat(m_vecMinS[2], m_vecMaxS[2]);

	VectorCopy(m_pParticleList[index].m_vPos, m_pParticleList[index].m_vPosPrev);
	VectorCopy(m_vecDirection, m_pParticleList[index].m_vAccel);
	VectorScale(m_vecDirection, m_fSpeed*gEngfuncs.pfnRandomFloat(0.9, 1.1), m_pParticleList[index].m_vVel);// UNDONE: randomize it the 'right' way
//LAGLAG	VectorClear(m_pParticleList[index].m_vVelAdd);

	m_pParticleList[index].m_fEnergy = 1.0f;
	m_pParticleList[index].m_fSizeX = m_fSizeX*0.1f;//2.0f;
	m_pParticleList[index].m_fSizeY = m_fSizeY*0.1f;//32.0f;
	m_pParticleList[index].m_fSizeDelta = m_fScaleDelta;// XDM3035: 20110504
	m_pParticleList[index].m_iFlags = 0;
	m_pParticleList[index].SetDefaultColor();

	if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
		m_pParticleList[index].FrameRandomize();
//	m_pParticleList[index].m_weight = 0.0;
//	m_pParticleList[index].m_weightDelta = 0.0;
}
/*
void CPSDrips::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	Vector delta;
	float d, f;
	int i;
	// particles closer to origin gets more velocity (radius-l)
	// partivles outside radius are not affected
	if (point)// random direction from origin
	{
		f = Length(force)*0.001;//*CVAR_GET_FLOAT("test1");
//		CON_PRINTF("CPSDrips: f = %f\n", f);
		for (i = 0; i < m_iNumParticles; ++i)
		{
			VectorSubtract(m_pParticleList[i].m_vPos, origin, delta);
			d = Length(delta);
			if (d <= radius)
			{
				// k = radius/d; this should normalize delta-vectors up to radius (I could use Normalize(), but it is SLOWER
// ADD to previous				VectorScale(delta, (radius/d)*(radius-d)*f, m_pParticleList[i].m_vVelAdd);
				VectorMA(m_pParticleList[i].m_vVelAdd, (radius/d)*(radius-d)*f, delta, m_pParticleList[i].m_vVelAdd);
//		debug		m_pParticleList[i].m_fColor[0]=1.0;
//				m_pParticleList[i].m_fColor[1]=0.1;
//				m_pParticleList[i].m_fColor[2]=0.1;
//				m_pParticleList[i].m_fColor[3]=1.0;
			}
		}
	}
	else
	{
		for (i = 0; i < m_iNumParticles; ++i)
		{
			VectorSubtract(m_pParticleList[i].m_vPos, origin, delta);
			d = Length(delta);
			if (d <= radius)
			{
				VectorMA(m_pParticleList[i].m_vVelAdd, (radius/d)*(radius-d), force, m_pParticleList[i].m_vVelAdd);
	//			VectorMA(m_pParticleList[i].m_vVelAdd, (radius-l)/radius, force, m_pParticleList[i].m_vVelAdd);
	//			VectorAdd(m_pParticleList[i].m_vVelAdd, force, m_pParticleList[i].m_vVelAdd);
	//			VectorCopy(m_pParticleList[i].m_vVelAdd, m_pParticleList[i].m_vAccel);
			}
		}
	}
}
*/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPSDrips::Render(void)
{
	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)
		return;

	if (m_iFlags & RENDERSYSTEM_FLAG_ZROTATION)// UNDONE: TODO: rotate around direction vector, not just Z axis
	{
		m_vecAngles[0] = 0.0f;
		m_vecAngles[2] = 0.0f;
	}
	Vector v_up, v_right;
	Vector v1, v2;// v_up, v_right for circle
// horizontal
	v1[0] = 1.0f;
	v1[1] = 0.0f;
	v1[2] = 0.0f;
	v2[0] = 0.0f;
	v2[1] = 1.0f;
	v2[2] = 0.0f;
	AngleVectors(m_vecAngles, NULL, v_right, v_up);// TODO: this is common angles for all particles which is fast but not as nice as individual sprite-like rotation
	Vector rx;// tmp for faster code
	Vector uy;

	CParticle *p = NULL;
	// We should draw rain as a single mesh which is faster but we can't do that because of different texture frames
	for (int i = 0; i < m_iNumParticles; ++i)
	{
		p = &m_pParticleList[i];
		if (p->m_fEnergy <= 0.0f)
			continue;

		if (!PointIsVisible(p->m_vPos))// faster? Can't perform check on system origin because it's a large brush entity
			continue;

		if (p->m_iFlags & PARTICLE_FLAG1)// water circle
		{
#ifdef DRIPSPARALLELTEST// testing: circles parallel to surface
			AngleVectors(p->m_vVel, NULL, v1, v2);
#endif
			p->Render(v1, v2, m_iRenderMode, true);
		}
		else// otherwise use custom render function
		{
			if (gEngfuncs.pTriAPI->SpriteTexture(p->m_pTexture, p->m_iFrame))
			{
//				float adiff = AngleBetweenVectors(g_vecViewForward, p->m_vPos - g_vecViewOrigin);
//				AngleVectors(m_vecAngles + Vector(0.0f,0.0f,adiff), NULL, v_right, v_up);
				rx = v_right * p->m_fSizeX;
				uy = v_up * p->m_fSizeY;
				// UNDONE: swap m_vPos and m_vPosPrev for reversed rain?
				gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
				gEngfuncs.pTriAPI->CullFace(TRI_NONE);
				gEngfuncs.pTriAPI->Begin(TRI_QUADS);
				gEngfuncs.pTriAPI->Color4f(p->m_fColor[0], p->m_fColor[1], p->m_fColor[2], p->m_fColor[3]);
				gEngfuncs.pTriAPI->Brightness(p->m_fColor[3]);
				gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
				gEngfuncs.pTriAPI->Vertex3fv(p->m_vPosPrev + rx + uy);
				gEngfuncs.pTriAPI->TexCoord2f(1.0f, 1.0f);
				gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos + rx - uy);
				gEngfuncs.pTriAPI->TexCoord2f(0.0f, 1.0f);
				gEngfuncs.pTriAPI->Vertex3fv(p->m_vPos - rx - uy);
				gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
				gEngfuncs.pTriAPI->Vertex3fv(p->m_vPosPrev - rx + uy);
				gEngfuncs.pTriAPI->End();
//				gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
			}

//			p->Render(v_right, v_up, rendermode, true);
		}
	}
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
