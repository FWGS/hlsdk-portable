#ifndef HOSTAGE_LOCALNAV_H
#define HOSTAGE_LOCALNAV_H

class CHostage;

typedef int node_index_t;

typedef struct localnode_s
{
	Vector			vecLoc;
	int				offsetX;
	int				offsetY;
	byte			bDepth;
	BOOL			fSearched;
	node_index_t	nindexParent;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
} localnode_t;

class CLocalNav
{
public:
	CLocalNav( CHostage* owner );
	virtual ~CLocalNav();
	void SetTargetEnt( CBaseEntity* entity ) { m_pTargetEnt = entity ? entity->edict() : NULL; }
	node_index_t FindPath( Vector& source, Vector& destination, float targetRadius, int ignoreMonsters );
	int SetupPathNodes( node_index_t node, Vector* path, int ignoreMonsters );
	int GetFurthestTraversableNode( Vector& source, Vector* path, int count, int ignoreMonsters );
	int PathTraversable( Vector& source, Vector& destination, int ignoreMonsters );
	BOOL PathClear( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace );
	BOOL PathClear( Vector& source, Vector& destination, int ignoreMonsters )
	{
		TraceResult trace;
		return PathClear( source, destination, ignoreMonsters, trace );
	}
	static void Think();
	void RequestNav( CHostage* hostage );
	static void Reset();
	static float s_flStepSize;

private:
	static void HostagePrethink();
	node_index_t AddNode( node_index_t parent, Vector& location, int offsetX, int offsetY, byte depth );
	localnode_t* GetNode( node_index_t node );
	node_index_t NodeExists( int offsetX, int offsetY );
	void AddPathNodes( node_index_t node, int ignoreMonsters );
	void AddPathNode( node_index_t node, int offsetX, int offsetY, int ignoreMonsters );
	node_index_t GetBestNode( Vector& source, Vector& destination );
	BOOL SlopeTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace );
	BOOL LadderTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace );
	BOOL StepTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace );
	BOOL StepJumpable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace );
	node_index_t FindDirectPath( Vector& source, Vector& destination, float targetRadius, int ignoreMonsters );
	BOOL LadderHit( Vector& source, Vector& destination, TraceResult& trace );

	CHostage*		m_pOwner;
	edict_t*		m_pTargetEnt;
	BOOL			m_fTargetEntHit;
	localnode_t*	m_nodeArr;
	node_index_t	m_nindexAvailableNode;
	Vector			m_vecStartingLoc;

	static EHANDLE queue[20];
	static int qptr;
	static int tot_inqueue;
	static float nodeval;
	static float flNextCvarCheck;
	static float flLastThinkTime;
	static EHANDLE hostages[20];
	static int tot_hostages;
};

#endif
