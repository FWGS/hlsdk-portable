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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"nail.h"


LINK_ENTITY_TO_CLASS(nail, CNail);

CNail *CNail::NailCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CNail *pBolt = GetClassPtr((CNail *)NULL);
	pBolt->pev->classname = MAKE_STRING("nail");
	pBolt->Spawn();

	return pBolt;
}

void CNail::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.0;

	SET_MODEL(ENT(pev), "models/nail.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CNail::NailTouch);
	SetThink(&CNail::NailThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CNail::Precache()
{
	PRECACHE_MODEL("models/nail.mdl");
	PRECACHE_SOUND("weapons/brad_hit1.wav");
	PRECACHE_SOUND("weapons/brad_hit2.wav");
}

int	CNail::Classify(void)
{
	return	CLASS_NONE;
}

void CNail::NailTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->TraceAttack(pevOwner, 0, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);

		ApplyMultiDamage(pev, pevOwner);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/brad_hit1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/brad_hit2.wav", 1, ATTN_NORM); break;
		}

		Killed(pev, GIB_NEVER);
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/brad_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CNail::SUB_Remove);
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		/*if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}*/
	}
}

void CNail::NailThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
}
