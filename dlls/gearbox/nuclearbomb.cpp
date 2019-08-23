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
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

//=========================================================
// Nuclear bomb
//=========================================================

class CNuclearBombTimer : public CBaseEntity
{
public:
	void Precache();
	void Spawn();
	void EXPORT NuclearBombTimerThink();
	void SetNuclearBombTimer(bool on);
	int ObjectCaps() {return FCAP_DONT_SAVE;}

	BOOL bPlayBombSound;
	BOOL bBombSoundPlaying;
};

LINK_ENTITY_TO_CLASS(item_nuclearbombtimer, CNuclearBombTimer)

void CNuclearBombTimer::Precache()
{
	PRECACHE_MODEL("models/nuke_timer.mdl");
	PRECACHE_SOUND("common/nuke_ticking.wav");
}

void CNuclearBombTimer::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/nuke_timer.mdl");
	pev->solid = SOLID_NOT;
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb timer fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	pev->skin = 0;
	bPlayBombSound = FALSE;
	bBombSoundPlaying = FALSE;
}

void CNuclearBombTimer::NuclearBombTimerThink()
{
	if (pev->skin <= 1)
		pev->skin++;
	else
		pev->skin = 0;
	if (bPlayBombSound)
	{
		EMIT_SOUND(ENT(pev), CHAN_BODY, "common/nuke_ticking.wav", 0.75, ATTN_IDLE);
		bBombSoundPlaying = TRUE;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CNuclearBombTimer::SetNuclearBombTimer(bool on)
{
	if (on)
	{
		SetThink(&CNuclearBombTimer::NuclearBombTimerThink);
		pev->nextthink = gpGlobals->time;
		bPlayBombSound = TRUE;
	}
	else
	{
		SetThink(NULL);
		pev->nextthink = gpGlobals->time;
		pev->skin = 3;
		if (bBombSoundPlaying)
		{
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "common/nuke_ticking.wav", 0.0, 0.0, SND_STOP, PITCH_NORM);
			bBombSoundPlaying = FALSE;
		}
	}
}

class CNuclearBombButton : public CBaseEntity
{
public:
	void Precache();
	void Spawn();
	void SetNuclearBombButton(bool on);
	int ObjectCaps() {return FCAP_DONT_SAVE;}
};

LINK_ENTITY_TO_CLASS(item_nuclearbombbutton, CNuclearBombButton)

void CNuclearBombButton::Precache()
{
	PRECACHE_MODEL("models/nuke_button.mdl");
}

void CNuclearBombButton::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/nuke_button.mdl");
	pev->solid = SOLID_NOT;
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb button fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	pev->skin = 0;
}

void CNuclearBombButton::SetNuclearBombButton(bool on)
{
	pev->skin = 1;
}

class CNuclearBomb : public CBaseToggle
{
public:
	void Precache();
	void Spawn();
	void KeyValue(KeyValueData* pkvd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps() {return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;}
	void UpdateOnRemove();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fOn;
	float m_flLastPush;
	int m_iPushCount;
	CNuclearBombTimer* m_pTimer;
	CNuclearBombButton* m_pButton;
};

LINK_ENTITY_TO_CLASS(item_nuclearbomb, CNuclearBomb)

TYPEDESCRIPTION CNuclearBomb::m_SaveData[] =
{
	DEFINE_FIELD(CNuclearBomb, m_fOn, FIELD_BOOLEAN),
	DEFINE_FIELD(CNuclearBomb, m_flLastPush, FIELD_TIME),
	DEFINE_FIELD(CNuclearBomb, m_iPushCount, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CNuclearBomb, CBaseToggle)

void CNuclearBomb::Precache()
{
	PRECACHE_MODEL("models/nuke_case.mdl");
	UTIL_PrecacheOther("item_nuclearbombtimer");
	UTIL_PrecacheOther("item_nuclearbombbutton");
	PRECACHE_SOUND("buttons/button4.wav");
	PRECACHE_SOUND("buttons/button6.wav");

	m_pTimer = (CNuclearBombTimer*)Create("item_nuclearbombtimer", pev->origin, pev->angles);
	if (m_pTimer)
		m_pTimer->SetNuclearBombTimer(m_fOn);
	m_pButton = (CNuclearBombButton*)Create("item_nuclearbombbutton", pev->origin, pev->angles);
	if (m_pButton)
		m_pButton->SetNuclearBombButton(m_fOn);
}

void CNuclearBomb::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/nuke_case.mdl");
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,32));
	pev->movetype = MOVETYPE_NONE;
	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Nuclear Bomb fell out of level at %f,%f,%f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove( this );
		return;
	}
	m_iPushCount = 0;
	m_flLastPush = gpGlobals->time;
}

void CNuclearBomb::KeyValue(KeyValueData *pkvd)
{
	if( FStrEq( pkvd->szKeyName, "initialstate" ) )
	{
		m_fOn = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "wait" ) )
	{
		m_flWait = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pkvd);
}

void CNuclearBomb::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if ((m_flWait >= 0 || m_iPushCount <= 0) && m_flWait <= gpGlobals->time - m_flLastPush && ShouldToggle(useType, m_fOn))
	{
		const char* sound = NULL;
		if (m_fOn)
		{
			m_fOn = FALSE;
			sound = "buttons/button4.wav";
		}
		else
		{
			m_fOn = TRUE;
			sound = "buttons/button6.wav";
		}
		EMIT_SOUND(ENT(pev), CHAN_VOICE, sound, VOL_NORM, ATTN_NORM);
		SUB_UseTargets(pActivator, USE_TOGGLE, 0);
		if (m_pButton)
		{
			m_pButton->SetNuclearBombButton(m_fOn);
		}
		if (m_pTimer)
		{
			m_pTimer->SetNuclearBombTimer(m_fOn);
		}
		m_iPushCount++;
		m_flLastPush = gpGlobals->time;
	}
}

void CNuclearBomb::UpdateOnRemove()
{
	CBaseToggle::UpdateOnRemove();
	if (m_pTimer)
	{
		UTIL_Remove(m_pTimer);
		m_pTimer = NULL;
	}
	if (m_pButton)
	{
		UTIL_Remove(m_pButton);
		m_pButton = NULL;
	}
}
