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


enum cannon_e {
	CANNON_DRAW = 0,
	CANNON_HOLSTER,
	CANNON_IDLE1,
	CANNON_IDLE2,
	CANNON_FIRE,
	CANNON_DRYFIRE,
	CANNON_RELOAD,
};

LINK_ENTITY_TO_CLASS(weapon_cannon, CCannon);

void CCannon::Spawn()
{
	Precache();
	m_iId = WEAPON_CANNON;
	SET_MODEL(ENT(pev), "models/w_cannon.mdl");

	m_iDefaultAmmo = CANNON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CCannon::Precache(void)
{
	PRECACHE_MODEL("models/w_cannon.mdl");
	PRECACHE_MODEL("models/v_cannon.mdl");
	PRECACHE_MODEL("models/p_cannon.mdl");

	PRECACHE_SOUND("weapons/cannon_empty1.wav");
	PRECACHE_SOUND("weapons/cannon_fire1.wav");
	PRECACHE_SOUND("weapons/cannon_reload1.wav");
}

int CCannon::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "cannon";
	p->iMaxAmmo1 = CANNON_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CANNON_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_CANNON;
	p->iWeight = CANNON_WEIGHT;
	p->iFlags = 0;

	return 1;
}


BOOL CCannon::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy("models/v_cannon.mdl", "models/p_cannon.mdl", CANNON_DRAW, "cannon");
}

BOOL CCannon::CanHolster(void)
{
	// can only holster hand grenades when not primed!
	return (m_flStartThrow == 0);
}

void CCannon::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(CANNON_HOLSTER);

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CCannon::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
			Reload();
		else
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = GetNextAttackDelay(0.15);
		}
		return;
	}

	if (!m_flStartThrow && /*m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]*/ m_iClip > 0)
	{
		m_flStartThrow = gpGlobals->time;
		m_flReleaseThrow = 0;

		SendWeaponAnim(CANNON_FIRE);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.8f;
	}
}


void CCannon::WeaponIdle(void)
{
	if (m_flReleaseThrow == 0 && m_flStartThrow)
		m_flReleaseThrow = gpGlobals->time;

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (m_flStartThrow)
	{
		Vector vecSrc, vecCannonBallVel, vecOwnerVel;
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);

		vecSrc = m_pPlayer->GetGunPosition();
		vecSrc = vecSrc + gpGlobals->v_forward * 8;
		vecSrc = vecSrc + gpGlobals->v_right * 4;
		vecSrc = vecSrc + gpGlobals->v_up * -2;

		vecCannonBallVel = gpGlobals->v_forward * 1000;

		// Apply 75% of the projectile's velocity impulse, backward, to the owner.
		vecOwnerVel = -vecCannonBallVel * 0.75f;
		vecOwnerVel.z = 0;

		m_pPlayer->pev->velocity = vecOwnerVel;

		// we don't add in player velocity anymore.
		CCannonBall::ShootContact(m_pPlayer->pev, vecSrc, vecCannonBallVel);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cannon_fire1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );

#ifndef CLIENT_DLL
		Vector vecSmokeOrigin;
		UTIL_MakeAimVectors(m_pPlayer->pev->v_angle);

		vecSmokeOrigin = m_pPlayer->GetGunPosition() +
			gpGlobals->v_forward * 16 +
			gpGlobals->v_right * 4 +
			gpGlobals->v_up * -16;


		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SMOKE);
		WRITE_COORD(vecSmokeOrigin.x);
		WRITE_COORD(vecSmokeOrigin.y);
		WRITE_COORD(vecSmokeOrigin.z);
		WRITE_SHORT(g_sModelIndexSmoke);
		WRITE_BYTE(12);
		WRITE_BYTE(12); // framerate
		MESSAGE_END();
#endif


		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

		m_flReleaseThrow = 0;
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = GetNextAttackDelay(0.5);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

		// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_iClip--;

		if (m_iClip <= 0 /*!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]*/)
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle in the future so we can see the whole throw
			// animation, weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay(0.5);// ensure that the animation can finish playing
		}
		else
		{
			Reload();
		}
		return;
	}
	else if (m_flReleaseThrow > 0)
	{
		// we've finished the throw, restart.
		m_flStartThrow = 0;

		if (m_iClip > 0 /* m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] */)
		{
			// SendWeaponAnim(CANNON_DRAW);
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

	if (m_iClip > 0 /*m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]*/)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
		if (flRand <= 0.5)
		{
			iAnim = CANNON_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);// how long till we do this again.
		}
		else
		{
			iAnim = CANNON_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 75.0 / 30.0;
		}

		SendWeaponAnim(iAnim);
	}
}

void CCannon::Reload(void)
{
	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		return;

	BOOL fRet = DefaultReload(CANNON_MAX_CLIP, CANNON_RELOAD, 2.7f);

	if (fRet)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/cannon_reload1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM);
	}
}

void CCannon::ItemPostFrame(void)
{
	if (m_flStartThrow || m_flReleaseThrow)
	{
		WeaponIdle();
	}

	CBasePlayerWeapon::ItemPostFrame();
}

class CCannonAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_cannonball.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_cannonball.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_CANNON_GIVE, "cannon", CANNON_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_cannon, CCannonAmmo);