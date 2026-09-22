#ifndef RSBEAMSTAR_H
#define RSBEAMSTAR_H

//-----------------------------------------------------------------------------
// A very nice "sunburst" effect
//-----------------------------------------------------------------------------
class CRSBeamStar : public CRenderSystem
{
public:
	CRSBeamStar(void);
	CRSBeamStar(const Vector &origin, int sprindex, int frame, unsigned short number, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive);
	virtual ~CRSBeamStar(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

protected:
	int m_iFrame;
	float *m_ang1;
	float *m_ang2;
	vec3_t *m_Coords;
	unsigned short m_iCount;
};

#endif // RSBEAMSTAR_H
