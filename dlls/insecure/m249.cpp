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
#include "soundent.h"
#include "gamerules.h"

LINK_ENTITY_TO_CLASS(weapon_m249, CM249);


//=========================================================
//=========================================================
void CM249::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m249"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_saw.mdl");
	m_iId = WEAPON_M249;

	m_iDefaultAmmo = M249_DEFAULT_GIVE;

	m_bAlternateShell = false;

	FallInit(); // get ready to fall down.
}


void CM249::Precache()
{
	PRECACHE_MODEL("models/v_saw.mdl");
	PRECACHE_MODEL("models/w_saw.mdl");
	PRECACHE_MODEL("models/p_saw.mdl");

	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");
	m_iLink = PRECACHE_MODEL("models/saw_link.mdl");

	PRECACHE_MODEL("models/w_saw_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/saw_fire1.wav"); // H to the K
	PRECACHE_SOUND("weapons/saw_fire2.wav"); // H to the K
	PRECACHE_SOUND("weapons/saw_fire3.wav"); // H to the K
	PRECACHE_SOUND("weapons/saw_reload.wav");
	PRECACHE_SOUND("weapons/saw_reload2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usM249 = PRECACHE_EVENT(1, "events/m249.sc");
}

int CM249::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = _556_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = M249_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M249;
	p->iWeight = M249_WEIGHT;

	return TRUE;
}

BOOL CM249::Deploy()
{
	return DefaultDeploy("models/v_saw.mdl", "models/p_saw.mdl", M249_DRAW, "mp5");
}

void CM249::Holster(int skiplocal /* = 0 */)
{
	m_fInSpecialReload = 0;

	m_fInReload = 0;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	SendWeaponAnim(M249_HOLSTER);
}

void CM249::PrimaryAttack()
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
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	pev->body = RecalculateBody(m_iClip);

	m_bAlternateShell = !m_bAlternateShell;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	Vector vecDir;
	Vector vecSpread;

	if ((m_pPlayer->pev->button & IN_DUCK) != 0)
	{
		vecSpread = VECTOR_CONE_2DEGREES;
	}
	else if ((m_pPlayer->pev->button & (IN_MOVERIGHT | IN_MOVELEFT | IN_FORWARD | IN_BACK)) != 0)
	{
		vecSpread = VECTOR_CONE_6DEGREES;
	}
	else
	{
		vecSpread = VECTOR_CONE_4DEGREES;
	}

	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined(CLIENT_WEAPONS)
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(
		flags, m_pPlayer->edict(), m_usM249, 0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, pev->body, 0, m_bAlternateShell ? 1 : 0, 0);

	if (0 == m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.067);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.067;

	float flOldPlayerVel = m_pPlayer->pev->velocity.z;
	m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + (50 * -gpGlobals->v_forward);
	m_pPlayer->pev->velocity.z = flOldPlayerVel;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

int CM249::RecalculateBody(int iClip)
{
	if (iClip == 0)
	{
		return 8;
	}
	else if (iClip >= 0 && iClip <= 7)
	{
		return 9 - iClip;
	}
	else
	{
		return 0;
	}
}

void CM249::Reload()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M249_MAX_CLIP)
		return;

	if (m_iClip != 50 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0)
	{
		m_fInSpecialReload = 1;
		DefaultReload(M249_MAX_CLIP, M249_RELOAD, 1.5, pev->body);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + (61 / 40) + (111 / 45);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 61 / 40;
	}
}


void CM249::WeaponIdle()
{
	if (m_iClip > 7)
	{
		pev->body = 0;
	}

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_fInSpecialReload)
		{
			m_fInSpecialReload = 0;
			pev->body = 0;
			SendWeaponAnim(M249_RELOAD2, pev->body);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 111.0 / 45.0;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 111.0 / 45.0;
		}
		else
		{
			ResetEmptySound();

			int iAnim;
			float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
			if (flRand <= 0.8)
			{
				iAnim = M249_SLOWIDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (45.0 / 9.0);
			}
			else
			{
				iAnim = M249_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + (74.0 / 12.0);// * RANDOM_LONG(2, 5);
			}
			SendWeaponAnim(iAnim, pev->body);
		}
	}
}

class C556Ammo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_saw_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_saw_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity* pOther) override
	{
		BOOL bResult = (pOther->GiveAmmo(AMMO_556BOX_GIVE, "556", _556_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS(ammo_556, C556Ammo);