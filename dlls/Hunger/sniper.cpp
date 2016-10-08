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
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

extern int gmsgZoom;

int CSniper::AddToPlayer(CBasePlayer *pPlayer)
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

void CSniper::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_fInZoom)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CSniper::SniperFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, int iActivity)
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
	Vector vecAiming;

	if (fUseAutoAim)
	{
		vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_SNIPER, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireSniper, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, iActivity, 0, 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	if (m_flNextSecondaryAttack < UTIL_WeaponTimeBase())
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CSniper::SecondaryAttack(void)
{
	ToggleZoom();

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.5);
}

void CSniper::PrimaryAttack()
{
	BOOL fUseAutoAim;
	int iAnim;
	float flSpread;
	float flCycleTime;

	fUseAutoAim		= TRUE;
	iAnim			= m_fInZoom ? GetZoomedAttackActivity() : GetPrimaryAttackActivity();
	flSpread		= m_fInZoom ? 0.01 : 0.03;
	flCycleTime		= m_fInZoom ? 0.7 : 0.1;

	SniperFire(flSpread, flCycleTime, fUseAutoAim, iAnim);
}

void CSniper::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	if (m_fInZoom)
	{
		SetZoomState(FALSE);
	}
}

void CSniper::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
}

void CSniper::SetZoomState(BOOL bState)
{
	m_fInZoom = bState;
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = bState ? 40 : 0; // 0 means reset to default fov

#ifndef CLIENT_DLL
	// Toggle Zoom HUD.
	MESSAGE_BEGIN(MSG_ONE, gmsgZoom, NULL, m_pPlayer->pev);
		WRITE_BYTE( bState );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
#endif
}

void CSniper::ToggleZoom(void)
{
	SetZoomState(!m_fInZoom);
}


class CSniperAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_antidote.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_SNIPER_GIVE, "sniper", SNIPER_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_th_sniper, CSniperAmmo);
LINK_ENTITY_TO_CLASS(ammo_einar1, CSniperAmmo);