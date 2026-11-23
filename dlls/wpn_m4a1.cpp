#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#define M4A1_SILENCED	4

LINK_ENTITY_TO_CLASS( weapon_m4a1, CM4A1 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CM4A1::m_SaveData[] =
{
	DEFINE_FIELD( CM4A1, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CM4A1, CBasePlayerWeapon );
#endif

void CM4A1::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m4a1" );
	Precache();
	m_iId = WEAPON_M4A1;
	SET_MODEL( edict(), "models/w_m4a1.mdl" );
	m_iShotsFired = 0;
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.2f;
	m_bDelayFire = true;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CM4A1::Precache()
{
	PRECACHE_MODEL( "models/v_m4a1.mdl" );
	PRECACHE_MODEL( "models/w_m4a1.mdl" );
	PRECACHE_SOUND( "weapons/m4a1-1.wav" );
	PRECACHE_SOUND( "weapons/m4a1_unsil-1.wav" );
	PRECACHE_SOUND( "weapons/m4a1_unsil-2.wav" );
	PRECACHE_SOUND( "weapons/m4a1_clipin.wav" );
	PRECACHE_SOUND( "weapons/m4a1_clipout.wav" );
	PRECACHE_SOUND( "weapons/m4a1_boltpull.wav" );
	PRECACHE_SOUND( "weapons/m4a1_deploy.wav" );
	PRECACHE_SOUND( "weapons/m4a1_silencer_on.wav" );
	PRECACHE_SOUND( "weapons/m4a1_silencer_off.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireM4A1 = PRECACHE_EVENT( 1, "events/m4a1.sc" );
}

int CM4A1::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = 90;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 0;
	p->iPosition = 6;
	p->iFlags = 0;
	m_iId = p->iId = WEAPON_M4A1;
	p->iWeight = 25;
	p->bIsAccessory = FALSE;
	return 1;
}

int CM4A1::iItemSlot()
{
	return 1;
}

float CM4A1::GetMaxSpeed()
{
	return 207;
}

BOOL CM4A1::UseDecrement()
{
	return TRUE;
}

BOOL CM4A1::Deploy()
{
	m_iShotsFired = 0;
	m_flAccuracy = 0.2f;
	m_bDelayFire = true;
	iShellOn = 1;
	if ( m_iWeaponState & M4A1_SILENCED )
		return DefaultDeploy( "models/v_m4a1.mdl", NULL, 5, "rifle", UseDecrement() != FALSE );
	else
		return DefaultDeploy( "models/v_m4a1.mdl", NULL, 12, "rifle", UseDecrement() != FALSE );
}

void CM4A1::SecondaryAttack()
{
	if ( m_iWeaponState & M4A1_SILENCED )
	{
		m_iWeaponState &= ~M4A1_SILENCED;
		SendWeaponAnim( 13, UseDecrement() );
	}
	else
	{
		m_iWeaponState |= M4A1_SILENCED;
		SendWeaponAnim( 6, UseDecrement() );
	}
	strcpy( m_pPlayer->m_szAnimExtention, "rifle" );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 2;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 2;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
}

void CM4A1::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		M4A1Fire( 0.4 * m_flAccuracy + 0.035, 0.0875f, FALSE );
	else if ( ( m_iWeaponState & M4A1_SILENCED )
		? !( m_pPlayer->pev->velocity.Length2D() <= 140 ) : m_pPlayer->pev->velocity.Length2D() > 140 )
		M4A1Fire( 0.07 * m_flAccuracy + 0.035, 0.0875f, FALSE );
	else
		M4A1Fire( ( ( m_iWeaponState & M4A1_SILENCED ) ? 0.025 : 0.02 ) * m_flAccuracy, 0.0875f, FALSE );
}

void CM4A1::M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	(void)fUseAutoAim;
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = m_iShotsFired * m_iShotsFired * m_iShotsFired / 220 + 0.3;
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
	m_iClip--;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->m_iWeaponVolume = ( m_iWeaponState & M4A1_SILENCED ) ? 1500 : 800;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir;
	if ( m_iWeaponState & M4A1_SILENCED )
		vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_556MM,
			30, 0.95f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	else
	{
		vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_556MM,
			32, 0.97f, m_pPlayer->pev, false, m_pPlayer->random_seed );
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	}
	m_pPlayer->ammo_556nato--;
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireM4A1, 0, (float*)&g_vecZero, (float*)&g_vecZero,
		vecDir.x, vecDir.y, (int)punchPitch, (int)punchYaw, ( m_iWeaponState & M4A1_SILENCED ) != 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
	if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1, 0.45f, 0.28f, 0.045f, 3.75f, 3, 7 );
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.2f, 0.5f, 0.23f, 0.15f, 5.5f, 3.5f, 6 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.6f, 0.3f, 0.2f, 0.0125f, 3.25f, 2, 7 );
	else
		KickBack( 0.65f, 0.35f, 0.25f, 0.015f, 3.5f, 2.25f, 7 );
}

void CM4A1::Reload()
{
	if ( m_pPlayer->ammo_556nato > 0 && DefaultReload( 30, ( m_iWeaponState & M4A1_SILENCED ) ? 4 : 11, 3.05f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CM4A1::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( ( m_iWeaponState & M4A1_SILENCED ) ? 0 : 7, UseDecrement() );
	}
}
