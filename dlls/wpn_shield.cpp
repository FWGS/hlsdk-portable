#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum shieldgun_e
{
	SHIELDGUN_IDLE,
	SHIELDGUN_SHOOT1,
	SHIELDGUN_SHOOT2,
	SHIELDGUN_SHOOT_EMPTY,
	SHIELDGUN_RELOAD,
	SHIELDGUN_DRAW,
	SHIELDGUN_DRAWN_IDLE,
	SHIELDGUN_UP,
	SHIELDGUN_DOWN
};

LINK_ENTITY_TO_CLASS( weapon_shieldgun, CShieldGun );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CShieldGun::m_SaveData[] =
{
	DEFINE_FIELD( CShieldGun, m_bShieldDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShieldGun, m_fMaxSpeed, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CShieldGun, CBasePlayerWeapon );
#endif

void CShieldGun::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_shieldgun" );
	Precache();
	m_iId = WEAPON_SHIELDGUN;
	SET_MODEL( edict(), "models/w_shield.mdl" );
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
	m_bShieldDrawn = FALSE;
	m_iDefaultAmmo = 20;
	m_fMaxSpeed = 180;
	m_bIsAccessory = false;
	FallInit();
}

void CShieldGun::Precache()
{
	PRECACHE_MODEL( "models/v_shield_r.mdl" );
	PRECACHE_MODEL( "models/w_shield.mdl" );
	PRECACHE_SOUND( "weapons/fiveseven-1.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_clipout.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_clipin.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_slidepull.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireShieldGun = PRECACHE_EVENT( 1, "events/shieldgun.sc" );
}

int CShieldGun::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "57mm";
	p->iMaxAmmo1 = 100;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 0;
	p->iPosition = 20;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHIELDGUN;
	p->iWeight = 50;
	p->bIsAccessory = false;
	return 1;
}

BOOL CShieldGun::Deploy()
{
	m_bShieldDrawn = FALSE;
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
	m_flAccuracy = 0.92f;
	return DefaultDeploy( "models/v_shield_r.mdl", NULL, SHIELDGUN_DRAW, "shieldgun", UseDecrement() );
}

void CShieldGun::Holster( int )
{
	m_pPlayer->UpdateShieldCrosshair( true );
	m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
}

void CShieldGun::PrimaryAttack()
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
	}
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
	{
		ShieldGunFire( ( 1 - m_flAccuracy ) * 0.255, 0.2f, FALSE );
	}
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
	{
		ShieldGunFire( ( 1 - m_flAccuracy ) * 0.35, 0.2f, FALSE );
	}
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
	{
		ShieldGunFire( ( 1 - m_flAccuracy ) * 0.075, 0.2f, FALSE );
	}
	else
	{
		ShieldGunFire( ( 1 - m_flAccuracy ) * 0.15, 0.2f, FALSE );
	}
}

void CShieldGun::ShieldGunFire( float spread, float cycleTime, BOOL )
{
	if ( ( m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) && m_bShieldDrawn )
		return;
	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.275 - ( gpGlobals->time - m_flLastFire ) ) * 0.25;
		if ( m_flAccuracy > 0.92 )
			m_flAccuracy = 0.92f;
		else if ( m_flAccuracy < 0.725 )
			m_flAccuracy = 0.725f;
	}
	m_flLastFire = gpGlobals->time;

	if ( m_iClip <= 0 )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}
		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	if ( ( m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) && m_bShieldDrawn )
		strcpy( m_pPlayer->m_szAnimExtention, "shield" );
	else
		strcpy( m_pPlayer->m_szAnimExtention, "shieldgun" );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->m_iWeaponVolume = 500;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 4096, 1, BULLET_PLAYER_57MM, 25,
		0.885f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float yaw = m_pPlayer->pev->punchangle.y * 10000000;
	float pitch = m_pPlayer->pev->punchangle.x * 10000000;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireShieldGun, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, direction.x, direction.y, (int)pitch, (int)yaw, m_iClip == 0, 0 );
	cycleTime -= 0.05;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	m_pPlayer->pev->punchangle.x -= 2;
	if ( ( m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) && m_bShieldDrawn )
		strcpy( m_pPlayer->m_szAnimExtention, "shieldgun" );
}

void CShieldGun::SecondaryAttack()
{
	if ( !( m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) )
	{
		if ( m_bShieldDrawn )
			return;
		m_iWeaponState |= WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim( SHIELDGUN_UP, UseDecrement() );
		strcpy( m_pPlayer->m_szAnimExtention, "shield" );
		m_bShieldDrawn = TRUE;
		m_fMaxSpeed = 150;
		m_pPlayer->m_bShieldDrawn = true;
	}
	else
	{
		if ( !m_bShieldDrawn )
			return;
		m_iWeaponState &= ~WPNSTATE_SHIELD_DRAWN;
		SendWeaponAnim( SHIELDGUN_DOWN, UseDecrement() );
		strcpy( m_pPlayer->m_szAnimExtention, "shieldgun" );
		m_bShieldDrawn = FALSE;
		m_fMaxSpeed = 180;
		m_pPlayer->m_bShieldDrawn = false;
	}
	m_pPlayer->UpdateShieldCrosshair( !m_bShieldDrawn );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.4;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.6;
}

void CShieldGun::Reload()
{
	if ( ( !( m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) || !m_bShieldDrawn ) && m_pPlayer->ammo_57mm > 0
		&& DefaultReload( 20, SHIELDGUN_RELOAD, 2.7f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.92f;
	}
}

void CShieldGun::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		if ( m_iWeaponState & WPNSTATE_SHIELD_DRAWN )
			SendWeaponAnim( SHIELDGUN_DRAWN_IDLE, UseDecrement() );
	}
}

int CShieldGun::iItemSlot()
{
	return 1;
}

float CShieldGun::GetMaxSpeed()
{
	return m_fMaxSpeed;
}

BOOL CShieldGun::UseDecrement()
{
	return TRUE;
}
