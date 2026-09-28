#ifndef RSCYLINDER_H
#define RSCYLINDER_H

//-----------------------------------------------------------------------------
// Renders cylinder, replacement for TE_BEAMCYLINDER
//-----------------------------------------------------------------------------
class CRSCylinder : public CRenderSystem
{
public:
	CRSCylinder(void);
	CRSCylinder(const Vector &origin, float radius, float radiusdelta, float width, unsigned short segments, int sprindex, int skin, int r_mode, unsigned char r, unsigned char g, unsigned char b, float a, float adelta, float timetolive);
	virtual ~CRSCylinder(void);

	virtual void ResetParameters(void);
	virtual void Render(void);

private:
	float m_fWidth;
	unsigned short m_usSegments;
};

#endif // RSCYLINDER_H
