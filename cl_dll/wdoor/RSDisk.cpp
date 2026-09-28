#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RotatingSystem.h"
#include "RSDisk.h"
#include "triangleapi.h"
#include "studio_util.h"// M_PI

CRSDisk::CRSDisk(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSDisk::~CRSDisk(void)
{
	KillSystem();
}

CRSDisk::CRSDisk(const Vector &origin, const Vector &angles, const Vector &anglesdelta, float radius, float radiusdelta, unsigned short segments, int sprindex, int r_mode, unsigned char r, unsigned char g, unsigned char b, float a, float adelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	m_pTexture = NULL;// MUST be before InitTexture()
	if (!InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	VectorCopy(origin, m_vecOrigin);
	VectorCopy(angles, m_vecAngles);
	VectorCopy(anglesdelta, m_vecAnglesDelta);
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	m_usSegments = segments;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;

	if (m_fScale < 2.0f)
		m_fScale = 2.0f;

	if (m_usSegments < 6)
		m_usSegments = 6;

	m_iFollowEntity = -1;

	// precalculations
	m_fAngleDelta = ((float)M_PI*2.0f/m_usSegments);// angle step
	m_fTexDelta = 1.0f/m_usSegments;// texture vertical step

	m_iRenderMode = r_mode;
	if (timetolive <= 0)
		m_fDieTime = timetolive;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CRSDisk::ResetParameters(void)
{
	CRotatingSystem::ResetParameters();
	m_fAngleDelta = 0.0f;
	m_fTexDelta = 0.0f;
	m_usSegments = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRSDisk::Render(void)
{
	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);

	float a = 0.0f, s = 0.0f, c = 0.0f;
	float v = 0.0f;

	gEngfuncs.pTriAPI->TexCoord2f(0.0f, v);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);

	Vector p;// current vertex
	p[2] = 0;
//	p[2] = m_vecOrigin[2];
	unsigned short i = 0;
	for (i = 0; i <= m_usSegments; ++i)// repeat first vertex twice to finish the disk
	{
		SinCos(a, &s, &c);
//		p[0] = s*m_fScale + m_vecOrigin[0];
//		p[1] = c*m_fScale + m_vecOrigin[1];
		p[0] = s*m_fScale;
		p[1] = c*m_fScale;

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v);
		gEngfuncs.pTriAPI->Vertex3fv(LocalToWorld(p));
		a += m_fAngleDelta;
		v += m_fTexDelta;
	}
	gEngfuncs.pTriAPI->End();
//	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
