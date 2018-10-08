// 02/08/02 November235: Particle System
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "event_api.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "particlemgr.h"
#include "particlesys.h"

ParticleSystemManager*	g_pParticleSystems = NULL;

ParticleSystemManager::ParticleSystemManager( void )
{
	m_pFirstSystem = NULL;
	//systemio = NULL;
}

ParticleSystemManager::~ParticleSystemManager( void )
{
	ClearSystems();
}

void ParticleSystemManager::AddSystem( ParticleSystem* pNewSystem )
{
	pNewSystem->m_pNextSystem = m_pFirstSystem;
	m_pFirstSystem = pNewSystem;
}

ParticleSystem *ParticleSystemManager::FindSystem( cl_entity_t* pEntity )
{
	for (ParticleSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNextSystem)
	{
		if (pEntity->index == pSys->m_iEntIndex)
//		if (pEntity == pSys->GetEntity())
		{
			return pSys;
		}
	}
	return NULL;
}

// blended particles don't use the z-buffer, so we need to sort them before drawing.
// for efficiency, only the systems are sorted - individual particles just get drawn in order of creation.
// (this should actually make things look better - no ugly popping when one particle passes through another.)
void ParticleSystemManager::SortSystems()
{
	ParticleSystem* pSystem;
	ParticleSystem* pLast;
	ParticleSystem* pBeforeCompare, *pCompare;
	
	if (!m_pFirstSystem) return;

	// calculate how far away each system is from the viewer
	for( pSystem = m_pFirstSystem; pSystem; pSystem = pSystem->m_pNextSystem )
		pSystem->CalculateDistance();

	// do an insertion sort on the systems
	pLast = m_pFirstSystem;
	pSystem = pLast->m_pNextSystem;
	while (pSystem)
	{
		if (pLast->m_fViewerDist < pSystem->m_fViewerDist)
		{
			// pSystem is in the wrong place! First, let's unlink it from the list
			pLast->m_pNextSystem = pSystem->m_pNextSystem;

			// then find somewhere to insert it
			if (m_pFirstSystem == pLast || m_pFirstSystem->m_fViewerDist < pSystem->m_fViewerDist)
			{
				// pSystem comes before the first system, insert it there
				pSystem->m_pNextSystem = m_pFirstSystem;
				m_pFirstSystem = pSystem;
			}
			else
			{
				// insert pSystem somewhere within the sorted part of the list
				pBeforeCompare = m_pFirstSystem;
				pCompare = pBeforeCompare->m_pNextSystem;
				while (pCompare != pLast)
				{
					if (pCompare->m_fViewerDist < pSystem->m_fViewerDist)
					{
						// pSystem comes before pCompare. We've found where it belongs.
						break;
					}

					pBeforeCompare = pCompare;
					pCompare = pBeforeCompare->m_pNextSystem;
				}

				// we've found where pSystem belongs. Insert it between pBeforeCompare and pCompare.
				pBeforeCompare->m_pNextSystem = pSystem;
				pSystem->m_pNextSystem = pCompare;
			}
		}
		else
		{
			//pSystem is in the right place, move on
			pLast = pSystem;
		}
		pSystem = pLast->m_pNextSystem;
	}
}

void ParticleSystemManager::UpdateSystems( float frametime ) //LRC - now with added time!
{
//	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
//	gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
	ParticleSystem* pSystem;
	ParticleSystem* pLast = NULL;
	ParticleSystem*pLastSorted = NULL;
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
//	vec3_t normal, forward, right, up;

//	gEngfuncs.GetViewAngles((float*)normal);
//	AngleVectors(normal,forward,right,up);

	//SortSystems();

	pSystem = m_pFirstSystem;
	while( pSystem )
	{
		if(	pSystem->UpdateSystem(frametime, /*right, up,*/ localPlayer->curstate.messagenum) )
		{
			pSystem->DrawSystem();
			pLast = pSystem;
			pSystem = pSystem->m_pNextSystem;
		}
		else // delete this system
		{
			if (pLast)
			{
				pLast->m_pNextSystem = pSystem->m_pNextSystem;
				delete pSystem;
				pSystem = pLast->m_pNextSystem;
			}
			else // deleting the first system
			{
				m_pFirstSystem = pSystem->m_pNextSystem;
				delete pSystem;
				pSystem = m_pFirstSystem;
			}
		}
	}
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

void ParticleSystemManager::ClearSystems( void )
{
	ParticleSystem* pSystem = m_pFirstSystem;
	ParticleSystem* pTemp;

	while( pSystem )
	{
		pTemp = pSystem->m_pNextSystem;
		delete pSystem;
		pSystem = pTemp;
	}

	m_pFirstSystem = NULL;
}