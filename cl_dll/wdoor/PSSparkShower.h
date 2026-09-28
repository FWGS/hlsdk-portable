#ifndef PSSparkShower_H
#define PSSparkShower_H

class PSSparkShower : public CParticleSystem
{
public:
	PSSparkShower(void);
	PSSparkShower(int maxParticles, float PartLife, float partvelocity, int SparkEffect, const Vector &origin, const Vector &direction, const Vector &spread, int sprindex, float PartEmitterLife);
	virtual ~PSSparkShower(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	float m_fParticleVelocity;
	bool m_flRandomDir, m_fAnimatedSpr;
	float m_fVelocity;
	float m_fLife;
	int m_fSparkEffect;
};

#endif // PSSparkShower_H
