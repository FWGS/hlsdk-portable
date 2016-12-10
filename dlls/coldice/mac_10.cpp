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

enum mac_e
{
	MAC_IDLE1 = 0,
	MAC_IDLE2,
	MAC_IDLE3,
	MAC_SHOOT,
	MAC_SHOOT_EMPTY,
	MAC_RELOAD,
	MAC_RELOAD_NOT_EMPTY,
	MAC_DRAW,
	MAC_HOLSTER,
	MAC_ADD_SILENCER
};


class CMac : public CBasePlayerWeapon
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
LINK_ENTITY_TO_CLASS( weapon_mac10, CMac );

//=========================================================
//=========================================================

void CMac::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_mac10"); 
	Precache( );
	SET_MODEL(ENT(pev), "models/wmodels/w_mac10.mdl");
	m_iId = WEAPON_MAC10;

	m_iDefaultAmmo = MAC10_DEFAULT_GIVE;

	FallInit();
}


void CMac::Precache( void )
{
	PRECACHE_MODEL("models/vmodels/v_mac10.mdl");
	PRECACHE_MODEL("models/wmodels/w_mac10.mdl");
	PRECACHE_MODEL("models/pmodels/p_mac10.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");        

	PRECACHE_SOUND("weapons/mac1.wav");

}

int CMac::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = MAC10_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = MAC10_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_MAC10;
	p->iWeight = MAC10_WEIGHT;

	return 1;
}

int CMac::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CMac::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_mac10.mdl", "models/pmodels/p_mac10.mdl", MAC_DRAW, "mp5" );
}


void CMac::PrimaryAttack()
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
		SendWeaponAnim( MAC_SHOOT );
		m_flNextAnimTime = gpGlobals->time + 0.2;
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

    EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/mac1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); 

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

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_7DEGREES, 8192, BULLET_PLAYER_MP5, 0 );
	
	m_flNextPrimaryAttack = gpGlobals->time + 0.09;
	m_flNextSecondaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( -2, 2 );
}

void CMac::Reload( void )
{
	DefaultReload( MAC10_MAX_CLIP, MAC_RELOAD, 1.5 );
}

void CMac::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = MAC_IDLE2;	
		break;
	
	default:
	case 1:
		iAnim = MAC_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}



















