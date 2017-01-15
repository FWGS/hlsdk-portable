/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"schedule.h"

//=========================================================
// Generic item
//=========================================================
class CItemGeneric : public CBaseAnimating
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	void Precache(void);
	void KeyValue(KeyValueData* pkvd);

	void EXPORT StartupThink(void);
	void EXPORT SequenceThink(void);

	string_t m_iszSequenceName;
};

LINK_ENTITY_TO_CLASS(item_generic, CItemGeneric);

TYPEDESCRIPTION CItemGeneric::m_SaveData[] = 
{
	DEFINE_FIELD(CItemGeneric, m_iszSequenceName, FIELD_STRING),
};
IMPLEMENT_SAVERESTORE(CItemGeneric, CBaseAnimating);

void CItemGeneric::Spawn(void)
{
	Precache();
	SET_MODEL(ENT(pev),(char*)STRING(pev->model));

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 32));

	pev->takedamage	 = DAMAGE_NO;
	pev->solid		 = SOLID_BBOX;
	pev->sequence	 = -1;

	// Call startup sequence to look for a sequence to play.
	SetThink(&CItemGeneric::StartupThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CItemGeneric::Precache(void)
{
	PRECACHE_MODEL((char*)STRING(pev->model));
}

void CItemGeneric::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "sequencename"))
	{
		m_iszSequenceName = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue(pkvd);
}

void CItemGeneric::StartupThink(void)
{
	// Try to look for a sequence to play.
	int iSequence = -1;
	iSequence = LookupSequence(STRING(m_iszSequenceName));

	// Validate sequence.
	if (iSequence != -1)
	{
		pev->sequence = iSequence;
		SetThink(&CItemGeneric::SequenceThink);
		pev->nextthink = gpGlobals->time + 0.01f;
	}
	else
	{
		// Cancel play sequence.
		SetThink(NULL);
	}
}

void CItemGeneric::SequenceThink(void)
{
	// Set next think time.
	pev->nextthink = gpGlobals->time + 0.1f;

	// Advance frames and dispatch events.
	StudioFrameAdvance();
	DispatchAnimEvents();

	// Restart sequence 
	if (m_fSequenceFinished)
	{
		pev->frame = 0;
		ResetSequenceInfo();

		if (!m_fSequenceLoops)
		{
			// Prevent from calling ItemThink.
			SetThink(NULL);
			m_fSequenceFinished = TRUE;
			return;
		}
		else
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}
}