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

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )

class CRopeSegment : public CBaseEntity
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);

	void EXPORT SegmentThink(void);
	void EXPORT SegmentTouch(CBaseEntity* pOther);

	CRopeSegment* m_pNext;
	CRopeSegment* m_pPrev;

	Vector m_vecJointPos;
};

LINK_ENTITY_TO_CLASS(rope_segment, CRopeSegment);

TYPEDESCRIPTION CRopeSegment::m_SaveData[] =
{
	DEFINE_FIELD(CRopeSegment, m_pNext, FIELD_CLASSPTR),
	DEFINE_FIELD(CRopeSegment, m_pPrev, FIELD_CLASSPTR),
	DEFINE_FIELD(CRopeSegment, m_vecJointPos, FIELD_POSITION_VECTOR),
};

IMPLEMENT_SAVERESTORE(CRopeSegment, CBaseEntity);


void CRopeSegment::Spawn(void)
{
	SET_MODEL(ENT(pev), "models/rope16.mdl");

	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_NONE;
	pev->gravity	= 0.0f;

	m_pPrev = NULL;
	m_pNext = NULL;
	m_vecJointPos = Vector(0, 0, 0);

	SetTouch(&CRopeSegment::Touch);
	SetThink(&CRopeSegment::SegmentThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRopeSegment::SegmentThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CRopeSegment::SegmentTouch(CBaseEntity* pOther)
{
	ALERT(at_console, "Touched segment!\n");

	SetTouch(NULL);
}




#define MAX_ROPE_SEGMENTS 64

class CRope : public CBaseEntity
{
public:

	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue(KeyValueData* pkvd);
	void Spawn(void);
	void Precache(void);

	void EXPORT StartupThink(void);
	void EXPORT RopeThink(void);

	void CreateSegments();
	void DestroySegments();

	int FindClosestSegment(Vector& vecTo, float epsilon, int iMin, int iMax);

	CRopeSegment*	m_pSegments[MAX_ROPE_SEGMENTS];

	int				m_nSegments;
	BOOL			m_fEnabled;

	string_t	m_iszBodyModel;
	string_t	m_iszEndingModel;

	void ApplyFunctor(void(*functor)(CRopeSegment* pSegment), int startIndex, int endIndex);
	void ApplyFunctor(void(*functor)(CRopeSegment* pSegment));
};

LINK_ENTITY_TO_CLASS(env_rope, CRope);

TYPEDESCRIPTION CRope::m_SaveData[] =
{
	DEFINE_ARRAY(CRope, m_pSegments, FIELD_CLASSPTR, MAX_ROPE_SEGMENTS),
	DEFINE_FIELD(CRope, m_nSegments, FIELD_INTEGER),
	DEFINE_FIELD(CRope, m_fEnabled, FIELD_BOOLEAN),
	DEFINE_FIELD(CRope, m_iszBodyModel, FIELD_STRING),
	DEFINE_FIELD(CRope, m_iszEndingModel, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CRope, CBaseEntity);

void CRope::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "segments"))
	{
		m_nSegments = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "disable"))
	{
		m_fEnabled = !atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "endingmodel"))
	{
		m_iszEndingModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bodymodel"))
	{
		m_iszBodyModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}

void CRope::Spawn(void)
{
	Precache();

	pev->solid		= SOLID_NOT;
	pev->movetype	= MOVETYPE_NONE;
	pev->effects	= 0;

	for (int i = 0; i < MAX_ROPE_SEGMENTS; i++)
		m_pSegments[i] = NULL;

	SetThink(&CRope::StartupThink);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CRope::Precache(void)
{

	PRECACHE_MODEL("models/rope16.mdl");
	PRECACHE_MODEL("models/rope24.mdl");
	PRECACHE_MODEL("models/rope32.mdl");

	UTIL_PrecacheOther("rope_segment");
}

void CRope::StartupThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	CreateSegments();

	SetThink(&CRope::RopeThink);
}

//===============================================
// Use binary search for closest point.
//===============================================
int CRope::FindClosestSegment(Vector& vecTo, float epsilon, int iMin, int iMax)
{
	if (iMax < iMin)
		return -1;
	else
	{
		int middle = iMin + ((iMax - iMin) / 2);

		if (vecTo.z > (m_pSegments[middle]->pev->origin.z + epsilon))
		{
			return FindClosestSegment(vecTo, epsilon, iMin, middle - 1); // Higher ropes are at vector head.
		}
		else if (vecTo.z < (m_pSegments[middle]->pev->origin.z - epsilon))
		{
			return FindClosestSegment(vecTo, epsilon, middle + 1, iMax); // Lower ropes are at vector tail.
		}
		else
		{
			return middle;
		}
	}
}

static void Functor_SetSegmentFxGlow(CRopeSegment* pSegment)
{
	pSegment->pev->renderfx = kRenderFxGlowShell;
}

static void Functor_SetSegmentFxNormal(CRopeSegment* pSegment)
{
	pSegment->pev->renderfx = kRenderFxNone;
}

void CRope::RopeThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	CBaseEntity* pEntity =  UTIL_PlayerByIndex(1);
	if (pEntity)
	{
		if ((pEntity->pev->origin - pev->origin).Length() > 384)
			return;

		ApplyFunctor(Functor_SetSegmentFxNormal);

		Vector v = ((CBasePlayer*)pEntity)->GetGunPosition();
		int index = FindClosestSegment(v, 16, 0, m_nSegments - 1);
		if (index >= 0 && index < m_nSegments)
		{
			CRopeSegment* pClosestSegment = m_pSegments[index];

			if (pClosestSegment)
			{
				pClosestSegment->SetTouch(&CRopeSegment::SegmentTouch);
				pClosestSegment->pev->renderfx = kRenderFxGlowShell;
				pClosestSegment->pev->rendercolor = Vector(255, 0, 0);
				ALERT(at_console, "Rope joint closest is %d at %f\n", index, pClosestSegment->pev->origin.z);
			}
		}
	}
}

//===============================================
// Create all rope segments.
//===============================================
void CRope::CreateSegments()
{
	CRopeSegment* pRopeSegment = NULL;
	Vector down = Vector(0, 0, -1);
	Vector angles = Vector(0, 0, -90);

	Vector joint;

	joint = pev->origin + (down * 16);

	m_pSegments[0] = (CRopeSegment*)CBaseEntity::Create(
		"rope_segment",
		joint,
		angles);

	m_pSegments[0]->m_vecJointPos = joint;

	m_pSegments[0]->m_pPrev = NULL;

	int i;
	for (i = 1; i < m_nSegments; i++)
	{
		joint = pev->origin + (down * 16 * i);

		m_pSegments[i] = (CRopeSegment*)CBaseEntity::Create("rope_segment", joint, angles);

		m_pSegments[i]->m_vecJointPos = joint;

		m_pSegments[i - 1]->m_pNext = m_pSegments[i];
		m_pSegments[i]->m_pPrev = m_pSegments[i - 1];
		m_pSegments[i]->m_pNext = NULL;
	}
}

//===============================================
// Destroy all rope segments
//===============================================
void CRope::DestroySegments(void)
{
	int i;
	for (i = 0; i < m_nSegments; i++)
	{
		if (m_pSegments[i])
		{
			UTIL_Remove(m_pSegments[i]);
			m_pSegments[i] = NULL;
		}
	}
}

//===============================================
// Apply a functor to a range of segments.
//===============================================
void CRope::ApplyFunctor(void(*functor)(CRopeSegment* pSegment), int startIndex, int endIndex)
{
	// No functor? Return.
	if (!functor)
		return;

	// To pass -1 as value for endindex means to apply the functor to all segments.
	int endindex = (endIndex != -1) ? endIndex : (m_nSegments - 1);

	// Apply functor to all segments.
	int i;
	for (i = 0; i <= endindex; i++)
	{
		// Apply functor.
		if (m_pSegments[i])
			functor(m_pSegments[i]);
	}
}

//===============================================
// Apply a functor to all segments.
//===============================================
void CRope::ApplyFunctor(void(*functor)(CRopeSegment* pSegment))
{
	ApplyFunctor(functor, 0, -1);
}
