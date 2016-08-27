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
#include "pipebomb.h"

LINK_ENTITY_TO_CLASS(monster_pipebomb, CPipeBombGrenade);


TYPEDESCRIPTION	CPipeBombGrenade::m_SaveData[] =
{
	DEFINE_FIELD(CPipeBombGrenade, m_hOwner, FIELD_EHANDLE),
	DEFINE_FIELD(CPipeBombGrenade, m_thrownByPlayer, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CPipeBombGrenade, CGrenade);

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CPipeBombGrenade::Deactivate(void)
{
	pev->solid = SOLID_NOT;
	UTIL_Remove(this);
}


void CPipeBombGrenade::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_pipebomb.mdl");
	//UTIL_SetSize(pev, Vector( -16, -16, -4), Vector(16, 16, 32));	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CPipeBombGrenade::BombSlide);
	SetUse(&CPipeBombGrenade::DetonateUse);
	SetThink(&CPipeBombGrenade::BombThink);
	pev->nextthink = gpGlobals->time + 0.1;

	pev->gravity = 0.5;
	pev->friction = 0.8;

	pev->dmg = gSkillData.plrDmgSatchel;
	// ResetSequenceInfo( );
	pev->sequence = 1;
}


void CPipeBombGrenade::BombSlide(CBaseEntity *pOther)
{
	entvars_t	*pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin - Vector(0, 0, 10), ignore_monsters, edict(), &tr);

	if (tr.flFraction < 1.0)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if (!(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10)
	{
		BounceSound();
	}
	StudioFrameAdvance();
}


void CPipeBombGrenade::BombThink(void)
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (pev->waterlevel == 3)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}
	if ((pev->flags & FL_ONGROUND) && pev->velocity.Length() <= 1)
	{
		if (pev->owner != NULL)
		{
			m_hOwner = CBaseEntity::Instance(pev->owner);
			pev->owner = NULL;
		}

		SetThink(NULL);
		SetTouch(&CPipeBombGrenade::PickupTouch);
	}
}

void CPipeBombGrenade::Precache(void)
{
	PRECACHE_MODEL("models/w_pipebomb.mdl");
	PRECACHE_SOUND("weapons/pb_bounce1.wav");
	PRECACHE_SOUND("weapons/pb_bounce2.wav");
	PRECACHE_SOUND("weapons/pb_bounce3.wav");
}

void CPipeBombGrenade::BounceSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/pb_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/pb_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/pb_bounce3.wav", 1, ATTN_NORM);	break;
	}
}

void CPipeBombGrenade::PickupTouch(CBaseEntity* pOther)
{
	if (pOther != m_hOwner || !pOther->IsPlayer())
		return;

	CBasePlayer* pPlayer = (CBasePlayer*)pOther;


	CPipeBomb *pSatchel = (CPipeBomb*)Create("weapon_pipebomb", pPlayer->pev->origin, pPlayer->pev->angles);
	if (pSatchel)
	{
		BOOL found = FALSE;
		CBasePlayerItem* pItem = NULL;
		for (int i = 0; i < MAX_ITEM_TYPES && !found; i++)
		{
			pItem = pPlayer->m_rgpPlayerItems[i];

			while (pItem)
			{
				if (pItem->m_iId == WEAPON_PIPEBOMB)
				{
					((CPipeBomb*)pItem)->m_chargeReady = 2;
					((CPipeBomb*)pItem)->WeaponIdle();
					found = TRUE;
					break;
				}

				pItem = pItem->m_pNext;
			}
		}
	}

	SetThink(&CPipeBombGrenade::SUB_Remove);
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time;

	//ALERT(at_console, "Pickup touch.\n");
}

//=========================================================
// DeactivateSatchels - removes all satchels owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivatePipebombs(CBasePlayer *pOwner)
{
	edict_t *pFind;

	pFind = FIND_ENTITY_BY_CLASSNAME(NULL, "monster_pipebomb");

	while (!FNullEnt(pFind))
	{
		CBaseEntity *pEnt = CBaseEntity::Instance(pFind);
		CPipeBombGrenade *pPipebomb = (CPipeBombGrenade *)pEnt;

		if (pPipebomb)
		{
			if (pPipebomb->pev->owner == pOwner->edict())
			{
				pPipebomb->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME(pFind, "monster_pipebomb");
	}
}