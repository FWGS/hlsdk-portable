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
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"
#include "player.h"

class CHudToggle : public CPointEntity
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void	Spawn(void);
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

protected:
	BOOL	m_fIsActive;
};

LINK_ENTITY_TO_CLASS(env_hudtoggle, CHudToggle);

TYPEDESCRIPTION	CHudToggle::m_SaveData[] =
{
	DEFINE_FIELD(CHudToggle, m_fIsActive, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CHudToggle, CPointEntity);

void CHudToggle::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;

	m_fIsActive = TRUE;
}

void CHudToggle::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer* pPlayer = (CBasePlayer*)pActivator;
	if (!pPlayer)
		return;

	switch (useType)
	{
	case USE_ON:
	case USE_TOGGLE:
		m_fIsActive = !m_fIsActive;
		break;

	case USE_OFF:
		m_fIsActive = FALSE;
		break;

	case USE_SET:
		m_fIsActive = value;
		break;
	}

	if (m_fIsActive)
	{
		pPlayer->m_iHideHUD &= ~HIDEHUD_ALL;
	}
	else
	{
		pPlayer->m_iHideHUD |= HIDEHUD_ALL;
	}
}