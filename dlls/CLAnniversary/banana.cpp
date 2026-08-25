/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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


#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	500
#define BOLT_WATER_VELOCITY	250


class CBananaPeel : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BananaThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);

	int m_iTrail;

public:
	static CBananaPeel *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(bananapeel, CBananaPeel);

CBananaPeel *CBananaPeel::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CBananaPeel *pBolt = GetClassPtr((CBananaPeel *)NULL);
	pBolt->pev->classname = MAKE_STRING("bananapeel");
	pBolt->Spawn();

	return pBolt;
}

void CBananaPeel::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/w_bananapeel.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	SetTouch(&CBananaPeel::BoltTouch);
	SetThink(&CBananaPeel::BananaThink);
	pev->nextthink = gpGlobals->time + 0.01;
}


void CBananaPeel::Precache()
{
	PRECACHE_MODEL("models/w_bananapeel.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CBananaPeel::Classify(void)
{
	return	CLASS_NONE;
}

void CBananaPeel::BoltTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh1.wav", 0.8, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "debris/flesh3.wav", 0.8, ATTN_NORM); break;
		}
	}
}

void CBananaPeel::BananaThink(void)
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 32)) != NULL)
	{
		if ((pEntity->pev->takedamage != DAMAGE_NO))
		{
			if (!FClassnameIs(pEntity->pev, "bananapeel") && pEntity->pev->size.z <= 80 && pEntity->IsAlive() && (pev->flags & FL_ONGROUND) && 
				!(pEntity->edict() == pev->owner) && pEntity->IsMoving() && !FClassnameIs(pEntity->pev, "monster_tentacle") && !FClassnameIs(pEntity->pev, "monster_georgedroid") && !FClassnameIs(pEntity->pev, "monster_oetker") && !FClassnameIs(pEntity->pev, "monster_ginastreamer") && !FClassnameIs(pEntity->pev, "monster_gargantua") && !FClassnameIs(pEntity->pev, "monster_nihilanth") && !FClassnameIs(pEntity->pev, "monster_bigmomma"))
			{
				pEntity->SetThink(&CBaseEntity::SlipThink);
				pEntity->pev->movetype = MOVETYPE_BOUNCE;
				pEntity->pev->solid = SOLID_BBOX;
				//pEntity->SetTouch(&CBaseEntity::SlipTouch);
				
				int randomdir = RANDOM_LONG(0, 1);
				int randomdir2 = RANDOM_LONG(0, 1);
				if (randomdir == 0)
					randomdir = -1;
				if (randomdir2 == 0)
					randomdir2 = -1;

				pEntity->pev->gravity = 0.3;
				pEntity->pev->avelocity.x = -600;
				pEntity->pev->avelocity.y = RANDOM_LONG(-300, 500);
				pEntity->pev->avelocity.z = RANDOM_LONG(-100, 200);
				pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_up * RANDOM_LONG(900, 1300) + gpGlobals->v_forward * RANDOM_LONG(800, 1600) * randomdir + gpGlobals->v_right * RANDOM_LONG(800, 1600) * randomdir2;
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/slip.wav", 1.0, ATTN_NORM);
				REMOVE_ENTITY(ENT(pev));
				break;		
			}
		}
	}
	pev->nextthink = gpGlobals->time + 0.01;
}

#endif


LINK_ENTITY_TO_CLASS(weapon_banana, CBanana);


enum gauss_e {
	BANANA_IDLE = 0,
	BANANA_THROW,
	BANANA_DRAW,
};


void CBanana::Spawn()
{
	Precache();
	m_iId = WEAPON_BANANA;
	SET_MODEL(ENT(pev), "models/w_banana.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.
	m_iDefaultAmmo = BANANA_DEFAULT_GIVE;
}


void CBanana::Precache(void)
{
	PRECACHE_MODEL("models/v_banana.mdl");
	PRECACHE_MODEL("models/w_banana.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_MODEL("models/w_bananapeel.mdl");
	PRECACHE_SOUND("debris/flesh1.wav");
	PRECACHE_SOUND("debris/flesh3.wav");
	PRECACHE_SOUND("weapons/slip.wav");

	m_usBanana = PRECACHE_EVENT(1, "events/banana.sc");
}

int CBanana::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "banana";
	p->iMaxAmmo1 = BANANA_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = WEAPON_BANANA;
	p->iWeight = BANANA_WEIGHT;
	return 1;
}


BOOL CBanana::Deploy()
{
	IsThrowing = false;
	return DefaultDeploy("models/v_banana.mdl", "models/p_crowbar.mdl", BANANA_DRAW, "banana");
}


void CBanana::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 && !IsThrowing)
	{
		SendWeaponAnim(BANANA_THROW);
		IsThrowing = true;
		SetThink(&CBanana::ThrowPeel);
		pev->nextthink = gpGlobals->time + 1;
	}
}

BOOL CBanana::CanHolster(void)
{
	return !IsThrowing;
}

void CBanana::ThrowPeel()
{
	SendWeaponAnim(BANANA_IDLE);
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	SetThink(NULL);
	IsThrowing = false;
	pev->nextthink = gpGlobals->time + 1.25;

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 20;
	Vector vecDir = gpGlobals->v_forward;
#ifndef CLIENT_DLL
	CBananaPeel *pBolt = CBananaPeel::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->movetype = MOVETYPE_BOUNCE;
	pBolt->pev->gravity = 0.5;
	pBolt->pev->friction = 0.8;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	//pBolt->pev->avelocity.x = -600;
	pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
	//pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
#endif
}




