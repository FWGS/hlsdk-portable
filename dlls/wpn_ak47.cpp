#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_ak47, CAK47 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CAK47::m_SaveData[] =
{
	DEFINE_FIELD( CAK47, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CAK47, CBasePlayerWeapon );
#endif

void CAK47::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_ak47" );
	Precache();
	m_iId = WEAPON_AK47;
	SET_MODEL( edict(), "models/w_ak47.mdl" );
	m_iShotsFired = 0;
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.2f;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CAK47::Precache()
{
	PRECACHE_MODEL( "models/v_ak47.mdl" );
	PRECACHE_MODEL( "models/w_ak47.mdl" );
	PRECACHE_MODEL( "models/p_ak47.mdl" );
	PRECACHE_SOUND( "weapons/ak47-1.wav" );
	PRECACHE_SOUND( "weapons/ak47-2.wav" );
	PRECACHE_SOUND( "weapons/ak47_clipout.wav" );
	PRECACHE_SOUND( "weapons/ak47_clipin.wav" );
	PRECACHE_SOUND( "weapons/ak47_boltpull.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireAK47 = PRECACHE_EVENT( 1, "events/ak47.sc" );
}

int CAK47::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = 90;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iFlags = 0;
	m_iId = p->iId = WEAPON_AK47;
	p->iWeight = 25;
	p->bIsAccessory = FALSE;
	return 1;
}

int CAK47::iItemSlot()
{
	return 1;
}

float CAK47::GetMaxSpeed()
{
	return 199;
}

BOOL CAK47::UseDecrement()
{
	return TRUE;
}

BOOL CAK47::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	return DefaultDeploy( "models/v_ak47.mdl", NULL, 2, "ak47", UseDecrement() );
}

void CAK47::SecondaryAttack()
{
}

void CAK47::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		AK47Fire( 0.4 * m_flAccuracy + 0.04, 0.0955f, FALSE );
	else if ( !( m_pPlayer->pev->velocity.Length2D() <= 140 ) )
		AK47Fire( 0.07 * m_flAccuracy + 0.04, 0.0955f, FALSE );
	else
		AK47Fire( 0.0275 * m_flAccuracy, 0.0955f, FALSE );
}

void CAK47::AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	(void)fUseAutoAim;
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = m_iShotsFired * m_iShotsFired * m_iShotsFired / 200 + 0.35;
	if ( m_flAccuracy > 1.25f )
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
	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_762MM,
		36, 0.98f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireAK47, 0, (float*)&g_vecZero, (float*)&g_vecZero,
		vecDir.x, vecDir.y, (int)( m_pPlayer->pev->punchangle.x * 100 ),
		(int)( m_pPlayer->pev->punchangle.y * 100 ), 0, 0 );
	m_pPlayer->m_iWeaponVolume = 1500;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.9;
	if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1.5f, 0.45f, 0.225f, 0.05f, 6.5f, 2.5f, 7 );
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 2, 1, 0.5f, 0.35f, 9, 6, 5 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.9f, 0.35f, 0.15f, 0.025f, 5.5f, 1.5f, 9 );
	else
		KickBack( 1, 0.375f, 0.175f, 0.0375f, 5.75f, 1.75f, 8 );
}

void CAK47::Reload()
{
	if ( m_pPlayer->ammo_762nato > 0 && DefaultReload( 30, 1, 2.45f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CAK47::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( 0, UseDecrement() );
	}
}
