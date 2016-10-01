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

class CItemFlashlight : public CItem
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_flashlight.mdl");
		CItem::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_flashlight.mdl");
		PRECACHE_SOUND("items/gunpickup2.wav");
	}
	BOOL MyTouch(CBasePlayer *pPlayer)
	{
		if (pPlayer->pev->weapons & (1 << WEAPON_FLASHLIGHT))
			return FALSE;

		EMIT_SOUND(pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM);

		pPlayer->pev->weapons |= (1 << WEAPON_FLASHLIGHT);
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_flashlight, CItemFlashlight);
