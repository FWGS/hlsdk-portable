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


#define	HANDGRENADE_PRIMARY_VOLUME		450
#define BOLT_AIR_VELOCITY	1800
#define BOLT_WATER_VELOCITY	1200

#ifndef CLIENT_DLL

class CProppiwo : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;

public:
	static CProppiwo *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(proppiwo, CProppiwo);

CProppiwo *CProppiwo::BoltCreate(void)
{
	// Create a new entity with CCrossbowBolt private data
	CProppiwo *pBolt = GetClassPtr((CProppiwo *)NULL);
	pBolt->pev->classname = MAKE_STRING("proppiwo");
	pBolt->Spawn();
	pBolt->pev->iuser1 = 0;

	return pBolt;
}

void CProppiwo::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.8;

	SET_MODEL(ENT(pev), "models/w_piwo.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	SetTouch(&CProppiwo::BoltTouch);
	SetThink(&CProppiwo::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CProppiwo::Precache()
{
	PRECACHE_MODEL("models/w_piwo.mdl");
	PRECACHE_SOUND("debris/can.wav");
}


int	CProppiwo::Classify(void)
{
	return	CLASS_NONE;
}

void CProppiwo::BoltTouch(CBaseEntity *pOther)
{
	//pev->angles.x = 0;
	//pev->angles.z = 0;
	pev->iuser1++;

	if (pev->iuser1 == 1)
	{
		SetThink(&CProppiwo::ExplodeThink);
		pev->nextthink = gpGlobals->time + 3;
	}


	if (pev->iuser1 > 100)
		UTIL_Remove(this);

	if (pev->velocity.z == 0)
	{
		UTIL_Remove(this);
	}

	if (pOther->edict() == pev->owner)
		return;
	// pev->avelocity = Vector (300, 300, 300);

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.15;

		if (pev->velocity <= Vector(1, 1, 1))
		{
			UTIL_Remove(this);
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "debris/can.wav", 0.35, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));
	}
}

void CProppiwo::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CProppiwo::ExplodeThink(void)
{
	UTIL_Remove(this);
}
#endif

enum handgrenade_e {
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_SPRAY,
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};


LINK_ENTITY_TO_CLASS(weapon_piwo, CPiwo);


void CPiwo::Spawn()
{
	Precache();
	m_iId = WEAPON_PIWO;
	SET_MODEL(ENT(pev), "models/w_piwo.mdl");

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif

	m_iDefaultAmmo = 1;

	FallInit();// get ready to fall down.
}


void CPiwo::Precache(void)
{
	PRECACHE_MODEL("models/w_piwo.mdl");
	PRECACHE_MODEL("models/v_piwo.mdl");
	PRECACHE_MODEL("models/p_grenade.mdl");
	PRECACHE_SOUND("weapons/piwo.wav");
	PRECACHE_SOUND("debris/can.wav");
	PRECACHE_SOUND("generic/burp.wav");
	PRECACHE_SOUND("generic/burp2.wav");
	PRECACHE_SOUND("generic/burp3.wav");
	PRECACHE_SOUND("items/ammopickup1.wav");
	
}

int CPiwo::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Piwo";
	p->iMaxAmmo1 = 6;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_PIWO;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD;

	return 1;
}


BOOL CPiwo::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy("models/v_piwo.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "Piwo");
}

BOOL CPiwo::CanHolster(void)
{
	// can only holster hand grenades when not primed!
	return ((m_flStartThrow == 0) && (SprayHealth <= 0));
}

void CPiwo::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		SendWeaponAnim(HANDGRENADE_HOLSTER);
	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_HANDGRENADE);
		SetThink(&CBasePlayerItem::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CPiwo::PrimaryAttack()
{
	if (!m_flStartThrow && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 && (SprayHealth <= 0))
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(HANDGRENADE_PINPULL);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void EXPORT CPiwo::Spray()
{
	if (SprayHealth > 0)
	{
		if (SprayHealth == 1)
		{
		#ifndef CLIENT_DLL
					UTIL_MakeVectors(m_pPlayer->pev->angles);
					CProppiwo *pBolt = CProppiwo::BoltCreate();
					pBolt->pev->origin = m_pPlayer->pev->origin + gpGlobals->v_up * 8 + gpGlobals->v_right * 8 + gpGlobals->v_forward * 8;

					pBolt->pev->movetype = MOVETYPE_BOUNCE;
					pBolt->pev->gravity = 0.5;
					pBolt->pev->friction = 0.8;
					pBolt->pev->angles = m_pPlayer->pev->angles;
					pBolt->pev->owner = m_pPlayer->edict();

					pBolt->pev->velocity = m_pPlayer->pev->velocity + gpGlobals->v_forward * 64 + gpGlobals->v_up * 8 + gpGlobals->v_right * 16;
					pBolt->pev->speed = 12;

					pBolt->pev->avelocity.x = -600;
					pBolt->pev->avelocity.y = RANDOM_LONG(-300, 500);
					pBolt->pev->avelocity.z = RANDOM_LONG(-100, 200);
		#endif
					switch (RANDOM_LONG(0, 2))
					{
					case 0: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "generic/burp.wav", 1.0, ATTN_NORM); break;
					case 1: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "generic/burp2.wav", 1.0, ATTN_NORM); break;
					case 2: EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "generic/burp3.wav", 1.0, ATTN_NORM); break;
					}
					
		}
		SprayHealth--;
		SetThink(&CPiwo::Spray);
		if (m_pPlayer->pev->health < 125)
			m_pPlayer->pev->health += 1;
	}
	pev->nextthink = gpGlobals->time + 0.08;
}

void CPiwo::WeaponIdle(void)
{
	if (m_flReleaseThrow == 0 && m_flStartThrow)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow)
	{
		Vector angThrow = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;

		if (angThrow.x < 0)
			angThrow.x = -10 + angThrow.x * ((90 - 10) / 90.0);
		else
			angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);

		float flVel = (90 - angThrow.x) * 4;
		if (flVel > 500)
			flVel = 500;

		UTIL_MakeVectors(angThrow);

		Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;

		Vector vecThrow = gpGlobals->v_forward * flVel + m_pPlayer->pev->velocity;

		// alway explode 3 seconds after the pin was pulled
		float time = m_flStartThrow - gpGlobals->time + 3.0;
		if (time < 0)
			time = 0;

		SendWeaponAnim(HANDGRENADE_SPRAY);

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/piwo.wav", 1.0, ATTN_NORM);
		SprayHealth = 25;
		SetThink(&CPiwo::Spray);
		pev->nextthink = gpGlobals->time + 0.01;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;// ensure that the animation can finish playing
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			SendWeaponAnim(HANDGRENADE_DRAW);
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		m_flReleaseThrow = -1;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.75)
		{
			iAnim = HANDGRENADE_IDLE;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// how long till we do this again.
		}
		else
		{
			iAnim = HANDGRENADE_FIDGET;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim(iAnim);
	}
}

class CSixpack : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();

		pev->gravity = 1;

		SET_MODEL(ENT(pev), "models/w_sixpack.mdl");
		CBasePlayerAmmo::Spawn();
		pev->movetype = MOVETYPE_FLY;
		//pev->solid = SOLID_BBOX;
		pev->friction = 1;
		UTIL_SetSize(pev, Vector(-12, -6, -12), Vector(12, 6, 12));
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_sixpack.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
		PRECACHE_SOUND("items/ammopickup1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		for (int i = 0; i < 6; i++)
		{
			CBaseEntity *pBeer = CBaseEntity::Create("weapon_piwo", pev->origin, Vector(0, 0, 0), edict());
			pBeer->pev->velocity = pBeer->pev->velocity + gpGlobals->v_up * 128 + gpGlobals->v_forward * RANDOM_LONG(-32, 32) + gpGlobals->v_right * RANDOM_LONG(-32, 32);
		}
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/ammopickup1.wav", 1, ATTN_NORM);
		return TRUE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_sixpack, CSixpack);
