#ifndef PSBlood_H
#define PSBlood_H

class CPSBlood : public CParticleSystem
{
public:
	CPSBlood(void);
	CPSBlood(int maxParticles, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float size, int sprindex, int frame, float PartEmitterLife,int color);
	virtual ~CPSBlood(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	float m_fParticleVelocity;
	bool m_flRandomDir;
	float m_fVelocity;
};

#endif // PSBlood_H
