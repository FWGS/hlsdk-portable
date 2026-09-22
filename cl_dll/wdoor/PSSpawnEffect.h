#ifndef PSSPAWNEFFECT_H
#define PSSPAWNEFFECT_H

//-----------------------------------------------------------------------------
// Player spawn effect (spiral of particles)
//-----------------------------------------------------------------------------
class CPSSpawnEffect : public CParticleSystem
{
public:
	CPSSpawnEffect(void);
	CPSSpawnEffect(int maxParticles, const Vector &origin, float scale, float scaledelta, float radius, float radiusdelta, float velocity, int sprindex, int r_mode, byte r, byte g, byte b, float a, float adelta, float timetolive);
	virtual ~CPSSpawnEffect(void);

	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

protected:
	float m_fRadius;
	float m_fRadiusDelta;
};

#endif // PSSPAWNEFFECT_H
