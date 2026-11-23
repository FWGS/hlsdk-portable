#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CAUG::m_SaveData[] =
{
	DEFINE_FIELD( CAUG, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CAUG, CBasePlayerWeapon );
#endif

enum aug_e
{
	AUG_IDLE1,
	AUG_RELOAD,
	AUG_DRAW,
	AUG_SHOOT1,
	AUG_SHOOT2,
	AUG_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_aug, CAUG );

void CAUG::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_aug" );
	Precache();
	m_iId = WEAPON_AUG;
	SET_MODEL( ENT( pev ), "models/w_aug.mdl" );
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.2f;
	m_iShotsFired = 0;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CAUG::Precache()
{
	PRECACHE_MODEL( "models/v_aug.mdl" );
	PRECACHE_MODEL( "models/w_aug.mdl" );
	PRECACHE_SOUND( "weapons/aug-1.wav" );
	PRECACHE_SOUND( "weapons/aug_clipout.wav" );
	PRECACHE_SOUND( "weapons/aug_clipin.wav" );
	PRECACHE_SOUND( "weapons/aug_boltpull.wav" );
	PRECACHE_SOUND( "weapons/aug_boltslap.wav" );
	PRECACHE_SOUND( "weapons/aug_forearm.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireAug = PRECACHE_EVENT( 1, "events/aug.sc" );
}

int CAUG::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "556Nato";
	info->iMaxAmmo1 = 90;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = 30;
	info->iSlot = 0;
	info->iPosition = 14;
	info->iFlags = 0;
	info->iId = m_iId = WEAPON_AUG;
	info->iWeight = 25;
	info->bIsAccessory = FALSE;
	return 1;
}

BOOL CAUG::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	return DefaultDeploy( "models/v_aug.mdl", NULL, AUG_DRAW, "carbine", UseDecrement() );
}

void CAUG::SecondaryAttack()
{
	if ( m_pPlayer->m_iFOV == 90 )
	{
		m_pPlayer->m_iFOV = 55;
		m_pPlayer->pev->fov = 55;
	}
	else
	{
		m_pPlayer->m_iFOV = 90;
		m_pPlayer->pev->fov = 90;
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CAUG::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		AUGFire( 0.4 * m_flAccuracy + 0.035, 0.0825f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 140 )
		AUGFire( 0.07 * m_flAccuracy + 0.035, 0.0825f, FALSE );
	else
	{
		float cycleTime = m_pPlayer->m_iFOV == 90 ? 0.0825f : 0.135f;
		AUGFire( 0.02 * m_flAccuracy, cycleTime, FALSE );
	}
}

void CAUG::AUGFire( float spread, float cycleTime, BOOL useAutoAim )
{
	m_bDelayFire = true;
	++m_iShotsFired;
	m_flAccuracy = ( m_iShotsFired * m_iShotsFired * m_iShotsFired / 215 ) + 0.3;
	if ( m_flAccuracy > 1 )
		m_flAccuracy = 1;
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
	m_pPlayer->m_iWeaponVolume = 1500;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector direction = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	direction = m_pPlayer->FireBullets3( source, direction, spread, 8192, 2,
		BULLET_PLAYER_556MM, 32, 0.96f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireAug, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, (int)punchPitch, (int)punchYaw, 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.9;
	if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1, 0.45f, 0.275f, 0.05f, 4, 2.5f, 7 );
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.25f, 0.45f, 0.22f, 0.18f, 5.5f, 4, 5 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.575f, 0.325f, 0.2f, 0.011f, 3.25f, 2, 8 );
	else
		KickBack( 0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 8 );
}

void CAUG::Reload()
{
	if ( m_pPlayer->ammo_556nato > 0 && DefaultReload( 30, AUG_RELOAD, 3.3f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		if ( m_pPlayer->m_iFOV != 90 )
			SecondaryAttack();
		m_flAccuracy = 0;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CAUG::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( AUG_IDLE1, UseDecrement() );
	}
}

int CAUG::iItemSlot()
{
	return 1;
}

float CAUG::GetMaxSpeed()
{
	return 216;
}

BOOL CAUG::UseDecrement()
{
	return TRUE;
}
