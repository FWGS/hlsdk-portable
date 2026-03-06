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
*	// SMG
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

enum smg_e
{
	SMG_LONGIDLE = 0,
	SMG_IDLE1,
	SMG_RELOAD,
	SMG_RELOAD_NOT_EMPTY,
	SMG_DEPLOY,
	SMG_FIRE1,
	SMG_FIRE2,
	SMG_HOLSTER,
	SMG_SILENCER_ADD,
	SMG_SILENCER_REMOVE,
};


LINK_ENTITY_TO_CLASS( weapon_smg, CSMG );

//=========================================================
//=========================================================
void CSMG::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_smg");
	Precache( );
	SET_MODEL(ENT(pev), "models/w_smg.mdl");
	m_iId = WEAPON_SMG;

	m_iDefaultAmmo = RANDOM_LONG(12,24);

	FallInit();
}

void CSMG::Precache( void )
{
	PRECACHE_MODEL("models/v_smg.mdl");
	PRECACHE_MODEL("models/w_smg.mdl");
	PRECACHE_MODEL("models/p_smg.mdl");

	m_iShell = PRECACHE_MODEL ("models/45acp_shell.mdl");

	PRECACHE_MODEL("models/w_45ACPclip.mdl");
	PRECACHE_SOUND("items/45ACP_ammo_pickup.wav");              

	PRECACHE_SOUND("items/smg_clipinsert1.wav");
	PRECACHE_SOUND("items/smg_cliprelease1.wav");
	PRECACHE_SOUND("items/smg_slide.wav");
	PRECACHE_SOUND("items/smg_silencer_add.wav");
	PRECACHE_SOUND("items/smg_silencer_remove.wav");

	PRECACHE_SOUND ("fvox/ammo_low.wav");

	PRECACHE_SOUND ("weapons/smg_fire1.wav");
	PRECACHE_SOUND ("weapons/smg_fire2.wav");
	PRECACHE_SOUND ("weapons/smg_deploy.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/common_hand2.wav");	

	m_usSMG = PRECACHE_EVENT( 1, "events/smg.sc" );
	m_usSMG2 = PRECACHE_EVENT( 1, "events/smg2.sc" );
}

int CSMG::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "45ACP";
	p->iMaxAmmo1 = _45ACP_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SMG_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SMG;
	p->iWeight = SMG_WEIGHT;
	
	return 1;
}

int CSMG::AddToPlayer( CBasePlayer *pPlayer )
{
	m_iSilencer = FALSE;

	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL CSMG::Deploy( )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_flSilencerTime = 0;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.5;
	return DefaultDeploy( "models/v_smg.mdl", "models/p_smg.mdl", SMG_DEPLOY, "smg" );

}

void CSMG::Holster( int skiplocal /* = 0 */ )
{
	g_engfuncs.pfnSetClientMaxspeed(m_pPlayer->edict(), 230 );
	m_fInReload = FALSE;// cancel any reload in progress.
	
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.45;
	SendWeaponAnim( SMG_HOLSTER );
}

void CSMG::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}


	m_iClip--;

	if (m_iClip == 8)
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_STATIC, "fvox/ammo_low.wav", 1.0, ATTN_NORM);

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

#ifndef CLIENT_DLL

	UTIL_ScreenShake( pev->origin, 1, 5.0, 0.15, 120); 
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );	// player "shoot" animation

#endif

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir;

	  int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	if ( !m_iSilencer )
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_5DEGREES, 8192, BULLET_PLAYER_45ACP, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSMG, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0 );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.07;
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_45ACP, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed );
		PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSMG2, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, pev->body, 0, 0, 0 );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.09;
	}

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	 m_flTimeWeaponIdle = 0.3;
}

void CSMG::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SMG_MAX_CLIP )
	{
		return;
	}
	else
	{
		if ( m_iClip == 0 )
		{
			DefaultReload( SMG_MAX_CLIP, SMG_RELOAD, 1.7 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.7;
			m_flTimeWeaponIdle = 2.8;
		}
		else
		{
			DefaultReload( SMG_MAX_CLIP, SMG_RELOAD_NOT_EMPTY, 1.7 );
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = 2.2;
			m_flTimeWeaponIdle = 2.3;
		}
	}
}


void CSMG::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = SMG_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = SMG_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = 0;
}

void CSMG::SecondaryAttack()
{
	if ( m_iSilencer )
	{
		m_iSilencer = FALSE;
		m_flSilencerTime = gpGlobals->time + 1.85;
	}
	else
	{
		pev->body = 1;
		m_iSilencer = TRUE;
	}
	Deploy( );
		
	if ( m_iSilencer )
	SendWeaponAnim( SMG_SILENCER_ADD );
	else
	SendWeaponAnim( SMG_SILENCER_REMOVE );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.85;
}

void CSMG::ItemPostFrame() // Hack to restore silencer on save-restore
{
	if ( m_iSilencer )
	pev->body = 1;

	if ( !m_iSilencer && (m_flSilencerTime < gpGlobals->time) )
	{
	pev->body = 0;
	m_flSilencerTime = 0;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

class CSMGAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_45ACPclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_45ACPclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_45ACPCLIP_GIVE, "45ACP", _45ACP_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/45ACP_ammo_pickup.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_45ACPclip, CSMGAmmoClip );




//=========================================================
// No silencer (basic complectation) version
//=========================================================

class CSMG_nosilencer : public CSMG
{
public:
	int GetItemInfo(ItemInfo *p)
	{
		p->pszName = STRING(pev->classname);
		p->pszAmmo1 = "45ACP";
		p->iMaxAmmo1 = _45ACP_MAX_CARRY;
		p->pszAmmo2 = NULL;
		p->iMaxAmmo2 = -1;
		p->iMaxClip = SMG_MAX_CLIP;
		p->iSlot = 2;
		p->iPosition = 4;
		p->iFlags = 0;
		p->iId = m_iId = WEAPON_SMG_NOSILENCER;
		p->iWeight = SMG_WEIGHT;
		
		return 1;
	}

	int AddToPlayer( CBasePlayer *pPlayer )
	{
		m_iSilencer = FALSE;

		if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
				WRITE_BYTE( m_iId );
			MESSAGE_END();
			return TRUE;
		}
		return FALSE;
	}

	void Spawn( )
	{
		pev->classname = MAKE_STRING("weapon_smg_nosilencer");
		Precache( );
		SET_MODEL(ENT(pev), "models/w_smg.mdl");
		m_iId = WEAPON_SMG_NOSILENCER;

		m_iDefaultAmmo = SMG_MAX_CLIP;

		FallInit();
	}

	void SecondaryAttack()
	{
		m_iSilencer = FALSE;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = 0.08;
	}
};
LINK_ENTITY_TO_CLASS( weapon_smg_nosilencer, CSMG_nosilencer );
