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
#include "xen.h"
#include "sporegrenade.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum sporelauncher_e {
	SPLAUNCHER_IDLE = 0,
	SPLAUNCHER_FIDGET,
	SPLAUNCHER_RELOAD_REACH ,
	SPLAUNCHER_RELOAD_LOAD,
	SPLAUNCHER_RELOAD_AIM,
	SPLAUNCHER_FIRE,
	SPLAUNCHER_HOLSTER1,
	SPLAUNCHER_DRAW1,
	SPLAUNCHER_IDLE2
};

LINK_ENTITY_TO_CLASS(weapon_sporelauncher, CSporelauncher);

void CSporelauncher::Spawn()
{
	Precache();
	m_iId = WEAPON_SPORELAUNCHER;
	SET_MODEL(ENT(pev), "models/w_spore_launcher.mdl");

	m_iDefaultAmmo = SPORELAUNCHER_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CSporelauncher::Precache(void)
{
	PRECACHE_MODEL("models/v_spore_launcher.mdl");
	PRECACHE_MODEL("models/w_spore_launcher.mdl");
	PRECACHE_MODEL("models/p_spore_launcher.mdl");

	PRECACHE_SOUND("weapons/splauncher_altfire.wav");
	PRECACHE_SOUND("weapons/splauncher_bounce.wav");
	PRECACHE_SOUND("weapons/splauncher_fire.wav");
	PRECACHE_SOUND("weapons/splauncher_impact.wav");
	PRECACHE_SOUND("weapons/splauncher_pet.wav");
	PRECACHE_SOUND("weapons/splauncher_reload.wav");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_MODEL("sprites/bigspit.spr");
	m_iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");
	UTIL_PrecacheOther("monster_spore");

	m_usSporeFire = PRECACHE_EVENT(1, "events/spore.sc");
}

int CSporelauncher::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}


int CSporelauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "spores";
	p->iMaxAmmo1 = SPORE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SPORELAUNCHER_MAX_CLIP;
	p->iSlot = 6;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SPORELAUNCHER;
	p->iWeight = SPORELAUNCHER_WEIGHT;

	return 1;
}



BOOL CSporelauncher::Deploy()
{
	return DefaultDeploy("models/v_spore_launcher.mdl", "models/p_spore_launcher.mdl", SPLAUNCHER_DRAW1, "splauncher");
}

void CSporelauncher::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif


	// m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir;
	Vector vecVel;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = vecSrc + gpGlobals->v_forward	* 16;
	vecSrc = vecSrc + gpGlobals->v_right	* 8;
	vecSrc = vecSrc + gpGlobals->v_up		* -12;

	vecVel = gpGlobals->v_forward * 900;
	vecDir = gpGlobals->v_forward + gpGlobals->v_right + gpGlobals->v_up;
	vecDir = vecDir;

#ifndef CLIENT_DLL
	CSporeGrenade::ShootContact(m_pPlayer->pev, vecSrc, vecVel);
#endif

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(), 
		m_usSporeFire, 
		0.0, 
		(float *)&g_vecZero, 
		(float *)&g_vecZero, 
		vecDir.x,
		vecDir.y,
		*(int*)&vecDir.z,
		m_iSquidSpitSprite, 
		0,
		TRUE);


	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

#if 1
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
#endif

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}


void CSporelauncher::SecondaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	Vector vecDir;
	Vector vecVel;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = vecSrc + gpGlobals->v_forward	* 16;
	vecSrc = vecSrc + gpGlobals->v_right	* 8;
	vecSrc = vecSrc + gpGlobals->v_up		* -12;

	vecVel = gpGlobals->v_forward * 800;
	vecDir = gpGlobals->v_forward + gpGlobals->v_right + gpGlobals->v_up;
	vecDir = vecDir;

#ifndef CLIENT_DLL
	CSporeGrenade::ShootTimed(m_pPlayer->pev, vecSrc, vecVel, RANDOM_FLOAT(5, 6));
#endif

	PLAYBACK_EVENT_FULL(
		flags, 
		m_pPlayer->edict(),
		m_usSporeFire, 
		0.0, 
		(float *)&g_vecZero,
		(float *)&g_vecZero,
		vecDir.x, 
		vecDir.y,
		*(int*)&vecDir.z,
		m_iSquidSpitSprite,
		0, 
		0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

#if 1
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
#endif

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInSpecialReload = 0;

}


void CSporelauncher::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SPORELAUNCHER_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(SPLAUNCHER_RELOAD_REACH);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.7;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.7;
		m_flNextPrimaryAttack = GetNextAttackDelay(1.0);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		// Play reload sound.
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/splauncher_reload.wav", 1, ATTN_NORM, 0, 100);

		SendWeaponAnim(SPLAUNCHER_RELOAD_LOAD);

		m_flNextReload = UTIL_WeaponTimeBase() + 1.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CSporelauncher::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != SPORELAUNCHER_DEFAULT_GIVE && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(SPLAUNCHER_RELOAD_AIM);

				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.5)
			{
				iAnim = SPLAUNCHER_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0f;
			}
			else
			{
				iAnim = SPLAUNCHER_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.0f;
			}

			SendWeaponAnim(iAnim);
		}
	}
}


class CSporeAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/spore.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/spore.mdl");
		PRECACHE_SOUND("weapons/spore_ammo.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_SPORE_GIVE, "Spores", SPORE_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/spore_ammo.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS(ammo_spore, CSporeAmmo);