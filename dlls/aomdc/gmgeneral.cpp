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

enum gmgeneral_e
{
	GMGENERAL_IDLE,
	GMGENERAL_FIRE1,
	GMGENERAL_FIRE2,
	GMGENERAL_RELOAD,
	GMGENERAL_DRAW
};


LINK_ENTITY_TO_CLASS( weapon_gmgeneral, CGMGeneral );


//=========================================================
//=========================================================
void CGMGeneral::Spawn( )
{
	Precache( );
	SET_MODEL(ENT(pev), "models/gmgeneral_around.aomdc");
	m_iId = WEAPON_GMGENERAL;

	m_iDefaultAmmo = GMGENERAL_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CGMGeneral::Precache( void )
{
	PRECACHE_MODEL("models/gmgeneral_display.aomdc");
	PRECACHE_MODEL("models/gmgeneral_around.aomdc");
	PRECACHE_MODEL("models/p_9mmAR.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_SOUND("gmgeneral/gm_expell.wav");
	PRECACHE_SOUND("gmgeneral/gm_fov.wav");

	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND ("weapons/357_cock1.wav");

	m_usGMGeneral = PRECACHE_EVENT( 1, "events/mp5k2.sc" );
}

int CGMGeneral::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = GMGENERAL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = GMGENERAL_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GMGENERAL;
	p->iWeight = GMGENERAL_WEIGHT;

	return 1;
}

int CGMGeneral::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CGMGeneral::Deploy( )
{
	return DefaultDeploy( "models/gmgeneral_display.aomdc", "models/p_9mmAR.mdl", GMGENERAL_DRAW, "L85 Spec Wep" );
}

void CGMGeneral::Holster( int skiplocal /* = 0 */ )
{
	if( m_fInZoom )	
		SecondaryAttack();
}

void CGMGeneral::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

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

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usGMGeneral, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.07;

	if ( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.07;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CGMGeneral::SecondaryAttack()
{
	if ( m_pPlayer->pev->fov != 0 )
        {
                m_fInZoom = FALSE;
                m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 0;  // 0 means reset to default fov
        }
        else if ( m_pPlayer->pev->fov != 20 )
        {
                m_fInZoom = TRUE;
                m_pPlayer->pev->fov = m_pPlayer->m_iFOV = 20;
        }

	EMIT_SOUND(ENT(pev), CHAN_ITEM, "gmgeneral/gm_fov.wav", 1, ATTN_NORM);
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.4;
}

void CGMGeneral::Reload( void )
{
	DefaultReload( GMGENERAL_MAX_CLIP, GMGENERAL_RELOAD, 2.63 );
}

void CGMGeneral::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	SendWeaponAnim( GMGENERAL_IDLE );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

class CGMGeneralAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/gmgeneral_around.aomdc");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/gmgeneral_around.aomdc");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_GMGENERALCLIP_GIVE, "", GMGENERAL_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_gmgeneral, CGMGeneralAmmoClip );
