#include "hud.h"
#include "cl_util.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "RSBeamStar.h"
#include "triangleapi.h"
#include "studio_util.h"// M_PI

CRSBeamStar::CRSBeamStar(void)
{
	// Calling constructors directly is forbidden!
	ResetParameters();
}

CRSBeamStar::~CRSBeamStar(void)
{
	// If, eventually, some class would be derived from this one, these lines should be moved to local KillSystem()
	if (m_Coords)
	{
		delete [] m_Coords;
		m_Coords = NULL;
	}
	if (m_ang1)
	{
		delete [] m_ang1;
		m_ang1 = NULL;
	}
	if (m_ang2)
	{
		delete [] m_ang2;
		m_ang2 = NULL;
	}
	KillSystem();
}

CRSBeamStar::CRSBeamStar(const Vector &origin, int sprindex, int frame, unsigned short number, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive)
{
	index = 0;// the only good place for this
	removenow = false;
	ResetParameters();
	m_pTexture = NULL;// MUST be before InitTexture()
	if (number <= 0 || !InitTexture(sprindex))
	{
		removenow = true;
		return;
	}
	m_iCount = number;
	VectorCopy(origin, m_vecOrigin);
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

	m_Coords = new vec3_t[m_iCount];
	m_ang1 = new float[m_iCount];
	m_ang2 = new float[m_iCount];

	int i;
	for (i=0; i<m_iCount; ++i)
	{
		m_ang1[i] = gEngfuncs.pfnRandomFloat(-M_PI, M_PI);
		m_ang2[i] = gEngfuncs.pfnRandomFloat(-M_PI, M_PI);
		VectorClear(m_Coords[i]);
	}

	if (timetolive <= 0)// if 0, just display all frames
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
void CRSBeamStar::ResetParameters(void)
{
	CRenderSystem::ResetParameters();
	m_Coords = NULL;
	m_ang1 = NULL;
	m_ang2 = NULL;
	m_iCount = 0;
}

bool CRSBeamStar::Update(const float &time, const double &elapsedTime)
{
	if (CRenderSystem::Update(time, elapsedTime) == 0)
	{
		if (m_fScale <= 0.000001 && m_fScaleDelta < 0.0f)// special fix for dark teleporter effect
		{
			dying = true;
		}
		else
		{
			int i;
			float s1, s2, c1, c2;
			for (i=0; i<m_iCount; ++i)
			{
				SinCos(time + m_ang1[i], &s1, &c1);
				SinCos(time + m_ang2[i], &s2, &c2);
				m_Coords[i][0] = c1*c2*m_fScale;
				m_Coords[i][1] = s1*c2*m_fScale;
				m_Coords[i][2] = -s2*m_fScale;
			}
		}
	}
	return dying;
}

void CRSBeamStar::Render(void)
{
	if (!m_pTexture)
		return;

	if (!gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, (int)m_fFrame))
		return;

	gEngfuncs.GetViewAngles(m_vecAngles);
	VectorAdd(m_vecAngles, g_ev_punchangle, m_vecAngles);

	vec3_t v_forward;
	AngleVectors(m_vecAngles, v_forward, NULL, NULL);

	gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame);
	gEngfuncs.pTriAPI->RenderMode(m_iRenderMode);
	gEngfuncs.pTriAPI->Color4ub(m_color.r, m_color.g, m_color.b, (unsigned char)(m_fBrightness*255.0f));
	gEngfuncs.pTriAPI->Brightness(m_fBrightness);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	if (m_iFlags & RENDERSYSTEM_FLAG_SIMULTANEOUS)//Special for BeamStar - another draw mode!
		gEngfuncs.pTriAPI->Begin(TRI_LINES);
	else
		gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);

	vec3_t pos, cross;
	int i;
	for (i=0; i<m_iCount; ++i)
	{
		CrossProduct(m_Coords[i], v_forward, cross);
		VectorAdd(m_vecOrigin, m_Coords[i], pos);

		gEngfuncs.pTriAPI->TexCoord2f(0.5f, 1.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vecOrigin);

		gEngfuncs.pTriAPI->TexCoord2f(0.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pos + cross);

		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(pos - cross);
	}

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}
