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
#include "player.h"
#include "saverestore.h"
#include "trains.h"
#include "gamerules.h"

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE m_hPlayer;
	int m_state;
};
LINK_ENTITY_TO_CLASS(player_freeze, CTriggerPlayerFreeze);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerPlayerFreeze::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerPlayerFreeze, m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(CTriggerPlayerFreeze, m_state, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CTriggerPlayerFreeze, CBaseDelay);

void CTriggerPlayerFreeze::Spawn(void)
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;							// Remove model & collisions
	pev->renderamt = 0;								// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_state = 0;
}

void CTriggerPlayerFreeze::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_state))
		return;

	if (!pActivator || !pActivator->IsPlayer())
	{
		pActivator = CBaseEntity::Instance(g_engfuncs.pfnPEntityOfEntIndex(1));
	}

	m_hPlayer = pActivator;

	switch (useType)
	{
	case  USE_ON:
		m_state = 1;
		((CBasePlayer *)pActivator)->EnableControl(FALSE);
		break;
	case  USE_OFF:
		m_state = 0;
		((CBasePlayer *)pActivator)->EnableControl(TRUE);
		break;
	case  USE_TOGGLE:
		m_state = !m_state;
		((CBasePlayer *)pActivator)->EnableControl(!m_state);
		break;
	case  USE_SET:
		m_state = 0;
		((CBasePlayer *)pActivator)->EnableControl(!m_state);
		break;
	default:
		break;
	}

	// If the player is frozen, remove any velocity.
	if (m_state)
		((CBasePlayer *)pActivator)->pev->velocity = g_vecZero;
}