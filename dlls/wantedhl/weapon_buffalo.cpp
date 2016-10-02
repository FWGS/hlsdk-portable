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


enum buffalo_e {
	BUFFALO_DRAW = 0,
	BUFFALO_HOLSTER,
	BUFFALO_IDLE1,
	BUFFALO_IDLE2,
	BUFFALO_FIDGET,
	BUFFALO_FIRE,
	BUFFALO_DRYFIRE,
	BUFFALO_RELOAD
};

LINK_ENTITY_TO_CLASS(weapon_buffalo, CBuffalo);

int CBuffalo::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "buffalo";
	p->iMaxAmmo1 = BUFFALO_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BUFFALO_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 2;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_BUFFALO;
	p->iWeight = BUFFALO_WEIGHT;

	return 1;
}

int CBuffalo::AddToPlayer(CBasePlayer *pPlayer)
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

void CBuffalo::Spawn()
{
	Precache();
	m_iId = WEAPON_BUFFALO;
	SET_MODEL(ENT(pev), "models/w_buffalogun.mdl");

	m_iDefaultAmmo = BUFFALO_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CBuffalo::Precache(void)
{
	PRECACHE_MODEL("models/v_buffalogun.mdl");
	PRECACHE_MODEL("models/w_buffalogun.mdl");
	PRECACHE_MODEL("models/p_buffalogun.mdl");

	PRECACHE_MODEL("models/w_buffalobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/buffalo_breakopen.wav");
	PRECACHE_SOUND("weapons/buffalo_close.wav");
	PRECACHE_SOUND("weapons/buffalo_dryfire.wav");
	PRECACHE_SOUND("weapons/buffalo_reload.wav");
	PRECACHE_SOUND("weapons/buffalo_shoot1.wav");
	PRECACHE_SOUND("weapons/buffalo_shoot2.wav");
	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usBuffalo = PRECACHE_EVENT(1, "events/buffalo.sc");
}

BOOL CBuffalo::Deploy()
{
	return DefaultDeploy("models/v_buffalogun.mdl", "models/p_buffalogun.mdl", BUFFALO_DRAW, "buffalo", UseDecrement());
}


void CBuffalo::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	SendWeaponAnim(BUFFALO_HOLSTER);
}

void CBuffalo::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15 );
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload();
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/buffalo_dryfire.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = GetNextAttackDelay( 0.15 );
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
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usBuffalo, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

#ifndef CLIENT_DLL
	Vector vecSmokeOrigin;
	UTIL_MakeAimVectors(m_pPlayer->pev->v_angle);

	vecSmokeOrigin = m_pPlayer->GetGunPosition() +
					 gpGlobals->v_forward * 16 +
					 gpGlobals->v_right * 4 +
					 gpGlobals->v_up * -8;


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

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( 3.0f );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 3.0f;
}


void CBuffalo::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		return;

	DefaultReload(BUFFALO_MAX_CLIP, BUFFALO_RELOAD, 3.6);
}


void CBuffalo::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = BUFFALO_IDLE1;
	}
	else
	{
		iAnim = BUFFALO_IDLE2;
	}

	m_flTimeWeaponIdle = (20.0 / 9.0);
	SendWeaponAnim(iAnim, UseDecrement());
}


class CBuffaloAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_buffalobox.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_buffalobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_BUFFALO_GIVE, "buffalo", BUFFALO_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_buffalo, CBuffaloAmmo);


#endif