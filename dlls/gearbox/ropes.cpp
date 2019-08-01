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


#define SetAbsOrigin(x) pev->origin = x;
#define SetAbsAngles(x) pev->angles = x;
#define SetAbsVelociy(x) pev->velocity = x;
#define SetNextThink(x) pev->nextthink = x;
#define SetEffects(x) pev->effects = x;
#define SetSolidType(x) pev->solid = x;
#define AddFlags(x) pev->flags |= x;
#define SetMoveType(x) pev->movetype = x;
#define AddEffectsFlags(x) pev->effects |= x;

/**
*	Represents a spring that keeps samples a given distance apart.
*/
struct Spring
{
	size_t p1;
	size_t p2;
	float restLength;
	float hookConstant;
	float springDampning;
};

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
};

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

	const RopeSampleData* GetData() const { return &data; }

	RopeSampleData* GetData() { return &data; }

	CRope* GetMasterRope() { return mMasterRope; }

	void SetMasterRope( CRope* pRope )
	{
		mMasterRope = pRope;
	}

private:
	RopeSampleData data;
	CRope* mMasterRope;
};


class CRopeSegment : public CBaseAnimating
{
public:

	void Precache();

	void Spawn();

	void Think();

	void Touch( CBaseEntity* pOther );

	static CRopeSegment* CreateSegment( CRopeSample* pSample, string_t iszModelName );

	CRopeSample* GetSample() { return m_Sample; }

	/**
	*	Applies external force to the segment.
	*	@param vecForce Force.
	*/
	void ApplyExternalForce( const Vector& vecForce );

	/**
	*	Resets the mass to the default value.
	*/
	void SetMassToDefault();

	/**
	*	Sets the default mass.
	*	@param flDefaultMass Mass.
	*/
	void SetDefaultMass( const float flDefaultMass );

	/**
	*	Sets the mass.
	*	@param flMass Mass.
	*/
	void SetMass( const float flMass );

	/**
	*	Sets whether the segment should cause damage on touch.
	*	@param bCauseDamage Whether to cause damage.
	*/
	void SetCauseDamageOnTouch( const bool bCauseDamage );

	/**
	*	Sets whether the segment can be grabbed.
	*	@param bCanBeGrabbed Whether the segment can be grabbed.
	*/
	void SetCanBeGrabbed( const bool bCanBeGrabbed );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];


private:
	CRopeSample* m_Sample;
	string_t mModelName;
	float mDefaultMass;
	bool mCauseDamage;
	bool mCanBeGrabbed;
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
	DEFINE_FIELD( CRope, m_HookConstant, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_SpringDampning, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_NumSamples, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, m_SpringCnt, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mObjectAttached, FIELD_CHARACTER ),
	DEFINE_FIELD( CRope, mAttachedObjectsSegment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, detachTime, FIELD_TIME ),
	DEFINE_ARRAY( CRope, seg, FIELD_CLASSPTR, MAX_SEGMENTS ),
	DEFINE_ARRAY( CRope, altseg, FIELD_CLASSPTR, MAX_SEGMENTS ),
	DEFINE_ARRAY( CRope, m_CurrentSys, FIELD_CLASSPTR, MAX_SAMPLES ),
	DEFINE_ARRAY( CRope, m_TargetSys, FIELD_CLASSPTR, MAX_SAMPLES ),
	DEFINE_FIELD( CRope, mDisallowPlayerAttachment, FIELD_INTEGER ),
	DEFINE_FIELD( CRope, mBodyModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mEndingModel, FIELD_STRING ),
	DEFINE_FIELD( CRope, mAttachedObjectsOffset, FIELD_FLOAT ),
	DEFINE_FIELD( CRope, m_bMakeSound, FIELD_CHARACTER ),
};

int CRope::Save( CSave &save )
{
	if( !CBaseDelay::Save( save ) )
		return 0;
	return save.WriteFields( "CBaseDelay", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CRope::Restore( CRestore &restore )
{
	if( !CBaseDelay::Restore( restore ) )
		return 0;
	int status = restore.ReadFields( "CBaseDelay", this, m_SaveData, ARRAYSIZE( m_SaveData ) );

	for( size_t uiIndex = 0; uiIndex < MAX_TEMP_SAMPLES; ++uiIndex )
	{
		delete[] m_TempSys[ uiIndex ];
		m_TempSys[ uiIndex ] = new RopeSampleData[ m_NumSamples ];

		memset( m_TempSys[ uiIndex ], 0, sizeof( RopeSampleData ) * m_NumSamples );
	}
	m_InitialDeltaTime = 0;
	m_Spring = NULL;

	return status;
}


LINK_ENTITY_TO_CLASS( env_rope, CRope )

/* leak
CRope::~CRope()
{
	for( size_t uiIndex = 0; uiIndex < MAX_TEMP_SAMPLES; ++uiIndex )
	{
		delete[] m_TempSys[ uiIndex ];
		m_TempSys[ uiIndex ] = NULL;
	}
}*/

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

	PRECACHE_SOUND_ARRAY( g_pszCreakSounds );
}

void CRope::Spawn()
{
	pev->classname = MAKE_STRING( "env_rope" );

	m_bMakeSound = true;

	Precache();

	mSpringsInitialized = false;

	m_Gravity.x = m_Gravity.y = 0;
	m_Gravity.z = -50;

	mObjectAttached = false;

	AddFlags( FL_ALWAYSTHINK );
	m_NumSamples = m_iSegments + 1;

	for( size_t uiSample = 0; uiSample < m_NumSamples; ++uiSample )
	{
		m_CurrentSys[ uiSample ] = CRopeSample::CreateSample();

		m_CurrentSys[ uiSample ]->SetMasterRope( this );
	}

	memset( m_CurrentSys + m_NumSamples, 0, sizeof( CRopeSample* ) * ( MAX_SAMPLES - m_NumSamples ) );

	{
		CRopeSegment* pSegment = seg[ 0 ] = CRopeSegment::CreateSegment( m_CurrentSys[ 0 ], GetBodyModel() );

		pSegment->SetAbsOrigin( pev->origin );

		pSegment = altseg[ 0 ] = CRopeSegment::CreateSegment( m_CurrentSys[ 0 ], GetBodyModel() );

		pSegment->SetAbsOrigin( pev->origin );
	}

	Vector origin;
	Vector angles;

	const Vector vecGravity = m_Gravity.Normalize();

	if( m_iSegments > 2 )
	{
		CRopeSample** ppCurrentSys = m_CurrentSys;

		for( size_t uiSeg = 1; uiSeg < m_iSegments - 1; ++uiSeg )
		{
			CRopeSample* pSegSample = m_CurrentSys[ uiSeg ];
			seg[ uiSeg ] = CRopeSegment::CreateSegment( pSegSample, GetBodyModel() );

			altseg[ uiSeg ] = CRopeSegment::CreateSegment( pSegSample, GetBodyModel() );

			CRopeSegment* pCurrent = seg[ uiSeg - 1 ];

			pCurrent->GetAttachment( 0, origin, angles );

			Vector vecPos = origin - pCurrent->pev->origin;

			const float flLength = vecPos.Length();

			origin = flLength * vecGravity + pCurrent->pev->origin;

			seg[ uiSeg ]->SetAbsOrigin( origin );
			altseg[ uiSeg ]->SetAbsOrigin( origin );
		}
	}

	CRopeSample* pSegSample = m_CurrentSys[ m_iSegments - 1 ];
	seg[ m_iSegments - 1 ] = CRopeSegment::CreateSegment( pSegSample, GetEndingModel() );

	altseg[ m_iSegments - 1 ] = CRopeSegment::CreateSegment( pSegSample, GetEndingModel() );

	CRopeSegment* pCurrent = seg[ m_iSegments - 2 ];

	pCurrent->GetAttachment( 0, origin, angles );

	Vector vecPos = origin - pCurrent->pev->origin;

	const float flLength = vecPos.Length();

	origin = flLength * vecGravity + pCurrent->pev->origin;

	seg[ m_iSegments - 1 ]->SetAbsOrigin( origin );
	altseg[ m_iSegments - 1 ]->SetAbsOrigin( origin );

	memset( seg + m_iSegments, 0, sizeof( CRopeSegment* ) * ( MAX_SEGMENTS - m_iSegments ) );
	memset( altseg + m_iSegments, 0, sizeof( CRopeSegment* ) * ( MAX_SEGMENTS - m_iSegments ) );

	memset( m_TempSys, 0, sizeof( m_TempSys ) );

	m_SpringCnt = 0;

	m_InitialDeltaTime = true;
	m_HookConstant = 2500;
	m_SpringDampning = 0.1;

	InitializeRopeSim();

	SetNextThink( gpGlobals->time + 0.01 );
}

void CRope::Think()
{
	if( !mSpringsInitialized )
	{
		InitializeSprings( m_iSegments );
	}

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

	SetNextThink( gpGlobals->time + 0.001 );
}

void CRope::Touch( CBaseEntity* pOther )
{
	//Nothing.
}

void CRope::InitializeRopeSim()
{
	size_t uiIndex;
	for( uiIndex = 0; uiIndex < MAX_TEMP_SAMPLES; ++uiIndex )
	{
		delete[] m_TempSys[ uiIndex ];
		m_TempSys[ uiIndex ] = NULL;
	}

	for( size_t uiSample = 0; uiSample < m_NumSamples; ++uiSample )
	{
		m_TargetSys[ uiSample ] = CRopeSample::CreateSample();

		m_TargetSys[ uiSample ]->SetMasterRope( this );
	}

	memset( m_TargetSys + m_NumSamples, 0, sizeof( CRopeSample* ) * ( MAX_SAMPLES - m_NumSamples ) );

	for( uiIndex = 0; uiIndex < MAX_TEMP_SAMPLES; ++uiIndex )
	{
		m_TempSys[ uiIndex ] = new RopeSampleData[ m_NumSamples ];

		memset( m_TempSys[ uiIndex ], 0, sizeof( RopeSampleData ) * m_NumSamples );
	}

	for( size_t uiSeg = 0; uiSeg < m_iSegments; ++uiSeg )
	{
		CRopeSegment* pSegment = seg[ uiSeg ];
		CRopeSample* pSample = pSegment->GetSample();

		RopeSampleData *data = pSample->GetData();

		data->mPosition = pSegment->pev->origin;

		data->mVelocity				= g_vecZero;
		data->mForce					= g_vecZero;
		data->mMassReciprocal		= 1;
		data->mApplyExternalForce	= false;
		data->mExternalForce			= g_vecZero;

		pSegment->SetDefaultMass( data->mMassReciprocal );
	}

	{
		//Zero out the anchored segment's mass so it stays in place.
		CRopeSample *pSample = m_CurrentSys[ 0 ];

		pSample->GetData()->mMassReciprocal = 0;
	}

	CRopeSegment* pSegment = seg[ m_iSegments - 1 ];

	Vector vecOrigin, vecAngles;

	pSegment->GetAttachment( 0, vecOrigin, vecAngles );

	Vector vecDistance = vecOrigin - pSegment->pev->origin;

	const float flLength = vecDistance.Length();

	const Vector vecGravity = m_Gravity.Normalize();

	vecOrigin = vecGravity * flLength + pSegment->pev->origin;

	CRopeSample* pSample = m_CurrentSys[ m_NumSamples - 1 ];

	RopeSampleData *data = pSample->GetData();

	data->mPosition = vecOrigin;

	m_LastEndPos = vecOrigin;

	data->mVelocity = g_vecZero;

	data->mForce = g_vecZero;

	data->mMassReciprocal = 0.2;

	data->mApplyExternalForce = false;

	size_t uiNumSegs = 4;

	if( m_iSegments <= 4 )
		uiNumSegs = m_iSegments;

	for( uiIndex = 0; uiIndex < uiNumSegs; ++uiIndex )
	{
		seg[ uiIndex ]->SetCanBeGrabbed( false );
		altseg[ uiIndex ]->SetCanBeGrabbed( false );
	}
}

void CRope::InitializeSprings( const size_t uiNumSprings )
{
	m_SpringCnt = uiNumSprings;

	m_Spring = new Spring[ uiNumSprings ];

	if( uiNumSprings > 0 )
	{
		Vector vecOrigin, vecAngles;

		for( size_t uiIndex = 0; uiIndex < m_SpringCnt; ++uiIndex )
		{
			Spring& spring = m_Spring[ uiIndex ];

			spring.p1 = uiIndex;
			spring.p2 = uiIndex + 1;

			if( uiIndex < m_iSegments )
			{
				CRopeSegment* pSegment = seg[ uiIndex ];

				pSegment->GetAttachment( 0, vecOrigin, vecAngles );

				spring.restLength = ( pSegment->pev->origin - vecOrigin ).Length();
			}
			else
				spring.restLength = 0;

			spring.hookConstant = m_HookConstant;
			spring.springDampning = m_SpringDampning;
		}
	}

	mSpringsInitialized = true;
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

	size_t uiIndex = 0;

	CRopeSample** ppSampleSource = m_CurrentSys;
	CRopeSample** ppSampleTarget = m_TargetSys;

	while( true )
	{
		++uiIndex;

		ComputeForces( ppSampleSource );
		RK4Integrate( flDeltaTime, ppSampleSource, ppSampleTarget );

		mLastTime += 0.007;

		if( gpGlobals->time <= mLastTime )
		{
			if( ( uiIndex % 2 ) != 0 )
				break;
		}
		CRopeSample **swap = ppSampleSource;
		ppSampleSource = ppSampleTarget;
		ppSampleTarget = swap;

		//std::swap( ppSampleSource, ppSampleTarget );

	}

	mLastTime = gpGlobals->time;
}

void CRope::ComputeForces( RopeSampleData* pSystem )
{
	size_t uiIndex;
	for( uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex )
	{
		ComputeSampleForce( pSystem[ uiIndex ] );
	}

	Spring* pSpring = m_Spring;

	for( uiIndex = 0; uiIndex < m_SpringCnt; ++uiIndex, ++pSpring )
	{
		ComputeSpringForce( pSystem[ pSpring->p1 ], pSystem[ pSpring->p2 ], *pSpring );
	}
}

void CRope::ComputeForces( CRopeSample** ppSystem )
{
	size_t uiIndex;
	for( uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex )
	{
		ComputeSampleForce( *ppSystem[ uiIndex ]->GetData() );
	}

	Spring* pSpring = m_Spring;

	for( uiIndex = 0; uiIndex < m_SpringCnt; ++uiIndex, ++pSpring )
	{
		ComputeSpringForce( *ppSystem[ pSpring->p1 ]->GetData(), *ppSystem[ pSpring->p2 ]->GetData(), *pSpring );
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

void CRope::ComputeSpringForce( RopeSampleData& first, RopeSampleData& second, const Spring& spring )
{
	Vector vecDist = first.mPosition - second.mPosition;

	const double flDistance = vecDist.Length();

	const double flForce = ( flDistance - spring.restLength ) * spring.hookConstant;

	const double flNewRelativeDist = DotProduct( first.mVelocity - second.mVelocity, vecDist ) * spring.springDampning;

	vecDist = vecDist.Normalize();

	const double flSpringFactor = -( flNewRelativeDist / flDistance + flForce );

	const Vector vecForce = flSpringFactor * vecDist;

	first.mForce = first.mForce + vecForce;

	second.mForce = second.mForce - vecForce;
}

void CRope::RK4Integrate( const float flDeltaTime, CRopeSample** ppSampleSource, CRopeSample** ppSampleTarget )
{
	const float flDeltas[ MAX_TEMP_SAMPLES - 1 ] =
	{
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime * 0.5f,
		flDeltaTime
	};

	{
		RopeSampleData* pTemp1 = m_TempSys[ 0 ];
		RopeSampleData* pTemp2 = m_TempSys[ 1 ];

		for( size_t uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData *data = ppSampleSource[ uiIndex ]->GetData();

			pTemp2->mForce = data->mMassReciprocal * data->mForce * flDeltas[ 0 ];

			pTemp2->mVelocity = data->mVelocity * flDeltas[ 0 ];

			pTemp1->mMassReciprocal = data->mMassReciprocal;
			pTemp1->mVelocity = data->mVelocity + pTemp2->mForce;
			pTemp1->mPosition = data->mPosition + pTemp2->mVelocity;
		}

		ComputeForces( m_TempSys[ 0 ] );
	}

	for( size_t uiStep = 2; uiStep < MAX_TEMP_SAMPLES - 1; ++uiStep )
	{
		RopeSampleData* pTemp1 = m_TempSys[ 0 ];
		RopeSampleData* pTemp2 = m_TempSys[ uiStep ];

		for( size_t uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData *data = ppSampleSource[ uiIndex ]->GetData();

			pTemp2->mForce = data->mMassReciprocal * pTemp1->mForce * flDeltas[ uiStep - 1 ];

			pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[ uiStep - 1 ];

			pTemp1->mMassReciprocal = data->mMassReciprocal;
			pTemp1->mVelocity = data->mVelocity + pTemp2->mForce;
			pTemp1->mPosition = data->mPosition + pTemp2->mVelocity;
		}

		ComputeForces( m_TempSys[ 0 ] );
	}

	{
		RopeSampleData* pTemp1 = m_TempSys[ 0 ];
		RopeSampleData* pTemp2 = m_TempSys[ 4 ];

		for( size_t uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2 )
		{
			RopeSampleData *data = ppSampleSource[ uiIndex ]->GetData();

			pTemp2->mForce = data->mMassReciprocal * pTemp1->mForce * flDeltas[ 3 ];

			pTemp2->mVelocity = pTemp1->mVelocity * flDeltas[ 3 ];
		}
	}

	RopeSampleData* pTemp1 = m_TempSys[ 1 ];
	RopeSampleData* pTemp2 = m_TempSys[ 2 ];
	RopeSampleData* pTemp3 = m_TempSys[ 3 ];
	RopeSampleData* pTemp4 = m_TempSys[ 4 ];

	for( size_t uiIndex = 0; uiIndex < m_NumSamples; ++uiIndex, ++pTemp1, ++pTemp2, ++pTemp3, ++pTemp4 )
	{
		CRopeSample *pSource = ppSampleSource[ uiIndex ];
		CRopeSample *pTarget = ppSampleTarget[ uiIndex ];

		const Vector vecPosChange = 1.0f / 6.0f * ( pTemp1->mVelocity + ( pTemp2->mVelocity + pTemp3->mVelocity ) * 2 + pTemp4->mVelocity );

		const Vector vecVelChange = 1.0f / 6.0f * ( pTemp1->mForce + ( pTemp2->mForce + pTemp3->mForce ) * 2 + pTemp4->mForce );

		pTarget->GetData()->mPosition = pSource->GetData()->mPosition + ( vecPosChange );//* flDeltaTime );

		pTarget->GetData()->mVelocity = pSource->GetData()->mVelocity + ( vecVelChange );//* flDeltaTime );
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
			m_CurrentSys[ 0 ]->GetData()->mPosition,
			m_CurrentSys[ 1 ]->GetData()->mPosition,
			vecAngles );

		( *ppPrimarySegs )->SetAbsAngles( vecAngles );
	}

	TraceResult tr;

	if( mObjectAttached )
	{
		for( size_t uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
		{
			CRopeSample* pSample = m_CurrentSys[ uiSeg ];

			Vector vecDist = pSample->GetData()->mPosition - ppHiddenSegs[ uiSeg ]->pev->origin;

			vecDist = vecDist.Normalize();

			const float flTraceDist = ( uiSeg - mAttachedObjectsSegment + 2 ) < 5 ? 50 : 10;

			const Vector vecTraceDist = vecDist * flTraceDist;

			const Vector vecEnd = pSample->GetData()->mPosition + vecTraceDist;

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

				RopeSampleData *data = ppPrimarySegs[ uiSeg ]->GetSample()->GetData();

				data->mApplyExternalForce = true;

				data->mExternalForce = vecNormal;

				data->mVelocity = g_vecZero;
			}
			else
			{
				Vector vecOrigin = pSample->GetData()->mPosition;

				TruncateEpsilon( vecOrigin );

				ppPrimarySegs[ uiSeg ]->SetAbsOrigin( vecOrigin );
			}
		}
	}
	else
	{
		for( size_t uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
		{
			UTIL_TraceLine(
				ppHiddenSegs[ uiSeg ]->pev->origin,
				m_CurrentSys[ uiSeg ]->GetData()->mPosition,
				ignore_monsters, edict(), &tr );

			if( tr.flFraction == 1.0 )
			{
				Vector vecOrigin = m_CurrentSys[ uiSeg ]->GetData()->mPosition;

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

				ppPrimarySegs[ uiSeg ]->GetSample()->GetData()->mApplyExternalForce = true;

				ppPrimarySegs[ uiSeg ]->GetSample()->GetData()->mExternalForce = vecNormal * 40000.0;
			}
		}
	}

	Vector vecAngles;

	for( size_t uiSeg = 1; uiSeg < m_iSegments; ++uiSeg )
	{
		CRopeSegment *pSegment = ppPrimarySegs[ uiSeg - 1 ];
		CRopeSegment *pSegment2 = ppPrimarySegs[ uiSeg ];

		GetAlignmentAngles( pSegment->pev->origin, pSegment2->pev->origin, vecAngles );

		pSegment->SetAbsAngles( vecAngles );
	}

	if( m_iSegments > 1 )
	{
		CRopeSample *pSample = m_CurrentSys[ m_NumSamples - 1 ];

		UTIL_TraceLine( m_LastEndPos, pSample->GetData()->mPosition, ignore_monsters, edict(), &tr );

		if( tr.flFraction == 1.0 )
		{
			m_LastEndPos = pSample->GetData()->mPosition;
		}
		else
		{
			m_LastEndPos = tr.vecEndPos;

			pSample->GetData()->mApplyExternalForce = true;

			pSample->GetData()->mExternalForce = tr.vecPlaneNormal.Normalize() * 40000.0;
		}

		CRopeSegment *pSegment = ppPrimarySegs[ m_NumSamples - 2 ];

		Vector vecAngles;

		GetAlignmentAngles( pSegment->pev->origin, m_LastEndPos, vecAngles );

		pSegment->SetAbsAngles( vecAngles );
	}
}

void CRope::SetRopeSegments( const size_t uiNumSegments,
							 CRopeSegment** ppPrimarySegs, CRopeSegment** ppHiddenSegs )
{
	if( uiNumSegments > 0 )
	{
		TraceModels( ppPrimarySegs, ppHiddenSegs );

		ppPrimarySegs[ 0 ]->SetSolidType( SOLID_TRIGGER );
		ppPrimarySegs[ 0 ]->SetEffects( 0 );

		ppHiddenSegs[ 0 ]->SetSolidType( SOLID_NOT );
		ppHiddenSegs[ 0 ]->SetEffects( EF_NODRAW );

		for( size_t uiIndex = 1; uiIndex < uiNumSegments; ++uiIndex )
		{
			CRopeSegment* pPrim = ppPrimarySegs[ uiIndex ];
			CRopeSegment* pHidden = ppHiddenSegs[ uiIndex ];

			pPrim->SetSolidType( SOLID_TRIGGER );
			pPrim->SetEffects( 0 );

			pHidden->SetSolidType( SOLID_NOT );
			pHidden->SetEffects( EF_NODRAW );

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

	return seg[ mAttachedObjectsSegment ]->GetSample()->GetData()->mVelocity;
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

void CRope::ApplyForceToSegment( const Vector& vecForce, const size_t uiSegment )
{
	if( uiSegment < m_iSegments )
	{
		seg[ uiSegment ]->ApplyExternalForce( vecForce );
	}
	else if( uiSegment == m_iSegments )
	{
		//Apply force to the last sample.

		RopeSampleData *data = m_CurrentSys[ uiSegment - 1 ]->GetData();

		data->mExternalForce = data->mExternalForce + vecForce;

		data->mApplyExternalForce = true;
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

		if( pSample->GetData()->mVelocity.Length() > 20.0 )
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

float CRope::GetSegmentLength( size_t uiSegmentIndex ) const
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

	for( size_t uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		CRopeSegment *pSegment = seg[ uiIndex ];

		pSegment->GetAttachment( 0, vecOrigin, vecAngles );

		flLength += ( pSegment->pev->origin - vecOrigin ).Length();
	}

	return flLength;
}

Vector CRope::GetRopeOrigin() const
{
	return m_CurrentSys[ 0 ]->GetData()->mPosition;
}

bool CRope::IsValidSegmentIndex( const size_t uiSegment ) const
{
	return uiSegment < m_iSegments;
}

Vector CRope::GetSegmentOrigin( const size_t uiSegment ) const
{
	if( !IsValidSegmentIndex( uiSegment ) )
		return g_vecZero;

	return m_CurrentSys[ uiSegment ]->GetData()->mPosition;
}

Vector CRope::GetSegmentAttachmentPoint( const size_t uiSegment ) const
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
	for( size_t uiIndex = 0; uiIndex < m_iSegments; ++uiIndex )
	{
		if( seg[ uiIndex ] == pSegment || altseg[ uiIndex ] == pSegment )
		{
			mAttachedObjectsSegment = uiIndex;
			break;
		}
	}
}

Vector CRope::GetSegmentDirFromOrigin( const size_t uiSegmentIndex ) const
{
	if( uiSegmentIndex >= m_iSegments )
		return g_vecZero;

	//There is one more sample than there are segments, so this is fine.
	const Vector vecResult =
		m_CurrentSys[ uiSegmentIndex + 1 ]->GetData()->mPosition -
		m_CurrentSys[ uiSegmentIndex ]->GetData()->mPosition;

	return vecResult.Normalize();
}

Vector CRope::GetAttachedObjectsPosition() const
{
	if( !mObjectAttached )
		return g_vecZero;

	Vector vecResult;

	if( mAttachedObjectsSegment < m_iSegments )
		vecResult = m_CurrentSys[ mAttachedObjectsSegment ]->GetData()->mPosition;

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
	DEFINE_FIELD( CRopeSample, mMasterRope, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE(CRopeSample, CBaseEntity)

LINK_ENTITY_TO_CLASS( rope_sample, CRopeSample );

void CRopeSample::Spawn()
{
	pev->classname = MAKE_STRING( "rope_sample" );

	AddEffectsFlags( EF_NODRAW );
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
};
IMPLEMENT_SAVERESTORE( CRopeSegment, CBaseAnimating )

LINK_ENTITY_TO_CLASS( rope_segment, CRopeSegment );

void CRopeSegment::Precache()
{
	CBaseAnimating::Precache();
	if( !mModelName )
		mModelName = MAKE_STRING( "models/rope16.mdl" );

	PRECACHE_MODEL( (char*)STRING( mModelName ) );
	PRECACHE_SOUND( "items/grab_rope.wav" );
}

void CRopeSegment::Spawn()
{
	pev->classname = MAKE_STRING( "rope_segment" );

	Precache();

	SET_MODEL( edict(), STRING( mModelName ) );

	SetMoveType( MOVETYPE_NOCLIP );
	SetSolidType( SOLID_TRIGGER );
	AddFlags( FL_ALWAYSTHINK );
	SetEffects( EF_NODRAW );
	SetAbsOrigin( pev->origin );

	UTIL_SetSize( pev, Vector( -30, -30, -30 ), Vector( 30, 30, 30 ) );

	SetNextThink( gpGlobals->time + 0.5 );
}

void CRopeSegment::Think()
{
	//Do nothing.
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

		if( m_Sample->GetMasterRope()->IsAcceptingAttachment() && !(pPlayer->m_afPhysicsFlags & PFLAG_ONROPE) )
		{
			if( mCanBeGrabbed )
			{
				RopeSampleData *data = m_Sample->GetData();

				pOther->SetAbsOrigin( data->mPosition );

				pPlayer->SetOnRopeState( true );
				pPlayer->SetRope( m_Sample->GetMasterRope() );
				m_Sample->GetMasterRope()->AttachObjectToSegment( this );

				const Vector& vecVelocity = pOther->pev->velocity;

				if( vecVelocity.Length() > 0.5 )
				{
					//Apply some external force to move the rope. - Solokiller
					data->mApplyExternalForce = true;

					data->mExternalForce = data->mExternalForce + vecVelocity * 750;
				}

				if( m_Sample->GetMasterRope()->IsSoundAllowed() )
				{
					EMIT_SOUND( edict(), CHAN_BODY, "items/grab_rope.wav", 1.0, ATTN_NORM );
				}
			}
			else
			{
				//This segment cannot be grabbed, so grab the highest one if possible. - Solokiller
				CRope *pRope = m_Sample->GetMasterRope();

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

CRopeSegment* CRopeSegment::CreateSegment( CRopeSample* pSample, string_t iszModelName )
{
	CRopeSegment* pSegment = GetClassPtr<CRopeSegment>( NULL );

	pSegment->mModelName = iszModelName;

	pSegment->Spawn();

	pSegment->m_Sample = pSample;

	pSegment->mCauseDamage = false;
	pSegment->mCanBeGrabbed = true;
	pSegment->mDefaultMass = pSample->GetData()->mMassReciprocal;

	return pSegment;
}

void CRopeSegment::ApplyExternalForce( const Vector& vecForce )
{
	m_Sample->GetData()->mApplyExternalForce = true;

	m_Sample->GetData()->mExternalForce = m_Sample->GetData()->mExternalForce + vecForce;
}

void CRopeSegment::SetMassToDefault()
{
	m_Sample->GetData()->mMassReciprocal = mDefaultMass;
}

void CRopeSegment::SetDefaultMass( const float flDefaultMass )
{
	mDefaultMass = flDefaultMass;
}

void CRopeSegment::SetMass( const float flMass )
{
	m_Sample->GetData()->mMassReciprocal = flMass;
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
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData* pkvd );

	void Precache();

	void Spawn();

	void Think();

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
	void DoSpark( const size_t uiSegment, const bool bExertForce );

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

	size_t m_uiNumUninsulatedSegments;
	size_t m_uiUninsulatedSegments[ MAX_SEGMENTS ];

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

	m_uiNumUninsulatedSegments = 0;
	m_bIsActive = true;

	if( m_iBodySparkFrequency > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < GetNumSegments(); ++uiIndex )
		{
			if( IsValidSegmentIndex( uiIndex ) )
			{
				m_uiUninsulatedSegments[ m_uiNumUninsulatedSegments++ ] = uiIndex;
			}
		}
	}

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
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
}

void CElectrifiedWire::Think()
{
	if( gpGlobals->time - m_flLastSparkTime > 0.1 )
	{
		m_flLastSparkTime = gpGlobals->time;

		if( m_uiNumUninsulatedSegments > 0 )
		{
			for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
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

	CRope::Think();
}

void CElectrifiedWire::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float flValue )
{
	m_bIsActive = !m_bIsActive;

	if( m_uiNumUninsulatedSegments > 0 )
	{
		for( size_t uiIndex = 0; uiIndex < m_uiNumUninsulatedSegments; ++uiIndex )
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

void CElectrifiedWire::DoSpark( const size_t uiSegment, const bool bExertForce )
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
	const size_t uiSegment1 = RANDOM_LONG( 0, GetNumSegments() - 1 );

	size_t uiSegment2;

	size_t uiIndex;

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
