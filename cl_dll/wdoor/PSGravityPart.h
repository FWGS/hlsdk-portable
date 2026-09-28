#ifndef PSGravityPart_H
#define PSGravityPart_H

class PSGravityPart : public CParticleSystem
{
public:
	PSGravityPart(void);
	PSGravityPart(int maxParticles, float PartLife, float partvelocity, const Vector &origin, const Vector &direction, const Vector &spread, float scale, float scaledelta, byte r, byte g, byte b, float a, float adelta, int sprindex, bool animate, int frame, int r_mode, float PartEmitterLife);
	virtual ~PSGravityPart(void);

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

#endif // PSGravityPart_H
