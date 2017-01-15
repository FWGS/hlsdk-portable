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
/*

===== gearbox_triggers.cpp ========================================================

spawn and use functions for editor-placed triggers

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"
#include "gamerules.h"
#include "triggers.h"

//=========================================================
// CTriggerXenReturn
//=========================================================

class CTriggerXenReturn : public CTriggerTeleport
{
public:
	void Spawn(void);
	void EXPORT TeleportTouch(CBaseEntity *pOther);
};


LINK_ENTITY_TO_CLASS(trigger_xen_return, CTriggerXenReturn);


void CTriggerXenReturn::Spawn(void)
{
	CTriggerTeleport::Spawn();

	SetTouch(&CTriggerXenReturn::TeleportTouch);
}

void CTriggerXenReturn::TeleportTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	// Only teleport monsters or clients
	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (!(pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS))
	{// no monsters allowed!
		if (FBitSet(pevToucher->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS))
	{// no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_CLASSNAME(pentTarget, "info_displacer_earth_target");
	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin(pevToucher, tmp);

	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;

	if (pOther->IsPlayer())
	{
		// Ensure the current player is marked as being
		// on earth.
		((CBasePlayer*)pOther)->m_fInXen = FALSE;

		// Reset gravity to default.
		pOther->pev->gravity = 1.0f;
	}

	// Play teleport sound.
	EMIT_SOUND(ENT(pOther->pev), CHAN_STATIC, "debris/beamstart7.wav", 1, ATTN_NORM );
}

//=========================================================
// CTriggerGenewormHit
//=========================================================

class CTriggerGenewormHit : public CTriggerMultiple
{
public:
};

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, CTriggerMultiple);

//=========================================================
// CPlayerFreeze
//=========================================================

class CPlayerFreeze : public CBaseDelay
{
public:
	void	Spawn(void);
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
};

LINK_ENTITY_TO_CLASS(trigger_playerfreeze, CPlayerFreeze);


TYPEDESCRIPTION CPlayerFreeze::m_SaveData[] =
{
	DEFINE_FIELD(CPlayerFreeze, m_hPlayer, FIELD_EHANDLE),
};

IMPLEMENT_SAVERESTORE(CPlayerFreeze, CBaseDelay);

void CPlayerFreeze::Spawn(void)
{
	CBaseDelay::Spawn();

	m_hPlayer.Set(NULL);

	CBaseEntity* pPlayer = UTIL_PlayerByIndex( 1 );

	if (pPlayer)
	{
		m_hPlayer = pPlayer;
	}
}

void CPlayerFreeze::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pPlayer = NULL;

	if (!m_hPlayer.Get())
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex(1);

		if (pPlayer)
		{
			m_hPlayer = pPlayer;
		}
	}

	if (m_hPlayer.Get())
	{
#ifdef DEBUG
		ASSERT(m_hPlayer != NULL);
#endif
		m_hPlayer->pev->movetype = (m_hPlayer->pev->movetype == MOVETYPE_NONE)
			? MOVETYPE_WALK
			: MOVETYPE_NONE;
	}
	else
	{
		ALERT(at_console, "ERROR: Couldn't find player entity.\n");
	}

	SetThink(&CPlayerFreeze::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}