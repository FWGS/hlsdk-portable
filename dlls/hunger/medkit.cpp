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

enum medkit_e
{
	MEDKIT_IDLE = 0,
	MEDKIT_LONGIDLE,
	MEDKIT_LONGUSE,
	MEDKIT_SHORTUSE,
	MEDKIT_HOLSTER,
	MEDKIT_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_th_medkit, CWeaponEinarMedkit )

void CWeaponEinarMedkit::Spawn()
{
	Precache();
	m_iId = WEAPON_MEDKIT;
	SET_MODEL( ENT( pev ), "models/w_tfc_medkit.mdl" );

	m_iDefaultAmmo = MEDKIT_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CWeaponEinarMedkit::Precache()
{
	PRECACHE_MODEL( "models/v_tfc_medkit.mdl" );
	PRECACHE_MODEL( "models/w_tfc_medkit.mdl" );
	PRECACHE_MODEL( "models/p_tfc_medkit.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshotno1.wav" );
}

int CWeaponEinarMedkit::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "health";
	p->iMaxAmmo1 = MEDKIT_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_MEDKIT;
	p->iWeight = MEDKIT_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

int CWeaponEinarMedkit::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWeaponEinarMedkit::Deploy()
{
	return DefaultDeploy( "models/v_tfc_medkit.mdl", "models/p_tfc_medkit.mdl", MEDKIT_DRAW, "trip" );
}

void CWeaponEinarMedkit::Holster(int skiplocal /*= 0*/)
{
	m_fInAttack = 0;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( MEDKIT_HOLSTER );
}

void CWeaponEinarMedkit::PrimaryAttack()
{
	if( m_pPlayer->pev->health >= m_pPlayer->pev->max_health
		|| m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		if( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		}
		return;
	}

	if( m_fInAttack )
	{
		if( m_flNextSecondaryAttack < UTIL_WeaponTimeBase() )
			Heal();
		return;
	}

	Use();
}

void CWeaponEinarMedkit::SecondaryAttack()
{
	if( m_fInAttack )
		Heal();
}

void CWeaponEinarMedkit::Use()
{
	SendWeaponAnim( MEDKIT_LONGUSE, 0 );
	m_fInAttack = 1;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2f;
}

void CWeaponEinarMedkit::Heal()
{
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_pPlayer->TakeHealth( 25, DMG_GENERIC );
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "items/medshot4.wav", 1.0, ATTN_NORM );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_fInAttack = 0;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.2f;
}

void CWeaponEinarMedkit::Reload()
{
	SecondaryAttack();
}

void CWeaponEinarMedkit::WeaponIdle()
{
	ResetEmptySound();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if( m_fInAttack )
	{
		Heal();
		return;
	}

	SendWeaponAnim( MEDKIT_IDLE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20.0f;
}

BOOL CWeaponEinarMedkit::PlayEmptySound()
{
	if( m_iPlayEmptySound )
	{
		EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "items/medshotno1.wav", 0.8, ATTN_NORM );
		m_iPlayEmptySound = 0;
	}
	return 0;
}
