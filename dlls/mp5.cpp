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

enum m4a1_e
{
	M4A1_IDLE1 = 0,
	M4A1_DEPLOY,
	M4A1_HOLSTER,
	M4A1_FIRE1,
	M4A1_RELOAD,
	M4A1_RELOAD_NOT_EMPTY,
	M4A1_LAUNCH_GREN,
	M4A1_LAST_GREN,
};

LINK_ENTITY_TO_CLASS( weapon_m4a1, CMP5 )

//=========================================================
//=========================================================
int CMP5::SecondaryAmmoIndex( void )
{
	return m_iSecondaryAmmoType;
}

void CMP5::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m4a1"); // hack to allow for old names
	Precache();
	m_iId = WEAPON_MP5;
	SET_MODEL(ENT(pev), "models/w_m4a1.mdl");

	m_iDefaultAmmo = RANDOM_LONG(8, 21);

#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
		m_iDefaultAmmo = MP5_DEFAULT_GIVE_MP;

	FallInit();// get ready to fall down.
}

void CMP5::Precache( void )
{
	PRECACHE_MODEL("models/v_m4a1.mdl");
	PRECACHE_MODEL("models/w_m4a1.mdl");
	PRECACHE_MODEL("models/p_m4a1.mdl");

	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/556mm_fire.wav");

	PRECACHE_SOUND("items/gl_insertshell.wav");
	PRECACHE_SOUND("items/glauncher_in.wav");
	PRECACHE_SOUND("items/glauncher_out.wav");
	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );
	PRECACHE_SOUND("items/AR_slide.wav");
	PRECACHE_SOUND("items/556mm_pickup.wav");

	PRECACHE_SOUND("fvox/ammo_low.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_iShell = PRECACHE_MODEL("models/rifleshell.mdl");// brass shell
	PRECACHE_MODEL("models/40mmshell.mdl");// 40mm shell

	m_usMP5 = PRECACHE_EVENT( 1, "events/mp5.sc" );
	m_usMP52 = PRECACHE_EVENT( 1, "events/mp52.sc" );
	m_usMP53 = PRECACHE_EVENT(1, "events/mp53.sc");
}

int CMP5::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "556mm";
	p->iMaxAmmo1 = _556MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	p->iMaxClip = MP5_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5;
	p->iWeight = 8;

	return 1;
}

int CMP5::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMP5::Deploy()
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	return DefaultDeploy( "models/v_m4a1.mdl", "models/p_m4a1.mdl", M4A1_DEPLOY, "mp5" );
}

void CMP5::Holster(int skiplocal /* = 0 */)
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230);
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.35;
	SendWeaponAnim(M4A1_HOLSTER);
}

void CMP5::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	if (m_iClip == 7)
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

#ifndef CLIENT_DLL

	UTIL_ScreenShake(pev->origin, 1, 5.0, 0.15, 120);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);	// player "shoot" animation

#endif

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;
#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		// optimized multiplayer. Widened to make it easier to hit a moving player
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_556MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// single player spread
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_2DEGREES, 8192, BULLET_PLAYER_556MM, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.095;

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.095;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMP5::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2f;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

#ifndef CLIENT_DLL
	UTIL_ScreenShake(pev->origin, 7, 150.0, 0.3, 120);
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);	// player "shoot" animation
 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
#endif

	// we don't add in player velocity anymore.
	CGrenade::ShootContact( m_pPlayer->pev,
					m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f,
					gpGlobals->v_forward * 1200 );

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usMP53);
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.85;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.85;
	}
	else
	{
		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usMP52 );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.8;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.8;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;// idle pretty soon after shooting.

	if( !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
}

void CMP5::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == MP5_MAX_CLIP )
		return;
	else
	{
		if ( m_iClip == 0 )
		{
			DefaultReload( MP5_MAX_CLIP, M4A1_RELOAD, 1.7 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.75;
		}
		else
		{
			DefaultReload( MP5_MAX_CLIP, M4A1_RELOAD_NOT_EMPTY, 1.7 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.20;
		}
	}
}

void CMP5::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( M4A1_IDLE1 );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}

class CMP5AmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_556mmARclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_556mmARclip.mdl");
		PRECACHE_SOUND("items/556mm_pickup.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_MP5CLIP_GIVE, "556mm", _556MM_MAX_CARRY) != -1);
		if( bResult )
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/556mm_pickup.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556mmclip, CMP5AmmoClip );

class CMP5AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache();
		SET_MODEL( ENT( pev ), "models/w_ARgrenade.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_ARgrenade.mdl" );
		PRECACHE_SOUND("items/556mm_pickup.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = ( pOther->GiveAmmo( AMMO_M203BOX_GIVE, "ARgrenades", M203_GRENADE_MAX_CARRY ) != -1 );

		if( bResult )
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/556mm_pickup.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_mp5grenades, CMP5AmmoGrenade )
LINK_ENTITY_TO_CLASS( ammo_ARgrenades, CMP5AmmoGrenade )
