#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

class CRenderSystem;

class CRenderManager
{
public:
	CRenderManager();
	virtual ~CRenderManager();

	int AddSystem(CRenderSystem *pSystem, int flags = 0, int followentindex = -1, int followflags = 0/*, float delay = 0.0f*/);
	void DeleteSystem(CRenderSystem *pSystem);
	void DeleteAllSystems(void);
	void Update(const float &time, const double &elapsedTime);
	void RenderOpaque(void);
	void RenderTransparent(void);
//	void Render(void);
	void CreateEntities(void);
	CRenderSystem *FindSystemByFollowEntity(int entindex);
	CRenderSystem *FindSystem(unsigned int index);
	void ApplyForce(const Vector &origin, const Vector &force, float radius, bool point);
	void DumpSystems(void);
	unsigned int GetFirstFreeRSUID(void);
//protected:
//	void CheckDelayedSystems(const float &time);

private:
	CRenderSystem *m_pFirstSystem;// RS list. NEVER reorder it manually!
};

extern CRenderManager *g_pRenderManager;

#endif // RENDERMANAGER_H
