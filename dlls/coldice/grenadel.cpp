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
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "customentity.h"
#include "gamerules.h"


enum grenadelauncher_e {
	GL_IDLE1 = 0,
	GL_IDLE2,
	GL_RELOAD1,
	GL_RELOAD2,
	GL_RELOAD3,	
	GL_FIRE1,
	GL_FIRE2,	
	GL_HOLSTER,
	GL_DRAW,


};


class CGrenadelauncher : public CBasePlayerWeapon
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void PrimaryAttack( void );
	void WeaponIdle( void );
	void SecondaryAttack( void );
	float m_flNextAnimTime;
	void Reload( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;
	float m_flPumpTime;
};
LINK_ENTITY_TO_CLASS( weapon_grenadel, CGrenadelauncher );
LINK_ENTITY_TO_CLASS( weapon_egon, CGrenadelauncher );

TYPEDESCRIPTION	CGrenadelauncher::m_SaveData[] = 
{
	DEFINE_FIELD( CGrenadelauncher, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( CGrenadelauncher, m_fInReload, FIELD_INTEGER ),
	DEFINE_FIELD( CGrenadelauncher, m_flNextReload, FIELD_TIME ),
	DEFINE_FIELD( CGrenadelauncher, m_flPumpTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CGrenadelauncher, CBasePlayerWeapon );

void CGrenadelauncher::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_grenadel"); 
	Precache( );
	m_iId = WEAPON_GRENADEL;

	SET_MODEL(ENT(pev), "models/wmodels/w_grenadel.mdl");

	m_iDefaultAmmo = GRENADEL_DEFAULT_GIVE;

	FallInit();
}


void CGrenadelauncher::Precache( void )
{
	PRECACHE_MODEL("models/wmodels/w_grenadel.mdl");
	PRECACHE_MODEL("models/vmodels/v_grenadel.mdl");
	PRECACHE_MODEL("models/pmodels/p_grenadel.mdl");
	PRECACHE_SOUND("weapons/glauncher.wav");
	PRECACHE_SOUND("weapons/glauncher2.wav");
	PRECACHE_SOUND("weapons/cannoncock.wav");
	PRECACHE_SOUND("weapons/reloadcannon1.wav");
}


BOOL CGrenadelauncher::Deploy( void )
{
	return DefaultDeploy( "models/vmodels/v_grenadel.mdl", "models/pmodels/p_grenadel.mdl", GL_DRAW, "rpg" );
}

int CGrenadelauncher::AddToPlayer( CBasePlayer *pPlayer )
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


int CGrenadelauncher::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "contact";
	p->iMaxAmmo1 = GRENADEL_MAX_CARRY;
	p->pszAmmo2 = "timed";
	p->iMaxAmmo2 = TIMED_MAX_CARRY;
	p->iMaxClip = GRENADEL_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_GRENADEL;
	p->iFlags = 0;
	p->iWeight = GRENADEL_WEIGHT;     

	return 1;
}
void CGrenadelauncher::PrimaryAttack( void )
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_iClip == 0)
	{
		Reload( );
	    SendWeaponAnim( GL_IDLE1 );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
			
	m_iClip--;

	
	  SendWeaponAnim( GL_FIRE1 );


	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher2.wav", 0.8, ATTN_NORM);
		break;
	}
 
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	CGrenade::ShootContact ( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 10, 
							gpGlobals->v_forward * 1000 );
	
	m_flNextPrimaryAttack = gpGlobals->time + 1.0;
	m_flNextSecondaryAttack = gpGlobals->time + 1.0;
	m_flTimeWeaponIdle = gpGlobals->time + .6;

	m_pPlayer->pev->punchangle.x -= 5;

}
void CGrenadelauncher::SecondaryAttack( void )
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_flNextSecondaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
	    SendWeaponAnim( GL_IDLE1 );
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
		m_flNextSecondaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
			
	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	SendWeaponAnim( GL_FIRE2 );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/glauncher2.wav", 0.8, ATTN_NORM);
		break;
	}
 
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	CGrenade::ShootTimed  ( m_pPlayer->pev, 
							m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 10, 
							gpGlobals->v_forward * 1000, 3.0 );
	
	
	m_flNextPrimaryAttack = gpGlobals->time + 1.2;
	m_flNextSecondaryAttack = gpGlobals->time + 1.2;
	
	if (m_iClip != 0)
		m_flTimeWeaponIdle = gpGlobals->time + .6;
	else
		m_flTimeWeaponIdle = 1.5;

	m_fInReload = 0;

	m_pPlayer->pev->punchangle.x -= 10;

}
void CGrenadelauncher::Reload( void )
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == GRENADEL_MAX_CLIP)
		return;

	if (m_flNextReload > gpGlobals->time)
		return;

	if (m_flNextPrimaryAttack > gpGlobals->time)
		return;

	if (m_fInReload == 0)
	{
		SendWeaponAnim( GL_RELOAD1 );
		m_fInReload = 1;
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.1;
		m_flTimeWeaponIdle = gpGlobals->time + 0.1;
		m_flNextPrimaryAttack = gpGlobals->time + .3;
		m_flNextSecondaryAttack = gpGlobals->time + .3;
		return;
	}
	else if (m_fInReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->time)
			return;
		m_fInReload = 2;

		if (RANDOM_LONG(0,1))
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reloadcannon1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));
		else
			EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/reloadcannon1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f));

		SendWeaponAnim( GL_RELOAD2 );

		m_flNextReload = gpGlobals->time + .8;
		m_flTimeWeaponIdle = gpGlobals->time + 0.2;
	}
	else
	{
		m_iClip += 1;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInReload = 1;
	}
}
void CGrenadelauncher::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if (m_flPumpTime && m_flPumpTime < gpGlobals->time)
	{
		EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < gpGlobals->time)
	{
		if (m_iClip == 0 && m_fInReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
		{
			Reload( );
		}
		else if (m_fInReload != 0)
		{
			if (m_iClip != 6 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
			{
				Reload( );
			}
			else
			{
				SendWeaponAnim( GL_IDLE1 );
				
				EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/cannoncock.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG(0,0x1f));
				m_fInReload = 0;
				m_flTimeWeaponIdle = gpGlobals->time + 1.5;
			}
		}
		else
		{
			int iAnim;
			float flRand = RANDOM_FLOAT(0, 1);
			if (flRand <= 0.8)
			{
				iAnim = GL_IDLE2;
				m_flTimeWeaponIdle = gpGlobals->time + (60.0/12.0);
			}
			else if (flRand <= 0.95)
			{
				iAnim = GL_IDLE1;
				m_flTimeWeaponIdle = gpGlobals->time + (20.0/9.0);
			}
			else
			{
				iAnim = GL_IDLE2;
				m_flTimeWeaponIdle = gpGlobals->time + (20.0/9.0);
			}
			SendWeaponAnim( iAnim );
		}
	}
}
	

class CContactAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_CONTACT_GIVE, "contact", GRENADEL_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_contact, CContactAmmo );

class CTimedAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_9mmclip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_9mmclip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		int bResult = (pOther->GiveAmmo( AMMO_TIMED_GIVE, "timed", TIMED_MAX_CARRY ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_timed, CTimedAmmo );



