#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "hostage.h"
#include "hostage_localnav.h"

#define MAX_LOCAL_NODES	100
#define LOCAL_NODE_SPACING	26

float CLocalNav::s_flStepSize;
EHANDLE CLocalNav::queue[20];
int CLocalNav::qptr;
int CLocalNav::tot_inqueue;
float CLocalNav::nodeval;
float CLocalNav::flNextCvarCheck;
float CLocalNav::flLastThinkTime;
EHANDLE CLocalNav::hostages[20];
int CLocalNav::tot_hostages;

void CONSOLE_ECHO( const char* message, ... )
{
}

void CONSOLE_ECHO_LOGGED( const char* message, ... )
{
}

void UTIL_DrawBeamPoints( Vector start, Vector end, int lifetime, byte red, byte green, byte blue )
{
	static int sprite = PRECACHE_MODEL( "sprites/smoke.spr" );
	(void)sprite;
}

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CLocalNav::CLocalNav( CHostage* owner )
{
	m_pOwner = owner;
	m_pTargetEnt = NULL;
	m_nodeArr = new localnode_t[MAX_LOCAL_NODES];
	CONSOLE_ECHO( "Allocated m_nodeArr: %d nodes, %lu bytes\n", MAX_LOCAL_NODES, sizeof( localnode_t ) * MAX_LOCAL_NODES );
	s_flStepSize = CVAR_GET_FLOAT( "sv_stepsize" );
	if ( tot_hostages >= (int)ARRAYSIZE( hostages ) )
	{
		CONSOLE_ECHO( "queue full (creation)\n" );
		return;
	}
	hostages[tot_hostages++] = owner;
}

CLocalNav::~CLocalNav()
{
	delete[] m_nodeArr;
}

node_index_t CLocalNav::AddNode( node_index_t parent, Vector& location, int offsetX, int offsetY, byte depth )
{
	if ( m_nindexAvailableNode == MAX_LOCAL_NODES )
		return -1;
	localnode_t* node = &m_nodeArr[m_nindexAvailableNode];
	node->vecLoc = location;
	node->bDepth = depth;
	node->fSearched = FALSE;
	node->nindexParent = parent;
	node->offsetX = offsetX;
	node->offsetY = offsetY;
	return m_nindexAvailableNode++;
}

node_index_t CLocalNav::NodeExists( int offsetX, int offsetY )
{
	for ( node_index_t node = m_nindexAvailableNode - 1; node != -1; --node )
	{
		if ( m_nodeArr[node].offsetX == offsetX && m_nodeArr[node].offsetY == offsetY )
			return node;
	}
	return -1;
}

void CLocalNav::AddPathNodes( node_index_t node, int ignoreMonsters )
{
	AddPathNode( node, 1, 0, ignoreMonsters );
	AddPathNode( node, -1, 0, ignoreMonsters );
	AddPathNode( node, 0, 1, ignoreMonsters );
	AddPathNode( node, 0, -1, ignoreMonsters );
	AddPathNode( node, 1, 1, ignoreMonsters );
	AddPathNode( node, 1, -1, ignoreMonsters );
	AddPathNode( node, -1, 1, ignoreMonsters );
	AddPathNode( node, -1, -1, ignoreMonsters );
}

void CLocalNav::AddPathNode( node_index_t parent, int offsetX, int offsetY, int ignoreMonsters )
{
	Vector source;
	Vector destination;
	byte depth;
	if ( parent == -1 )
	{
		depth = 1;
		source = m_vecStartingLoc;
		destination = source + Vector( offsetX * LOCAL_NODE_SPACING, offsetY * LOCAL_NODE_SPACING, 0 );
	}
	else
	{
		localnode_t* parentNode = &m_nodeArr[parent];
		int absoluteX = parentNode->offsetX + offsetX;
		int absoluteY = parentNode->offsetY + offsetY;
		if ( NodeExists( absoluteX, absoluteY ) != -1 )
			return;
		source = parentNode->vecLoc;
		destination = source + Vector( offsetX * LOCAL_NODE_SPACING, offsetY * LOCAL_NODE_SPACING, 0 );
		for ( node_index_t node = m_nindexAvailableNode - 1; node != -1; --node )
		{
			localnode_t* candidate = &m_nodeArr[node];
			if ( abs( candidate->offsetX - absoluteX ) <= 1 && abs( candidate->offsetY - absoluteY ) <= 1 &&
				PathTraversable( candidate->vecLoc, destination, ignoreMonsters ) )
			{
				parent = node;
				parentNode = candidate;
			}
		}
		depth = (byte)( parentNode->bDepth + 1 );
		source = parentNode->vecLoc;
		offsetX = absoluteX;
		offsetY = absoluteY;
	}
	if ( PathTraversable( source, destination, ignoreMonsters ) )
		AddNode( parent, destination, offsetX, offsetY, depth );
}

node_index_t CLocalNav::GetBestNode( Vector& source, Vector& destination )
{
	float bestCost = 1000000;
	node_index_t bestNode = -1;
	for ( node_index_t index = 0; index < m_nindexAvailableNode; ++index )
	{
		localnode_t* node = &m_nodeArr[index];
		if ( node->fSearched )
			continue;
		float cost = node->bDepth * LOCAL_NODE_SPACING + ( node->vecLoc - destination ).Length();
		if ( !( fabs( node->vecLoc.z - destination.z ) <= s_flStepSize ) )
			cost *= 1.25f;
		if ( !( bestCost <= cost ) )
		{
			bestCost = cost;
			bestNode = index;
		}
	}
	return bestNode;
}

int CLocalNav::SetupPathNodes( node_index_t node, Vector* path, int ignoreMonsters )
{
	int count = 0;
	while ( node != -1 )
	{
		path[count++] = m_nodeArr[node].vecLoc;
		node = m_nodeArr[node].nindexParent;
	}
	return count;
}

int CLocalNav::GetFurthestTraversableNode( Vector& source, Vector* path, int count, int ignoreMonsters )
{
	for ( int node = 0; node < count; ++node )
	{
		if ( PathTraversable( source, path[node], ignoreMonsters ) )
			return node;
	}
	return -1;
}

void CLocalNav::Reset()
{
	flNextCvarCheck = 0;
	flLastThinkTime = 0;
	tot_inqueue = 0;
	nodeval = 0;
	qptr = 0;
	tot_hostages = 0;
}

void CLocalNav::HostagePrethink()
{
	for ( int index = 0; index < tot_hostages; ++index )
	{
		if ( hostages[index] )
			GetClassPtr( (CHostage*)hostages[index]->pev )->PreThink();
	}
}

void CLocalNav::Think()
{
	if ( gpGlobals->time >= flNextCvarCheck )
	{
		flNextCvarCheck = gpGlobals->time + 1.0;
		s_flStepSize = CVAR_GET_FLOAT( "sv_stepsize" );
	}
	float elapsed = gpGlobals->time - flLastThinkTime;
	flLastThinkTime = gpGlobals->time;
	HostagePrethink();
	nodeval -= elapsed * 250;
	if ( nodeval < 0 )
		nodeval = 0;
	else if ( nodeval > 50 )
		return;
	if ( !tot_inqueue )
		return;
	EHANDLE callback;
	callback = NULL;
	while ( TRUE )
	{
		callback = queue[qptr];
		if ( callback || tot_inqueue <= 0 )
			break;
		if ( ++qptr == ARRAYSIZE( queue ) )
			qptr = 0;
		if ( --tot_inqueue == 0 )
		{
			callback = NULL;
			break;
		}
	}
	if ( callback )
	{
		CHostage* hostage = GetClassPtr( (CHostage*)callback->pev );
		if ( ++qptr == ARRAYSIZE( queue ) )
			qptr = 0;
		--tot_inqueue;
		hostage->NavReady();
	}
}

void CLocalNav::RequestNav( CHostage* hostage )
{
	if ( !( nodeval > 50 ) && !tot_inqueue )
	{
		hostage->NavReady();
		return;
	}
	if ( tot_inqueue >= (int)ARRAYSIZE( queue ) )
	{
		CONSOLE_ECHO( "queue full\n" );
		return;
	}
	int slot = qptr;
	for ( int count = 0; count < tot_inqueue; ++count )
	{
		if ( GetClassPtr( (CHostage*)queue[slot]->pev ) == hostage )
			return;
		if ( ++slot == ARRAYSIZE( queue ) )
			slot = 0;
	}
	queue[slot] = hostage;
	++tot_inqueue;
}

BOOL CLocalNav::PathClear( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace )
{
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), source, destination, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction == 1 )
		return TRUE;
	if ( trace.pHit == m_pTargetEnt )
	{
		CONSOLE_ECHO( "target edict hit!\n" );
		m_fTargetEntHit = TRUE;
		return TRUE;
	}
	return FALSE;
}

BOOL CLocalNav::StepTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace )
{
	Vector start = source;
	start.z += s_flStepSize;
	Vector end( destination.x, destination.y, start.z );
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), start, end, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( !( ( trace.vecEndPos - start ).Length() >= 1 ) )
			return FALSE;
	}
	start = trace.vecEndPos;
	end = start;
	end.z -= s_flStepSize;
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), start, end, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		destination = start;
	else
	{
		if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		destination = trace.vecEndPos;
	}
	return TRUE;
}

BOOL CLocalNav::StepJumpable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace )
{
	float height = s_flStepSize + 1;
	Vector start = source;
	start.z += height;
	while ( height < 40 )
	{
		Vector end( destination.x, destination.y, start.z );
		g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), start, end, ignoreMonsters, m_pOwner->edict(), &trace );
		if ( trace.fStartSolid )
			return FALSE;
		if ( trace.flFraction == 1 )
			break;
		if ( trace.pHit == m_pTargetEnt )
		{
			m_fTargetEntHit = TRUE;
			break;
		}
		if ( ( trace.vecEndPos - start ).Length2D() >= 1 )
			break;
		height += 10;
		start.z += 10;
	}
	if ( !( height < 40 ) )
		return FALSE;
	start = trace.vecEndPos;
	Vector end = start;
	end.z -= s_flStepSize;
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), start, end, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		destination = start;
	else
	{
		if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		destination = trace.vecEndPos;
	}
	return TRUE;
}

BOOL CLocalNav::SlopeTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace )
{
	Vector end = destination;
	Vector normalAngles = UTIL_VecToAngles( trace.vecPlaneNormal );
	end.z = source.z + tan( ( 90 - normalAngles.x ) * M_PI / 180 ) * ( destination - source ).Length2D();
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), source, end, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( !( ( trace.vecEndPos - source ).Length2D() >= 1 ) )
			return FALSE;
	}
	end = trace.vecEndPos;
	Vector down = end;
	down.z -= s_flStepSize;
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), end, down, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		destination = end;
	else
	{
		if ( trace.flFraction != 1 && trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		destination = trace.vecEndPos;
	}
	return TRUE;
}

BOOL CLocalNav::LadderTraversable( Vector& source, Vector& destination, int ignoreMonsters, TraceResult& trace )
{
	Vector start = trace.vecEndPos;
	Vector end = start;
	end.z += LOCAL_NODE_SPACING;
	g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), start, end, ignoreMonsters, m_pOwner->edict(), &trace );
	if ( trace.fStartSolid )
		return FALSE;
	if ( trace.flFraction != 1 )
	{
		if ( trace.pHit == m_pTargetEnt )
			m_fTargetEntHit = TRUE;
		else if ( !( ( trace.vecEndPos - start ).Length() >= 1 ) )
			return FALSE;
	}
	start = trace.vecEndPos;
	destination.z = start.z;
	return PathTraversable( start, destination, ignoreMonsters );
}

int CLocalNav::PathTraversable( Vector& source, Vector& destination, int ignoreMonsters )
{
	Vector current = source;
	Vector direction = ( destination - source ).Normalize();
	direction.z = 0;
	float distance = ( destination - source ).Length2D();
	Vector next = source;
	int traversable = 0;
	BOOL continueTraversal = distance > 1;
	while ( continueTraversal )
	{
		if ( s_flStepSize <= distance )
			next = current + direction * s_flStepSize;
		else
			next = destination;
		m_fTargetEntHit = FALSE;
		TraceResult trace;
		g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), current, next, ignoreMonsters, m_pOwner->edict(), &trace );
		if ( trace.fStartSolid )
			return 0;
		if ( trace.flFraction == 1 || trace.pHit == m_pTargetEnt )
		{
			if ( trace.flFraction != 1 )
				m_fTargetEntHit = TRUE;
			next = trace.vecEndPos;
			if ( !traversable )
				traversable = 1;
		}
		else
		{
			if ( !ignoreMonsters && trace.pHit && FClassnameIs( &trace.pHit->v, "monster_hostage" ) )
				return 0;
			current = trace.vecEndPos;
			if ( trace.vecPlaneNormal.z > 0.7 )
			{
				if ( !SlopeTraversable( current, next, ignoreMonsters, trace ) )
					return 0;
				if ( !traversable )
					traversable = 1;
			}
			else if ( StepTraversable( current, next, ignoreMonsters, trace ) )
			{
				if ( !traversable )
					traversable = 2;
			}
			else
			{
				if ( !StepJumpable( current, next, ignoreMonsters, trace ) )
					return 0;
				if ( !traversable )
					traversable = 3;
			}
		}
		Vector down = next;
		down.z -= 300;
		g_engfuncs.pfnTraceMonsterHull( m_pOwner->edict(), next, down, ignoreMonsters, m_pOwner->edict(), &trace );
		if ( !trace.fStartSolid )
		{
			if ( trace.flFraction == 1 )
				return 0;
			if ( trace.pHit == m_pTargetEnt )
			{
				m_fTargetEntHit = TRUE;
				return 0;
			}
			next = trace.vecEndPos;
		}
		current = next;
		if ( m_fTargetEntHit )
			break;
		distance = ( destination - current ).Length2D();
		continueTraversal = !( distance <= 1 );
	}
	destination = next;
	return traversable;
}

BOOL CLocalNav::LadderHit( Vector& source, Vector& destination, TraceResult& trace )
{
	Vector angles = UTIL_VecToAngles( -trace.vecPlaneNormal );
	Vector forward, right, up;
	g_engfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector center = forward * 15 + trace.vecEndPos;
	if ( UTIL_PointContents( center + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center - up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center + right * 16 + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center - right * 16 + up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	if ( UTIL_PointContents( center + right * 16 - up * 36 ) == CONTENTS_LADDER )
		return TRUE;
	return UTIL_PointContents( center - right * 16 - up * 36 ) == CONTENTS_LADDER;
}

node_index_t CLocalNav::FindDirectPath( Vector& source, Vector& destination, float targetRadius, int ignoreMonsters )
{
	Vector direction = ( destination - source ).Normalize();
	Vector actualDestination = destination - direction * targetRadius;
	if ( !PathTraversable( source, actualDestination, ignoreMonsters ) )
		return -1;
	m_nindexAvailableNode = 0;
	Vector current = source;
	node_index_t parent = -1;
	while ( !( ( current - actualDestination ).Length2D() < LOCAL_NODE_SPACING ) )
	{
		if ( m_nindexAvailableNode == MAX_LOCAL_NODES )
			break;
		Vector next = current + direction * LOCAL_NODE_SPACING;
		node_index_t node = AddNode( parent, next, 0, 0, 0 );
		if ( node == -1 )
			break;
		current = next;
		parent = node;
	}
	return parent;
}

node_index_t CLocalNav::FindPath( Vector& source, Vector& destination, float targetRadius, int ignoreMonsters )
{
	CONSOLE_ECHO( "findpath: %f\n", gpGlobals->time );
	node_index_t result = FindDirectPath( source, destination, targetRadius, ignoreMonsters );
	if ( result != -1 )
		return result;
	m_vecStartingLoc = source;
	m_nindexAvailableNode = 0;
	AddPathNodes( -1, ignoreMonsters );
	node_index_t node = GetBestNode( source, destination );
	while ( node != -1 )
	{
		localnode_t* current = &m_nodeArr[node];
		current->fSearched = TRUE;
		float distance = ( destination - current->vecLoc ).Length2D();
		if ( targetRadius >= distance || distance <= LOCAL_NODE_SPACING )
		{
			result = node;
			break;
		}
		if ( distance - targetRadius > ( MAX_LOCAL_NODES - m_nindexAvailableNode ) * LOCAL_NODE_SPACING )
			break;
		if ( m_nindexAvailableNode == MAX_LOCAL_NODES )
			break;
		AddPathNodes( node, ignoreMonsters );
		node = GetBestNode( source, destination );
	}
	if ( m_nindexAvailableNode <= 10 )
		nodeval += 2;
	else if ( m_nindexAvailableNode <= 20 )
		nodeval += 4;
	else if ( m_nindexAvailableNode <= 30 )
		nodeval += 8;
	else if ( m_nindexAvailableNode <= 40 )
		nodeval += 13;
	else if ( m_nindexAvailableNode <= 50 )
		nodeval += 19;
	else if ( m_nindexAvailableNode <= 60 )
		nodeval += 26;
	else if ( m_nindexAvailableNode <= 70 )
		nodeval += 34;
	else if ( m_nindexAvailableNode <= 80 )
		nodeval += 43;
	else if ( m_nindexAvailableNode <= 90 )
		nodeval += 53;
	else if ( m_nindexAvailableNode <= 100 )
		nodeval += 64;
	else if ( m_nindexAvailableNode <= 110 )
		nodeval += 76;
	else if ( m_nindexAvailableNode <= 120 )
		nodeval += 89;
	else if ( m_nindexAvailableNode <= 130 )
		nodeval += 103;
	else if ( m_nindexAvailableNode <= 140 )
		nodeval += 118;
	else if ( m_nindexAvailableNode <= 150 )
		nodeval += 134;
	else if ( m_nindexAvailableNode <= 160 )
		nodeval += 151;
	else
		nodeval += 169;
	return result;
}
