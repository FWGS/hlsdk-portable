#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"

enum m3_e
{
	M3_IDLE,
	M3_FIRE1,
	M3_FIRE2,
	M3_RELOAD,
	M3_PUMP,
	M3_START_RELOAD,
	M3_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_m3, CM3 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CM3::m_SaveData[] =
{
	DEFINE_FIELD( CM3, m_flPumpTime, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CM3, CBasePlayerWeapon );
#endif

void CM3::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m3" );
	Precache();
	m_iId = WEAPON_M3;
	SET_MODEL( edict(), "models/w_m3.mdl" );
	m_iDefaultAmmo = 8;
	m_bIsAccessory = false;
	FallInit();
}

void CM3::Precache()
{
	PRECACHE_MODEL( "models/v_m3.mdl" );
	PRECACHE_MODEL( "models/w_m3.mdl" );
	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
	m_iShellId = m_iShell;
	PRECACHE_SOUND( "weapons/m3-1.wav" );
	PRECACHE_SOUND( "weapons/m3_insertshell.wav" );
	PRECACHE_SOUND( "weapons/m3_pump.wav" );
	PRECACHE_SOUND( "weapons/reload1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );
	m_usFireM3 = PRECACHE_EVENT( 1, "events/m3.sc" );
}

int CM3::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "buckshot";
	p->iMaxAmmo1 = 32;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 8;
	p->iSlot = 0;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M3;
	p->iWeight = 20;
	p->bIsAccessory = false;
	return 1;
}

BOOL CM3::Deploy()
{
	m_pPlayer->m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	return DefaultDeploy( "models/v_m3.mdl", NULL, M3_DRAW, "shotgun", UseDecrement() );
}

void CM3::Holster( int )
{
	m_pPlayer->m_iHideHUD |= HIDEHUD_FLASHLIGHT;
}

void CM3::PrimaryAttack()
{
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = m_iShotsFired * m_iShotsFired * m_iShotsFired / 200 + 0.35;
	if ( m_flAccuracy > 1.25 )
		m_flAccuracy = 1.25f;

	if ( m_iClip <= 0 && m_fFireOnEmpty )
	{
		if ( !m_fInSpecialReload )
			PlayEmptySound();
		Reload();
		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	m_pPlayer->FireBullets( 9, source, aiming, Vector( 0.0675f, 0.0675f, 0 ), 3000,
		BULLET_PLAYER_BUCKSHOT, 0, (int)gSkillData.plrDmgBuckshot, NULL );
	float yaw = m_pPlayer->pev->punchangle.y * 100;
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireM3, 0, (float*)&g_vecZero.x, (float*)&g_vecZero.x,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, (int)pitch, (int)yaw, 0, 0 );
	m_pPlayer->m_iWeaponVolume = 2250;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.875;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.9;
}

void CM3::SecondaryAttack()
{
	if ( m_pPlayer->FlashlightIsOn() )
		m_pPlayer->FlashlightTurnOff();
	else
		m_pPlayer->FlashlightTurnOn();
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CM3::Reload()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == 8 )
		return;
	if ( !( m_flNextPrimaryAttack <= UTIL_WeaponTimeBase() ) )
		return;

	if ( !m_fInSpecialReload )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_fInSpecialReload = 1;
		SendWeaponAnim( M3_START_RELOAD, UseDecrement() );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.55;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.55;
	}
	else if ( m_fInSpecialReload == 1 )
	{
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			m_fInSpecialReload = 2;
			SendWeaponAnim( M3_RELOAD, UseDecrement() );
			m_flNextReload = UTIL_WeaponTimeBase() + 0.45;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.45;
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

void CM3::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	if ( m_flPumpTime != 0 && UTIL_WeaponTimeBase() > m_flPumpTime )
		m_flPumpTime = 0;
	if ( !( UTIL_WeaponTimeBase() > m_flTimeWeaponIdle ) )
		return;

	if ( m_fInSpecialReload )
	{
		if ( m_iClip == 8 || !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( M3_PUMP, UseDecrement() );
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
		SendWeaponAnim( M3_IDLE, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
	}
}

int CM3::iItemSlot()
{
	return 1;
}

float CM3::GetMaxSpeed()
{
	return 207;
}

BOOL CM3::UseDecrement()
{
	return TRUE;
}
