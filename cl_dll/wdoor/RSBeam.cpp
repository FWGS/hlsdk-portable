#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeam.h"
#include "triangleapi.h"

CRSBeam::CRSBeam(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSBeam::~CRSBeam(void)
{
	KillSystem();
}

CRSBeam::CRSBeam(const Vector &start, const Vector &end, int sprindex, int frame, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
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
	VectorCopy(start, m_vecOrigin);
	VectorCopy(end, m_vecEnd);
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
	m_fBrightness = a;
	m_fBrightnessDelta = adelta;
	m_fScale = scale;
	m_fScaleDelta = scaledelta;
	m_iFollowEntity = -1;
	m_iRenderMode = r_mode;
	m_iFrame = frame;

	m_fTextureTile = 1.0f;// For texture tiling. May (and will) screw the whole effect! (because of valve's strange GL setup?)

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
void CRSBeam::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	VectorClear(m_vecEnd);
	m_fTextureTile = 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRSBeam::Render(void)
{
	if (!InitTexture(texindex))
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	Vector rt, up;
	AngleVectors(m_vecAngles, NULL, rt, up);

	gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame);
	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);
	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(0.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecEnd - rt*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(0.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecEnd + rt*m_fScale);

	gEngfuncs.pTriAPI->TexCoord2f(0.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin + up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, 0.0);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin - up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(1.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecEnd - up*m_fScale);
	gEngfuncs.pTriAPI->TexCoord2f(0.0, m_fTextureTile);
	gEngfuncs.pTriAPI->Vertex3fv(m_vecEnd + up*m_fScale);

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
