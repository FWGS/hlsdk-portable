/*
	Copyright (c) 1999, Cold Ice Modification. 
	
	This code has been written by SlimShady ( darcuri@optonline.net )

    Use, distribution, and modification of this source code and/or resulting
    object code is restricted to non-commercial enhancements to products from
    Valve LLC.  All other use, distribution, or modification is prohibited
    without written permission from Valve LLC and from the Cold Ice team.

    Please if you use this code in any public form, please give us credit.

*/


#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum uzi_e
{
	UZI_LONGIDLE = 0,
	UZI_IDLE1,
	UZI_LAUNCH,
	UZI_RELOAD,
	UZI_DEPLOY,
	UZI_FIRE1,
	UZI_FIRE2,
	UZI_FIRE3,
};


class CUzi : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_uzi, CUzi );

//=========================================================
//=========================================================

void CUzi::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_uzi"); 
	Precache( );
	SET_MODEL(ENT(pev), "models/wmodels/w_uzi.mdl");
	m_iId = WEAPON_UZI;

	m_iDefaultAmmo = UZI_DEFAULT_GIVE;

	FallInit();
}


void CUzi::Precache( void )
{
	PRECACHE_MODEL("models/vmodels/v_uzi.mdl");
	PRECACHE_MODEL("models/wmodels/w_uzi.mdl");
	PRECACHE_MODEL("models/pmodels/p_uzi.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");        

	PRECACHE_SOUND("weapons/uzi1.wav");

}

int CUzi::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = UZI_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = UZI_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_UZI;
	p->iWeight = UZI_WEIGHT;

	return 1;
}

int CUzi::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CUzi::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_uzi.mdl", "models/pmodels/p_uzi.mdl", UZI_DEPLOY, "mp5" );
}


void CUzi::PrimaryAttack()
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;


	if (1 || m_flNextAnimTime < gpGlobals->time)
	{
		SendWeaponAnim( UZI_FIRE1 + RANDOM_LONG(0,2));
		m_flNextAnimTime = gpGlobals->time + 0.2;
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

    EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/uzi1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); 

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

   Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 35;

	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs
					+ gpGlobals->v_up * -12 
					+ gpGlobals->v_forward * 35 
					+ gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_8DEGREES, 8192, BULLET_PLAYER_MP5, 0 );
	
	m_flNextPrimaryAttack = gpGlobals->time + 0.08;
	m_flNextSecondaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -2, 2 );
}

void CUzi::Reload( void )
{
	DefaultReload( UZI_MAX_CLIP, UZI_RELOAD, 1.5 );
}

void CUzi::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = UZI_LONGIDLE;	
		break;
	
	default:
	case 1:
		iAnim = UZI_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}

class CUziAmmoClip : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmARclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmARclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_UZICLIP_GIVE, "9mm", UZI_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_uziclip, CUziAmmoClip );


















