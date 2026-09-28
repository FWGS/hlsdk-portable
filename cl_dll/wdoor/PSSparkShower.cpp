//===========================================================================//
//Purpose: generates gravity-dependant spark effects, which removes in water!//
//===========================================================================//
#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSSparkShower.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "game_fx.h"
#include "PSBlastCone.h"
#include "PSSparks.h"
#include "PSBlood.h"
#include "studio_util.h"

PSSparkShower::PSSparkShower(void)
{
	ResetParameters();
}

PSSparkShower::~PSSparkShower(void)
{
	KillSystem();
}

PSSparkShower::PSSparkShower(int maxParticles, float PartLife, float partvelocity, int SparkEffect, const Vector &origin, const Vector &direction, const Vector &spread, int sprindex, float PartEmitterLife)
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
	m_iRenderMode = kRenderTransAdd;
	m_fLife = PartLife;
	m_fSparkEffect = SparkEffect;

	if (PartEmitterLife <= 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + PartEmitterLife;

	InitializeSystem();
}

void PSSparkShower::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	VectorClear(m_vecSpread);
	m_flRandomDir = true;
	m_fParticleVelocity = 100.0f;
}

bool PSSparkShower::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

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
				curPart->m_vVel = curPart->m_vVel * 0.7f;
			}
		}
		switch (m_fSparkEffect)
		{
			case SPARKSHOWER_SPARKS:
				gEngfuncs.pEfxAPI->R_SparkEffect(curPart->m_vPos, 1, -128, 128);
			break;

			case SPARKSHOWER_SPARKS2:
//���� ���� �襭�� �஡����, �⮡� �� ᮧ������ ����� ��⥬� ��� ����� ����!!!
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 0, curPart->m_vPos, curPart->m_vPos, Vector(0,0,0), 5, 0, 255,167,17, 0.7, -0.8, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr1.spr"), TRUE, -1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_RANDOMFRAME, -1);
				gEngfuncs.pEfxAPI->R_StreakSplash ( curPart->m_vPos, vec3_origin, 5, gEngfuncs.pfnRandomFloat(2,3), 120, -128, 128);
			break;

			case SPARKSHOWER_EXP:
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 20, curPart->m_vPos, curPart->m_vPos, Vector(1,1,1), 15, 40, 255,255,255, 0.5, -0.5, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_6, kRenderTransAdd, 0.05), 0, -1);
			break;

			case SPARKSHOWER_STREAKS:
				gEngfuncs.pEfxAPI->R_StreakSplash ( curPart->m_vPos, vec3_origin, 5, gEngfuncs.pfnRandomFloat(3,7), 400, -150, 150);
			break;

			case SPARKSHOWER_FLICKER:
				gEngfuncs.pEfxAPI->R_FlickerParticles(curPart->m_vPos);
			break;

			case SPARKSHOWER_SPARKSMOKE:
				gEngfuncs.pEfxAPI->R_BulletImpactParticles(curPart->m_vPos);
			break;

			case SPARKSHOWER_SMOKE:
				gEngfuncs.pEfxAPI->R_RocketTrail ( curPart->m_vPosPrev, curPart->m_vPos, 1 );
			break;

			case SPARKSHOWER_FIRESMOKE:
				gEngfuncs.pEfxAPI->R_RocketTrail ( curPart->m_vPosPrev, curPart->m_vPos, 0 );
			break;

			case SPARKSHOWER_BLOODDRIPS:
				gEngfuncs.pEfxAPI->R_RocketTrail ( curPart->m_vPosPrev, curPart->m_vPos, 2 );
			break;

			case SPARKSHOWER_FIREEXP:
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, curPart->m_vPos, curPart->m_vPos, Vector(1,1,1), 10, 25, 255,255,255, 0.8, -0.95, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/flame.spr"), TRUE, -1, kRenderTransAdd, 0.1), RENDERSYSTEM_FLAG_RANDOMFRAME, -1);
			break;

			case SPARKSHOWER_ENERGY:
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 30, curPart->m_vPos, curPart->m_vPos, Vector(1,1,1), 10, 15, 255,255,255, 0.8, -0.95, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/particles_red.spr"), FALSE, PARTICLE_RED_3, kRenderTransAdd, 0.1), 0, -1);
			break;

			case SPARKSHOWER_LAVA_FLAME:
				g_pRenderManager->AddSystem(new CPSBlastCone(1, 15, curPart->m_vPos, Vector(0,0,1), Vector(0.2,0.2,0.4), 5, 15, 128,128,128, 1, -0.9, gEngfuncs.pEventAPI->EV_FindModelIndex("sprites/anim_spr13.spr"), TRUE, 0, kRenderTransAdd, 0.1), 0, -1);
			break;
		}
		curPart->m_fEnergy -= 1.5f * elapsedTime;
	}
	return 0;
}

void PSSparkShower::InitializeParticle(const int &index)
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
	m_pParticleList[index].m_fSizeX = 1.0f;
	m_pParticleList[index].m_fSizeY = 1.0f;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].SetColor(m_color, 0.1f);
}
