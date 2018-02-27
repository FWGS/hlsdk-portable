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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"


enum revolver_e {
	REVOLVER_IDLE = 0,
	REVOLVER_SHOOT1,
	REVOLVER_SHOOT2,
	REVOLVER_SHOOT_EMPTY,
	REVOLVER_RELOAD,
	REVOLVER_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_revolver, CRevolver );
LINK_ENTITY_TO_CLASS( weapon_357, CRevolver );

int CRevolver::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = REVOLVER_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = REVOLVER_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_REVOLVER;
	p->iWeight = REVOLVER_WEIGHT;

	return 1;
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

void CRevolver::Spawn( )
{
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
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/357_shot1.wav");
	PRECACHE_SOUND ("weapons/357_shot2.wav");
	PRECACHE_SOUND ("weapons/revolver_draw.wav");
	PRECACHE_SOUND ("weapons/revolver_fire.wav");
	PRECACHE_SOUND ("weapons/revolver_reload.wav");


	m_usFireRevolver = PRECACHE_EVENT( 1, "events/deagle1.sc" );
}

BOOL CRevolver::Deploy( )
{
	pev->body = 0;
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/revolver_draw.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
	return DefaultDeploy( "models/v_revolver.mdl", "models/p_357.mdl", REVOLVER_DRAW, "Revolver", UseDecrement(), pev->body );
}

void CRevolver::PrimaryAttack()
{
	if( m_iClip <= 0 ||
		( m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD ) ) // don't fire underwater
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, gSkillData.plrDmgRevolver, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFireRevolver, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = 0.9;
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}


void CRevolver::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == REVOLVER_MAX_CLIP )
		return;

	if (DefaultReload( REVOLVER_MAX_CLIP, REVOLVER_RELOAD, 2.76))
	{
		m_flSoundDelay = 1.5;
	}
}


void CRevolver::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (m_flSoundDelay != 0 && m_flSoundDelay <= UTIL_WeaponTimeBase() )
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/revolver_reload.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
                return;

	SendWeaponAnim( REVOLVER_IDLE, 1 );
         
	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}



class CRevolverAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_weaponclips/w_revolverrounds.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_weaponclips/w_revolverrounds.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_REVOLVERBOX_GIVE, "357", REVOLVER_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_revolver, CRevolverAmmo );


#endif
