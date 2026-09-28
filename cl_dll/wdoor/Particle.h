#ifndef PARTICLE_H
#define PARTICLE_H
/*
#include <stdio.h>
#include <stdlib.h>

#ifndef COM_MODEL_H
#include "com_model.h"
#endif
*/
//#include "color.h"

typedef struct model_s model_t;

// custom flags for systems to use
#define PARTICLE_FLAG1		1
#define PARTICLE_FLAG2		2
#define PARTICLE_FLAG3		4
#define PARTICLE_FLAG4		8
#define PARTICLE_FLAG5		16
#define PARTICLE_FLAG6		32
#define PARTICLE_FLAG7		64
#define PARTICLE_FLAG8		128

class CParticle
{
public:
	CParticle();
	virtual ~CParticle();

	virtual void UpdateColor(const float &elapsed_time);
	virtual void UpdateSize(const float &elapsed_time);
	virtual void Render(const Vector &rt, const Vector &up, const int &rendermode, const bool &doubleside = false);
	void UpdateEnergyByBrightness(void);
	void FrameIncrease(void);
	void FrameRandomize(void);
	void SetDefaultColor(void);
	void SetColor(const color24 &rgb, const float &a);
	void SetColorDelta(const color24 &rgb, const float &a);
	void SetColorDelta(float *rgb, const float &a);
	void SetSizeFromTexture(const float &multipl_x = 1.0f, const float &multipl_y = 1.0f);

	Vector m_vPos;// current position
	Vector m_vPosPrev;// previous position
	Vector m_vVel;// velocity
	Vector m_vAccel;// acceleration (gravity, etc.)
//	Vector m_vAngles;?

	float m_fEnergy;// should change from 1 to 0 during particle lifetime (linear)  // do we really need this?
	float m_fSizeX;// particle size in world units
	float m_fSizeY;
	float m_fSizeDelta;
//	float m_fWeight;// particle weight, modifies acceleration (default is 1.0)
//	float m_fWeightDelta;
	float m_fColor[4];// color in RGBA (0...1) format
	float m_fColorDelta[4];

	unsigned int m_iFlags;
	int m_iFrame;

//protected:
	model_t *m_pTexture;
//	CParticleSystem *m_pContainingSystem;// particle system I am member of
//	void *pData;// some user data?
};

#endif // PARTICLE_H
