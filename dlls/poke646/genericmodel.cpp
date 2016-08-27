/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
#include "animation.h"
#include "weapons.h"
#include "player.h"

class CGenericModel : public CBaseAnimating
{
public:

	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);

	void EXPORT IdleThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	string_t m_iszSequence;
};

LINK_ENTITY_TO_CLASS(model_generic, CGenericModel);

TYPEDESCRIPTION	CGenericModel::m_SaveData[] =
{
	DEFINE_FIELD(CGenericModel, m_iszSequence, FIELD_STRING),
};

IMPLEMENT_SAVERESTORE(CGenericModel, CBaseAnimating);

void CGenericModel::KeyValue(KeyValueData* pkvd)
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		m_iszSequence = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CGenericModel::Spawn(void)
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetSize( pev, Vector(0, 0, 0), Vector(0, 0, 0) );

	if (!FStringNull(m_iszSequence))
	{
		pev->sequence = LookupSequence(STRING(m_iszSequence));

		if (pev->sequence < 0)
		{
			ALERT(at_warning, "Generic model %s: Unknown sequence named: %s\n", STRING(pev->model), STRING(m_iszSequence));
			pev->sequence = 0;
		}
	}
	else
	{
		pev->sequence = 0;
	}

	pev->frame = 0;
	pev->framerate = 1.0f;

	SetThink(&CGenericModel::IdleThink);
	SetTouch(NULL);
	SetUse(NULL);

	pev->nextthink = gpGlobals->time + 0.1;
}

void CGenericModel::Precache(void)
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CGenericModel::IdleThink(void)
{
	float flInterval = StudioFrameAdvance();

	pev->nextthink = gpGlobals->time + 0.5;

	DispatchAnimEvents(flInterval);

	if (m_fSequenceFinished)
	{
		if (m_fSequenceLoops)
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
		else
		{
			SetThink( NULL );
		}
	}
}