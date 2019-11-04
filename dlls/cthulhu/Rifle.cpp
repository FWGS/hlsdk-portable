/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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


enum rifle_e {
	RIFLE_IDLE1 = 0,
	RIFLE_FIRE1,
	RIFLE_RELOAD,
	RIFLE_CLOSEBREAK,
	RIFLE_BREAK,
	RIFLE_DRAW,
	RIFLE_HOLSTER
};


#include "Rifle.h"


LINK_ENTITY_TO_CLASS( weapon_rifle, CRifle );

int CRifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Rifle";
	p->iMaxAmmo1 = RIFLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RIFLE_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_RIFLE;
	p->iWeight = RIFLE_WEIGHT;

	return 1;
}

int CRifle::AddToPlayer( CBasePlayer *pPlayer )
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

void CRifle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_rifle"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_RIFLE;
	SET_MODEL(ENT(pev), "models/w_rifle.mdl");

	m_iDefaultAmmo = RIFLE_DEFAULT_GIVE;

	m_fInZoom = FALSE;

	FallInit();// get ready to fall down.
}


void CRifle::Precache( void )
{
	PRECACHE_MODEL("models/v_rifle.mdl");
	PRECACHE_MODEL("models/w_rifle.mdl");
//	PRECACHE_MODEL("models/p_rifle.mdl");

	PRECACHE_MODEL("models/w_rifleammo.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/rifle_reload.wav");
	PRECACHE_SOUND ("weapons/rifle_break.wav");
	//PRECACHE_SOUND ("weapons/rifle_closebreak.wav");
	PRECACHE_SOUND ("weapons/rifle_fire.wav");
	PRECACHE_SOUND ("weapons/rifle_dryfire.wav");

	m_usFireRifle = PRECACHE_EVENT( 1, "events/rifle.sc" );
}

BOOL CRifle::Deploy( )
{
	if ( g_pGameRules->IsMultiplayer() )
	{
		// enable laser sight geometry.
		pev->body = 1;
	}
	else
	{
		pev->body = 0;
	}

	return DefaultDeploy( "models/v_rifle.mdl", "", RIFLE_DRAW, "rifle" );
}


void CRifle::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if ( m_fInZoom )
	{
		m_fInZoom = FALSE;
		m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + 10 + RANDOM_FLOAT ( 0, 5 );
	SendWeaponAnim( RIFLE_HOLSTER );
}

void CRifle::SecondaryAttack( void )
{
	if (m_iClip <= 0) return;

	if ( m_fInZoom )
	{
		m_fInZoom = FALSE;
		m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}
	else
	{
		m_fInZoom = TRUE;
		m_pPlayer->m_iFOV = 40;
	}

	m_flNextSecondaryAttack = gpGlobals->time + 0.5;
}

void CRifle::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (m_fInZoom)
			SecondaryAttack();

		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rifle_dryfire.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		}

		return;
	}

	PLAYBACK_EVENT( 0, m_pPlayer->edict(), m_usFireRifle );

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	if (m_iClip <= 0)
	{
		if (m_fInZoom)
			SecondaryAttack();
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );
	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_RIFLE, 0 );

	//if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
	//	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = gpGlobals->time + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}


void CRifle::Reload( void )
{
	if ( m_fInZoom )
	{
		m_fInZoom = FALSE;
		m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == RIFLE_MAX_CLIP)
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	// check to see if we're ready to reload
	if (m_fInReload == 0)
	{
		SendWeaponAnim( RIFLE_BREAK );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rifle_break.wav", 0.8, ATTN_NORM);
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.75;
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;
		m_flNextPrimaryAttack = gpGlobals->time + 1.0;
		m_flNextSecondaryAttack = gpGlobals->time + 1.0;
		return;
	}
	else if (m_fInReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		// was waiting for gun to move to side
		m_fInReload = 2;

		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/rifle_reload.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		//if (RANDOM_LONG(0,1))
		//	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/rifle_reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		//else
		//	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/rifle_reload2.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( RIFLE_RELOAD );

		m_flNextReload = gpGlobals->time + 0.5;
		m_flTimeWeaponIdle = gpGlobals->time + 0.5;
	}
	else
	{
		// Add them to the clip
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInReload = 1;
	}

	// like the HL shotgun reload...
	//if (DefaultReload( 6, RIFLE_RELOAD, 2.0 ))
	//{
	//	m_flSoundDelay = gpGlobals->time + 1.5;
	//}
}


void CRifle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_iClip == 0 && m_fInReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		Reload( );
	}
	else if (m_fInReload != 0)
	{
		if (m_iClip != RIFLE_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else
		{
			// reload debounce has timed out
			SendWeaponAnim( RIFLE_CLOSEBREAK );
			
			// play cocking sound
			//EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/rifle_closebreak.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/rifle_break.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
			m_fInReload = 0;
			m_flTimeWeaponIdle = gpGlobals->time + 0.75;
		}
	}
	else
	{
		SendWeaponAnim( RIFLE_IDLE1 );
		m_flTimeWeaponIdle = gpGlobals->time + (70.0/30.0);// * RANDOM_LONG(2, 5);
	}
}

//////////////////////////////////////////////////////////////////////////////

void CRifleAmmo::Spawn( void )
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/w_rifleammo.mdl");
	CBasePlayerAmmo::Spawn( );
}

void CRifleAmmo::Precache( void )
{
	PRECACHE_MODEL ("models/w_rifleammo.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CRifleAmmo::AddAmmo( CBaseEntity *pOther ) 
{ 
	if (pOther->GiveAmmo( AMMO_RIFLE_GIVE, "Rifle", RIFLE_MAX_CARRY ) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		return TRUE;
	}
	return FALSE;
}

LINK_ENTITY_TO_CLASS( ammo_rifle, CRifleAmmo );


#endif