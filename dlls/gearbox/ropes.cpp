/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "player.h"
#include "weapons.h"
#include "decals.h"
#include "gamerules.h"
#include "effects.h"
#include "saverestore.h"

#include "ropes.h"

#define HOOK_CONSTANT	2500.0f
#define SPRING_DAMPING	0.1f
#define ROPE_IGNORE_SAMPLES	4		// integrator may be hanging if less than

/**
*	Data for a single rope joint.
*/
struct RopeSampleData
{
	Vector mPosition;
	Vector mVelocity;
	Vector mForce;
	Vector mExternalForce;

	bool mApplyExternalForce;
	float mMassReciprocal;
	float restLength;
};

#define MAX_LIST_SEGMENTS	5
RopeSampleData g_pTempList[MAX_LIST_SEGMENTS][MAX_SEGMENTS];

/**
*	Represents a single joint in a rope. There are numSegments + 1 samples in a rope.
*/
class CRopeSample : public CBaseEntity
{
public:

	void Spawn();
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


	static CRopeSample* CreateSample();

	RopeSampleData& GetData()
	{
		return data;
	}

	RopeSampleData& GetSourceData() {
		if (swapped)
			return data2;
		else
			return data;
	}
	RopeSampleData& GetTargetData() {
		if (swapped)
			return data;
		else
			return data2;
	}

	void Swap()
	{
		swapped = !swapped;
	}

	void ResetSwap()
	{
		swapped = FALSE;
	}

private:
	RopeSampleData data;
	RopeSampleData data2;
	BOOL swapped;
};


class CRopeSegment : public CBaseAnimating
{
public:

	void Precache();

	void Spawn();

	void Touch( CBaseEntity* pOther );

	void SetAbsOrigin(const Vector& pos)
	{
		pev->origin = pos;
	}

	static CRopeSegment* CreateSegment(CRopeSample* pSample, string_t iszModelName , CRope *rope);

	CRopeSample* GetSample() { return m_Sample; }

	void ApplyExternalForce( const Vector& vecForce );

	void SetCauseDamageOnTouch( const bool bCauseDamage );
	void SetCanBeGrabbed( const bool bCanBeGrabbed );
	CRope* GetMasterRope() { return mMasterRope; }
	void SetMasterRope( CRope* pRope )
	{
		mMasterRope = pRope;
	}

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


private:
	CRopeSample* m_Sample;
	string_t mModelName;
	float mDefaultMass;
	bool mCauseDamage;
	bool mCanBeGrabbed;
	CRope* mMasterRope;
};

static const char* const g_pszCreakSounds[] =
{
	"items/rope1.wav",
	"items/rope2.wav",
	"items/rope3.wav"
};

TYPEDESCRIPTION	CRope::m_SaveData[] =
{	DEFINE_FIELD( CRope, m_iSegments, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_bToggle, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, m_InitialDeltaTime, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, mLastTime, FIELD_TIME ),
	DEFINE_FIELD( CRope, m_LastEndPos, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CRope, m_Gravity, FIELD_VECTOR ),
	DEFINE_FIELD( CRope, m_NumSamples, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mObjectAttached, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, mAttachedObjectsSegment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, detachTime, FIELD_TIME ),
	DEFINE_ARRAY( CRope, seg, FIELD_CLASSPTR, MAX_SEGMENTS ),
	DEFINE_ARRAY( CRope, altseg, FIELD_CLASSPTR, MAX_SEGMENTS ),
	DEFINE_ARRAY( CRope, m_Samples, FIELD_CLASSPTR, MAX_SAMPLES ),
	DEFINE_FIELD( CRope, mDisallowPlayerAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mBodyModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mEndingModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mAttachedObjectsOffset, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_bMakeSound, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, m_activated, FIELD_CHARACTER ),
};

IMPLEMENT_SAVERESTORE( CRope, CBaseDelay )

LINK_ENTITY_TO_CLASS( env_rope, CRope )

void CRope::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "segments" ) )
	{
		pkvd->fHandled = true;

		m_iSegments = strtol( pkvd->szValue, NULL, 10 );

		if( m_iSegments >= MAX_SEGMENTS )
			m_iSegments = MAX_SEGMENTS - 1;
	}
	else if( FStrEq( pkvd->szKeyName, "bodymodel" ) )
	{
		pkvd->fHandled = true;

		mBodyModel = ALLOC_STRING( pkvd->szValue );
	}
	else if( FStrEq( pkvd->szKeyName, "endingmodel" ) )
	{
		pkvd->fHandled = true;

		mEndingModel = ALLOC_STRING( pkvd->szValue );
	}
	else if( FStrEq( pkvd->szKeyName, "disable" ) )
	{
		pkvd->fHandled = true;

		mDisallowPlayerAttachment = strtol( pkvd->szValue, NULL, 10 );
	}
	else
		CBaseDelay::KeyValue( pkvd );
}

CRope::CRope()
{
	mBodyModel = MAKE_STRING( "models/rope16.mdl" );
	mEndingModel = MAKE_STRING( "models/rope16.mdl" );

}

void CRope::Precache()
{
	CBaseDelay::Precache();

	UTIL_PrecacheOther( "rope_segment" );
	UTIL_PrecacheOther( "rope_sample" );

	PRECACHE_MODEL(STRING(GetBodyModel()));
	PRECACHE_MODEL(STRING(GetEndingModel()));
	PRECACHE_SOUND_ARRAY( g_pszCreakSounds );
}

void CRope::Spawn()
{
	pev->classname = MAKE_STRING( "env_rope" );

	m_bMakeSound = true;

	Precache();

	m_Gravity.x = m_Gravity.y = 0;
	m_Gravity.z = -50;

	mObjectAttached = false;


	m_NumSamples = m_iSegments + 1;

	m_activated = false;
}

void CRope::Activate()
{
	if (!m_activated)
	{
		InitRope();
		m_activated = true;
	}
}

void CRope::InitRope()
{
	pev->flags |= FL_ALWAYSTHINK;

	for( int uiSample = 0; uiSample < m_NumSamples; ++uiSample )
	{
		m_Samples[ uiSample ] = CRopeSample::CreateSample();
		UTIL_SetOrigin(m_Samples[ uiSample ]->pev, pev->origin);
	}

	memset( m_Samples + m_NumSamples, 0, sizeof( CRopeSample* ) * ( MAX_SAMPLES - m_NumSamples ) );

	{
		CRopeSegment* pSegment = seg[ 0 ] = CRopeSegment::CreateSegment( m_Samples[ 0 ], GetBodyModel(), this );

		pSegment->SetAbsOrigin( pev->origin );

		pSegment = altseg[ 0 ] = CRopeSegment::CreateSegment( m_Samples[ 0 ], GetBodyModel(), this );

		pSegment->SetAbsOrigin( pev->origin );
	}

	Vector origin;
	Vector angles;

	const Vector vecGravity = m_Gravity.Normalize();

	if( m_iSegments > 2 )
	{
		CRopeSample** ppCurrentSys = m_Samples;

		for( int uiSeg = 1; uiSeg < m_iSegments - 1; ++uiSeg )
		{
			CRopeSample* pSegSample = m_Samples[ uiSeg ];
			seg[ uiSeg ] = CRopeSegment::CreateSegment( pSegSample, GetBodyModel(), this );

			altseg[ uiSeg ] = CRopeSegment::CreateSegment( pSegSample, GetBodyModel(), this );

			CRopeSegment* pCurrent = seg[ uiSeg - 1 ];

			pCurrent->GetAttachment( 0, origin, angles );

			Vector vecPos = origin - pCurrent->pev->origin;

			const float flLength = vecPos.Length();

			origin = flLength * vecGravity + pCurrent->pev->origin;

			seg[ uiSeg ]->SetAbsOrigin( origin );
			altseg[ uiSeg ]->SetAbsOrigin( origin );
		}
	}

	CRopeSample* pSegSample = m_Samples[ m_iSegments - 1 ];
	seg[ m_iSegments - 1 ] = CRopeSegment::CreateSegment( pSegSample, GetEndingModel(), this );

	altseg[ m_iSegments - 1 ] = CRopeSegment::CreateSegment( pSegSample, GetEndingModel(), this );

	CRopeSegment* pCurrent = seg[ m_iSegments - 2 ];

	pCurrent->GetAttachment( 0, origin, angles );

	Vector vecPos = origin - pCurrent->pev->origin;

	const float flLength = vecPos.Length();

	origin = flLength * vecGravity + pCurrent->pev->origin;

	seg[ m_iSegments - 1 ]->SetAbsOrigin( origin );
	altseg[ m_iSegments - 1 ]->SetAbsOrigin( origin );

	memset( seg + m_iSegments, 0, sizeof( CRopeSegment* ) * ( MAX_SEGMENTS - m_iSegments ) );
	memset( altseg + m_iSegments, 0, sizeof( CRopeSegment* ) * ( MAX_SEGMENTS - m_iSegments ) );

	m_InitialDeltaTime = true;

	InitializeRopeSim();

	SetThink(&CRope::RopeThink);
	pev->nextthink = gpGlobals->time + 0.01;
}

void CRope::RopeThink()
{
	m_bToggle = !m_bToggle;

	RunSimOnSamples();

	CRopeSegment** ppPrimarySegs;
	CRopeSegment** ppHiddenSegs;

	if( m_bToggle )
	{
		ppPrimarySegs = altseg;
		ppHiddenSegs = seg;
	}
	else
	{
		ppPrimarySegs = seg;
		ppHiddenSegs = altseg;
	}

	SetRopeSegments( m_iSegments, ppPrimarySegs, ppHiddenSegs );

	if( ShouldCreak() )
	{
		Creak();
	}

	pev->nextthink = gpGlobals->time + 0.001;
}

void CRope::InitializeRopeSim()
{
	int uiIndex;

	for( int uiSeg = 0; uiSeg < m_iSegments; ++uiSeg )
	{
		CRopeSegment* pSegment = seg[ uiSeg ];
		CRopeSample* pSample = pSegment->GetSample();

		RopeSampleData& data = pSample->GetData();

		data.mPosition = pSegment->pev->origin;

		data.mVelocity				= g_vecZero;
		data.mForce					= g_vecZero;
		data.mMassReciprocal		= 1;
		data.mApplyExternalForce	= false;
		data.mExternalForce			= g_vecZero;

		Vector vecOrigin, vecAngles;
		pSegment->GetAttachment( 0, vecOrigin, vecAngles );
		data.restLength = ( pSegment->pev->origin - vecOrigin ).Length();
	}

	{
		//Zero out the anchored segment's mass so it stays in place.
		CRopeSample *pSample = m_Samples[ 0 ];

		pSample->GetData().mMassReciprocal = 0;
	}

	CRopeSegment* pSegment = seg[ m_iSegments - 1 ];

	Vector vecOrigin, vecAngles;

	pSegment->GetAttachment( 0, vecOrigin, vecAngles );

	Vector vecDistance = vecOrigin - pSegment->pev->origin;

	const float flLength = vecDistance.Length();

	const Vector vecGravity = m_Gravity.Normalize();

	vecOrigin = vecGravity * flLength + pSegment->pev->origin;

	CRopeSample* pSample = m_Samples[ m_NumSamples - 1 ];

	RopeSampleData& data = pSample->GetData();

	data.mPosition = vecOrigin;

	m_LastEndPos = vecOrigin;

	data.mVelocity = g_vecZero;

	data.mForce = g_vecZero;

	data.mMassReciprocal = 0.2;

	data.mApplyExternalForce = false;

	int uiNumSegs = ROPE_IGNORE_SAMPLES;

	if( m_iSegments <= ROPE_IGNORE_SAMPLES )
		uiNumSegs = m_iSegments;

	for( uiIndex = 0; uiIndex < uiNumSegs; ++uiIndex )
	{
		seg[ uiIndex ]->SetCanBeGrabbed( false );
		altseg[ uiIndex ]->SetCanBeGrabbed( false );
	}
}

void CRope::RunSimOnSamples()
{
	float flDeltaTime = 0.025;

	if( m_InitialDeltaTime )
	{
		m_InitialDeltaTime = false;
		mLastTime = gpGlobals->time;
		flDeltaTime = 0;
	}

	int uiIndex = 0;

	bool swapped = false;
	while( true )
	{
		++uiIndex;

		ComputeForces( m_Samples );
		RK4Integrate( flDeltaTime );

		mLastTime += 0.007;

		if( gpGlobals->time <= mLastTime )
		{
			if( ( uiIndex % 2 ) != 0 )
				break;
		}

		for (int i=0; i<m_NumSamples; ++i)
		{
			m_Samples[i]->Swap();
		}
		swapped = !swapped;
	}

	if (swapped)
	{
		for (int i=0; i<m_NumSamples; ++i)
		{
			m_Samples[i]->ResetSwap();
		}
	}

	mLastTime = gpGlobals->time;
}

void CRope::ComputeForces( RopeSampleData* pSystem )
{
	int uiIndex;
	for( uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex )
	{
		ComputeSampleForce( pSystem[ uiIndex ] );
	}

	for( uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		ComputeSpringForce( pSystem[ uiIndex ], pSystem[ uiIndex+1 ] );
	}
}

void CRope::ComputeForces( CRopeSample** ppSystem )
{
	int uiIndex;
	for( uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex )
	{
		ComputeSampleForce( ppSystem[ uiIndex ]->GetSourceData() );
	}

	for( uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		ComputeSpringForce( ppSystem[ uiIndex ]->GetSourceData(), ppSystem[ uiIndex+1 ]->GetSourceData() );
	}
}

void CRope::ComputeSampleForce( RopeSampleData& data )
{
	data.mForce = g_vecZero;

	if( data.mMassReciprocal != 0.0 )
	{
		data.mForce = data.mForce + ( m_Gravity / data.mMassReciprocal );
	}

	if( data.mApplyExternalForce )
	{
		data.mForce = data.mForce + data.mExternalForce;

		data.mExternalForce = g_vecZero;
		data.mApplyExternalForce = false;
	}

	if( DotProduct( m_Gravity, data.mVelocity ) >= 0 )
	{
		data.mForce = data.mForce + data.mVelocity * -0.04;
	}
	else
	{
		data.mForce = data.mForce - data.mVelocity;
	}
}

void CRope::ComputeSpringForce( RopeSampleData& first, RopeSampleData& second )
{
	Vector vecDist = first.mPosition - second.mPosition;

	const double flDistance = vecDist.Length();

	const double flForce = ( flDistance - first.restLength ) * HOOK_CONSTANT;

	const double flNewRelativeDist = DotProduct( first.mVelocity - second.mVelocity, vecDist ) * SPRING_DAMPING;

	vecDist = vecDist.Normalize();

	const double flSpringFactor = -( flNewRelativeDist / flDistance + flForce );

	const Vector vecForce = flSpringFactor * vecDist;

	first.mForce = first.mForce + vecForce;

	second.mForce = second.mForce - vecForce;
}

void CRope::RK4Integrate( const float flDeltaTime )
{
	const float flDeltas[ MAX_LIST_SEGMENTS - 1 ] =
	{
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime
	};

	{
		RopeSampleData* pTemp1 = g_pTempList[ 0 ];
		RopeSampleData* pTemp2 = g_pTempList[ 1 ];

		for( int uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData& data = m_Samples[ uiIndex ]->GetSourceData();

			pTemp2->mForce = data.mMassReciprocal * data.mForce * flDeltas[ 0 ];
			pTemp2->mVelocity = data.mVelocity * flDeltas[ 0 ];
			pTemp2->restLength = data.restLength;

			pTemp1->mMassReciprocal = data.mMassReciprocal;
			pTemp1->mVelocity = data.mVelocity + pTemp2->mForce;
			pTemp1->mPosition = data.mPosition + pTemp2->mVelocity;
			pTemp1->restLength = data.restLength;
		}

		ComputeForces( g_pTempList[ 0 ] );
	}

	for( int uiStep = 2; uiStep < MAX_LIST_SEGMENTS - 1; ++uiStep )
	{
		RopeSampleData* pTemp1 = g_pTempList[ 0 ];
		RopeSampleData* pTemp2 = g_pTempList[ uiStep ];

		for( int uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData& data = m_Samples[ uiIndex ]->GetSourceData();

			pTemp2->mForce = data.mMassReciprocal * pTemp1->mForce * flDeltas[ uiStep - 1 ];
			pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[ uiStep - 1 ];
			pTemp2->restLength = data.restLength;

			pTemp1->mMassReciprocal = data.mMassReciprocal;
			pTemp1->mVelocity = data.mVelocity + pTemp2->mForce;
			pTemp1->mPosition = data.mPosition + pTemp2->mVelocity;
			pTemp1->restLength = data.restLength;
		}

		ComputeForces( g_pTempList[ 0 ] );
	}

	{
		RopeSampleData* pTemp1 = g_pTempList[ 0 ];
		RopeSampleData* pTemp2 = g_pTempList[ 4 ];

		for( int uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData& data = m_Samples[ uiIndex ]->GetSourceData();

			pTemp2->mForce = data.mMassReciprocal * pTemp1->mForce * flDeltas[ 3 ];

			pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[ 3 ];
		}
	}

	RopeSampleData* pTemp1 = g_pTempList[ 1 ];
	RopeSampleData* pTemp2 = g_pTempList[ 2 ];
	RopeSampleData* pTemp3 = g_pTempList[ 3 ];
	RopeSampleData* pTemp4 = g_pTempList[ 4 ];

	for( int uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2, ++pTemp3, ++pTemp4 )
	{
		RopeSampleData& pSource = m_Samples[ uiIndex ]->GetSourceData();
		RopeSampleData& pTarget = m_Samples[ uiIndex ]->GetTargetData();

		const Vector vecPosChange = 1.0f / 6.0f * ( pTemp1->mVelocity + ( pTemp2->mVelocity + pTemp3->mVelocity ) * 2 + pTemp4->mVelocity );

		const Vector vecVelChange = 1.0f / 6.0f * ( pTemp1->mForce + ( pTemp2->mForce + pTemp3->mForce ) * 2 + pTemp4->mForce );

		pTarget.mPosition = pSource.mPosition + ( vecPosChange );//* flDeltaTime );

		pTarget.mVelocity = pSource.mVelocity + ( vecVelChange );//* flDeltaTime );
	}
}

//TODO move to common header - Solokiller
static const Vector DOWN( 0, 0, -1 );

static const Vector RIGHT( 0, 1, 0 );

void GetAlignmentAngles( const Vector& vecTop, const Vector& vecBottom, Vector& vecOut )
{
	Vector vecDist = vecBottom - vecTop;

	Vector vecResult = vecDist.Normalize();

	const float flRoll = acos( DotProduct( vecResult, RIGHT ) ) * ( 180.0 / M_PI );

	vecOut.z = -flRoll;

	vecDist.y = 0;

	vecResult = vecDist.Normalize();

	const float flPitch = acos( DotProduct( vecResult, DOWN ) ) * ( 180.0 / M_PI );

	vecOut.x = ( vecResult.x >= 0.0 ) ? flPitch : -flPitch;
	vecOut.y = 0;
}

void TruncateEpsilon( Vector& vec )
{
	Vector vec1 =  vec * 10.0;
	vec1.x += 0.5;
	vec = vec1 / 10;
}

void CRope::TraceModels( CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs )
{
	if( m_iSegments > 1 )
	{
		Vector vecAngles;

		GetAlignmentAngles(
			m_Samples[ 0 ]->GetData().mPosition,
			m_Samples[ 1 ]->GetData().mPosition,
			vecAngles );

		( *ppPrimarySegs )->pev->angles = vecAngles;
	}

	TraceResult tr;

	if( mObjectAttached )
	{
		for( unsigned int uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
		{
			CRopeSample* pSample = m_Samples[ uiSeg ];

			Vector vecDist = pSample->GetData().mPosition - ppHiddenSegs[ uiSeg ]->pev->origin;

			vecDist = vecDist.Normalize();

			// HACK: this code relies on integer underflow. uiSeg must be unsigned!
			const float flTraceDist = ( uiSeg - mAttachedObjectsSegment + 2 ) < 5 ? 50 : 10;

			const Vector vecTraceDist = vecDist * flTraceDist;

			const Vector vecEnd = pSample->GetData().mPosition + vecTraceDist;

			UTIL_TraceLine( ppHiddenSegs[ uiSeg ]->pev->origin, vecEnd, ignore_monsters, edict(), &tr );

			if( tr.flFraction == 1.0 && tr.fAllSolid )
			{
				break;
			}

			if( tr.flFraction != 1.0 || tr.fStartSolid || !tr.fInOpen )
			{
				Vector vecOrigin = tr.vecEndPos - vecTraceDist;

				TruncateEpsilon( vecOrigin );

				ppPrimarySegs[ uiSeg ]->SetAbsOrigin( vecOrigin );

				Vector vecNormal = tr.vecPlaneNormal.Normalize() * 20000.0;

				RopeSampleData& data = ppPrimarySegs[ uiSeg ]->GetSample()->GetData();

				data.mApplyExternalForce = true;

				data.mExternalForce = vecNormal;

				data.mVelocity = g_vecZero;
			}
			else
			{
				Vector vecOrigin = pSample->GetData().mPosition;

				TruncateEpsilon( vecOrigin );

				ppPrimarySegs[ uiSeg ]->SetAbsOrigin( vecOrigin );
			}
		}
	}
	else
	{
		for( unsigned int uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
		{
			UTIL_TraceLine(
				ppHiddenSegs[ uiSeg ]->pev->origin,
				m_Samples[ uiSeg ]->GetData().mPosition,
				ignore_monsters, edict(), &tr );

			if( tr.flFraction == 1.0 )
			{
				Vector vecOrigin = m_Samples[ uiSeg ]->GetData().mPosition;

				TruncateEpsilon( vecOrigin );

				ppPrimarySegs[ uiSeg ]->SetAbsOrigin( vecOrigin );
			}
			else
			{
				CBaseEntity* pEnt = (CBaseEntity*)GET_PRIVATE( tr.pHit );
				const Vector vecNormal = tr.vecPlaneNormal.Normalize();

				Vector vecOrigin = tr.vecEndPos + vecNormal * 10.0;

				TruncateEpsilon( vecOrigin );

				ppPrimarySegs[ uiSeg ]->SetAbsOrigin( vecOrigin );

				ppPrimarySegs[ uiSeg ]->GetSample()->GetData().mApplyExternalForce = true;

				ppPrimarySegs[ uiSeg ]->GetSample()->GetData().mExternalForce = vecNormal * 40000.0;
			}
		}
	}

	Vector vecAngles;

	for( int uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
	{
		CRopeSegment *pSegment = ppPrimarySegs[ uiSeg - 1 ];
		CRopeSegment *pSegment2 = ppPrimarySegs[ uiSeg ];

		GetAlignmentAngles( pSegment->pev->origin, pSegment2->pev->origin, vecAngles );

		pSegment->pev->angles = vecAngles;
	}

	if( m_iSegments > 1 )
	{
		CRopeSample *pSample = m_Samples[ m_NumSamples - 1 ];

		UTIL_TraceLine( m_LastEndPos, pSample->GetData().mPosition, ignore_monsters, edict(), &tr );

		if( tr.flFraction == 1.0 )
		{
			m_LastEndPos = pSample->GetData().mPosition;
		}
		else
		{
			m_LastEndPos = tr.vecEndPos;

			pSample->GetData().mApplyExternalForce = true;

			pSample->GetData().mExternalForce = tr.vecPlaneNormal.Normalize() * 40000.0;
		}

		CRopeSegment *pSegment = ppPrimarySegs[ m_NumSamples - 2 ];

		Vector vecAngles;

		GetAlignmentAngles( pSegment->pev->origin, m_LastEndPos, vecAngles );

		pSegment->pev->angles = vecAngles;
	}
}

void CRope::SetRopeSegments( const int uiNumSegments,
							 CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs )
{
	if( uiNumSegments > 0 )
	{
		TraceModels( ppPrimarySegs, ppHiddenSegs );

		ppPrimarySegs[ 0 ]->pev->solid = SOLID_TRIGGER;
		ppPrimarySegs[ 0 ]->pev->effects = 0;

		ppHiddenSegs[ 0 ]->pev->solid = SOLID_NOT;
		ppHiddenSegs[ 0 ]->pev->effects = EF_NODRAW;

		for( int uiIndex = 1; uiIndex < uiNumSegments; ++uiIndex )
		{
			CRopeSegment* pPrim = ppPrimarySegs[ uiIndex ];
			CRopeSegment* pHidden = ppHiddenSegs[ uiIndex ];

			pPrim->pev->solid = SOLID_TRIGGER;
			pPrim->pev->effects = 0;

			pHidden->pev->solid = SOLID_NOT;
			pHidden->pev->effects = EF_NODRAW;

			Vector vecOrigin = pPrim->pev->origin;

			//vecOrigin.x += 10.0;
			//vecOrigin.y += 10.0;

			pHidden->SetAbsOrigin( vecOrigin );
		}
	}
}

bool CRope::MoveUp( const float flDeltaTime )
{
	if( mAttachedObjectsSegment > 4 )
	{
		float flDistance = flDeltaTime * 128.0;

		Vector vecOrigin, vecAngles;

		while( true )
		{
			float flOldDist = flDistance;

			flDistance = 0;

			if( flOldDist <= 0 )
				break;

			if( mAttachedObjectsSegment <= 3 )
				break;

			if( flOldDist > mAttachedObjectsOffset )
			{
				flDistance = flOldDist - mAttachedObjectsOffset;

				--mAttachedObjectsSegment;

				float flNewOffset = 0;

				if( mAttachedObjectsSegment < m_iSegments )
				{
					CRopeSegment *pSegment = seg[ mAttachedObjectsSegment ];

					pSegment->GetAttachment( 0, vecOrigin, vecAngles );

					flNewOffset = ( pSegment->pev->origin - vecOrigin ).Length();
				}

				mAttachedObjectsOffset = flNewOffset;
			}
			else
			{
				mAttachedObjectsOffset -= flOldDist;
			}
		}
	}

	return true;
}

bool CRope::MoveDown( const float flDeltaTime )
{
	if( !mObjectAttached )
		return false;

	float flDistance = flDeltaTime * 128.0;

	Vector vecOrigin, vecAngles;

	CRopeSegment* pSegment;

	bool bOnRope = true;

	bool bDoIteration = true;

	while( bDoIteration )
	{
		bDoIteration = false;

		if( flDistance > 0.0 )
		{
			float flNewDist = flDistance;
			float flSegLength = 0.0;

			while( bOnRope )
			{
				if( mAttachedObjectsSegment < m_iSegments )
				{
					pSegment = seg[ mAttachedObjectsSegment ];

					pSegment->GetAttachment( 0, vecOrigin, vecAngles );

					flSegLength = ( pSegment->pev->origin - vecOrigin ).Length();
				}

				const float flOffset = flSegLength - mAttachedObjectsOffset;

				if( flNewDist <= flOffset )
				{
					mAttachedObjectsOffset += flNewDist;
					flDistance = 0;
					bDoIteration = true;
					break;
				}

				if( mAttachedObjectsSegment + 1 == m_iSegments )
					bOnRope = false;
				else
					++mAttachedObjectsSegment;

				flNewDist -= flOffset;
				flSegLength = 0;

				mAttachedObjectsOffset = 0;

				if( flNewDist <= 0 )
					break;
			}
		}
	}

	return bOnRope;
}

Vector CRope::GetAttachedObjectsVelocity() const
{
	if( !mObjectAttached )
		return g_vecZero;

	return seg[ mAttachedObjectsSegment ]->GetSample()->GetData().mVelocity;
}

void CRope::ApplyForceFromPlayer( const Vector& vecForce )
{
	if( !mObjectAttached )
		return;

	float flForce = 20000.0;

	if( m_iSegments < 26 )
		flForce *= ( m_iSegments / 26.0 );

	const Vector vecScaledForce = vecForce * flForce;

	ApplyForceToSegment( vecScaledForce, mAttachedObjectsSegment );
}

void CRope::ApplyForceToSegment( const Vector& vecForce, const int uiSegment )
{
	if( uiSegment < m_iSegments )
	{
		seg[ uiSegment ]->ApplyExternalForce( vecForce );
	}
	else if( uiSegment == m_iSegments )
	{
		//Apply force to the last sample.

		RopeSampleData& data = m_Samples[ uiSegment - 1 ]->GetData();

		data.mExternalForce = data.mExternalForce + vecForce;

		data.mApplyExternalForce = true;
	}
}

void CRope::AttachObjectToSegment( CRopeSegment* pSegment )
{
	mObjectAttached = true;

	detachTime = 0;

	SetAttachedObjectsSegment( pSegment );

	mAttachedObjectsOffset = 0;
}

void CRope::DetachObject()
{
	mObjectAttached = false;
	detachTime = gpGlobals->time;
}

bool CRope::IsAcceptingAttachment() const
{
	if( gpGlobals->time - detachTime > 2.0 && !mObjectAttached )
	{
		return mDisallowPlayerAttachment != 1;
	}

	return false;
}

bool CRope::ShouldCreak() const
{
	if( mObjectAttached && m_bMakeSound )
	{
		CRopeSample* pSample = seg[ mAttachedObjectsSegment ]->GetSample();

		if( pSample->GetData().mVelocity.Length() > 20.0 )
			return RANDOM_LONG( 1, 5 ) == 1;
	}

	return false;
}

void CRope::Creak()
{
	EMIT_SOUND( edict(), CHAN_BODY,
				g_pszCreakSounds[ RANDOM_LONG( 0, ARRAYSIZE( g_pszCreakSounds ) - 1 ) ],
				VOL_NORM, ATTN_NORM );
}

float CRope::GetSegmentLength( int uiSegmentIndex ) const
{
	if( uiSegmentIndex < m_iSegments )
	{
		Vector vecOrigin, vecAngles;

		CRopeSegment *pSegment = seg[ uiSegmentIndex ];

		pSegment->GetAttachment( 0, vecOrigin, vecAngles );

		return ( pSegment->pev->origin - vecOrigin ).Length();
	}

	return 0;
}

float CRope::GetRopeLength() const
{
	float flLength = 0;

	Vector vecOrigin, vecAngles;

	for( int uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		CRopeSegment *pSegment = seg[ uiIndex ];

		pSegment->GetAttachment( 0, vecOrigin, vecAngles );

		flLength += ( pSegment->pev->origin - vecOrigin ).Length();
	}

	return flLength;
}

Vector CRope::GetRopeOrigin() const
{
	return m_Samples[ 0 ]->GetData().mPosition;
}

bool CRope::IsValidSegmentIndex( const int uiSegment ) const
{
	return uiSegment < m_iSegments;
}

Vector CRope::GetSegmentOrigin( const int uiSegment ) const
{
	if( !IsValidSegmentIndex( uiSegment ) )
		return g_vecZero;

	return m_Samples[ uiSegment ]->GetData().mPosition;
}

Vector CRope::GetSegmentAttachmentPoint( const int uiSegment ) const
{
	if( !IsValidSegmentIndex( uiSegment ) )
		return g_vecZero;

	Vector vecOrigin, vecAngles;

	CRopeSegment *pSegment = m_bToggle ? altseg[ uiSegment ] : seg[ uiSegment ];

	pSegment->GetAttachment( 0, vecOrigin, vecAngles );

	return vecOrigin;
}

void CRope::SetAttachedObjectsSegment( CRopeSegment* pSegment )
{
	for( int uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		if( seg[ uiIndex ] == pSegment || altseg[ uiIndex ] == pSegment )
		{
			mAttachedObjectsSegment = uiIndex;
			break;
		}
	}
}

Vector CRope::GetSegmentDirFromOrigin( const int uiSegmentIndex ) const
{
	if( uiSegmentIndex >= m_iSegments )
		return g_vecZero;

	//There is one more sample than there are segments, so this is fine.
	const Vector vecResult =
		m_Samples[ uiSegmentIndex + 1 ]->GetData().mPosition -
		m_Samples[ uiSegmentIndex ]->GetData().mPosition;

	return vecResult.Normalize();
}

Vector CRope::GetAttachedObjectsPosition() const
{
	if( !mObjectAttached )
		return g_vecZero;

	Vector vecResult;

	if( mAttachedObjectsSegment < m_iSegments )
		vecResult = m_Samples[ mAttachedObjectsSegment ]->GetData().mPosition;

	vecResult = vecResult +
		( mAttachedObjectsOffset * GetSegmentDirFromOrigin( mAttachedObjectsSegment ) );

	return vecResult;
}



// Global Savedata for player
TYPEDESCRIPTION	CRopeSample::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSample, data.mPosition, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mExternalForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data.mApplyExternalForce, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSample, data.mMassReciprocal, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, data.restLength, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, data2.mPosition, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data2.mVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data2.mForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data2.mExternalForce, FIELD_VECTOR ),
	DEFINE_FIELD( CRopeSample, data2.mApplyExternalForce, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSample, data2.mMassReciprocal, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, data2.restLength, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSample, swapped, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE(CRopeSample, CBaseEntity)

LINK_ENTITY_TO_CLASS( rope_sample, CRopeSample );

void CRopeSample::Spawn()
{
	pev->classname = MAKE_STRING( "rope_sample" );

	pev->effects |= EF_NODRAW;
}

CRopeSample* CRopeSample::CreateSample()
{
	CRopeSample* pSample = GetClassPtr<CRopeSample>( NULL );

	pSample->Spawn();

	return pSample;
}


TYPEDESCRIPTION	CRopeSegment::m_SaveData[] =
{
	DEFINE_FIELD( CRopeSegment, m_Sample, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRopeSegment, mModelName, FIELD_STRING ),
	DEFINE_FIELD( CRopeSegment, mDefaultMass, FIELD_FLOAT ),
	DEFINE_FIELD( CRopeSegment, mCauseDamage, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSegment, mCanBeGrabbed, FIELD_CHARACTER ),
	DEFINE_FIELD( CRopeSegment, mMasterRope, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CRopeSegment, CBaseAnimating )

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );

void CRopeSegment::Precache()
{
	CBaseAnimating::Precache();
	if( !mModelName )
		mModelName = MAKE_STRING( "models/rope16.mdl" );

	PRECACHE_MODEL( STRING( mModelName ) );
	PRECACHE_SOUND( "items/grab_rope.wav" );
}

void CRopeSegment::Spawn()
{
	pev->classname = MAKE_STRING( "rope_segment" );

	Precache();

	SET_MODEL( edict(), STRING( mModelName ) );

	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_TRIGGER;
	pev->effects = EF_NODRAW;
	SetAbsOrigin( pev->origin );

	UTIL_SetSize( pev, Vector( -30, -30, -30 ), Vector( 30, 30, 30 ) );

	pev->nextthink = gpGlobals->time + 0.5;
}

void CRopeSegment::Touch( CBaseEntity* pOther )
{
	if( pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pOther );

		//Electrified wires deal damage. - Solokiller
		if( mCauseDamage )
		{
			pOther->TakeDamage( pev, pev, 1, DMG_SHOCK );
		}

		if (pPlayer->m_afPhysicsFlags & PFLAG_ONBARNACLE)
			return;

		if( GetMasterRope()->IsAcceptingAttachment() && !(pPlayer->m_afPhysicsFlags & PFLAG_ONROPE) )
		{
			if( mCanBeGrabbed )
			{
				RopeSampleData& data = m_Sample->GetData();

				pOther->pev->origin = data.mPosition;

				pPlayer->SetOnRopeState( true );
				pPlayer->SetRope( GetMasterRope() );
				GetMasterRope()->AttachObjectToSegment( this );

				const Vector& vecVelocity = pOther->pev->velocity;

				if( vecVelocity.Length() > 0.5 )
				{
					//Apply some external force to move the rope. - Solokiller
					data.mApplyExternalForce = true;

					data.mExternalForce = data.mExternalForce + vecVelocity * 750;
				}

				if( GetMasterRope()->IsSoundAllowed() )
				{
					EMIT_SOUND( edict(), CHAN_BODY, "items/grab_rope.wav", 1.0, ATTN_NORM );
				}
			}
			else
			{
				//This segment cannot be grabbed, so grab the highest one if possible. - Solokiller
				CRope *pRope = GetMasterRope();

				CRopeSegment* pSegment;

				if( pRope->GetNumSegments() <= 4 )
				{
					//Fewer than 5 segments exist, so allow grabbing the last one. - Solokiller
					pSegment = pRope->GetSegments()[ pRope->GetNumSegments() - 1 ];
					pSegment->SetCanBeGrabbed( true );
				}
				else
				{
					pSegment = pRope->GetSegments()[ 4 ];
				}

				pSegment->Touch( pOther );
			}
		}
	}
}

CRopeSegment* CRopeSegment::CreateSegment( CRopeSample* pSample, string_t iszModelName, CRope* rope )
{
	CRopeSegment* pSegment = GetClassPtr<CRopeSegment>( NULL );

	pSegment->mModelName = iszModelName;

	pSegment->Spawn();

	pSegment->m_Sample = pSample;

	pSegment->mCauseDamage = false;
	pSegment->mCanBeGrabbed = true;
	pSegment->mDefaultMass = pSample->GetData().mMassReciprocal;
	pSegment->SetMasterRope(rope);

	return pSegment;
}

void CRopeSegment::ApplyExternalForce( const Vector& vecForce )
{
	m_Sample->GetData().mApplyExternalForce = true;

	m_Sample->GetData().mExternalForce = m_Sample->GetData().mExternalForce + vecForce;
}

void CRopeSegment::SetCauseDamageOnTouch( const bool bCauseDamage )
{
	mCauseDamage = bCauseDamage;
}

void CRopeSegment::SetCanBeGrabbed( const bool bCanBeGrabbed )
{
	mCanBeGrabbed = bCanBeGrabbed;
}


class CElectrifiedWire : public CRope
{
public:
	CElectrifiedWire();
	void InitElectrifiedRope();
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData* pkvd );

	void Precache();

	void Spawn();
	void Activate();

	void EXPORT ElectrifiedRopeThink();

	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue );

	/**
	*	@return Whether the wire is active.
	*/
	bool IsActive() const { return m_bIsActive; }

	/**
	*	@param iFrequency Frequency.
	*	@return Whether the spark effect should be performed.
	*/
	bool ShouldDoEffect( const int iFrequency );

	/**
	*	Do spark effects.
	*/
	void DoSpark( const int uiSegment, const bool bExertForce );

	/**
	*	Do lightning effects.
	*/
	void DoLightning();

private:
	bool m_bIsActive;

	int m_iTipSparkFrequency;
	int m_iBodySparkFrequency;
	int m_iLightningFrequency;

	int m_iXJoltForce;
	int m_iYJoltForce;
	int m_iZJoltForce;

	int m_uiNumUninsulatedSegments;
	int m_uiUninsulatedSegments[ MAX_SEGMENTS ];

	int m_iLightningSprite;

	float m_flLastSparkTime;
};

CElectrifiedWire::CElectrifiedWire()
{
	m_bIsActive = true;
	m_iTipSparkFrequency = 3;
	m_iBodySparkFrequency = 100;
	m_iLightningFrequency = 150;
	m_iXJoltForce = 0;
	m_iYJoltForce = 0;
	m_iZJoltForce = 0;
	m_uiNumUninsulatedSegments = 0;
}


TYPEDESCRIPTION CElectrifiedWire::m_SaveData[] =
{
	DEFINE_FIELD( CElectrifiedWire, m_bIsActive, FIELD_CHARACTER ),

	DEFINE_FIELD( CElectrifiedWire, m_iTipSparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iBodySparkFrequency, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iLightningFrequency, FIELD_INTEGER ),

	DEFINE_FIELD( CElectrifiedWire, m_iXJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iYJoltForce, FIELD_INTEGER ),
	DEFINE_FIELD( CElectrifiedWire, m_iZJoltForce, FIELD_INTEGER ),

	DEFINE_FIELD( CElectrifiedWire, m_uiNumUninsulatedSegments, FIELD_INTEGER ),
	DEFINE_ARRAY( CElectrifiedWire, m_uiUninsulatedSegments, FIELD_INTEGER, MAX_SEGMENTS ),

	//DEFINE_FIELD( m_iLightningSprite, FIELD_INTEGER ), //Not restored, reset in Precache. - Solokiller

	DEFINE_FIELD( CElectrifiedWire, m_flLastSparkTime, FIELD_TIME ),
};

LINK_ENTITY_TO_CLASS( env_electrified_wire, CElectrifiedWire );
IMPLEMENT_SAVERESTORE( CElectrifiedWire, CRope );


void CElectrifiedWire::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "sparkfrequency" ) )
	{
		m_iTipSparkFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "bodysparkfrequency" ) )
	{
		m_iBodySparkFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "lightningfrequency" ) )
	{
		m_iLightningFrequency = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "xforce" ) )
	{
		m_iXJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "yforce" ) )
	{
		m_iYJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else if( FStrEq( pkvd->szKeyName, "zforce" ) )
	{
		m_iZJoltForce = strtol( pkvd->szValue, NULL, 10 );

		pkvd->fHandled = true;
	}
	else
		CRope::KeyValue( pkvd );
}

void CElectrifiedWire::Precache()
{
	CRope::Precache();

	m_iLightningSprite = PRECACHE_MODEL( "sprites/lgtning.spr" );
}

void CElectrifiedWire::Spawn()
{
	CRope::Spawn();
	pev->classname = MAKE_STRING( "env_electrified_wire" );
}

void CElectrifiedWire::Activate()
{
	if (!m_activated)
	{
		InitElectrifiedRope();
		m_activated = true;
	}
}

void CElectrifiedWire::InitElectrifiedRope()
{
	InitRope();

	m_uiNumUninsulatedSegments = 0;
	m_bIsActive = true;

	if( m_iBodySparkFrequency > 0 )
	{
		for( int uiIndex = 0; uiIndex < GetNumSegments(); ++uiIndex )
		{
			if( IsValidSegmentIndex( uiIndex ) )
			{
				m_uiUninsulatedSegments[ m_uiNumUninsulatedSegments++ ] = uiIndex;
			}
		}
	}

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( int uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
		{
			GetSegments()[ uiIndex ]->SetCauseDamageOnTouch( m_bIsActive );
			GetAltSegments()[ uiIndex ]->SetCauseDamageOnTouch( m_bIsActive );
		}
	}

	if( m_iTipSparkFrequency > 0 )
	{
		GetSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
		GetAltSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
	}

	m_flLastSparkTime = gpGlobals->time;

	SetSoundAllowed( false );

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&CElectrifiedWire::ElectrifiedRopeThink);
}

void CElectrifiedWire::ElectrifiedRopeThink()
{
	if( gpGlobals->time - m_flLastSparkTime > 0.1 )
	{
		m_flLastSparkTime = gpGlobals->time;

		if( m_uiNumUninsulatedSegments > 0 )
		{
			for( int uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
			{
				if( ShouldDoEffect( m_iBodySparkFrequency ) )
				{
					DoSpark( m_uiUninsulatedSegments[ uiIndex ], false );
				}
			}
		}

		if( ShouldDoEffect( m_iTipSparkFrequency ) )
		{
			DoSpark( GetNumSegments() - 1, true );
		}

		if( ShouldDoEffect( m_iLightningFrequency ) )
			DoLightning();
	}

	CRope::RopeThink();
}

void CElectrifiedWire::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue )
{
	m_bIsActive = !m_bIsActive;

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( int uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
		{
			GetSegments()[ m_uiUninsulatedSegments[ uiIndex ] ]->SetCauseDamageOnTouch( m_bIsActive );
			GetAltSegments()[ m_uiUninsulatedSegments[ uiIndex ] ]->SetCauseDamageOnTouch( m_bIsActive );
		}
	}

	if( m_iTipSparkFrequency > 0 )
	{
		GetSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
		GetAltSegments()[ GetNumSegments() - 1 ]->SetCauseDamageOnTouch( m_bIsActive );
	}
}

bool CElectrifiedWire::ShouldDoEffect( const int iFrequency )
{
	if( iFrequency <= 0 )
		return false;

	if( !IsActive() )
		return false;

	return RANDOM_LONG( 1, iFrequency ) == 1;
}

void CElectrifiedWire::DoSpark( const int uiSegment, const bool bExertForce )
{
	const Vector vecOrigin = GetSegmentAttachmentPoint( uiSegment );

	UTIL_Sparks( vecOrigin );

	if( bExertForce )
	{
		const Vector vecSparkForce(
			RANDOM_FLOAT( -m_iXJoltForce, m_iXJoltForce ),
			RANDOM_FLOAT( -m_iYJoltForce, m_iYJoltForce ),
			RANDOM_FLOAT( -m_iZJoltForce, m_iZJoltForce )
		);

		ApplyForceToSegment( vecSparkForce, uiSegment );
	}
}

void CElectrifiedWire::DoLightning()
{
	const int uiSegment1 = RANDOM_LONG( 0, GetNumSegments() - 1 );

	int uiSegment2;

	int uiIndex;

	//Try to get a random segment.
	for( uiIndex = 0; uiIndex < 10; ++uiIndex )
	{
		uiSegment2 = RANDOM_LONG( 0, GetNumSegments() - 1 );

		if( uiSegment2 != uiSegment1 )
			break;
	}

	if( uiIndex >= 10 )
		return;

	CRopeSegment* pSegment1;
	CRopeSegment* pSegment2;

	if( GetToggleValue() )
	{
		pSegment1 = GetAltSegments()[ uiSegment1 ];
		pSegment2 = GetAltSegments()[ uiSegment2 ];
	}
	else
	{
		pSegment1 = GetSegments()[ uiSegment1 ];
		pSegment2 = GetSegments()[ uiSegment2 ];
	}

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTS );
		WRITE_SHORT( pSegment1->entindex() );
		WRITE_SHORT( pSegment2->entindex() );
		WRITE_SHORT( m_iLightningSprite );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 80 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
}
