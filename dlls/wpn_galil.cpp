#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CGalil::m_SaveData[] =
{
	DEFINE_FIELD( CGalil, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CGalil, CBasePlayerWeapon );
#endif

enum galil_e
{
	GALIL_IDLE1,
	GALIL_RELOAD,
	GALIL_DRAW,
	GALIL_SHOOT1,
	GALIL_SHOOT2,
	GALIL_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_galil, CGalil );

void CGalil::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_galil" );
	Precache();
	m_iId = WEAPON_GALIL;
	SET_MODEL( ENT( pev ), "models/w_galil.mdl" );
	m_iDefaultAmmo = 35;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CGalil::Precache()
{
	PRECACHE_MODEL( "models/v_galil_r.mdl" );
	PRECACHE_MODEL( "models/w_galil.mdl" );
	PRECACHE_SOUND( "weapons/galil-1.wav" );
	PRECACHE_SOUND( "weapons/galil-2.wav" );
	PRECACHE_SOUND( "weapons/galil_clipout.wav" );
	PRECACHE_SOUND( "weapons/galil_clipin.wav" );
	PRECACHE_SOUND( "weapons/galil_boltpull.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireGalil = PRECACHE_EVENT( 1, "events/galil.sc" );
}

int CGalil::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "556Nato";
	info->iMaxAmmo1 = 70;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = 35;
	info->iSlot = 0;
	info->iPosition = 17;
	info->iFlags = 0;
	info->iId = m_iId = WEAPON_GALIL;
	info->iWeight = 25;
	info->bIsAccessory = FALSE;
	return 1;
}

BOOL CGalil::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	return DefaultDeploy( "models/v_galil_r.mdl", NULL, GALIL_DRAW, "ak47", UseDecrement() );
}

void CGalil::PrimaryAttack()
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		GalilFire( 0.3 * m_flAccuracy + 0.04, 0.0875f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() <= 140 )
		GalilFire( 0.0375 * m_flAccuracy, 0.0875f, FALSE );
	else
		GalilFire( 0.07 * m_flAccuracy + 0.04, 0.0875f, FALSE );
}

void CGalil::SecondaryAttack()
{
}

void CGalil::GalilFire( float spread, float cycleTime, BOOL useAutoAim )
{
	m_bDelayFire = true;
	++m_iShotsFired;
	m_flAccuracy = ( m_iShotsFired * m_iShotsFired * m_iShotsFired / 200 ) + 0.35;
	if ( m_flAccuracy > 1.25 )
		m_flAccuracy = 1.25f;
	if ( m_iClip <= 0 )
	{
		if ( m_fFireOnEmpty )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		}
		return;
	}
	--m_iClip;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector direction = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	direction = m_pPlayer->FireBullets3( source, direction, spread, 8192, 2,
		BULLET_PLAYER_556MM, 30, 0.98f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 10000000;
	float punchPitch = m_pPlayer->pev->punchangle.x * 10000000;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireGalil, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, (int)punchPitch, (int)punchYaw, 0, 0 );
	m_pPlayer->m_iWeaponVolume = 1500;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.28;
	if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1, 0.45f, 0.28f, 0.045f, 3.75f, 3, 7 );
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.2f, 0.5f, 0.23f, 0.15f, 5.5f, 3.5f, 6 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.6f, 0.3f, 0.2f, 0.0125f, 3.25f, 2, 7 );
	else
		KickBack( 0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 7 );
}

void CGalil::Reload()
{
	if ( m_pPlayer->ammo_556nato > 0 && DefaultReload( 35, GALIL_RELOAD, 2.45f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CGalil::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( GALIL_IDLE1, UseDecrement() );
	}
}

int CGalil::iItemSlot()
{
	return 1;
}

float CGalil::GetMaxSpeed()
{
	return 216;
}

BOOL CGalil::UseDecrement()
{
	return TRUE;
}
