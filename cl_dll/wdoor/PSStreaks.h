#ifndef PSSTREAKS_H
#define PSSTREAKS_H

//-----------------------------------------------------------------------------
// UNDONE Set of tracer particles, emitting from a point
//-----------------------------------------------------------------------------
class CPSStreaks : public CParticleSystem
{
public:
	CPSStreaks(void);
	CPSStreaks(int maxParticles, const Vector &origin, const Vector &direction, float speed, float velocitymin, float velocitymax, float scalex, float scaley, float scaledelta, color24 color, float a, float adelta, int sprindex, int r_mode, float timetolive);
	virtual ~CPSStreaks(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);
	virtual void InitializeParticle(const int &index);

protected:
	float m_fVelocityMin;
	float m_fVelocityMax;
	float m_fSpeed;
	bool m_bReversed;
};

#endif // PSSTREAKS_H
