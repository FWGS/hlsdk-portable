/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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

#include "revolver.h"


enum revolver_e {
	REVOLVER_IDLE1 = 0,
	REVOLVER_FIDGET1,
	REVOLVER_FIRE,
	REVOLVER_RELOAD,
	REVOLVER_HOLSTER,
	REVOLVER_DRAW,
	REVOLVER_IDLE2,
	REVOLVER_IDLE3,
	REVOLVER_QUICKFIRE_READY,
	REVOLVER_QUICKFIRE_SHOOT,
	REVOLVER_QUICKFIRE_RELAX
};

LINK_ENTITY_TO_CLASS( weapon_revolver, CRevolver );


void CRevolver::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_revolver"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_REVOLVER;
	SET_MODEL(ENT(pev), "models/w_revolver.mdl");

	m_iDefaultAmmo = REVOLVER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CRevolver::Precache( void )
{
	PRECACHE_MODEL("models/v_revolver.mdl");
	PRECACHE_MODEL("models/w_revolver.mdl");
//	PRECACHE_MODEL("models/p_revolver.mdl");

	m_iShell = PRECACHE_MODEL ("models/revolver_rounds.mdl");// brass shell

	PRECACHE_SOUND ("weapons/revolver_cock1.wav");
	PRECACHE_SOUND ("weapons/revolver_reload1.wav");
	PRECACHE_SOUND ("weapons/revolver_shot1.wav");
	PRECACHE_SOUND ("weapons/revolver_shot2.wav");

	m_usFireRevolver1 = PRECACHE_EVENT( 1, "events/revolver1.sc" );
	m_usFireRevolver2 = PRECACHE_EVENT( 1, "events/revolver2.sc" );
}

int CRevolver::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Revolver";
	p->iMaxAmmo1 = REVOLVER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = REVOLVER_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_REVOLVER;
	p->iWeight = REVOLVER_WEIGHT;

	return 1;
}

BOOL CRevolver::Deploy( )
{
	// pev->body = 1;
	return DefaultDeploy( "models/v_revolver.mdl", "", REVOLVER_DRAW, "revolver", /*UseDecrement() ? 1 : 0*/ 0 );
}

int CRevolver::AddToPlayer( CBasePlayer *pPlayer )
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

void CRevolver::SecondaryAttack( void )
{
	// no difference
	PrimaryAttack();
}

void CRevolver::PrimaryAttack( void )
{
	RevolverFire( 0.01, 1.0, TRUE );
}

void CRevolver::RevolverFire( float flSpread , float flCycleTime, BOOL fUseAutoAim )
{
	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	int flags = 0;

//#if defined( CLIENT_WEAPONS )
//	flags = FEV_NOTHOST;
//#else
//	flags = 0;
//#endif

	if (fUseAutoAim)
	{
		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usFireRevolver1);
	}
	else
	{
		PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usFireRevolver2);
	}

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming;
	
	if ( fUseAutoAim )
	{
		vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	}
	else
	{
		vecAiming = gpGlobals->v_forward;
	}

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, Vector( flSpread, flSpread, flSpread ), 8192, BULLET_PLAYER_REVOLVER, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

	//if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
	//	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CRevolver::Reload( void )
{
	int iResult;

	iResult = DefaultReload( REVOLVER_MAX_CLIP, REVOLVER_RELOAD, 111.0/36.0 );

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

void CRevolver::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	// only idle if the slid isn't back
	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0, 1.0 );

		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = REVOLVER_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = REVOLVER_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0 / 16.0;
		}
		else
		{
			iAnim = REVOLVER_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim, 1 );
	}
}



/////////////////////////////////////////////////////////////////////////////


void CRevolverAmmo::Spawn( void )
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/w_revolverammo.mdl");
	CBasePlayerAmmo::Spawn( );
}

void CRevolverAmmo::Precache( void )
{
	PRECACHE_MODEL ("models/w_revolverammo.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");
}

BOOL CRevolverAmmo::AddAmmo( CBaseEntity *pOther ) 
{ 
	if (pOther->GiveAmmo( AMMO_REVOLVER_GIVE, "Revolver", REVOLVER_MAX_CARRY ) != -1)
	{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		//EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/revolver_rounds1.wav", 1, ATTN_NORM);
		return TRUE;
	}
	return FALSE;
}

LINK_ENTITY_TO_CLASS( ammo_revolver, CRevolverAmmo );















