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
#include "effects.h"

//=========================================================
// CPitwormGibShooter
//=========================================================
class CPitwormGibShooter : public CGibShooter
{
public:

	void	Precache(void);

	virtual CGib *CreateGib(void);
};

LINK_ENTITY_TO_CLASS(pitworm_gibshooter, CPitwormGibShooter);

//---------------------------------------------------------
// Purpose:
//---------------------------------------------------------
void CPitwormGibShooter::Precache(void)
{
	m_iGibModelIndex = PRECACHE_MODEL("models/pit_worm_gibs.mdl");
}

//---------------------------------------------------------
// Purpose:
//---------------------------------------------------------
CGib *CPitwormGibShooter::CreateGib(void)
{
	if (CVAR_GET_FLOAT("violence_hgibs") == 0)
		return NULL;

	CGib *pGib = GetClassPtr((CGib *)NULL);
	pGib->Spawn("models/pit_worm_gibs.mdl");
	pGib->m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body <= 1)
	{
		ALERT(at_aiconsole, "PitwormGibShooter Body is <= 1!\n");
	}

	pGib->pev->body = RANDOM_LONG(1, pev->body - 1);// avoid throwing random amounts of the 0th gib. (skull).

	return pGib;
}
