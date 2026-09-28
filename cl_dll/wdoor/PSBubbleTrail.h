#ifndef PSBubbleTrail_H
#define PSBubbleTrail_H

class CPSBubbleTrail : public CParticleSystem
{
public:
	CPSBubbleTrail(void);
	CPSBubbleTrail(const Vector &start, const Vector &end, int sprindex, int frame, float scale, float dist_delta, float timetolive);
	virtual ~CPSBubbleTrail(void);

	virtual void ResetParameters(void);
	virtual void InitializeParticle(const int &index);
	virtual bool Update(const float &time, const double &elapsedTime);
	virtual void Render(void);

protected:
	Vector m_vecStart;
	Vector m_vecDelta;
};

#endif // PSBubbleTrail_H
