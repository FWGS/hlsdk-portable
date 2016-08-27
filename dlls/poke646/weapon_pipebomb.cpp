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


enum pipebomb_e {
	PIPEBOMB_IDLE1 = 0,
	PIPEBOMB_FIDGET1,
	PIPEBOMB_DRAW,
	PIPEBOMB_DROP
};

enum pipebomb_watch_e {
	PIPEBOMB_WATCH_IDLE1 = 0,
	PIPEBOMB_WATCH_FIDGET1,
	PIPEBOMB_WATCH_DRAW,
	PIPEBOMB_WATCH_FIRE,
	PIPEBOMB_WATCH_HOLSTER
};


LINK_ENTITY_TO_CLASS(weapon_pipebomb, CPipeBomb);


//=========================================================
//=========================================================
int CPipeBomb::AddToPlayer(CBasePlayer *pPlayer)
{
	int bResult = CBasePlayerItem::AddToPlayer(pPlayer);

	pPlayer->pev->weapons |= (1 << m_iId);
	m_chargeReady = 0;// this satchel charge weapon now forgets that any satchels are deployed by it.

	if (bResult)
	{
		return AddWeapon();
	}
	return FALSE;
}

void CPipeBomb::Spawn()
{
	Precache();
	m_iId = WEAPON_PIPEBOMB;
	SET_MODEL(ENT(pev), "models/w_pipebomb.mdl");

	m_iDefaultAmmo = PIPEBOMB_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CPipeBomb::Precache(void)
{
	PRECACHE_MODEL("models/v_pipebomb.mdl");
	PRECACHE_MODEL("models/v_pipebomb_watch.mdl");
	PRECACHE_MODEL("models/w_pipebomb.mdl");
	PRECACHE_MODEL("models/p_pipebomb.mdl");
	PRECACHE_MODEL("models/p_pipebomb_watch.mdl");

	UTIL_PrecacheOther("monster_pipebomb");

	m_usReload = PRECACHE_EVENT(1, "events/reload.sc");
}


int CPipeBomb::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Pipe Bombs";
	p->iMaxAmmo1 = PIPEBOMB_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_PIPEBOMB;
	p->iWeight = PIPEBOMB_WEIGHT;

	return 1;
}


BOOL CPipeBomb::Deploy()
{

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	if (m_chargeReady)
		return DefaultDeploy("models/v_pipebomb_watch.mdl", "models/p_pipebomb_watch.mdl", PIPEBOMB_WATCH_DRAW, "hive");
	else
		return DefaultDeploy("models/v_pipebomb.mdl", "models/p_pipebomb.mdl", PIPEBOMB_DRAW, "trip");


	return TRUE;
}


void CPipeBomb::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (m_chargeReady)
	{
		SendWeaponAnim(PIPEBOMB_WATCH_HOLSTER);
	}
	else
	{
		SendWeaponAnim(PIPEBOMB_DROP);
	}

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && !m_chargeReady)
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_PIPEBOMB);
		SetThink(&CPipeBomb::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}



void CPipeBomb::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
	{
		Throw();
	}
	break;
	case 1:
	{
		SendWeaponAnim(PIPEBOMB_WATCH_FIRE);
	
		edict_t *pPlayer = m_pPlayer->edict();

		CBaseEntity *pSatchel = NULL;

		while ((pSatchel = UTIL_FindEntityInSphere(pSatchel, m_pPlayer->pev->origin, 4096)) != NULL)
		{
			if (FClassnameIs(pSatchel->pev, "monster_pipebomb"))
			{
				if ( pSatchel->pev->owner == pPlayer || pSatchel->m_thrownByPlayer == 1)
				{
					pSatchel->Use(m_pPlayer, m_pPlayer, USE_ON, 0);
					m_chargeReady = 2;
				}
			}
		}

		m_chargeReady = 2;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		break;
	}

	case 2:
		// we're reloading, don't allow fire
	{
	}
	break;
	}
}


void CPipeBomb::SecondaryAttack(void)
{
	if (m_chargeReady != 2)
	{
		Throw();
	}
}


void CPipeBomb::Redraw(void)
{
#ifndef CLIENT_DLL
	m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_pipebomb.mdl");
	m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_pipebomb.mdl");
#else
	LoadVModel("models/v_pipebomb.mdl", m_pPlayer);
#endif

	// SendWeaponAnim(PIPEBOMB_DRAW, 0);
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usReload, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, PIPEBOMB_DRAW, 0, 0, 0);

	// use tripmine animations
	strcpy(m_pPlayer->m_szAnimExtention, "trip");

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_chargeReady = 0;
}

void CPipeBomb::Throw(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
		CBaseEntity *pSatchel = Create("monster_pipebomb", vecSrc, Vector(0, 0, 0), m_pPlayer->edict());
		pSatchel->pev->velocity = vecThrow;
		pSatchel->pev->avelocity.y = 400;
		pSatchel->m_thrownByPlayer = 1;

		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_pipebomb_watch.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_pipebomb_watch.mdl");
#else
		LoadVModel("models/v_pipebomb_watch.mdl", m_pPlayer);
#endif

		// SendWeaponAnim(PIPEBOMB_WATCH_DRAW);
		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usReload, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, PIPEBOMB_WATCH_DRAW, 0, 0, 0);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_chargeReady = 1;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}

BOOL CPipeBomb::ShouldWeaponIdle(void)
{
	return FALSE;
}

void CPipeBomb::WeaponIdle(void)
{
	if (m_chargeReady == 2)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() - 0.1;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	switch (m_chargeReady)
	{
	case 0:
		SendWeaponAnim(PIPEBOMB_FIDGET1);
		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");
		break;
	case 1:
		SendWeaponAnim(PIPEBOMB_WATCH_FIDGET1);
		// use hivehand animations
		strcpy(m_pPlayer->m_szAnimExtention, "hive");
		break;
	case 2:
		if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_pipebomb.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_pipebomb.mdl");
#else
		LoadVModel("models/v_pipebomb.mdl", m_pPlayer);
#endif

		// SendWeaponAnim(PIPEBOMB_DRAW, 0);
		PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usReload, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, PIPEBOMB_DRAW, 0, 0, 0);

		// use tripmine animations
		strcpy(m_pPlayer->m_szAnimExtention, "trip");

		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// how long till we do this again.
}