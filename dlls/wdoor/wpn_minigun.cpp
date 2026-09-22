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
#include "soundent.h"
#include "gamerules.h"

enum m134_e
{
	M134_IDLE1 = 0,
	M134_IDLE2,
	M134_SPINUP,
	M134_SPINDOWN,
	M134_FIRE1,
	M134_FIRE2,
	M134_FIRE3,
	M134_RELOAD,
	M134_DEPLOY,
	M134_HOLSTER,
	M134_SPIN,
};

LINK_ENTITY_TO_CLASS( weapon_minigun, CM134 );
LINK_ENTITY_TO_CLASS( weapon_minigun_drop, CM134 );
void CM134::Spawn( )
{
	Precache( );

	SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
	pev->body = 1;

	m_iId = WEAPON_M134;

	m_iDefaultAmmo = 100;

	#ifndef CLIENT_DLL
	if ( FClassnameIs( pev, "weapon_minigun_drop" ) ){
	m_iDefaultAmmo = 50;
	}
	#endif

	pev->classname = MAKE_STRING("weapon_minigun"); // hack to allow for old names
	FallInit();// get ready to fall down.
}


void CM134::Precache( void )
{
	PRECACHE_MODEL("models/v_minigun.mdl");

	PRECACHE_SOUND ("weapons/minigun_fire.wav");
	PRECACHE_SOUND ("weapons/minigun_reload.wav");
	PRECACHE_SOUND ("weapons/minigun_spinup.wav");
}

int CM134::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "762natobox";
	p->iMaxAmmo1 = MINIGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 100;
	p->iSlot = 5;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M134;
	p->iWeight = M134_WEIGHT;

	return 1;
}

int CM134::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CM134::Deploy( )
{
	m_pPlayer->m_newcross_active = 1;
	m_minigunspin = 0;
	return DefaultDeploy( "models/v_minigun.mdl", 0, M134_DEPLOY, "mp5",0,114 );
}

void CM134::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	SendWeaponAnim( M134_HOLSTER );
	m_minigunspin = 0;
	m_fInReload = FALSE;
}

void CM134::PrimaryAttack()
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

	if ( m_minigunspin == 0)
	{
		// spin up
		SendWeaponAnim(M134_SPINUP);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
		m_minigunspin += 1;
		#ifndef CLIENT_DLL
		EMIT_SOUND_DYN ( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/minigun_spinup.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		#endif
	}
	else if ( m_minigunspin == 1)
	{
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
			m_minigunspin = 2;
	}
	else if ( m_minigunspin == 2){

	#ifndef CLIENT_DLL
	UTIL_MakeVectors (m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	float flZVel = m_pPlayer->pev->velocity.z;
	m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 20;
	m_pPlayer->pev->velocity.z = flZVel;
	#endif

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim(M134_FIRE1 + RANDOM_LONG(0,2));

	#ifndef CLIENT_DLL
	FX_FireGun(m_pPlayer->pev->v_angle, m_pPlayer->entindex(), 200, 0, FIREGUN_MINIGUN );
	#endif

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, Vector(0.04,0.04,0.04), 5000, BULLET_762, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.07;

//	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	m_pPlayer->pev->punchangle.x += RANDOM_LONG(-1,1);

	KickBack(0.25, 0.35, 0.25, 0.025, 3.0, 2.5, 9);

	}

}

void CM134::SecondaryAttack()
{
	if ( m_minigunspin == 0)
	{
		if (m_pPlayer->pev->waterlevel == 3 || m_iClip <= 0)
	 	{
			PlayEmptySound( );
			m_flNextPrimaryAttack = gpGlobals->time + 0.5;
			return;
		}

		// spin up
		SendWeaponAnim(M134_SPINUP);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1;
		m_minigunspin += 1;
		#ifndef CLIENT_DLL
		EMIT_SOUND_DYN ( ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/minigun_spinup.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		#endif
	}
	else if (m_minigunspin == 1)
	{
		if ( m_iClip <= 0 )
		{
			WeaponIdle();
			return;
		}
		if (m_flTimeWeaponIdle < UTIL_WeaponTimeBase())
			m_minigunspin = 2;
	}
	if (m_minigunspin == 2)
	{
		if ( m_iClip <= 0 )
		{
			WeaponIdle();
			return;
		}

		SendWeaponAnim(M134_SPIN);
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
	}
}


void CM134::Reload( void )
{
	if ( m_pPlayer->ammo_762natobox <= 0 )
		return;

	DefaultReload( 100, M134_RELOAD, 6.0 );
	m_minigunspin = 0;
}


void CM134::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if (m_minigunspin != 0)
	{
		SendWeaponAnim(M134_SPINDOWN);
		m_minigunspin = 0;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
		return;
	}

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = M134_IDLE1;	
		break;
	
	default:
	case 1:
		iAnim = M134_IDLE2;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CM134AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_all_items5.mdl");
		pev->body = 0;
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( 100, "762natobox", MINIGUN_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_m134box, CM134AmmoClip );
