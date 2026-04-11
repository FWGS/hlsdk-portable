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
#include "player.h"

#define VECTOR_CONE_EAGLE Vector(0.00025, 0.00025, 0.00025)

LINK_ENTITY_TO_CLASS(weapon_eagle, CEagle);

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS(eagle_laser, CEagleLaser);

//=========================================================
//=========================================================
CEagleLaser* CEagleLaser::CreateSpot()
{
	CEagleLaser* pSpot = GetClassPtr((CEagleLaser*)NULL);
	pSpot->Spawn();

	pSpot->pev->classname = MAKE_STRING("eagle_laser");

	return pSpot;
}

//=========================================================
//=========================================================
void CEagleLaser::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	pev->scale = 0.5;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");
	UTIL_SetOrigin(pev, pev->origin);
};

//=========================================================
// Suspend- make the laser sight invisible.
//=========================================================
void CEagleLaser::Suspend(float flSuspendTime)
{
	pev->effects |= EF_NODRAW;

	SetThink(&CEagleLaser::Revive);
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

//=========================================================
// Revive - bring a suspended laser sight back.
//=========================================================
void CEagleLaser::Revive()
{
	pev->effects &= ~EF_NODRAW;

	SetThink(NULL);
}

void CEagleLaser::Precache()
{
	PRECACHE_MODEL("sprites/laserdot.spr");
};

#endif

void CEagle::Spawn()
{
	pev->classname = MAKE_STRING("weapon_eagle"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_EAGLE;
	SET_MODEL(ENT(pev), "models/w_desert_eagle.mdl");

	m_iDefaultAmmo = EAGLE_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}


void CEagle::Precache()
{
	PRECACHE_MODEL("models/v_desert_eagle.mdl");
	PRECACHE_MODEL("models/w_desert_eagle.mdl");
	PRECACHE_MODEL("models/p_desert_eagle.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl"); // brass shell

	PRECACHE_SOUND("weapons/desert_eagle_fire.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/desert_eagle_sight.wav"); //silenced handgun
	PRECACHE_SOUND("weapons/desert_eagle_sight2.wav"); //handgun

	m_usFireEagle = PRECACHE_EVENT(1, "events/eagle.sc");
}

int CEagle::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = EAGLE_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_EAGLE;
	p->iWeight = EAGLE_WEIGHT;

	return true;
}

BOOL CEagle::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy("models/v_desert_eagle.mdl", "models/p_desert_eagle.mdl", EAGLE_DRAW, "onehanded");
}

void CEagle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = false; // cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(EAGLE_HOLSTER);

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif
}

void CEagle::SecondaryAttack()
{
#ifndef CLIENT_DLL
	m_fSpotActive = !m_fSpotActive;

	if (!m_fSpotActive && m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NORMAL);
		m_pSpot = NULL;
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/desert_eagle_sight2.wav", 1.0, ATTN_NORM);
	}
#endif

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CEagle::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_iClip--;

#ifndef CLIENT_DLL
	if (m_pSpot && m_fSpotActive)
	{
		m_pSpot->Suspend(0.5);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	const float flSpread = m_fSpotActive ? 0.001 : 0.1;

	const Vector vecSpread = m_pPlayer->FireBulletsPlayer(1,vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread),8192.0, BULLET_PLAYER_EAGLE, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + (m_fSpotActive ? 0.5 : 0.25);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireEagle, 0, g_vecZero, g_vecZero, vecSpread.x, vecSpread.y, 0, 0, static_cast<int>(m_iClip == 0), 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CEagle::Reload()
{
	bool iResult;

	if (m_iClip == 0)
		iResult = DefaultReload(7, EAGLE_RELOAD, 1.5);
	else
		iResult = DefaultReload(7, EAGLE_RELOAD_NOT_EMPTY, 1.5);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

#ifndef CLIENT_DLL
		if (m_pSpot && m_fSpotActive)
		{
			m_pSpot->Suspend(1.5);
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		}
#endif
	}
}



void CEagle::WeaponIdle()
{
	ResetEmptySound();

	UpdateSpot();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);
		if (!m_fSpotActive)
		{
			switch (RANDOM_LONG(0, 2))
			{
			case 0:
				iAnim = EAGLE_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 25.0 / 16.0;
				break;
			case 1:
				iAnim = EAGLE_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
				break;
			case 2:
				iAnim = EAGLE_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16;
				break;
			}
		}
		else// if (!m_fSpotActive)
		{
			switch (RANDOM_LONG(0, 1))
			{
			case 0:
				iAnim = EAGLE_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
				break;
			case 1:
				iAnim = EAGLE_IDLE5;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 34.0 / 16.0;
				break;
			}
		}
		SendWeaponAnim(iAnim);
	}
}

void CEagle::UpdateSpot()
{

#ifndef CLIENT_DLL
	// Don't turn on the laser if we're in the middle of a reload.
	if (m_fInReload)
	{
		return;
	}

	if (m_fSpotActive)
	{
		if (!m_pSpot)
		{
			m_pSpot = CEagleLaser::CreateSpot();
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/desert_eagle_sight.wav", 1.0, ATTN_NORM);
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

		UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);
	}
#endif
}