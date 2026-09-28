#ifndef PSBubbles_H
#define PSBubbles_H

class CPSBubbles : public CParticleSystem
{
public:
	CPSBubbles(void);
	CPSBubbles(int maxParticles, const Vector &origin, const Vector &direction, const Vector &spread, float partvelocity, float size, int sprindex, int frame, float PartEmitterLife);
	virtual ~CPSBubbles(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	float m_fParticleVelocity;
	bool m_flRandomDir;
	float m_fVelocity;
};

#endif // PSBubbles_H
