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

enum pistol_e {
	PISTOL_IDLE1 = 0,
	PISTOL_FIDGET1,
	PISTOL_SHOOT,
	PISTOL_RELOAD,
	PISTOL_HOLSTER,
	PISTOL_DRAW,
	PISTOL_IDLE2,
	PISTOL_IDLE3,
	PISTOL_QUICKFIRE_READY,
	PISTOL_QUICKFIRE_SHOOT,
	PISTOL_QUICKFIRE_RELAX,
};

LINK_ENTITY_TO_CLASS(weapon_pistol, CPistol);

void CPistol::Spawn()
{
	Precache();
	m_iId = WEAPON_PISTOL;
	SET_MODEL(ENT(pev), "models/w_pistol.mdl");

	m_iDefaultAmmo = PISTOL_DEFAULT_GIVE;

	m_iQuickFireState = QFSTATE_NONE;

	FallInit();// get ready to fall down.
}


void CPistol::Precache(void)
{
	PRECACHE_MODEL("models/v_pistol.mdl");
	PRECACHE_MODEL("models/w_pistol.mdl");
	PRECACHE_MODEL("models/p_pistol.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shell

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/pistol_cock1.wav");
	PRECACHE_SOUND("weapons/pistol_reload1.wav");
	PRECACHE_SOUND("weapons/pistol_shot1.wav");
	PRECACHE_SOUND("weapons/pistol_shot2.wav");

	m_usPistol = PRECACHE_EVENT(1, "events/pistol.sc");
}

int CPistol::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "pistol";
	p->iMaxAmmo1 = PISTOL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PISTOL_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PISTOL;
	p->iWeight = PISTOL_WEIGHT;

	return 1;
}

BOOL CPistol::Deploy()
{
	m_iQuickFireState = QFSTATE_NONE;

	// pev->body = 1;
	return DefaultDeploy("models/v_pistol.mdl", "models/p_pistol.mdl", PISTOL_DRAW, "pistol", /*UseDecrement() ? 1 : 0*/ 0);
}


void CPistol::Holster(int skiplocal /* = 0 */)
{
	m_iQuickFireState = QFSTATE_NONE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PISTOL_HOLSTER);
}



void CPistol::SecondaryAttack(void)
{
	m_iQuickFireState = QFSTATE_READY;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(1.0f);
}

void CPistol::PrimaryAttack(void)
{
	GlockFire(0.01, 1.0f, TRUE, FALSE);
}

void CPistol::GlockFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL fSecondary)
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			m_fInReload = TRUE;

			Reload();
		}
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.2);
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags;

#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

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
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_9MM, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	PLAYBACK_EVENT_FULL(
		flags,
		m_pPlayer->edict(), 
		m_usPistol, 
		0.0, 
		(float *)&g_vecZero, 
		(float *)&g_vecZero, 
		vecDir.x, 
		vecDir.y, 
		0,
		0, 
		fSecondary, 
		0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}


void CPistol::Reload(void)
{
	if (m_pPlayer->ammo_pistol <= 0)
		return;

	int iResult;

	iResult = DefaultReload(PISTOL_MAX_CLIP, PISTOL_RELOAD, 1.5);

	if (iResult)
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/pistol_reload1.wav", VOL_NORM, ATTN_NORM);

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}



void CPistol::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0.0, 1.0);

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = PISTOL_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = PISTOL_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = PISTOL_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim(iAnim, 1);
	}
}

void CPistol::ItemPostFrame(void)
{
	if ((m_fInReload) && (m_pPlayer->m_flNextAttack <= UTIL_WeaponTimeBase()))
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

	if (m_iQuickFireState != QFSTATE_NONE && (m_pPlayer->m_flNextAttack < UTIL_WeaponTimeBase()))
	{ 
		if (!(m_pPlayer->pev->button & IN_ATTACK2))
		{
			m_iQuickFireState = QFSTATE_RELAX;
		}

		if (m_iQuickFireState == QFSTATE_READY)
		{
			StartQuickFire();
		}
		else if (m_iQuickFireState == QFSTATE_SHOOT)
		{
			m_flNextSecondaryAttack = GetNextAttackDelay(0.0f);
		}
		else if (m_iQuickFireState == QFSTATE_RELAX)
		{
			FinishQuickFire();
		}
	}

	if (!(m_pPlayer->pev->button & IN_ATTACK))
	{
		m_flLastFireTime = 0.0f;
	}

	if ((m_pPlayer->pev->button & IN_ATTACK2) && CanAttack(m_flNextSecondaryAttack, gpGlobals->time, UseDecrement()))
	{
		if (pszAmmo2() && !m_pPlayer->m_rgAmmo[SecondaryAmmoIndex()])
		{
			m_fFireOnEmpty = TRUE;
		}

#ifndef CLIENT_DLL
		m_pPlayer->TabulateAmmo();
#endif
		if (m_iQuickFireState == QFSTATE_SHOOT)
		{
			FireQuick();
		}
		else
		{
			SecondaryAttack();
		}

		// m_pPlayer->pev->button &= ~IN_ATTACK2;
	}
	else if ((m_pPlayer->pev->button & IN_ATTACK) && CanAttack(m_flNextPrimaryAttack, gpGlobals->time, UseDecrement()))
	{
		if ((m_iClip == 0 && pszAmmo1()) || (iMaxClip() == -1 && !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]))
		{
			m_fFireOnEmpty = TRUE;
		}

#ifndef CLIENT_DLL
		m_pPlayer->TabulateAmmo();
#endif
		PrimaryAttack();
	}
	else if (m_pPlayer->pev->button & IN_RELOAD && iMaxClip() != WEAPON_NOCLIP && !m_fInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
	}
	else if (!(m_pPlayer->pev->button & (IN_ATTACK | IN_ATTACK2)))
	{
		// no fire buttons down

		m_fFireOnEmpty = FALSE;

		if (!IsUseable() && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
		{
#ifndef CLIENT_DLL
			// weapon isn't useable, switch.
			if (!(iFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && g_pGameRules->GetNextBestWeapon(m_pPlayer, this))
			{
				m_flNextPrimaryAttack = (UseDecrement() ? 0.0 : gpGlobals->time) + 0.3;
				return;
			}
#endif // !defined ( CLIENT_DLL )
		}
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip == 0 && !(iFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < (UseDecrement() ? 0.0 : gpGlobals->time))
			{
				Reload();
				return;
			}
		}

		WeaponIdle();
		return;
	}

	// catch all
	if (ShouldWeaponIdle())
	{
		WeaponIdle();
	}
}

BOOL CPistol::CanAttack(float attack_time, float curtime, BOOL isPredicted)
{
#if defined( CLIENT_WEAPONS )
	if (!isPredicted)
#else
	if (1)
#endif
	{
		return (attack_time <= curtime) ? TRUE : FALSE;
	}
	else
	{
		return (attack_time <= 0.0) ? TRUE : FALSE;
	}
}

void CPistol::StartQuickFire(void)
{
	pev->framerate = 1.25f;

	SendWeaponAnim(PISTOL_QUICKFIRE_READY);

	m_iQuickFireState = QFSTATE_SHOOT;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.2f);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2f;
}

void CPistol::FireQuick(void)
{
	GlockFire(0.1, 0.2f, FALSE, TRUE);

	m_iQuickFireState = QFSTATE_SHOOT;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.25f);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.25f;
}

void CPistol::FinishQuickFire(void)
{
	SendWeaponAnim(PISTOL_QUICKFIRE_RELAX);

	m_iQuickFireState = QFSTATE_NONE;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( 1.2f );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.2f;
}

class CPistolAmmoBox : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_pistolammobox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_pistolammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_PISTOL_GIVE, "pistol", PISTOL_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_pistol, CPistolAmmoBox);