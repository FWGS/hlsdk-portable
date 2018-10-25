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
#include "soundent.h"
#include "gamerules.h"

enum par21_e
{
	PAR21_LONGIDLE = 0,
	PAR21_IDLE1,
	PAR21_LAUNCH,
	PAR21_RELOAD,
	PAR21_DEPLOY,
	PAR21_FIRE1,
	PAR21_FIRE2,
	PAR21_FIRE3,
};


LINK_ENTITY_TO_CLASS(weapon_par21, CPar21);


//=========================================================
//=========================================================
int CPar21::SecondaryAmmoIndex(void)
{
	return m_iSecondaryAmmoType;
}

void CPar21::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_par21.mdl");
	m_iId = WEAPON_PAR21;

	m_iDefaultAmmo = PAR21_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CPar21::Precache(void)
{
	PRECACHE_MODEL("models/v_par21.mdl");
	PRECACHE_MODEL("models/w_par21.mdl");
	PRECACHE_MODEL("models/p_par21.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_par21_clip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("weapons/par21_1.wav");
	PRECACHE_SOUND("weapons/par21_2.wav");
	PRECACHE_SOUND("weapons/par21_3.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usPar21 = PRECACHE_EVENT(1, "events/par21.sc");
	m_usM203 = PRECACHE_EVENT(1, "events/m203.sc");
}

int CPar21::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "NATO";
	p->iMaxAmmo1 = PAR21_MAX_CARRY;
	p->pszAmmo2 = "M203grenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = PAR21_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PAR21;
	p->iWeight = PAR21_WEIGHT;

	return 1;
}

int CPar21::AddToPlayer(CBasePlayer *pPlayer)
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

BOOL CPar21::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	return DefaultDeploy("models/v_par21.mdl", "models/p_par21.mdl", PAR21_DEPLOY, "par21");
}


void CPar21::PrimaryAttack()
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


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;

	// single player spread
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usPar21, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.1);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}



void CPar21::SecondaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// we don't add in player velocity anymore.
	CGrenade::ShootContact(m_pPlayer->pev,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 800);

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT(0, m_pPlayer->edict(), m_usM203);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.9f;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.9f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;// idle pretty soon after shooting.
}

void CPar21::Reload(void)
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == PAR21_MAX_CLIP )
                return;
                
        DefaultReload( PAR21_MAX_CLIP, PAR21_RELOAD, 1.6 );
}

BOOL CPar21::IsUseable()
{
	//Can be used if the player has AR grenades. - Solokiller
	return CBasePlayerWeapon::IsUseable() || m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] > 0;
}

void CPar21::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = PAR21_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = PAR21_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}



class CPar21AmmoClip : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_par21_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_par21_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_PAR21CLIP_GIVE, "NATO", PAR21_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_par21_clip, CPar21AmmoClip)
LINK_ENTITY_TO_CLASS(ammo_natoclip, CPar21AmmoClip)

class CM203GrenadeAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_par21_grenades.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_par21_grenades.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(AMMO_M203BOX_GIVE, "M203grenades", M203_GRENADE_MAX_CARRY) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_par21_grenade, CM203GrenadeAmmo)
LINK_ENTITY_TO_CLASS(ammo_m203grenade, CM203GrenadeAmmo)
