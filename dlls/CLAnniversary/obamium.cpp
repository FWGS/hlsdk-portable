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
#define BOLT_AIR_VELOCITY	250
#define BOLT_WATER_VELOCITY	100


class CPrism : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT PrismThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	CBeam *m_pBeam;
	int Health;

	int m_iTrail;

public:
	static CPrism *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(prism, CPrism);

CPrism *CPrism::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CPrism *pBolt = GetClassPtr((CPrism *)NULL);
	pBolt->pev->classname = MAKE_STRING("prism");
	pBolt->Spawn();

	return pBolt;
}

void CPrism::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	Health = 100;

	pev->gravity = 0.001;
	pev->friction = 0;

	SET_MODEL(ENT(pev), "models/w_obamium.mdl");


	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-4, -4, -7), Vector(4, 4, 7));

	SetTouch(&CPrism::BoltTouch);
	SetThink(&CPrism::PrismThink);
	pev->nextthink = gpGlobals->time + 0.01;
}


void CPrism::Precache()
{
	PRECACHE_MODEL("models/w_obamium.mdl");
	PRECACHE_MODEL("sprites/laserbeam.spr");
	PRECACHE_SOUND("weapons/debris1.wav");
	PRECACHE_SOUND("weapons/debris2.wav");
	PRECACHE_SOUND("weapons/debris3.wav");
	PRECACHE_MODEL("sprites/lgtning.spr");
}


int	CPrism::Classify(void)
{
	return	CLASS_NONE;
}

void CPrism::BoltTouch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this grenade
	if (pOther->edict() == pev->owner)
		return;

	if (pev->flags & FL_ONGROUND)
	{
		UTIL_Sparks(pev->origin);
		pev->velocity = -pev->velocity;
		
	}
}

void CPrism::PrismThink(void)
{
	CBaseEntity *pEntity = NULL;
	Health--;
	if (Health <= 0)
	{
		UTIL_Sparks(pev->origin);
		UTIL_Remove(this);
	}
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 250)) != NULL)
	{
		if (pEntity->pev->takedamage != DAMAGE_NO)
		{
			if (!FClassnameIs(pEntity->pev, "prism") && pEntity->IsAlive() && !pEntity->IsPlayer() && (pEntity->IsBSPModel() == false))
			{
				TraceResult tr;
				UTIL_TraceLine(pev->origin, pEntity->pev->origin, dont_ignore_monsters, ENT(pev), &tr);

				if (tr.flFraction != 1.0 && tr.pHit != pEntity->edict())
					continue;

				m_pBeam = CBeam::BeamCreate("sprites/laserbeam.spr", 30);
				if (!m_pBeam)
					return;

				entvars_t *pevOwner = pev;
				if (pev->owner)
					pevOwner = VARS(pev->owner);

				m_pBeam->PointsInit(pev->origin, pEntity->pev->origin + (gpGlobals->v_up * (pEntity->pev->size.z/2)));
				m_pBeam->SetColor(0, 255, 255);
				m_pBeam->SetBrightness(255);
				m_pBeam->LiveForTime(0.05);
				UTIL_Sparks(pEntity->pev->origin);
				m_pBeam->SetNoise(1);
				pEntity->TakeDamage(pev, pevOwner, 0.1, DMG_ENERGYBEAM);
				

				CBaseMonster *Monster = NULL;
				Monster = pEntity->MyMonsterPointer();
				if (Monster)
				{
					if (!FClassnameIs(Monster->pev, "monster_ginastreamer") && !FClassnameIs(Monster->pev, "monster_varg") && !FClassnameIs(Monster->pev, "monster_georgedroid") && !FClassnameIs(Monster->pev, "monster_oetker"))
					{
						Monster->AddShockEffect(0, 255, 255, 16, 0.01);
						Monster->frozen = 30;
					}
				}

				/*switch (RANDOM_LONG(0, 2))
				{
				case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
				case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
				case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
				}*/

				//break;
			}
		}
	}
	pev->nextthink = gpGlobals->time + 0.05;
}

#endif



LINK_ENTITY_TO_CLASS(weapon_obamium, CObamium);


enum obamium_e {
	OBAMIUM_IDLE = 0,
	OBAMIUM_IDLE1,
	OBAMIUM_DRAW,
	OBAMIUM_HOLSTER,
	OBAMIUM_THROW
};


void CObamium::Spawn()
{
	Precache();
	m_iId = WEAPON_OBAMIUM;
	SET_MODEL(ENT(pev), "models/w_obamium.mdl");
	m_iClip = -1;
	m_iDefaultAmmo = OBAMIUM_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CObamium::Precache(void)
{
	PRECACHE_MODEL("models/v_obamium.mdl");
	PRECACHE_MODEL("models/w_obamium.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/obamium1.wav");
	PRECACHE_SOUND("weapons/obamium2.wav");
	PRECACHE_SOUND("weapons/obamium3.wav");
	PRECACHE_SOUND("boid/boid_alert1.wav");
	UTIL_PrecacheOther("prism");

	m_usObamium = PRECACHE_EVENT(1, "events/obamium.sc");
}

int CObamium::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "obamium";
	p->iMaxAmmo1 = OBAMIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_OBAMIUM;
	p->iWeight = OBAMIUM_WEIGHT;
	return 1;
}



BOOL CObamium::Deploy()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/obamium1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF)); break;
	case 1: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/obamium2.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF)); break;
	case 2: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/obamium3.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF)); break;
	}
	m_flTimeWeaponIdle = 1.0;
	return DefaultDeploy("models/v_obamium.mdl", "models/p_crowbar.mdl", OBAMIUM_DRAW, "obamium");
}

void CObamium::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		SendWeaponAnim(OBAMIUM_THROW);
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2.0;
		m_flTimeWeaponIdle = 2.0;
		

		if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
			// HEV suit - indicate out of ammo condition
			m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "boid/boid_alert1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

		Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		UTIL_MakeVectors(anglesAim);
		anglesAim.x = -anglesAim.x;
		Vector vecSrc = m_pPlayer->pev->origin + gpGlobals->v_up * 20;
		Vector vecDir = gpGlobals->v_forward;
#ifndef CLIENT_DLL
		CPrism *pBolt = CPrism::BoltCreate();
		pBolt->pev->origin = vecSrc;
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
		pBolt->pev->avelocity.y = -400;

#endif
	}
}

void CObamium::SwingAgain(void)
{
	Swing(0);
}

int CObamium::Swing(int fFirst)
{
	SendWeaponAnim(OBAMIUM_THROW);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	return true;
}


void CObamium::WeaponIdle(void)
{
	ResetEmptySound();
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(OBAMIUM_IDLE);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

