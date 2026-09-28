#ifndef PSSPARKS_H
#define PSSPARKS_H

//-----------------------------------------------------------------------------
// Set of particles, emitting from a point
//-----------------------------------------------------------------------------
class CPSSparks : public CParticleSystem
{
public:
	CPSSparks(void);
	CPSSparks(int maxParticles, const Vector &origin, float scalex, float scaley, float velocity, float partlife, int sprindex, float timetolive);
	virtual ~CPSSparks(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);
	virtual void InitializeParticle(const int &index);

protected:
	float m_fVelocity;
	bool m_bReversed;
};

#endif // PSSPARKS_H
