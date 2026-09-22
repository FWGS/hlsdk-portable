//====================================================================
//
// Purpose: Render System: particles.
// This code affects performance thousands times, optimization is critical!
//
//====================================================================

#include "hud.h"
#include "cl_util.h"
#include "Particle.h"
#include "triangleapi.h"
#ifndef COM_MODEL_H
#include "com_model.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: default constructor, no need for others
//-----------------------------------------------------------------------------
CParticle::CParticle()
{
//NO! this destroys function pointers!	memset(this,0,sizeof(CParticle));
	VectorClear(m_vPos);
	VectorClear(m_vPosPrev);
	VectorClear(m_vVel);
	VectorClear(m_vAccel);
	m_fEnergy = 0.0f;
	m_fSizeX = 1.0f;
	m_fSizeY = 1.0f;
	m_fSizeDelta = 0.0f;
	m_fColor[0] = 1.0f;
	m_fColor[1] = 1.0f;
	m_fColor[2] = 1.0f;
	m_fColor[3] = 1.0f;
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fColorDelta[3] = 0.0f;
	m_iFlags = 0;
	m_iFrame = 0;
	m_pTexture = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CParticle::~CParticle()
{
#ifdef _DEBUG
	// memset?
	VectorClear(m_vPos);
/*	VectorClear(m_vPosPrev);
	VectorClear(m_vVel);
	VectorClear(m_vVelAdd);
	VectorClear(m_vAccel);
//	VectorClear(m_vAngles);
	m_fEnergy = 0.0;
	m_fSizeX = 0.0;
	m_fSizeY = 0.0;
	m_fSizeDelta = 0.0;
//	m_weight = 0.0;
//	m_weightDelta = 0.0;
	m_iFrame = 0;*/
	m_iFlags = 0;
	m_pTexture = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: update color and brightness depending on m_fSizeDelta
//-----------------------------------------------------------------------------
void CParticle::UpdateColor(const float &elapsed_time)
{
/*	m_fColor[0] += m_fColorDelta[0] * elapsed_time;
	m_fColor[1] += m_fColorDelta[1] * elapsed_time;
	m_fColor[2] += m_fColorDelta[2] * elapsed_time;
	m_fColor[3] += m_fColorDelta[3] * elapsed_time;*/
	m_fColor[0] = Q_max(0.0f, m_fColor[0] + m_fColorDelta[0] * elapsed_time);// clamp for software renderer
	m_fColor[1] = Q_max(0.0f, m_fColor[1] + m_fColorDelta[1] * elapsed_time);
	m_fColor[2] = Q_max(0.0f, m_fColor[2] + m_fColorDelta[2] * elapsed_time);
	m_fColor[3] = Q_max(0.0f, m_fColor[3] + m_fColorDelta[3] * elapsed_time);
}

//-----------------------------------------------------------------------------
// Purpose: update size depending on m_fSizeDelta
//-----------------------------------------------------------------------------
void CParticle::UpdateSize(const float &elapsed_time)
{
	m_fSizeX += m_fSizeDelta * elapsed_time;
	m_fSizeY += m_fSizeDelta * elapsed_time;
}

//-----------------------------------------------------------------------------
// Purpose: carefully update energy depending on m_fColor[3]
//-----------------------------------------------------------------------------
void CParticle::UpdateEnergyByBrightness(void)
{
	if (m_fColorDelta[3] < 0.0f)
		m_fEnergy = m_fColor[3];// fade out: erase when become invisible
	else
		m_fEnergy = Q_max(0.0f, 1.0f-m_fColor[3]);// fade in: erase when fully visible?
}

//-----------------------------------------------------------------------------
// Purpose: Default particle render procedure. Basically, a particle gets drawn
//  as a sprite.
// Input  : &rt - AngleVectors() outputs - right and
//			&up - up
//			rendermode - kRenderTransAdd
//			doubleside - draw both front and back sides (useful for faces not
//						aligned parallel to the screen
//-----------------------------------------------------------------------------
void CParticle::Render(const Vector &rt, const Vector &up, const int &rendermode, const bool &doubleside)
{
	if (m_pTexture == NULL)
		return;

	if (gEngfuncs.pTriAPI->SpriteTexture(m_pTexture, m_iFrame))
	{
		gEngfuncs.pTriAPI->RenderMode(rendermode);
		gEngfuncs.pTriAPI->CullFace(doubleside == false?TRI_FRONT:TRI_NONE);// TRI_NONE - two-sided
		gEngfuncs.pTriAPI->Begin(TRI_QUADS);
		gEngfuncs.pTriAPI->Color4f(m_fColor[0], m_fColor[1], m_fColor[2], m_fColor[3]);

		if (rendermode != kRenderTransAlpha)// sprites like smoke need special care // TODO: revisit
			gEngfuncs.pTriAPI->Brightness(m_fColor[3]);

		// really stupid thing: when drawing with proper coords order, face is reversed
		gEngfuncs.pTriAPI->TexCoord2f(1.0f, 0.0f);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rt * m_fSizeX + up * m_fSizeY);
		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos + rt * m_fSizeX - up * m_fSizeY);
		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rt * m_fSizeX - up * m_fSizeY);
		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		gEngfuncs.pTriAPI->Vertex3fv(m_vPos - rt * m_fSizeX + up * m_fSizeY);
		gEngfuncs.pTriAPI->End();
	}
}

//-----------------------------------------------------------------------------
// Purpose: frame++
//-----------------------------------------------------------------------------
void CParticle::FrameIncrease(void)
{
	if (m_pTexture == NULL || m_pTexture->numframes <= 1)
		return;

	if (m_iFrame < m_pTexture->numframes - 1)
		++m_iFrame;
	else
		m_iFrame = 0;//-= maxframes;
}

//-----------------------------------------------------------------------------
// Purpose: set current frame to random possible value
//-----------------------------------------------------------------------------
void CParticle::FrameRandomize(void)
{
	if (m_pTexture == NULL || m_pTexture->numframes <= 1)
		return;

	m_iFrame = gEngfuncs.pfnRandomLong(0, m_pTexture->numframes - 1);
}

//-----------------------------------------------------------------------------
// Purpose: an easy way to reset color
//-----------------------------------------------------------------------------
void CParticle::SetDefaultColor(void)
{
	m_fColor[0] = 1.0f;
	m_fColor[1] = 1.0f;
	m_fColor[2] = 1.0f;
	m_fColor[3] = 1.0f;
	m_fColorDelta[0] = 0.0f;
	m_fColorDelta[1] = 0.0f;
	m_fColorDelta[2] = 0.0f;
	m_fColorDelta[3] = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Set render color
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColor(const color24 &rgb, const float &a)
{
	m_fColor[0] = (float)rgb.r/255.0f;
	m_fColor[1] = (float)rgb.g/255.0f;
	m_fColor[2] = (float)rgb.b/255.0f;
	m_fColor[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : &rgb - bytes (0-255)
//			a - byte (0-255)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(const color24 &rgb, const float &a)
{
	m_fColorDelta[0] = (float)rgb.r/255.0f;
	m_fColorDelta[1] = (float)rgb.g/255.0f;
	m_fColorDelta[2] = (float)rgb.b/255.0f;
	m_fColorDelta[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Color will be updated with  += m_fColorDelta * frametime
// Input  : *rgb - floats (0-1)
//			a - float (0-1)
//-----------------------------------------------------------------------------
void CParticle::SetColorDelta(float *rgb, const float &a)
{
	m_fColorDelta[0] = rgb[0];
	m_fColorDelta[1] = rgb[1];
	m_fColorDelta[2] = rgb[2];
	m_fColorDelta[3] = a;
}

//-----------------------------------------------------------------------------
// Purpose: Used to display sprite using its texture size, so the bitmap always has 1:1 scale
// Input  : multipl_x - post-multipliers
//			multipl_y - 
//-----------------------------------------------------------------------------
void CParticle::SetSizeFromTexture(const float &multipl_x, const float &multipl_y)
{
	if (m_pTexture)
	{
		m_fSizeX = (m_pTexture->maxs[1] - m_pTexture->mins[1])*multipl_x;
		m_fSizeY = (m_pTexture->maxs[2] - m_pTexture->mins[2])*multipl_y;
	}
}
