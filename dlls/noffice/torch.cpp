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
#include "flashlightspot.h"

extern int gmsgGlow;

enum torch_e {
	TORCH_IDLE_OFF = 0,
	TORCH_DRAW,
	TORCH_IDLE_ON,
	TORCH_SWITCH,
	TORCH_HOLSTER_OFF,
	TORCH_HOLSTER_ON,
};

LINK_ENTITY_TO_CLASS(weapon_torch, CTorch);

#ifndef CLIENT_DLL
TYPEDESCRIPTION	CTorch::m_SaveData[] =
{
	DEFINE_FIELD(CTorch, m_pSpot, FIELD_CLASSPTR),
	DEFINE_FIELD(CTorch, m_pGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CTorch, m_fIsOn, FIELD_BOOLEAN),
};

int CTorch::Save(CSave &save)
{
	if (!CBasePlayerWeapon::Save(save))
		return 0;

	return save.WriteFields("TORCH", this, m_SaveData, ARRAYSIZE(m_SaveData));
}


int CTorch::Restore(CRestore &restore)
{
	if (!CBasePlayerWeapon::Restore(restore))
		return 0;

	int status = restore.ReadFields("TORCH", this, m_SaveData, ARRAYSIZE(m_SaveData));

	if (status)
	{
		if (m_fIsOn)
		{
			m_fUpdate = TRUE;
		}
	}

	return status;
}
#endif

void CTorch::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/w_torch.mdl");
	m_iId = WEAPON_TORCH;
	m_iClip = -1;

	m_fIsOn = FALSE;
	m_fUpdate = FALSE;

#ifndef CLIENT_DLL
	m_pSpot = NULL;
	m_pGlow = NULL;
#endif

	FallInit();// get ready to fall down.
}


void CTorch::Precache(void)
{
	PRECACHE_MODEL("models/v_torch.mdl");
	PRECACHE_MODEL("models/w_torch.mdl");
	PRECACHE_MODEL("models/p_torch.mdl");

	PRECACHE_SOUND( SOUND_FLASHLIGHT_ON );

	PRECACHE_MODEL("sprites/glow01.spr");
	PRECACHE_MODEL("sprites/glow02.spr");
	PRECACHE_MODEL("sprites/glow03.spr");
	PRECACHE_MODEL("sprites/glow04.spr");
	PRECACHE_MODEL("sprites/glow05.spr");

	PRECACHE_MODEL("sprites/flare1.spr");
	PRECACHE_MODEL("sprites/flare2.spr");
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/flare4.spr");
	PRECACHE_MODEL("sprites/flare5.spr");
	PRECACHE_MODEL("sprites/flare6.spr");

	PRECACHE_MODEL("sprites/blueflare1.spr");

	m_usTorch = PRECACHE_EVENT(1, "events/torch.sc");

	UTIL_PrecacheOther( "flashlight_spot" );
}

int CTorch::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_TORCH;
	p->iWeight = TORCH_WEIGHT;

	return 1;
}

int CTorch::AddToPlayer(CBasePlayer *pPlayer)
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

BOOL CTorch::Deploy()
{
	return DefaultDeploy("models/v_torch.mdl", "models/p_torch.mdl", TORCH_DRAW, "torch");
}

void CTorch::Holster(int skiplocal /*= 0*/)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if ( m_fIsOn )
		TurnOff();

	SendWeaponAnim(TORCH_HOLSTER_OFF);
}

void CTorch::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	ToggleFlashlight();

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usTorch, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, m_fIsOn, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(0.2);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	UpdateSpot();
}

void CTorch::WeaponIdle(void)
{
	UpdateSpot();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;

	if (m_fIsOn)
		iAnim = TORCH_IDLE_ON;
	else
		iAnim = TORCH_IDLE_OFF;

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}


void CTorch::TurnOn(void)
{
	m_fIsOn = TRUE;

#ifndef CLIENT_DLL
	if (!m_pSpot)
	{
		m_pSpot = CFlashlightSpot::CreateSpot();
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgGlow, NULL, m_pPlayer->pev);
		WRITE_BYTE(1);
	MESSAGE_END();
#endif

	m_pPlayer->pev->effects |= EF_DIMLIGHT;
}

void CTorch::TurnOff(void)
{
	m_fIsOn = FALSE;

#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NORMAL);
		m_pSpot = NULL;
	}

	if (m_pGlow)
	{
		UTIL_Remove(m_pGlow);
		m_pGlow = NULL;
	}

	MESSAGE_BEGIN(MSG_ONE, gmsgGlow, NULL, m_pPlayer->pev);
		WRITE_BYTE(0);
	MESSAGE_END();
#endif

	m_pPlayer->pev->effects &= ~EF_DIMLIGHT;
}

void CTorch::ToggleFlashlight(void)
{
	if (!m_fIsOn)
		TurnOn();
	else
		TurnOff();
}

#define TORCH_TRACE_DISTANCE	144

#define TORCH_SPOT_MAX_ALPHA	128

#define TORCH_GLOW_MAX_ALPHA	200

void CTorch::UpdateSpot(void)
{

#ifndef CLIENT_DLL
	if (m_fUpdate)
	{
		TurnOn();
		m_fUpdate = FALSE;
	}

	if (m_fIsOn && m_pSpot)
	{
		if (!m_pSpot)
		{
			m_pSpot = CFlashlightSpot::CreateSpot();
		}

		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

		UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);

		if (!m_pGlow)
		{
			m_pGlow = CSprite::SpriteCreate("sprites/flare1.spr", pev->origin, FALSE);
			m_pGlow->SetTransparency(kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation);
			m_pGlow->SetScale(1.0f);
		}

		UTIL_SetOrigin(m_pGlow->pev, tr.vecEndPos);

		if (UTIL_PointContents(tr.vecEndPos) == CONTENTS_SKY)
		{
			m_pSpot->pev->renderamt = 0;
			m_pGlow->SetBrightness(0);
		}
		else
		{
			m_pSpot->pev->renderamt = TORCH_SPOT_MAX_ALPHA;

			float dist = (tr.vecEndPos - vecSrc).Length();

			float brightness;

			if (dist < TORCH_TRACE_DISTANCE)
				brightness = ((TORCH_TRACE_DISTANCE - dist) * TORCH_GLOW_MAX_ALPHA) / TORCH_TRACE_DISTANCE;
			else
				brightness = 0;

			m_pGlow->SetBrightness(brightness);

			// ALERT(at_console, "Server: Glow brightness: %f\n", brightness);
		}
	}
#endif
}