#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"

enum xm1014_e
{
	XM1014_IDLE,
	XM1014_FIRE1,
	XM1014_FIRE2,
	XM1014_RELOAD,
	XM1014_PUMP,
	XM1014_START_RELOAD,
	XM1014_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_xm1014, CXM1014 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CXM1014::m_SaveData[] =
{
	DEFINE_FIELD( CXM1014, m_flPumpTime, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CXM1014, CBasePlayerWeapon );
#endif

void CXM1014::Spawn()
{
	Precache();
	m_iId = WEAPON_XM1014;
	pev->classname = MAKE_STRING( "weapon_xm1014" );
	SET_MODEL( edict(), "models/w_xm1014.mdl" );
	m_iDefaultAmmo = 7;
	m_bIsAccessory = false;
	FallInit();
}

void CXM1014::Precache()
{
	PRECACHE_MODEL( "models/v_xm1014.mdl" );
	PRECACHE_MODEL( "models/w_xm1014.mdl" );
	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
	m_iShellId = m_iShell;
	PRECACHE_SOUND( "weapons/xm1014-1.wav" );
	PRECACHE_SOUND( "weapons/reload1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );
	m_usFireXM1014 = PRECACHE_EVENT( 1, "events/xm1014.sc" );
}

int CXM1014::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = 32;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 7;
	p->iSlot = 0;
	p->iPosition = 12;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_XM1014;
	p->iWeight = 20;
	p->bIsAccessory = false;
	return 1;
}

BOOL CXM1014::Deploy()
{
	return DefaultDeploy( "models/v_xm1014.mdl", NULL, XM1014_DRAW, "m249", UseDecrement() );
}

void CXM1014::PrimaryAttack()
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	if ( m_iClip <= 0 )
	{
		if ( !m_fInSpecialReload )
			PlayEmptySound();
		Reload();
		return;
	}

	m_pPlayer->m_iWeaponVolume = 2250;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	m_pPlayer->FireBullets( 6, source, aiming, Vector( 0.0725f, 0.0725f, 0 ), 3048,
		BULLET_PLAYER_BUCKSHOT, 0, (int)gSkillData.plrDmgBuckshot, NULL );
	float pitch = m_vVecAiming.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireXM1014, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, m_vVecAiming.x, m_vVecAiming.y, 7, (int)pitch, m_iClip == 0, 0 );
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	if ( m_iClip )
		m_flPumpTime = UTIL_WeaponTimeBase() + 0.125;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25;
	if ( m_iClip )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.25;
	else
		m_flTimeWeaponIdle = 0.75f;
	m_fInSpecialReload = 0;
	int kick;
	if ( m_pPlayer->pev->flags & FL_ONGROUND )
		kick = UTIL_SharedRandomLong( m_pPlayer->random_seed + 1, 3, 5 );
	else
		kick = UTIL_SharedRandomLong( m_pPlayer->random_seed + 1, 7, 10 );
	m_pPlayer->pev->punchangle.x -= kick;
}

void CXM1014::Reload()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 7 )
		return;
	if ( !( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() ) )
		return;

	if ( !m_fInSpecialReload )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		SendWeaponAnim( XM1014_START_RELOAD, UseDecrement() );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.55;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.55;
	}
	else if ( m_fInSpecialReload == 1 )
	{
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			m_fInSpecialReload = 2;
			const char* sound = RANDOM_LONG( 0, 1 ) ? "weapons/reload1.wav" : "weapons/reload3.wav";
			int pitch = RANDOM_LONG( 0, 31 ) + 85;
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, sound, 1, ATTN_NORM, 0, pitch );
			SendWeaponAnim( XM1014_RELOAD, UseDecrement() );
			m_flNextReload = UTIL_WeaponTimeBase() + 0.3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.3;
		}
	}
	else if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_fInSpecialReload = 1;
		m_iClip++;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_pPlayer->ammo_buckshot--;
	}
}

void CXM1014::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	if ( m_flPumpTime != 0 && UTIL_WeaponTimeBase() > m_flPumpTime )
		m_flPumpTime = 0;
	if ( !( UTIL_WeaponTimeBase() > m_flTimeWeaponIdle ) )
		return;

	if ( m_fInSpecialReload )
	{
		if ( m_iClip == 7 || !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( XM1014_PUMP, UseDecrement() );
			m_fInSpecialReload = 0;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
		}
		else
		{
			Reload();
		}
	}
	else if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		Reload();
	}
	else
	{
		SendWeaponAnim( XM1014_IDLE, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
	}
}

int CXM1014::iItemSlot()
{
	return 1;
}

float CXM1014::GetMaxSpeed()
{
	return 216;
}

BOOL CXM1014::UseDecrement()
{
	return TRUE;
}
