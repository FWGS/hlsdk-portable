#ifndef PSFLATTRAIL_H
#define PSFLATTRAIL_H

//-----------------------------------------------------------------------------
// Trail made of world-oriented (non-rotating) particles
//-----------------------------------------------------------------------------
class CPSFlatTrail : public CParticleSystem
{
public:
	CPSFlatTrail(void);
	CPSFlatTrail(const Vector &start, const Vector &end, int sprindex, int frame, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float dist_delta, float timetolive);
	virtual ~CPSFlatTrail(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

protected:
	Vector m_vecStart;
	Vector m_vecDelta;
};

#endif // PSFLATTRAIL_H
