#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CP90::m_SaveData[] =
{
	DEFINE_FIELD( CP90, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CP90, CBasePlayerWeapon );
#endif

enum p90_e
{
	P90_IDLE1,
	P90_RELOAD,
	P90_DRAW,
	P90_SHOOT1,
	P90_SHOOT2,
	P90_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_p90, CP90 );

void CP90::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_p90" );
	Precache();
	m_iId = WEAPON_P90;
	SET_MODEL( ENT( pev ), "models/w_p90.mdl" );
	m_iDefaultAmmo = 50;
	m_flAccuracy = 0.2f;
	m_iShotsFired = 0;
	m_bDelayFire = false;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CP90::Precache()
{
	PRECACHE_MODEL( "models/v_p90.mdl" );
	PRECACHE_MODEL( "models/w_p90.mdl" );
	PRECACHE_SOUND( "weapons/p90-1.wav" );
	PRECACHE_SOUND( "weapons/p90_clipout.wav" );
	PRECACHE_SOUND( "weapons/p90_clipin.wav" );
	PRECACHE_SOUND( "weapons/p90_boltpull.wav" );
	PRECACHE_SOUND( "weapons/p90_cliprelease.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireP90 = PRECACHE_EVENT( 1, "events/p90.sc" );
}

int CP90::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "57mm";
	info->iMaxAmmo1 = 100;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = 50;
	info->iSlot = 0;
	info->iPosition = 8;
	info->iFlags = 0;
	info->iId = m_iId = WEAPON_P90;
	info->iWeight = 26;
	info->bIsAccessory = FALSE;
	return 1;
}

BOOL CP90::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	m_bDelayFire = false;
	return DefaultDeploy( "models/v_p90.mdl", NULL, P90_DRAW, "carbine", UseDecrement() );
}

void CP90::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		P90Fire( 0.3 * m_flAccuracy, 0.066f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 170 )
		P90Fire( 0.115 * m_flAccuracy, 0.066f, FALSE );
	else
		P90Fire( 0.045 * m_flAccuracy, 0.066f, FALSE );
}

void CP90::P90Fire( float spread, float cycleTime, BOOL useAutoAim )
{
	m_bDelayFire = true;
	++m_iShotsFired;
	m_flAccuracy = ( m_iShotsFired * m_iShotsFired / 175 ) + 0.45;
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
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->m_iWeaponVolume = 500;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	Vector direction = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	direction = m_pPlayer->FireBullets3( source, direction, spread, 8192, 1,
		BULLET_PLAYER_57MM, 21, 0.885f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireP90, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, (int)punchPitch, (int)punchYaw, 5, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 0.9f, 0.45f, 0.35f, 0.04f, 5.25f, 3.5f, 4 );
	else if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 0.45f, 0.3f, 0.2f, 0.0275f, 4, 2.25f, 7 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.275f, 0.2f, 0.125f, 0.02f, 3, 1, 9 );
	else
		KickBack( 0.3f, 0.225f, 0.125f, 0.02f, 3.25f, 1.25f, 8 );
}

void CP90::Reload()
{
	if ( m_pPlayer->ammo_57mm > 0 && DefaultReload( 50, P90_RELOAD, 3.4f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_iShotsFired = 0;
	}
}

void CP90::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( P90_IDLE1, UseDecrement() );
	}
}

int CP90::iItemSlot()
{
	return 1;
}

float CP90::GetMaxSpeed()
{
	return 221;
}

BOOL CP90::UseDecrement()
{
	return TRUE;
}
