#ifndef RSDISK_H
#define RSDISK_H

//-----------------------------------------------------------------------------
// Renders disk, replacement for TE_BEAMDISK
//-----------------------------------------------------------------------------
class CRSDisk : public CRotatingSystem
{
public:
	CRSDisk(void);
	CRSDisk(const Vector &origin, const Vector &angles, const Vector &anglesdelta, float radius, float radiusdelta, unsigned short segments, int sprindex, int r_mode, unsigned char r, unsigned char g, unsigned char b, float a, float adelta, float timetolive);
	virtual ~CRSDisk(void);

	virtual void ResetParameters(void);
	virtual void Render(void);

private:
	float m_fAngleDelta;
	float m_fTexDelta;
	unsigned short m_usSegments;
};

#endif // RSDISK_H
