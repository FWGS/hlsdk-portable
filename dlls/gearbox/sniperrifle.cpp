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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

enum sniper_e {
	SNIPER_DRAW = 0,
	SNIPER_SLOWIDLE,
	SNIPER_FIRE,
	SNIPER_FIRELASTROUND,
	SNIPER_RELOAD1,
	SNIPER_RELOAD2,
	SNIPER_RELOAD3,
	SNIPER_SLOWIDLE2,
	SNIPER_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_sniperrifle, CSniperrifle);

int CSniperrifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762";
	p->iMaxAmmo1 = _762_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SNIPERRIFLE_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 5;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_SNIPERRIFLE;
	p->iWeight = SNIPERRIFLE_WEIGHT;

	return 1;
}

int CSniperrifle::AddToPlayer(CBasePlayer *pPlayer)
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

void CSniperrifle::Spawn()
{
	Precache();
	m_iId = WEAPON_SNIPERRIFLE;
	SET_MODEL(ENT(pev), "models/w_m40a1.mdl");

	m_iDefaultAmmo = SNIPERRIFLE_DEFAULT_GIVE;

	m_fNeedAjustBolt = FALSE;
	m_iBoltState = BOLTSTATE_FINE;

	FallInit();// get ready to fall down.
}


void CSniperrifle::Precache(void)
{
	PRECACHE_MODEL("models/v_m40a1.mdl");
	PRECACHE_MODEL("models/w_m40a1.mdl");
	PRECACHE_MODEL("models/p_m40a1.mdl");

	PRECACHE_MODEL("models/w_m40a1clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");

	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_bolt2.wav");

	PRECACHE_SOUND("weapons/sniper_fire.wav");
	PRECACHE_SOUND("weapons/sniper_fire_last_round.wav");

	PRECACHE_SOUND("weapons/sniper_miss.wav");

	PRECACHE_SOUND("weapons/sniper_reload_first_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload_second_seq.wav");
	PRECACHE_SOUND("weapons/sniper_reload3.wav");

	PRECACHE_SOUND("weapons/sniper_zoom.wav");

	m_usSniper = PRECACHE_EVENT(1, "events/sniper.sc");
}

BOOL CSniperrifle::Deploy()
{
	if (m_fNeedAjustBolt)
	{
		m_iBoltState = BOLTSTATE_ADJUST;
	}

	return DefaultDeploy("models/v_m40a1.mdl", "models/p_m40a1.mdl", SNIPER_DRAW, "m40a1", UseDecrement());
}


void CSniperrifle::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_fInZoom)
	{
		SecondaryAttack();
	}

	if (m_fNeedAjustBolt)
	{
		m_iBoltState = BOLTSTATE_ADJUST;
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(SNIPER_HOLSTER);
}

void CSniperrifle::SecondaryAttack(void)
{
	if (m_pPlayer->pev->fov != 0)
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else if (m_pPlayer->pev->fov != 18)
	{
		m_fInZoom = TRUE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 18;
	}

	// Play zoom sound.
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sniper_zoom.wav", 1, ATTN_NORM);

	m_flNextSecondaryAttack = GetNextAttackDelay(0.5f);
}

void CSniperrifle::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload();
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);


	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(0, 0, 0), 8192, BULLET_PLAYER_762, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	m_fNeedAjustBolt = (m_iClip <= 0) ? 1 : 0;

	// If this was the last round in the clip, make sure to schedule
	// bolt adjustment.
	if (m_fNeedAjustBolt)
		m_iBoltState = BOLTSTATE_ADJUST;

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, m_fNeedAjustBolt, 0, 0, 0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(1.8f);
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CSniperrifle::Reload(void)
{
	if (m_pPlayer->ammo_762 <= 0)
		return;

	if (m_pPlayer->pev->fov != 0)
	{
		m_fInZoom = FALSE;
		m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	int bUseScope = FALSE;
#ifdef CLIENT_DLL
	bUseScope = bIsMultiplayer();
#else
	bUseScope = g_pGameRules->IsMultiplayer();
#endif

	// Select the appropriate sequence for reload.
	// One has bolt adjusted, the other does not.
	int iReloadAnim = (m_iClip > 0) 
		? SNIPER_RELOAD3	// Regular reload.
		: SNIPER_RELOAD1;	// No ammo in current clip.

	DefaultReload(SNIPERRIFLE_MAX_CLIP, iReloadAnim, 2.3);
}


void CSniperrifle::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// Select the appropriate sequence for idle.
	// One has bolt adjusted, the other does not.
	int iIdleAnim = (m_iClip > 0) 
		? SNIPER_SLOWIDLE	// Clip has at least one bullet. (Bolt adjusted)
		: SNIPER_SLOWIDLE2; // Clip is empty. (Bolt unadjusted)

	SendWeaponAnim(iIdleAnim, UseDecrement());
}

void CSniperrifle::ItemPostFrame(void)
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
	{
		if (m_fNeedAjustBolt)
		{
			switch (m_iBoltState)
			{
			case BOLTSTATE_ADJUST:
			{
				m_iBoltState = BOLTSTATE_ADJUSTING;

				// Send the bolt 'adjustment' weapon anim.
				SendWeaponAnim(SNIPER_RELOAD2);

				m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.8f;
			}
			break;

			case BOLTSTATE_ADJUSTING:
			{
				m_fNeedAjustBolt = FALSE;
				m_iBoltState = BOLTSTATE_FINE;
			}
			break;
			default: ALERT(at_aiconsole, "Warning: Unknown bolt state!\n"); break;
			}

			return;
		}
		else
		{
			// complete the reload. 
			int j = min(iMaxClip() - m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);

			// Add them to the clip
			m_iClip += j;
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= j;

#ifndef CLIENT_DLL
			m_pPlayer->TabulateAmmo();
#endif
			m_fInReload = FALSE;
		}
	}

	CBasePlayerWeapon::ItemPostFrame();
}


class CSniperAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_m40a1clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_m40a1clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_762BOX_GIVE, "762", _762_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_762, CSniperAmmo);

#endif