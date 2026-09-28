#ifndef RSBEAM_H
#define RSBEAM_H


//-----------------------------------------------------------------------------
// Simple beam made of two crossed quads (like those in Unreal)
//-----------------------------------------------------------------------------
class CRSBeam : public CRenderSystem
{
public:
	CRSBeam(void);
	CRSBeam(const Vector &start, const Vector &end, int frame, int sprindex, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive);
	virtual ~CRSBeam(void);

	virtual void ResetParameters(void);
	virtual void Render(void);

	int m_iFrame;
	Vector	m_vecEnd;
	float	m_fTextureTile;
};

#endif // RSBEAM_H
