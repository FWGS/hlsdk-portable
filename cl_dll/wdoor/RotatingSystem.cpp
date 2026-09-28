#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
#include "triangleapi.h"
#include "studio_util.h"

//-----------------------------------------------------------------------------
// Purpose: Default constructor. Should never be used.
//-----------------------------------------------------------------------------
CRotatingSystem::CRotatingSystem(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor. Used for memory cleaning. Destroy all data here.
//-----------------------------------------------------------------------------
CRotatingSystem::~CRotatingSystem(void)
{
	// do not clear anything HERE, do it in KillSystem() because it's a base class
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor for external everyday use.
// Input  : origin - 
//			velocity - 
//			angles - 
//			anglesdelta - 
//			sprindex - 
//			r_mode - 
//			r - 
//			g - 
//			b - 
//			a - 
//			adelta - 
//			scale - 
//			scaledelta - 
//			timetolive - 0 means the system removes itself after the last frame
//-----------------------------------------------------------------------------
CRotatingSystem::CRotatingSystem(const Vector &origin, const Vector &velocity, const Vector &angles, const Vector &anglesdelta, int sprindex, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	// Calling constructors directly is forbidden!
	ResetParameters();
	m_pTexture = NULL;
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_vecOrigin = origin;
	m_vecVelocity = velocity;
	m_vecAngles = angles;
	m_vecAnglesDelta = anglesdelta;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iFollowEntity = -1;
	m_iRenderMode = r_mode;

	if (timetolive < 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
//	dying = true;
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
//-----------------------------------------------------------------------------
void CRotatingSystem::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	VectorClear(m_vecAnglesDelta);
	for (int i=0; i<3; ++i)
		for (int j=0; j<4; ++j)
			m_fMatrix[i][j] = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Clear-out and free dynamically allocated memory
//-----------------------------------------------------------------------------
void CRotatingSystem::KillSystem(void)
{
	VectorClear(m_vecAnglesDelta);
//	m_fMatrix = NULL;?? DEBUG: we MAY want to fill this with 0x000000 or smth?
	CRenderSystem::KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize SYSTEM (non-user) startup variables.
// Must be called from class constructor.
//-----------------------------------------------------------------------------
void CRotatingSystem::InitializeSystem(void)
{
	CRenderSystem::InitializeSystem();
	UpdateAngleMatrix();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &time - 
//			&elapsedTime - 
// Output : Returns true if needs to be removed
//-----------------------------------------------------------------------------
bool CRotatingSystem::Update(const float &time, const double &elapsedTime)
{
	bool ret = CRenderSystem::Update(time, elapsedTime);

	if (ret == 0)
		UpdateAngles(elapsedTime, true);

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Draw system to screen. May get called in various situations, so
// DON'T change any RS variables here (do it in Update() instead).
//-----------------------------------------------------------------------------
void CRotatingSystem::Render(void)
{
	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(0,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(-m_fScale*m_fSizeX, -m_fScale*m_fSizeY, 0.0f));// - -
	gEngfuncs.pTriAPI->TexCoord2f(0,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(-m_fScale*m_fSizeX, m_fScale*m_fSizeY, 0.0f));// - +
	gEngfuncs.pTriAPI->TexCoord2f(1,1);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_fSizeX, m_fScale*m_fSizeY, 0.0f));// + +
	gEngfuncs.pTriAPI->TexCoord2f(1,0);
	gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(m_fScale*m_fSizeX, -m_fScale*m_fSizeY, 0.0f));// + -

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate angle matrix using local origin and angles
//-----------------------------------------------------------------------------
void CRotatingSystem::UpdateAngleMatrix(void)
{
	AngleMatrix2(m_vecOrigin, m_vecAngles, m_fMatrix);
}

//-----------------------------------------------------------------------------
// Purpose: Convert local RS coordinates into absolute world coordinates
// Input  : local - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CRotatingSystem::LocalToWorld(const Vector &local)
{
	Vector v;
	VectorTransform(local, m_fMatrix, v);
	return v;
}

//-----------------------------------------------------------------------------
// Purpose: Convert local RS coordinates into absolute world coordinates
// Input  : localx - 
//			localy - 
//			localz - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CRotatingSystem::LocalToWorld(float localx, float localy, float localz)
{
	Vector local;
	local[0] = localx;
	local[1] = localy;
	local[2] = localz;
	return LocalToWorld(local);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : timedelta - 
//			updatematrix - 
//-----------------------------------------------------------------------------
void CRotatingSystem::UpdateAngles(float timedelta, bool updatematrix)
{
	if (m_vecAnglesDelta != Vector(vec3_origin))
	{
		VectorMA(m_vecAngles, timedelta, m_vecAnglesDelta, m_vecAngles);
		if (updatematrix)
			UpdateAngleMatrix();
	}
}