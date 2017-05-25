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

#define CHAINGUN_BULLETS_PER_SHOT 2

enum chaingun_e {
	CHAINGUN_IDLE = 0,
	CHAINGUN_IDLE2,
	CHAINGUN_SPINUP,
	CHAINGUN_SPINDOWN,
	CHAINGUN_FIRE,
	CHAINGUN_DRAW,
	CHAINGUN_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_th_chaingun, CChaingun);

void CChaingun::Spawn()
{
	Precache();
	m_iId = WEAPON_CHAINGUN;
	SET_MODEL(ENT(pev), "models/w_tfac.mdl");

	m_iDefaultAmmo = CHAINGUN_DEFAULT_GIVE;

	m_fInAttack = 0;
	m_fInSpecialReload = 0;

	FallInit();// get ready to fall down.
}


void CChaingun::Precache(void)
{
	PRECACHE_MODEL("models/v_tfac.mdl");
	PRECACHE_MODEL("models/w_tfac.mdl");
	PRECACHE_MODEL("models/p_tfac.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("weapons/reload3.wav");

	PRECACHE_SOUND("weapons/asscan1.wav");
	PRECACHE_SOUND("weapons/asscan2.wav");
	PRECACHE_SOUND("weapons/asscan3.wav");
	PRECACHE_SOUND("weapons/asscan4.wav");

	m_usFireChaingun1 = PRECACHE_EVENT(1, "events/chaingun1.sc");
	m_usFireChaingun2 = PRECACHE_EVENT(1, "events/chaingun2.sc");
}

int CChaingun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CHAINGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_CHAINGUN;
	p->iWeight = CHAINGUN_WEIGHT;

	return 1;
}

BOOL CChaingun::Deploy()
{
	// pev->body = 1;
	return DefaultDeploy("models/v_tfac.mdl", "models/p_tfac.mdl", CHAINGUN_DRAW, "chaingun", UseDecrement());
}

void CChaingun::Holster(int skiplocal /*= 0*/)
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(CHAINGUN_HOLSTER);

	// Stop chaingun sounds.
	StopSounds();

	m_fInAttack = 0;
	m_fInSpecialReload = 0;
}

void CChaingun::PrimaryAttack(void)
{
	// Don't fire while in reload.
	if (m_fInSpecialReload != 0)
	{
		WeaponIdle();
		return;
	}

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		if (m_fInAttack != 0)
		{
			// spin down
			SpinDown();

			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
		}
		else
		{
			PlayEmptySound();
		}

		m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		return;
	}

	if (m_iClip <= 0)
	{
		Reload();
		if (m_iClip == 0)
			PlayEmptySound();
		return;
	}

	if (m_fInAttack == 0)
	{
		// Spin up
		SpinUp();
	}
	else if (m_fInAttack == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
		{
			// fire
			SendWeaponAnim(CHAINGUN_FIRE);
			m_fInAttack = 2;
		}
	}
	else
	{
		// Spin
		Spin();
	}

	m_fInSpecialReload = 0;
}

void CChaingun::SecondaryAttack(void)
{
	WeaponIdle();
}

void CChaingun::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == CHAINGUN_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack != 0)
		return;

	// Stop sounds.
	StopSounds();

	// Stop spin up or firing.
	m_fInAttack = 0;

	// Restore player speed.
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireChaingun2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, FALSE, 0);

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(CHAINGUN_HOLSTER);
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.6);
		m_flNextSecondaryAttack = GetNextAttackDelay(1.5);
		return;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.8;
	}
	else
	{
		if ( DefaultReload(SNIPER_MAX_CLIP, CHAINGUN_DRAW, 0.53 ))
		{
			m_fInSpecialReload = 3;

			m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() - 0.1;

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		}
	}
}

void CChaingun::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_fInAttack != 0)
	{
		// Spin down
		SpinDown();

		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
	}
	else
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != CHAINGUN_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				m_fInSpecialReload = 0;

				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
				m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.0);
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.5)
			{
				iAnim = CHAINGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (41.0 / 10.0);
			}
			else
			{
				iAnim = CHAINGUN_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (51.0 / 10.0);
			}
			SendWeaponAnim(iAnim);
		}
	}
}

BOOL CChaingun::ShouldWeaponIdle(void)
{
	return m_iClip == 0 || m_fInSpecialReload != 0;
}

void CChaingun::SpinUp(void)
{
	// spin up
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	SendWeaponAnim(CHAINGUN_SPINUP);

	// Slowdown player.
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireChaingun2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, TRUE, 0);

	m_fInAttack = 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/asscan1.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 0x3f));
}

void CChaingun::SpinDown(void)
{
	// Spin down
	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	SendWeaponAnim(CHAINGUN_SPINDOWN);

	// Restore player speed.
	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usFireChaingun2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, FALSE, 0);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/asscan3.wav", 1.0, ATTN_NORM, 0, 80 + RANDOM_LONG(0, 0x3f));

	m_fInAttack = 0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
}

void CChaingun::Spin(void)
{
	// out of ammo!
	if (m_iClip <= 0)
	{
		// Spin down
		SpinDown();

		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
		return;
	}

	m_fInAttack = 2;

	// Spin sound.
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/asscan4.wav", 0.8, ATTN_NORM);

#ifdef CLIENT_DLL
	if (!bIsMultiplayer())
#else
	if (!g_pGameRules->IsMultiplayer())
#endif
	{
		// single player spread
		Fire(0.1, 0.1, FALSE);
	}
	else
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		Fire(0.2, 0.1, FALSE);
	}
}

void CChaingun::Fire(float flSpread, float flCycleTime, BOOL fUseAutoAim)
{
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= CHAINGUN_BULLETS_PER_SHOT;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer(CHAINGUN_BULLETS_PER_SHOT, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_CHAINGUN, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireChaingun1, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(flCycleTime);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
}

void CChaingun::StopSounds(void)
{
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/asscan1.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/asscan2.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/asscan3.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/asscan4.wav");
}
