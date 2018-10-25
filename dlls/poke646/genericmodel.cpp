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

#define SF_MODEL_ANIMATE	1

class CGenericModel : public CBaseAnimating
{
public:

	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);

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
	if( FStrEq( pkvd->szKeyName, "sequencename" ) )
	{
		m_iszSequence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue(pkvd);
}

void CGenericModel::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	if( FBitSet( pev->spawnflags, SF_MODEL_ANIMATE ) )
	{
		pev->sequence = LookupSequence( STRING( m_iszSequence ) );
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0f;
	}
}

void CGenericModel::Precache()
{
	PRECACHE_MODEL( STRING( pev->model ) );
}
