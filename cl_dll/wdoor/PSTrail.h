#ifndef PSTrail_H
#define PSTrail_H

class CPSTrail : public CParticleSystem
{
public:
	CPSTrail(void);

	CPSTrail(int maxParticles, float MinPartLife, float MaxPartLife, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float size, float sizedelta, byte r, byte g, byte b, float a, float adelta, int sprindex, bool animate, int frame, int r_mode, float PartEmitterLife);
	virtual ~CPSTrail(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	float m_fParticleVelocity;
	bool m_flRandomDir, m_fAnimatedSpr;
	float m_fVelocity;
	float m_fMinLife, m_fMaxLife;
};

#endif // PSTrail_H
