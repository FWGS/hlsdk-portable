#ifndef PSBlastCone_H
#define PSBlastCone_H

class CPSBlastCone : public CParticleSystem
{
public:
	CPSBlastCone(void);

	CPSBlastCone(int maxParticles, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float size, float sizedelta, byte r, byte g, byte b, float a, float adelta, int sprindex, bool animate, int frame, int r_mode, float PartEmitterLife);
	virtual ~CPSBlastCone(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	float m_fParticleVelocity;
	bool m_flRandomDir, m_fAnimatedSpr;
	float m_fVelocity;
	float m_fLife;
};

#endif // PSBlastCone_H
