/*----------------------------------------------------------------
*	Render System
*
*	Copyright © 2001-2011, Xawari. All rights reserved.
*	Created for X-Half-Life: Deathmatch, a Half-Life modification.
*	http://x.netheaven.ru
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
*  USAGE NOTES:
* - please do not use vectors other than class Vector
* - please try to reuse as much code as possible and keep OOP model
* - optimize your code at the time you write it, consider every operator
* - please standartize commentaries as you see it is done here
*
*
* This source code contains no secret or confidential information.
*---------------------------------------------------------------*/
#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "triangleapi.h"

//-----------------------------------------------------------------------------
// Purpose: Default constructor. Should never be used.
//-----------------------------------------------------------------------------
CRenderSystem::CRenderSystem(void)
{
	ResetParameters();
/* should never be used anyway
	texindex = 0;
//?	m_pTexture = NULL;
	m_pNext = NULL;*/
	removenow = false;
	index = 0;
//	UpdateCallback = NULL;// !!!
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. Used for memory cleaning. Destroy all data here.
//-----------------------------------------------------------------------------
CRenderSystem::~CRenderSystem(void)
{
	// do not clear anything HERE, do it in KillSystem() because it's a base class
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Main constructor for external use (with all nescessary parameters)
//			e,g, g_pRenderManager->AddSystem(new CRenderSystem(a,b,...), 0, -1);
// Input  : origin - absolute position
//			velocity - 
//			angles - 
//			sprindex - precached sprite index (used as texture)
//			r_mode - kRenderTransAdd
//			r,g,b - RGB (0...255 each)
//			a - alpha (0...1)
//			adelta - alpha velocity, any value is acceptable
//			scale - positive values base on texture size, negative are absolute
//			scaledelta - scale velocity, any value is acceptable
//			framerate - texture frame rate (if animated), == FPS if negative
//			timetolive - 0 means the system removes itself after the last frame
//-----------------------------------------------------------------------------

CRenderSystem::CRenderSystem(const Vector &origin, const Vector &velocity, const Vector &angles, int sprindex, float frame, float framerate, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	// Calling constructors directly is forbidden!
	ResetParameters();// should be called in all constructors so no parameters will left uninitialized
	if (!InitTexture(sprindex))
	{
		removenow = true;// tell render manager to delete this system
		return;// no texture - no system
	}

	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_vecAngles = angles;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iRenderMode = r_mode;
	m_fFrameRate = framerate;
	m_fFrame = frame;
	m_iFrame = frame;

	if (timetolive <= 0.0f)
		m_fDieTime = 0.0f;// persist forever OR cycle through all texture frames and die (depends on Update function)
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CRenderSystem::ResetParameters(void)
{
	m_fScale = 1.0f;
	m_fScaleDelta = 0.0f;
	m_fBrightness = 1.0f;
	m_fBrightnessDelta = 0.0f;
	m_color.r = 255;
	m_color.g = 255;
	m_color.b = 255;
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fSizeX = 1.0f;
	m_fSizeY = 1.0f;
	m_fDieTime = 0.0f;
	m_fFrameRate = 0.0f;
	m_iRenderMode = 0;
	m_fFrame = 0;

	m_iAngleLife = 0;

	m_iFlags = 0;
	m_iFollowFlags = 0;
	m_iFollowEntity = -1;// does not follow any entities by default
	VectorClear(m_vecOrigin);
	VectorClear(m_vecVelocity);
	VectorClear(m_vecAngles);
	m_pTexture = NULL;// TESTME
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
void CRenderSystem::KillSystem(void)
{
	m_iRenderMode = 0;
	m_fFrame = 0;
	m_iFlags = 0;
	texindex = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Must be called from class constructor.
//-----------------------------------------------------------------------------
void CRenderSystem::InitializeSystem(void)
{
	m_fFrame = 0.0f;
	dying = false;
	removenow = false;
}

//-----------------------------------------------------------------------------
// Purpose: Load texture by index
// Input  : texture_index - precached sprite index
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::InitTexture(int texture_index)
{
	if (texture_index <= 0)
		return false;

	if (m_pTexture == NULL || texindex != texture_index)
	{
		model_s *pTexture = IEngineStudio.GetModelByIndex(texture_index);
		if (pTexture == NULL || pTexture->type != mod_sprite)
			return false;

		m_pTexture = pTexture;
		texindex = texture_index;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Update system parameters along with time
//			DO NOT PERFORM ANY DRAWING HERE!
// Input  : &time - current client time
//			&elapsedTime - time elapsed since last frame
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRenderSystem::Update(const float &time, const double &elapsedTime)
{
	dying = false;

	if (m_fDieTime > 0.0f && m_fDieTime <= time)
		dying = true;

	m_fBrightness += m_fBrightnessDelta*(float)elapsedTime;// fix for software mode: don't allow to be negative

	if( m_fBrightness <= 0.0f)
		m_fBrightness = 0.0f;

	if( m_fScale <= 0.0f)
		m_fScale = 0.0f;

	if (!dying && m_fBrightness <= 0.0f && m_fBrightnessDelta < 0.0f)
		dying = true;

	if (m_fFrameRate > 0.0f)
		UpdateFrame(time, elapsedTime);

	if (dying)// all vital calculations and checks should be made before this point
		return 1;

	FollowEntity();
	VectorMA(m_vecOrigin, elapsedTime, m_vecVelocity, m_vecOrigin);

	m_color.r += m_fColorDelta[0] * elapsedTime;
	m_color.g += m_fColorDelta[1] * elapsedTime;
	m_color.b += m_fColorDelta[2] * elapsedTime;
	m_fScale += m_fScaleDelta*elapsedTime;
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRenderSystem::Render(void)
{
	if (m_iFlags & RENDERSYSTEM_FLAG_NODRAW)
		return;

	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	Vector right, up;
	AngleVectors(m_vecAngles, NULL, right, up);

	gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame);
	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - right*m_fSizeX*m_fScale + up*m_fSizeY*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + right*m_fSizeX*m_fScale + up*m_fSizeY*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + right*m_fSizeX*m_fScale - up*m_fSizeY*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - right*m_fSizeX*m_fScale - up*m_fSizeY*m_fScale);
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine, allows to add user entities to render list
//-----------------------------------------------------------------------------
void CRenderSystem::CreateEntities(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: Find and follow specified entity
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
cl_entity_t *CRenderSystem::FollowEntity(void)
{
	if (dying == false && m_iFollowEntity > 0)
	{
		cl_entity_t *ent = gEngfuncs.GetEntityByIndex(m_iFollowEntity);
		if (ent)// != NULL)
		{
			if (!(m_iFollowFlags & RENDERSYSTEM_FFLAG_DONTFOLLOW))
			{
				if (ent->attachment[0] == Vector(0.0f,0.0f,0.0f))
				{
					if (ent->origin != Vector(0.0f,0.0f,0.0f))
					{
//						if (m_iFollowFlags & RENDERSYSTEM_FFLAG_USEOFFSET)// UNDONE
//						m_vecOrigin = ent->origin + ent->forward*offset[0] ... ?;
						VectorCopy(ent->origin, m_vecOrigin);
					}
				}
				else
					VectorCopy(ent->attachment[0], m_vecOrigin);// LAST
			}
			if (m_iFollowFlags & RENDERSYSTEM_FFLAG_ICNF_NODRAW)
			{
				m_iFlags &= ~RENDERSYSTEM_FLAG_NODRAW;// unhide
			}
		}
		else// Entity may got out of the PVS, or has been removed and this index will soon be occupied again!! Whad should we do?
		{
			if (m_iFollowFlags & RENDERSYSTEM_FFLAG_ICNF_STAYANDFORGET)
			{
				m_iFollowEntity = -1;
			}
			if (m_iFollowFlags & RENDERSYSTEM_FFLAG_ICNF_REMOVE)
			{
				dying = true;
			}
			if (m_iFollowFlags & RENDERSYSTEM_FFLAG_ICNF_NODRAW)
			{
				m_iFlags |= RENDERSYSTEM_FLAG_NODRAW;// hide
			}
		}
		return ent;// can be NULL
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: pick next frame if needed
// Input  : &time - 
//			&elapsedTime - 
//-----------------------------------------------------------------------------
void CRenderSystem::UpdateFrame(const float &time, const double &elapsedTime)
{
	if (m_pTexture == NULL || m_pTexture->numframes <= 1)
		return;

	if (m_iFlags & RENDERSYSTEM_FLAG_RANDOMFRAME)// UNDONE: display ALL frames but in RANDOM ORDER, then destroy if nescessary
	{
		m_fFrame = gEngfuncs.pfnRandomFloat(0, m_pTexture->numframes - 1);
		return;
	}

	if (m_fFrameRate < 0)// framerate == fps
	{
		m_fFrame += 1.0f;
	}
	else
	{
		m_fFrame += m_fFrameRate * elapsedTime;
	}

	if ((int)m_fFrame >= m_pTexture->numframes)
	{
		// don't remove after last frame
		if (m_fDieTime == 0.0f)
		{
			if (!(m_iFlags & RENDERSYSTEM_FLAG_LOOPFRAMES))
			{
				dying = true;
				return;
			}
		}
		m_fFrame -= (int)m_fFrame;// = 0.0f; // leave fractional part
	}
}

//-----------------------------------------------------------------------------
// Purpose: An external phusical force must be applied. Wind, shockwave, etc.
// Input  : origin - 
//			force - 
//			radius - 
//			point - 
//-----------------------------------------------------------------------------
void CRenderSystem::ApplyForce(const Vector &origin, const Vector &force, float radius, bool point)
{
	// nothing here
}

//-----------------------------------------------------------------------------
// Purpose: Check specified 3D point for visibility. This function decides what is to be drawn.
// Be EXTREMELY careful with this when porting! This thing may screw up your whole work!
// Input  : &point - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CRenderSystem::PointIsVisible(const Vector &point)
{
	// TODO: make render systems render pass-dependant?
	//m_iRenderPass == gHUD.m_iRenderPass

	if (m_iFlags & RENDERSYSTEM_FLAG_DRAWALWAYS)// may be used by 3D skybox elements
		return true;

	return true;
}
