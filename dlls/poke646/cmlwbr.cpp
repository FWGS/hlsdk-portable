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
#include "shake.h"
#include "gamerules.h"
#include "crossbow.h"

extern int gmsgScope;

enum cmlwbr_e {
	CMLWBR_IDLE1 = 0,	// drawn
	CMLWBR_IDLE2,		// undrawn
	CMLWBR_FIDGET1,		// drawn
	CMLWBR_FIDGET2,		// undrawn
	CMLWBR_FIRE1,
	CMLWBR_RELOAD,		// drawn
	CMLWBR_DRAWBACK,
	CMLWBR_UNDRAW,
	CMLWBR_DRAW1,		// drawn
	CMLWBR_DRAW2,		// undrawn
	CMLWBR_HOLSTER1,	// drawn
	CMLWBR_HOLSTER2,	// undrawn
};

LINK_ENTITY_TO_CLASS(weapon_cmlwbr, CCmlwbr);

void CCmlwbr::Spawn()
{
	Precache();
	m_iId = WEAPON_CMLWBR;
	SET_MODEL(ENT(pev), "models/w_crossbow.mdl");

	m_iDefaultAmmo = CMLWBR_DEFAULT_GIVE;

	SetDrawn( FALSE );

	m_fInSpecialReload = 0;

	FallInit();// get ready to fall down.
}

int CCmlwbr::AddToPlayer(CBasePlayer *pPlayer)
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

void CCmlwbr::Precache(void)
{
	PRECACHE_MODEL("models/w_crossbow.mdl");
	PRECACHE_MODEL("models/v_cmlwbr.mdl");
	PRECACHE_MODEL("models/p_cmlwbr.mdl");

	PRECACHE_SOUND("weapons/cmlwbr_drawback.wav");
	PRECACHE_SOUND("weapons/cmlwbr_fire.wav");
	PRECACHE_SOUND("weapons/cmlwbr_reload.wav");
	PRECACHE_SOUND("weapons/cmlwbr_undraw.wav");
	PRECACHE_SOUND("weapons/cmlwbr_zoom.wav");

	PRECACHE_SOUND("weapons/xbow_fire1.wav");
	PRECACHE_SOUND("weapons/xbow_reload1.wav");

	UTIL_PrecacheOther("crossbow_bolt");

	m_usReload = PRECACHE_EVENT(1, "events/reload.sc");
	m_usCmlwbr = PRECACHE_EVENT(1, "events/cmlwbr.sc");
}


int CCmlwbr::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "bolts";
	p->iMaxAmmo1 = BOLT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = CMLWBR_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = WEAPON_CMLWBR;
	p->iFlags = 0;
	p->iWeight = CMLWBR_WEIGHT;
	return 1;
}


BOOL CCmlwbr::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6f;

	if (IsDrawn())
		return DefaultDeploy("models/v_cmlwbr.mdl", "models/p_cmlwbr.mdl", CMLWBR_DRAW1, "bow");

	return DefaultDeploy("models/v_cmlwbr.mdl", "models/p_cmlwbr.mdl", CMLWBR_DRAW2, "bow");
}

void CCmlwbr::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if (m_fInZoom)
	{
		ZoomOut();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (IsDrawn())
		SendWeaponAnim(CMLWBR_HOLSTER1);
	else
		SendWeaponAnim(CMLWBR_HOLSTER2);

	m_fInSpecialReload = 0;

	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_reload.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_zoom.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_drawback.wav");
	STOP_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_undraw.wav");
}

void CCmlwbr::PrimaryAttack(void)
{
	if (!IsDrawn() || (m_fInSpecialReload != 0))
		return;

	FireBolt();
}

void CCmlwbr::FireBolt()
{
#ifndef CLIENT_DLL
	//ALERT(at_console, "FireBolt\n");
#endif

	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(0, m_pPlayer->edict(), m_usCmlwbr, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0);

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CCrossbowBolt *pBolt = CCrossbowBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;
#endif

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.6f);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;

	// Undraw.
	SetDrawn(FALSE);
}


void CCmlwbr::SecondaryAttack()
{
	if (!IsDrawn() || (m_fInSpecialReload != 0))
		return;

	ToggleZoom();

	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	// m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}


void CCmlwbr::Reload(void)
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == CMLWBR_MAX_CLIP)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > UTIL_WeaponTimeBase())
		return;

#ifndef CLIENT_DLL
	//ALERT(at_console, "RELOAD\n");
#endif

	if (m_pPlayer->pev->fov != 0)
	{
		ZoomOut();
	}

	if (m_fInAttack != 0)
	{
		m_fInSpecialReload = 1;
	}

	if (m_fInSpecialReload == 0)
	{
		Undraw();

		m_fInSpecialReload = 1;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
			return;

		DoReload();

		m_fInSpecialReload = 0;
	}
}


void CCmlwbr::WeaponIdle(void)
{
	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound();

	if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
	{
		if (m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload();
		}
		else
		{
			if (m_fInSpecialReload != 0 && m_iClip != CMLWBR_MAX_CLIP)
			{
				Reload();
			}
			else if(!IsDrawn())
			{
				DrawBack();
			}
			else
			{
				int iAnim;
				float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
				if (flRand <= 0.75)
				{
					if (m_iClip)
					{
						iAnim = CMLWBR_IDLE1;
						m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 93.0 / 30.0;
					}
					else
					{
						iAnim = CMLWBR_IDLE2;
						m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 93.0 / 30.0;
					}
				}
				else
				{
					if (m_iClip)
					{
						iAnim = CMLWBR_FIDGET1;
						m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0 / 30.0;
					}
					else
					{
						iAnim = CMLWBR_FIDGET2;
						m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 30.0;
					}
				}
				SendWeaponAnim(iAnim, UseDecrement());
			}
		}
	}
}

BOOL CCmlwbr::ShouldWeaponIdle(void)
{
	return (!IsDrawn() || m_fInSpecialReload != 0);
}

BOOL CCmlwbr::IsDrawn(void)
{
	return (m_fInAttack == 0);
}

void CCmlwbr::SetDrawn(BOOL bDrawn)
{
	m_fInAttack = (bDrawn) ? 0 : 1;
}


void CCmlwbr::ToggleZoom(void)
{
	if (m_pPlayer->pev->fov == 0)
	{
		ZoomIn(50);
	}
	else if (m_pPlayer->pev->fov == 50)
	{
		ZoomIn(25);
	}
	else
	{
		ZoomOut();
	}
}

void CCmlwbr::ZoomIn(int iFOV)
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = iFOV;

	m_fInZoom = 1;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_zoom.wav", VOL_NORM, ATTN_NORM);

#ifndef CLIENT_DLL
	MESSAGE_BEGIN(MSG_ONE, gmsgScope, NULL, m_pPlayer->pev);
		WRITE_BYTE(1);
	MESSAGE_END();

	UTIL_ScreenFade(m_pPlayer, Vector(0, 0, 0), 0.1, 0.1, 255, FFADE_IN | FFADE_OUT);
#endif
}

void CCmlwbr::ZoomOut(void)
{
	m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;

	m_fInZoom = 0;

	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_zoom.wav", VOL_NORM, ATTN_NORM);

#ifndef CLIENT_DLL
	MESSAGE_BEGIN(MSG_ONE, gmsgScope, NULL, m_pPlayer->pev);
		WRITE_BYTE(0);
	MESSAGE_END();
#endif
}

void CCmlwbr::DoReload()
{
	if (DefaultReload(CMLWBR_MAX_CLIP, CMLWBR_RELOAD, 4.7)) // 4.7
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_reload.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));
	}

	SetDrawn(FALSE);

	m_flNextPrimaryAttack = GetNextAttackDelay(6.0);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 4.8;
}

void CCmlwbr::DrawBack(void)
{
	SendWeaponAnim(CMLWBR_DRAWBACK);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_drawback.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	SetDrawn(TRUE);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(1.3);
}

void CCmlwbr::Undraw()
{
	SendWeaponAnim(CMLWBR_UNDRAW);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cmlwbr_undraw.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0xF));

	SetDrawn(FALSE);

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.4;
	m_flNextPrimaryAttack = GetNextAttackDelay(1.4);
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 7.4;
}

class CCmlwbrAmmo : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_crossbow_clip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_crossbow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		if (pOther->GiveAmmo(AMMO_CMLWBRCLIP_GIVE, "bolts", BOLT_MAX_CARRY) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS(ammo_bolts, CCmlwbrAmmo);
