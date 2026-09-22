#ifndef RSLIGHT_H
#define RSLIGHT_H

#include "dlight.h"

#define LIGHT_INDEX_TE_RSLIGHT 0// 0 allows multiple dlight instances

//-----------------------------------------------------------------------------
// Controls a dynamic light, makes it possible to move, change color, etc.
//-----------------------------------------------------------------------------
class CRSLight : public CRenderSystem
{
public:
	CRSLight(void);
	CRSLight(const Vector &origin, byte r, byte g, byte b, float radius, float timetolive, int type, int followentity = -1);
	virtual ~CRSLight(void);

	virtual void ResetParameters(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

private:
	vec3_t m_fBaseOrigin, dir;
	dlight_t *m_pLight;
	int FX_Type;
	float m_flTimeUpdate;
};

#endif // RSLIGHT_H
