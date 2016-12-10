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

enum ppk_e {
	PPK_IDLE1 = 0,
	PPK_IDLE2,
	PPK_IDLE3,
	PPK_SHOOT,
	PPK_SHOOT_EMPTY,
	PPK_RELOAD,
	PPK_RELOAD_NOT_EMPTY,
	PPK_DRAW,
	PPK_HOLSTER,
	PPK_ADD_SILENCER
};

class CWalterppk : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	int menu_on;

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void AddSilencer( void );
	void ScrewMe( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;
};
LINK_ENTITY_TO_CLASS( weapon_ppk, CWalterppk );


void CWalterppk::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_ppk"); 
	Precache( );
	m_iId = WEAPON_PPK;
	SET_MODEL(ENT(pev), "models/w_9mmhandgun.mdl");

	m_iDefaultAmmo = PPK_DEFAULT_GIVE;

	FallInit();
}


void CWalterppk::Precache( void )
{
	PRECACHE_MODEL("models/vmodels/v_ppk.mdl");
	PRECACHE_MODEL("models/w_9mmhandgun.mdl");
	PRECACHE_MODEL("models/pmodels/p_ppk.mdl");
	PRECACHE_MODEL("models/pmodels/p_ppks.mdl");

	m_iShell = PRECACHE_MODEL ("models/shell.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("items/9mmclip2.wav");
	PRECACHE_SOUND ("weapons/ppk1.wav");
	PRECACHE_SOUND ("weapons/ppk2.wav");
	PRECACHE_SOUND ("weapons/ppkbond.wav");
}

int CWalterppk::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "32acp";
	p->iMaxAmmo1 = PPK_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PPK_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_PPK;
	p->iWeight = PPK_WEIGHT;

	return 1;
}
int CWalterppk::AddToPlayer( CBasePlayer *pPlayer )
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

BOOL CWalterppk::Deploy( )
{
	if ( pev->body == 1 )
		return DefaultDeploy( "models/vmodels/v_ppk.mdl", "models/pmodels/p_ppks.mdl", PPK_DRAW, "onehanded" );
	else
		return DefaultDeploy( "models/vmodels/v_ppk.mdl", "models/pmodels/p_ppk.mdl", PPK_DRAW, "onehanded" );
}

void CWalterppk::PrimaryAttack( void )
{
	if (m_iClip <= 0 )
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->time + 0.2;
		}

		return;
	}

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	//if (m_iClip != 0)
	SendWeaponAnim( PPK_SHOOT );
	//else
	//	SendWeaponAnim( PPK_SHOOT_EMPTY );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
		
	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(50,70) 
							 + gpGlobals->v_up * RANDOM_FLOAT(100,150) 
							 + gpGlobals->v_forward * 25;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_up * -12 + gpGlobals->v_forward * 32 + gpGlobals->v_right * 6 , vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL ); 

	if (pev->body == 1)
	{
		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;

		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ppk2.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
	
		
	}
	else
	{
		m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/ppk1.wav", RANDOM_FLOAT(0.92, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,3));
	}

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_6DEGREES, 8192, BULLET_PLAYER_9MM, 0 );

	m_flNextPrimaryAttack = gpGlobals->time + .3;
	m_flNextSecondaryAttack = gpGlobals->time + .3;

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x -= 2;
}

void CWalterppk::SecondaryAttack( void )
{
	if (pev->body == 1)
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 2.4;
	    m_flTimeWeaponIdle = gpGlobals->time + 2.4;
		SetThink( AddSilencer );
		pev->nextthink = gpGlobals->time + 1.5;
		SendWeaponAnim( PPK_HOLSTER );
	}
	else
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 2.4;
		m_flTimeWeaponIdle = gpGlobals->time + 2.4;
		SetThink( AddSilencer );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/ppkbond.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
		pev->nextthink = gpGlobals->time + 1.5;
		SendWeaponAnim( PPK_HOLSTER );
	}
}

void CWalterppk::AddSilencer( void )
{
	if(pev->body == 1)
        pev->body = 0;
    else
		pev->body = 1;

    SendWeaponAnim( PPK_DRAW );
}

void CWalterppk::Reload( void )
{
	int iResult;

	//if (m_iClip == 0)
	//	iResult = DefaultReload( 8, PPK_RELOAD, 1.5 );
	//else
		iResult = DefaultReload( 8, PPK_RELOAD_NOT_EMPTY, 1.5 );

	if (iResult)
	{
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
}



void CWalterppk::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	if (m_iClip != 0)
	{
		int iAnim;
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.3 + 0 * 0.75)
		{
			iAnim = PPK_IDLE3;
			m_flTimeWeaponIdle = gpGlobals->time + 49.0 / 16;
		}
		else if (flRand <= 0.6 + 0 * 0.875)
		{
			iAnim = PPK_IDLE1;
			m_flTimeWeaponIdle = gpGlobals->time + 60.0 / 16.0;
		}
		else
		{
			iAnim = PPK_IDLE2;
			m_flTimeWeaponIdle = gpGlobals->time + 40.0 / 16.0;
		}
		SendWeaponAnim( iAnim );
	}
}


class CWalterppkAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/ammo/w_32acp.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/ammo/w_32acp.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_PPKCLIP_GIVE, "32acp", PPK_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_ppkclip, CWalterppkAmmo );















