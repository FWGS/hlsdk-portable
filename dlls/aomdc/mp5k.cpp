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

enum mp5_e
{
	MP5K_IDLE = 0,
	MP5K_FIDGET,
	MP5K_FIRE1,
	MP5K_FIRE2,
	MP5K_RELOAD,
	MP5K_DRAW
};



LINK_ENTITY_TO_CLASS( weapon_mp5k, CMP5K );

//=========================================================
//=========================================================
void CMP5K::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_mp5k.mdl");
	m_iId = WEAPON_MP5K;

	m_iDefaultAmmo = MP5K_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CMP5K::Precache( void )
{
	PRECACHE_MODEL("models/v_mp5k.mdl");
	PRECACHE_MODEL("models/w_mp5k.mdl");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/mp5k_boltback.wav");
	PRECACHE_SOUND ("weapons/mp5k_boltforward.wav");
	PRECACHE_SOUND ("weapons/mp5k_fire.wav");
	PRECACHE_SOUND ("weapons/mp5k_magin.wav");
	PRECACHE_SOUND ("weapons/mp5k_magout.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usMP5k = PRECACHE_EVENT( 1, "events/mp5k.sc" );
}

int CMP5K::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mmar";
	p->iMaxAmmo1 = MP5K_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MP5K_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MP5K;
	p->iWeight = MP5K_WEIGHT;

	return 1;
}

int CMP5K::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMP5K::Deploy( )
{
	return DefaultDeploy( "models/v_mp5k.mdl", "models/p_9mmAR.mdl", MP5K_DRAW, "MP5k" );
}


void CMP5K::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 2, gSkillData.plrDmgMP5K, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usMP5k, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06f;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.06f;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMP5K::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == MP5K_MAX_CLIP )
		return;

	DefaultReload( MP5K_MAX_CLIP, MP5K_RELOAD, 3.6f );
}


void CMP5K::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( RANDOM_LONG( MP5K_IDLE, MP5K_FIDGET ) );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CMP5KAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponclips/w_mp5kclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponclips/w_mp5kclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{
		int bResult = (pOther->GiveAmmo( AMMO_MP5KCLIP_GIVE, "9mmar", MP5K_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_mp5k, CMP5KAmmoClip );
LINK_ENTITY_TO_CLASS( ammo_9mmAR, CMP5KAmmoClip );

