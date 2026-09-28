#ifndef PSBEAM_H
#define PSBEAM_H

//-----------------------------------------------------------------------------
// Beam of particles
//-----------------------------------------------------------------------------
class CPSBeam : public CParticleSystem
{
public:
	CPSBeam(void);
	CPSBeam(int maxParticles, const Vector &origin, const Vector &end, int sprindex, int r_mode, float timetolive);
	virtual ~CPSBeam(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void InitializeParticle(const int &index);

private:
	Vector m_vecEnd;
};

#endif // PSBEAM_H
