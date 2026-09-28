#ifndef ROTATINGSYSTEM_H
#define ROTATINGSYSTEM_H

//-----------------------------------------------------------------------------
// A RenderSystem with rotation matrix, can rotate in any way
//-----------------------------------------------------------------------------
class CRotatingSystem : public CRenderSystem
{
public:
	CRotatingSystem(void);
	CRotatingSystem(const Vector &origin, const Vector &velocity, const Vector &angles, const Vector &anglesdelta, int sprindex, int r_mode, byte r, byte g, byte b, float a, float adelta, float scale, float scaledelta, float timetolive);
	virtual ~CRotatingSystem(void);

	virtual void ResetParameters(void);
	virtual void InitializeSystem(void);
	virtual void KillSystem(void);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

	void UpdateAngles(float timedelta, bool updatematrix/* = true*/);
	void UpdateAngleMatrix(void);
	Vector LocalToWorld(const Vector &local);
	Vector LocalToWorld(float localx, float localy, float localz);

	Vector m_vecAnglesDelta;

private:
	float m_fMatrix[3][4];
};

#endif // ROTATINGSYSTEM_H
