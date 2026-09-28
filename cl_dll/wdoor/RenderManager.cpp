/*----------------------------------------------------------------
*	Render System
*
*	Copyright � 2001-2010, Xawari. All rights reserved.
*	Created for X-Half-Life: Deathmatch, a Half-Life modification.
*	http://xwider.wtr.ru
*
*	This code partially depends on software technology
*	created by Valve LLC.
*
*	Author of this code allows redistribution and use of it only
*	in non-commercial (non-profit) and open-source projects.
*
*	If this code to be used along with Valve Gold Source(tm) Endine,
*	the developer must also accept and follow its license agreement.
*
*
* This source code contains no secret or confidential information.
*---------------------------------------------------------------*/

#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "triangleapi.h"

CRenderManager *g_pRenderManager = NULL;

//-----------------------------------------------------------------------------
//  CRenderManager
// Purpose: global render system manager,
// adds, updates and removes all rendersystems
//-----------------------------------------------------------------------------
CRenderManager::CRenderManager()
{
	m_pFirstSystem = NULL;
	if (gEngfuncs.pTriAPI)
		if (gEngfuncs.pTriAPI->version != TRI_API_VERSION)
			gEngfuncs.Con_DPrintf(" WARNING: unexpected triangle API version: %d!\n", gEngfuncs.pTriAPI->version);
}

CRenderManager::~CRenderManager()
{
//	KillAllSystems();
	m_pFirstSystem = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: add render system to manager's list
// Input  : *pSystem - newly created RS
//			flags - render system custom flags
//			followentindex - entity to follow (if any)
// Output : int RS unique index
// Changes: 20100107: added return index
//-----------------------------------------------------------------------------
int CRenderManager::AddSystem(CRenderSystem *pSystem, int flags, int followentindex, int followflags/*, float delay*/)
{
	if (pSystem == NULL)
		return 0;

	if (pSystem->IsRemoving())//removenow == true)
	{
	//	gEngfuncs.Con_DPrintf("ERROR: Render system failed to initialize!\n");
		delete pSystem;
		return 0;
	}

	// TODO: if (delay > 0.0f)
	// add system to a temporary queue and then pick it in Update() and activate

	pSystem->m_iFlags |= flags;
	pSystem->m_iFollowFlags |= followflags;
//	pSystem->m_fStartTime = gEngfuncs.GetClientTime() + delay;

	if (followentindex > 0)
		pSystem->m_iFollowEntity = followentindex;

	pSystem->index = (unsigned int)(size_t)pSystem;//GetFirstFreeRSUID();// HACK: use real ID instead of memory address!

	pSystem->m_pNext = m_pFirstSystem;
	m_pFirstSystem = pSystem;
	return pSystem->index;
}

//-----------------------------------------------------------------------------
// Purpose: find a render system and remove it from manager's list
//-----------------------------------------------------------------------------
void CRenderManager::DeleteSystem(CRenderSystem *pSystem)
{
	if (pSystem == NULL)
		return;

	CRenderSystem *pSys = m_pFirstSystem;
	CRenderSystem *pLast = NULL;

	while (pSys)
	{
		if (pSys == pSystem)
		{
			pSys->dying = true;
//			pSys->m_fDieTime = gEngfuncs.GetClientTime();// ?

			if (pLast)
			{
				pLast->m_pNext = pSys->m_pNext;
			}
			else
			{
				m_pFirstSystem = pSys->m_pNext;
			}
			delete pSys;
			return;// or pSys = NULL;
		}
		else
		{
			pLast = pSys;
			pSys = pSys->m_pNext;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: delete all systems
//-----------------------------------------------------------------------------
void CRenderManager::DeleteAllSystems(void)
{
	CRenderSystem *pSys = m_pFirstSystem;
	CRenderSystem *pNext = NULL;

	while (pSys)
	{
		pNext = pSys->m_pNext;
		delete pSys;
		pSys = pNext;
	}

	m_pFirstSystem = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Update all systems. Called from HUD_Redraw()
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
//-----------------------------------------------------------------------------
void CRenderManager::Update(const float &time, const double &elapsedTime)
{
    // UNDONE: TODO: how to disable this during loading screens?!

    CRenderSystem *pSys = m_pFirstSystem;
    CRenderSystem *pLast = NULL;
    bool remove = 0;

    while (pSys)
    {
        remove = pSys->IsRemoving();

        if (remove == 0)
            remove |= pSys->Update(time, elapsedTime); // == 1

        if (remove) // delete system
        {
            if (pLast)
            {
                pLast->m_pNext = pSys->m_pNext;
                delete pSys;
                pSys = pLast->m_pNext;
            }
            else // first system
            {
                m_pFirstSystem = pSys->m_pNext;
                delete pSys;
                pSys = m_pFirstSystem;
            }
        }
        else
        {
            pLast = pSys;
            pSys = pSys->m_pNext;
        }
    }
}
//-----------------------------------------------------------------------------
// Purpose: render all active systems with depth-dependant render modes
//-----------------------------------------------------------------------------
void CRenderManager::RenderOpaque(void)
{
// UNDONE: this makes smoke appear as one-color surfaces
/*	if (m_pFirstSystem == NULL)
		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
		if (pSys->m_iRenderMode == kRenderNormal
			|| pSys->m_iRenderMode == kRenderTransAlpha)
				pSys->Render();
//				pSys->RenderOpaque();
*/
}

//-----------------------------------------------------------------------------
// Purpose: render all active systems with transparent render modes
//-----------------------------------------------------------------------------
void CRenderManager::RenderTransparent(void)
{
	if (m_pFirstSystem == NULL)
		return;
/*
	cl_entity_t *pClient = gEngfuncs.GetLocalPlayer();
	if (pClient == NULL)
		return;
*/
//	if (gHUD.m_Spectator.m_iDrawCycle == ???)
//		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
// UNDONE		if (pSys->m_iRenderMode != kRenderNormal
//			&& pSys->m_iRenderMode != kRenderTransAlpha)
			pSys->Render();
}

//-----------------------------------------------------------------------------
// Purpose: render all active systems   TEST
//-----------------------------------------------------------------------------
/*id CRenderManager::Render(void)
{
	if (m_pFirstSystem == NULL)
		return;

//	if (gHUD.m_Spectator.m_iDrawCycle == ???)
//		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
		pSys->Render();
}*/

//-----------------------------------------------------------------------------
// Purpose: called by the engine to add special entities to render
//-----------------------------------------------------------------------------
void CRenderManager::CreateEntities(void)
{
	if (m_pFirstSystem == NULL)
		return;

	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
		pSys->CreateEntities();
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by follow entity index
// WARNING! There may be more than one RS following this entity!
// Input  : entindex - entity index
// Output : CRenderSystem pointer or NULL if not found
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystemByFollowEntity(int entindex)
{
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->m_iFollowEntity == entindex)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a render system by its index
// Input  : index - RS UID
// Output : CRenderSystem pointer or NULL if not found
//-----------------------------------------------------------------------------
CRenderSystem *CRenderManager::FindSystem(unsigned int index)
{
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		if (pSys->index == index)
			return pSys;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Apply physical force to render systems (for wind+rain effects)
// Input  : origin - origin from which the force will be applied
//			force - force vector
//			radius - affecto only systems in this radius
//			point - if true, force goes from origin, value = Length(force)
//-----------------------------------------------------------------------------
void CRenderManager::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		// DO NOT check radius HERE! Some systems like rain or snow MUST be affected regardless to its 'origin'
		pSys->ApplyForce(origin, force, radius, point);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get first available unique index for render system
// Output : int UID
//-----------------------------------------------------------------------------
//
// test pattern:
// 1 3 7 6 4 9 12 2
//
unsigned int CRenderManager::GetFirstFreeRSUID(void)
{
/*	// TODO: generate sequental RS indexes
	unsigned int previous_lesser_index = 0xFFFFFFFF;
	unsigned int system_index = 0xFFFFFFFF;
	unsigned int parsed = 0;

	unsigned int first_less_index = 0xFFFFFFFF;
	unsigned int second_less_index = 0xFFFFFFFF;


	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext)
	{
		system_index = pSys->GetIndex();
		++parsed;

		if (system_index < first_less_index)
		{

			if (system_index - second_less_index > 1)
		}
		wtf
		
		if (system_index < previous_lesser_index)
		{
//			if (system_index <= 1)// there is no index lesser than 1
//				return previous_lesser_index;

			if (previous_lesser_index - system_index > 1)// we skipped at least one index which is probably free
				previous_lesser_index = system_index + 1;
		}
	}
	if (parsed < 1)
		previous_lesser_index = 1;

	return previous_lesser_index;
*/
	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: print all systems information to console
// Output : does not (yet?) print system index or name
//-----------------------------------------------------------------------------
void CRenderManager::DumpSystems(void)
{
	int n = 0;
	for (CRenderSystem *pSys = m_pFirstSystem; pSys; pSys = pSys->m_pNext, ++n)
		gEngfuncs.Con_DPrintf(" > %d: follow entity: %d, dying: %d\n", pSys->GetIndex(), pSys->m_iFollowEntity, pSys->dying);

	gEngfuncs.Con_DPrintf(" %d systems\n", n);
}

