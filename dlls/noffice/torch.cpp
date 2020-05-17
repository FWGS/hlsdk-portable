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

enum torch_e {
	TORCH_IDLE_OFF = 0,
	TORCH_DRAW,
	TORCH_IDLE_ON,
	TORCH_SWITCH,
	TORCH_HOLSTER_OFF,
	TORCH_HOLSTER_ON
};

LINK_ENTITY_TO_CLASS( weapon_torch, CTorch )

void CTorch::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_torch.mdl" );
	m_iId = WEAPON_TORCH;
	m_iClip = -1;

	FallInit();// get ready to fall down.
}

void CTorch::Precache()
{
	PRECACHE_MODEL( "models/v_torch.mdl" );
	PRECACHE_MODEL( "models/w_torch.mdl" );
	PRECACHE_MODEL( "models/p_torch.mdl" );

	PRECACHE_SOUND( "items/flashlight1.wav" );

	UTIL_PrecacheOther( "light_spot2" );
	UTIL_PrecacheOther( "wall_spot" );
}

int CTorch::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 2;
	p->iId = WEAPON_TORCH;
	p->iWeight = TORCH_WEIGHT;

	return 1;
}

int CTorch::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CTorch::Deploy()
{
	return DefaultDeploy( "models/v_torch.mdl", "models/p_torch.mdl", TORCH_DRAW, "torch" );
}

void CTorch::Holster( int skiplocal /*= 0*/ )
{
	if( m_fIsOn )
		PrimaryAttack();

	SendWeaponAnim( TORCH_HOLSTER_OFF );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CTorch::PrimaryAttack()
{
	SendWeaponAnim( TORCH_SWITCH );

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "items/flashlight1.wav", VOL_NORM, ATTN_NORM );

	m_fIsOn = ( m_fIsOn == FALSE );

	if( !m_fIsOn )
	{
		SetBits( m_pLightSpot->pev->effects, EF_NODRAW );
		ClearBits( m_pLightSpot->pev->effects, EF_DIMLIGHT );

		SetBits( m_pWallSpot->pev->effects, EF_NODRAW );
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4f;
}

void CTorch::WeaponIdle()
{
	UpdateSpot();

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f; // how long till we do this again.
}

void CTorch::UpdateSpot()
{
	if( m_fIsOn )
	{
		if( !m_pLightSpot )
		{
			m_pLightSpot = CLightSpot::CreateSpot();
			m_pWallSpot = CWallSpot::CreateSpot();
		}
		else
		{
			ClearBits( m_pLightSpot->pev->effects, EF_NODRAW );
			SetBits( m_pLightSpot->pev->effects, EF_DIMLIGHT );

			ClearBits( m_pWallSpot->pev->effects, EF_NODRAW );
		}

		UTIL_MakeVectors( m_pPlayer->pev->v_angle );

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = gpGlobals->v_forward;

		TraceResult tr;

		UTIL_SetOrigin( m_pLightSpot->pev, vecSrc + vecAiming * 10.0f );

		UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 250.0f, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

		UTIL_SetOrigin( m_pWallSpot->pev, tr.vecEndPos );
	}
}
