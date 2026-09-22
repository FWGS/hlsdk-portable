#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSCylinder.h"
#include "triangleapi.h"
#include "studio_util.h"// M_PI

CRSCylinder::CRSCylinder(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSCylinder::~CRSCylinder(void)
{
	KillSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor.
// UNDONE: don't build mesh every render pass, precalculate its vertexes coords (into array) and then just scale them!
// Input  : origin - 
//			radius - 
//			radiusdelta - 
//			width - 
//			segments - 
//			sprindex - 
//			r_mode - 
//			r - 
//			g - 
//			b - 
//			a - 
//			adelta - 
//			timetolive - 
//-----------------------------------------------------------------------------
CRSCylinder::CRSCylinder(const Vector &origin, float radius, float radiusdelta, float width, unsigned short segments, int sprindex, int skin, int r_mode, unsigned char r, unsigned char g, unsigned char b, float a, float adelta, float timetolive)
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
	m_fScale = radius;
	m_fScaleDelta = radiusdelta;
	m_fWidth = width;
	m_usSegments = segments;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_iFrame = skin;

	if (m_fScale < 2.0f)
		m_fScale = 2.0f;

	if (m_usSegments < 6)
		m_usSegments = 6;

	m_iFollowEntity = -1;
	m_iRenderMode = r_mode;

	if (timetolive < 0)
		m_fDieTime = -1;
	else
		m_fDieTime = gEngfuncs.GetClientTime() + timetolive;

	InitializeSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Set default (external, public, non-system) values for all class variables.
//   Each derived class MUST call its ParentClass::ResetParameters()!
// DO NOT call any functions from here.
//-----------------------------------------------------------------------------
void CRSCylinder::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	m_fWidth = 0.0f;
	m_usSegments = 0;
}

void CRSCylinder::Render(void)
{
	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame);
	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	float h = m_fWidth/2.0f;
	float step = ((float)M_PI*2.0f)/m_usSegments;
	float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
	float v = 0.0f;
	float vs = 0.25f;

	// UNDONE: ^
	for (float a = 0.0f; a < M_PI*2.0f; a += step)
	{
		SinCos(a, &x1, &y1);
		x1 = x1*m_fScale + m_vecOrigin[0];
		y1 = y1*m_fScale + m_vecOrigin[1];

		SinCos(a + step, &x2, &y2);
		x2 = x2*m_fScale + m_vecOrigin[0];
		y2 = y2*m_fScale + m_vecOrigin[1];

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v);
		gEngfuncs.pTriAPI->Vertex3f(x1, y1, m_vecOrigin[2]+h);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x1, y1, m_vecOrigin[2]-h);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, v+vs);
		gEngfuncs.pTriAPI->Vertex3f(x2, y2, m_vecOrigin[2]-h);

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, v+vs);// exchange these to rotate by 90
		gEngfuncs.pTriAPI->Vertex3f(x2, y2, m_vecOrigin[2]+h);

		v += vs;
	}
	gEngfuncs.pTriAPI->End();
}
