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
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "xen.h"

class CSporeFruit : public CActAnimating
{
public:
	void		Spawn(void);
	void		Precache(void);
	void		Touch(CBaseEntity *pOther);
	void		Think(void);

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fAmmoReady;

private:

};

LINK_ENTITY_TO_CLASS(spore_fruit, CSporeFruit);

TYPEDESCRIPTION	CSporeFruit::m_SaveData[] =
{
	DEFINE_FIELD(CSporeFruit, m_fAmmoReady, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CSporeFruit, CActAnimating);

void CSporeFruit::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/spore_ammo.mdl");
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 8));
	SetActivity(ACT_IDLE);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;

	// idle
	// spawnup
	// snatchup
	// spawndn
	// snatchdn
	// idle2
	// idle3

	int i;
	int seq = LookupSequence("idle");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("spawnup");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("snatchup");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("spawndn");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("snatchdn");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("idle2");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

	seq = LookupSequence("idle3");
	if (seq != -1)
	{
		for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
		{
			if (seq == LookupActivity(i))
			{
				break;
			}
		}
	}

}

void CSporeFruit::Precache(void)
{
	PRECACHE_MODEL("models/spore_ammo.mdl");
}

void CSporeFruit::Think(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	switch (GetActivity())
	{
	case ACT_CROUCH:
		if (m_fSequenceFinished)
		{
			SetActivity(ACT_CROUCHIDLE);
		}
		break;

	case ACT_CROUCHIDLE:
		if (gpGlobals->time > pev->dmgtime)
		{
			SetActivity(ACT_STAND);
		}
		break;

	case ACT_STAND:
		if (m_fSequenceFinished)
			SetActivity(ACT_IDLE);
		break;

	case ACT_IDLE:
	default:
		break;
	}
}


void CSporeFruit::Touch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		if (GetActivity() == ACT_IDLE || GetActivity() == ACT_STAND)
		{
			SetActivity(ACT_CROUCH);
		}
	}
}