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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00  )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum winchester_e {
	WINCHESTER_IDLE = 0,
	WINCHESTER_FIRE,
	WINCHESTER_RELOAD,
	WINCHESTER_CLOSE_BREAK,	// pump
	WINCHESTER_BREAK,		// start reload
	WINCHESTER_DRAW,
	WINCHESTER_RETHOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_winchester, CWinchester);

void CWinchester::Spawn()
{
	Precache();
	m_iId = WEAPON_WINCHESTER;
	SET_MODEL(ENT(pev), "models/w_winchester.mdl");

	m_iDefaultAmmo = WINCHESTER_DEFAULT_GIVE;

	FallInit();// get ready to fall
}


void CWinchester::Precache(void)
{
	PRECACHE_MODEL("models/v_winchester.mdl");
	PRECACHE_MODEL("models/w_winchester.mdl");
	PRECACHE_MODEL("models/p_winchester.mdl");

	m_iShell = PRECACHE_MODEL("models/shotgunshell.mdl");// shotgun shell

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/winchester_break.wav");
	PRECACHE_SOUND("weapons/winchester_closebreak.wav");
	PRECACHE_SOUND("weapons/winchester_dryfire.wav");
	PRECACHE_SOUND("weapons/winchester_fire1.wav");
	PRECACHE_SOUND("weapons/winchester_fire2.wav");
	PRECACHE_SOUND("weapons/winchester_fire3.wav");
	PRECACHE_SOUND("weapons/winchester_reload1.wav");
	PRECACHE_SOUND("weapons/winchester_reload2.wav");

	m_usWinchester = PRECACHE_EVENT(1, "events/winchester.sc");
}

int CWinchester::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "winchesterclip";
	p->iMaxAmmo1 = BUCKSHOT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WINCHESTER_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_WINCHESTER;
	p->iWeight = WINCHESTER_WEIGHT;

	return 1;
}



BOOL CWinchester::Deploy()
{
	return DefaultDeploy("models/v_winchester.mdl", "models/p_winchester.mdl", WINCHESTER_DRAW, "winchester");
}

void CWinchester::PrimaryAttack()
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


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir;

#ifdef CLIENT_DLL
	if (bIsMultiplayer())
#else
	if (g_pGameRules->IsMultiplayer())
#endif
	{
		vecDir = m_pPlayer->FireBulletsPlayer(4, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}
	else
	{
		// regular old, untouched spread. 
		vecDir = m_pPlayer->FireBulletsPlayer(6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	}

	// PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usSingleFire, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usWinchester, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);
	if (m_iClip != 0)
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = GetNextAttackDelay(0.95);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.95;
	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.95;
	m_fInSpecialReload = 0;
}


void CWinchester::SecondaryAttack(void)
{
}


void CWinchester::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == WINCHESTER_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		SendWeaponAnim(WINCHESTER_BREAK, UseDecrement());

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_break.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
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

		if (RANDOM_LONG(0, 1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_reload2.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

		SendWeaponAnim(WINCHESTER_RELOAD, UseDecrement());

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}


void CWinchester::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		// play pumping sound
		// EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_closebreak.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle <  UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip != WINCHESTER_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim(WINCHESTER_CLOSE_BREAK, UseDecrement());

				// play cocking sound
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/winchester_dryfire.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0, 0x1f));
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			SendWeaponAnim(WINCHESTER_IDLE);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (1.0f / 9.0f);
		}
	}
}


BOOL CWinchester::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/winchester_dryfire.wav", 0.8, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

class CWinchesterClipAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_winchesterclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_winchesterclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_WINCHESTERCLIP_GIVE, "winchesterclip", WINCHESTER_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_winchesterclip, CWinchesterClipAmmo);


