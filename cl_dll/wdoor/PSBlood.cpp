//=============================================================================//
//Purpose: generates gravity-dependant particles. When touch WORLD, make decals//
//=============================================================================//
#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "ParticleSystem.h"
#include "PSBlood.h"
#include "pm_defs.h"
#include "event_api.h"
#include "r_efx.h"
#include "studio_util.h"

void EV_DecalTrace( pmtrace_t *pTrace, char *decalName )
{
	physent_t *pe;
	pe = gEngfuncs.pEventAPI->EV_GetPhysent( pTrace->ent );

	// Only decal brush models such as the world etc.
	if (  decalName && decalName[0] && pe && ( pe->solid == SOLID_BSP || pe->movetype == MOVETYPE_PUSHSTEP ) )
	{
		if ( CVAR_GET_FLOAT( "r_decals" ) )
		{
			gEngfuncs.pEfxAPI->R_DecalShoot( 
				gEngfuncs.pEfxAPI->Draw_DecalIndex( gEngfuncs.pEfxAPI->Draw_DecalIndexFromName( decalName ) ), 
				gEngfuncs.pEventAPI->EV_IndexFromTrace( pTrace ), 0, pTrace->endpos, 0 );
		}
	}
}

char *EV_BloodDecal( void )
{
        static char blooddecal[ 32 ];
		int idx = gEngfuncs.pfnRandomLong(0,7);
		sprintf( blooddecal, "{blood%is", idx + 1 );

        return blooddecal;
}

char *EV_BloodDecal2( void )
{
        static char blooddecal[ 32 ];

		int idx2 = gEngfuncs.pfnRandomLong(0,5);
		sprintf( blooddecal, "{yblood%is", idx2 + 1 );

        return blooddecal;
}


CPSBlood::CPSBlood(void)
{
	ResetParameters();
}

CPSBlood::~CPSBlood(void)
{
	KillSystem();
}

CPSBlood::CPSBlood(int maxParticles, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float size, int sprindex, int frame, float PartEmitterLife,int color)
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
	m_iRenderMode = kRenderTransTexture;
	m_iFrame = frame;
	m_fScale = size;
	m_color.r = 80;
	if(color == 1){
	m_color.g = 80;
	}
	else{
	m_color.g = 0;
	}
	m_color.b = 0;

	if (PartEmitterLife <= 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + PartEmitterLife;

	InitializeSystem();
}

void CPSBlood::ResetParameters(void)
{
	CParticleSystem::ResetParameters();
	VectorClear(m_vecSpread);
	m_flRandomDir = true;
	m_fParticleVelocity = 100.0f;
}

bool CPSBlood::Update(const float &time, const double &elapsedTime)
{
	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	m_fBrightness += m_fBrightnessDelta*elapsedTime;

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

		if (pmtrace.fraction >= 1.0f){
			curPart->m_fEnergy -= 1.5f * elapsedTime;
		}
		else{
			curPart->m_fEnergy = -1.0f;
				if(gEngfuncs.pfnRandomLong(0,1)){
					if(m_color.g > 0){
					EV_DecalTrace(&pmtrace, EV_BloodDecal2());
					}
					else{
					EV_DecalTrace(&pmtrace, EV_BloodDecal());
					}
				}
		}
	}
	return 0;
}

void CPSBlood::InitializeParticle(const int &index)
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

	m_pParticleList[index].m_vVel = m_pParticleList[index].m_vVel * m_fParticleVelocity;
	m_pParticleList[index].m_vAccel.z -= g_cl_gravity/2;
	m_pParticleList[index].m_fEnergy = 10.0f;
	m_pParticleList[index].m_fSizeX = m_fScale;
	m_pParticleList[index].m_fSizeY = m_fScale;
	m_pParticleList[index].m_pTexture = m_pTexture;
	m_pParticleList[index].m_iFrame = m_iFrame;
	m_pParticleList[index].SetColor(m_color, 1.0f);

	if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)
		m_pParticleList[index].FrameRandomize();
}
