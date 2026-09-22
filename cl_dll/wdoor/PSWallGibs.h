#ifndef PSWallGibs_H
#define PSWallGibs_H

class PSWallGibs : public CParticleSystem
{
public:
	PSWallGibs(void);
	PSWallGibs(int maxParticles, float life, float partvelocity, float PartMass, const Vector &origin, const Vector &direction, const Vector &spread, float scale, byte r, byte g, byte b, int sprindex, int Sframe, int Eframe, float PartEmitterLife);
	virtual ~PSWallGibs(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);

protected:
	Vector m_vecSpread;
	bool m_flRandomDir;
	float m_fVelocity;
	float m_fParticleVelocity;
	float m_fMass;
	int m_iSFrame, m_iEFrame;
	int m_fLife;
	int m_Bounce;
};

#endif // PSWallGibs_H
