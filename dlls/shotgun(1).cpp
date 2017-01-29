/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e
{
	PEPSIGUN_IDLE = 0,
	PEPSIGUN_FIRE,
	PEPSIGUN_OPEN,
	PEPSIGUN_INSERT,
	PEPSIGUN_CLOSE,
	PEPSIGUN_DRAW
};

class CPepsigun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);


	void PrimaryAttack( void );
	void Reload( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	int m_fInReload;
	float m_flNextReload;

	virtual BOOL UseDecrement( void )
	{ 
		return FALSE;
	}
};


LINK_ENTITY_TO_CLASS( weapon_pepsigun, CPepsigun )

void CPepsigun::Spawn()
{
	Precache();
	m_iId = WEAPON_PEPSIGUN;
	SET_MODEL( ENT( pev ), "models/w_pepsigun.mdl" );

	m_iDefaultAmmo = PEPSIGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CPepsigun::Precache( void )
{
	PRECACHE_MODEL( "models/v_pepsigun.mdl" );
	PRECACHE_MODEL( "models/w_pepsigun.mdl" );
	PRECACHE_MODEL( "models/p_pepsigun.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );// shotgun shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/dbarrel1.wav" );//shotgun
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );//shotgun

	PRECACHE_SOUND( "weapons/reload1.wav" );	// shotgun reload
	PRECACHE_SOUND( "weapons/reload3.wav" );	// shotgun reload

	//PRECACHE_SOUND( "weapons/sshell1.wav" );	// shotgun reload - played on client
	//PRECACHE_SOUND( "weapons/sshell3.wav" );	// shotgun reload - played on client
	
	PRECACHE_SOUND( "weapons/357_cock1.wav" ); // gun empty sound
	PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun
}

int CPepsigun::AddToPlayer( CBasePlayer *pPlayer )
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

int CPepsigun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 8;
	p->iSlot = 2;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PEPSIGUN;
	p->iWeight = 16;

	return 1;
}

BOOL CPepsigun::Deploy()
{
	return DefaultDeploy( "models/v_pepsigun.mdl", "models/p_pepsigun.mdl", PEPSIGUN_DRAW, "PEPSIGUN" );
}

void CPepsigun::PrimaryAttack()
{
	m_iClip--;
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	if( m_iClip != 0 )
		m_flPumpTime = gpGlobals->time + 0.5;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	m_fInSpecialReload = 0;
}

void CPepsigun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP )
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;

	// check to see if we're ready to reload
	if( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( PEPSIGUN_OPEN );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.6;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0;
		return;
	}
	else if( m_fInSpecialReload == 1 )
	{
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );
		else
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );

		SendWeaponAnim( PEPSIGUN_INSERT );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}

void CPepsigun::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
		m_flPumpTime = 0;
	}

	if( m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if( m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			Reload();
		}
		else if( m_fInSpecialReload != 0 )
		{
			if( m_iClip != 8 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( PEPSIGUN_PUMP );
				
				// play cocking sound
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );

				iAnim = PEPSIGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0 / 9.0 );
			SendWeaponAnim( iAnim );
		}
	}
}

class CPepsigunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_shotbox.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_shotbox.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if( pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CPepsigunAmmo )
