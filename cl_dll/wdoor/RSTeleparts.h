#ifndef RSTELEPARTS_H
#define RSTELEPARTS_H

#include "particledef.h"
#include "dlight.h"

#define NUMVERTEXNORMALS	162

static const float vdirs[NUMVERTEXNORMALS][3] =
{
#include "anorms.h"
};

//-----------------------------------------------------------------------------
// Controls particles created by the engine, used by displacer's projectile
//-----------------------------------------------------------------------------
class CRSTeleparts : public CRenderSystem
{
public:
	CRSTeleparts(void);
	CRSTeleparts(const Vector &origin, float radius, byte color, byte type, float timetolive, int followentity, byte r, byte g, byte b);
	virtual ~CRSTeleparts(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

private:
	dlight_t *m_pLight;
	particle_t *m_pParticles[NUMVERTEXNORMALS];

	float m_vecAng[NUMVERTEXNORMALS][2];//3 for vectors

	byte m_colorpal;
	int m_iType;
};

#endif // RSTELEPARTS_H
