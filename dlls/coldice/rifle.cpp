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
#include "gamerules.h"
#include "shake.h"


enum rifle_e {
	RIFLE_LONGIDLE = 0,
	RIFLE_IDLE1,
	RIFLE_LAUNCH,
	RIFLE_RELOAD,
	RIFLE_DRAW,
	RIFLE_FIRE,
	RIFLE_FIRE2,
	RIFLE_FIRE3,
	RIFLE_HOLSTER,
};


class CRifle : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	int m_fInZoom; 
};
LINK_ENTITY_TO_CLASS( weapon_rifle, CRifle );
LINK_ENTITY_TO_CLASS( weapon_crossbow, CRifle );

void CRifle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_rifle"); 
	Precache( );
	m_iId = WEAPON_RIFLE;
	SET_MODEL(ENT(pev), "models/wmodels/w_rifle.mdl");

	m_iDefaultAmmo = RIFLE_DEFAULT_GIVE;

	FallInit();
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

void CRifle::Precache( void )
{
	PRECACHE_SOUND("weapons/zoomout.wav");
	PRECACHE_SOUND("weapons/zoomin.wav");
	PRECACHE_MODEL("models/wmodels/w_rifle.mdl");
	PRECACHE_MODEL("models/vmodels/v_rifle.mdl");
	PRECACHE_MODEL("models/pmodels/p_rifle.mdl");

	m_iShell = PRECACHE_MODEL ("models/shotgunshell.mdl");

	PRECACHE_SOUND("weapons/rifle1.wav");
	PRECACHE_SOUND("weapons/reloadrifle.wav");
}


int CRifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "7.65mm";
	p->iMaxAmmo1 = RIFLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = RIFLE_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 7;
	p->iId = WEAPON_RIFLE;
	p->iFlags = 0;
	p->iWeight = RIFLE_WEIGHT;       
	return 1;
}


BOOL CRifle::Deploy( )
{
		return DefaultDeploy( "models/vmodels/v_rifle.mdl", "models/pmodels/p_rifle.mdl", RIFLE_DRAW, "rpg" );
}

void CRifle::Holster( )
{
	m_fInReload = FALSE;

	if ( m_fInZoom )
	{
		SecondaryAttack( );
	}

	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	
	if (m_iClip)
		SendWeaponAnim( RIFLE_HOLSTER );
}

void CRifle::PrimaryAttack( void )
{

	if (m_pPlayer->pev->waterlevel == 3)	
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->time + 0.3;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.3;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	SendWeaponAnim( RIFLE_FIRE );


	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	 EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/rifle1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG(0,0xf)); 


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector	vecShellVelocity = m_pPlayer->pev->velocity 
							 + gpGlobals->v_right * RANDOM_FLOAT(400,400) 
							 + gpGlobals->v_up * RANDOM_FLOAT(400,400) 
							 + gpGlobals->v_forward * 32;
	EjectBrass ( pev->origin + m_pPlayer->pev->view_ofs
					+ gpGlobals->v_up * -12 
					+ gpGlobals->v_forward * 32
					+ gpGlobals->v_right * 6, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL); 
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBullets( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_765MM, 0 );

	m_flNextPrimaryAttack =  gpGlobals->time + 1.0;
	
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT( 0, 0 );
}

void CRifle::SecondaryAttack()
{
	if (m_fInZoom)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/zoomout.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
		//UTIL_ScreenFade(m_pPlayer, Vector(40,40,240), 2.0, .5, 64, FFADE_IN);
		m_pPlayer->m_iFOV = 0; 
		m_fInZoom = 0;
		
	}
	else
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/zoomin.wav", RANDOM_FLOAT(0.9, 1.0), ATTN_NORM);
		//UTIL_ScreenFade(m_pPlayer, Vector(40,40,240), 4.0, 400.0, 64, FFADE_OUT | FFADE_STAYOUT);
		m_pPlayer->m_iFOV = 30;
		m_fInZoom = 1;
	}

	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextSecondaryAttack = gpGlobals->time + .5;
	m_flNextPrimaryAttack = gpGlobals->time + .5;
}


void CRifle::Reload( void )
{
	if ( m_fInZoom )
	{
		SecondaryAttack();
	}

	if (DefaultReload( RIFLE_MAX_CLIP, RIFLE_RELOAD, 1.5) )
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/reloadrifle.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 93 + RANDOM_LONG(0,0xF));
	}
}


void CRifle::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES ); 

	ResetEmptySound( );
	
	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		float flRand = RANDOM_FLOAT(0, 1);
		if (flRand <= 0.75)
		{
			if (m_iClip)
			{
				SendWeaponAnim( RIFLE_LONGIDLE );
			}
			else
			{
				SendWeaponAnim( RIFLE_IDLE1 );
			}
			m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
		}
		else
		{
			if (m_iClip)
			{
				SendWeaponAnim( RIFLE_IDLE1 );
				m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 30.0;
			}
			else
			{
				SendWeaponAnim( RIFLE_LONGIDLE );
				m_flTimeWeaponIdle = gpGlobals->time + 80.0 / 30.0;
			}
		}
	}
}



class CRifleAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/ammo/w_765mm.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/ammo/w_765mm.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int cResult = (pOther->GiveAmmo( AMMO_RIFLECLIP_GIVE, "7.65mm", RIFLE_MAX_CARRY ) != -1);
		
		if (cResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return cResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_rifleclip, CRifleAmmo );
