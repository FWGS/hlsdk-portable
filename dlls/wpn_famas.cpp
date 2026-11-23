#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#define FAMAS_BURST	16

LINK_ENTITY_TO_CLASS( weapon_famas, CFamas );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CFamas::m_SaveData[] =
{
	DEFINE_FIELD( CFamas, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CFamas, CBasePlayerWeapon );
#endif

void CFamas::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_famas" );
	Precache();
	m_iId = WEAPON_FAMAS;
	SET_MODEL( edict(), "models/w_famas.mdl" );
	m_iDefaultAmmo = 25;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CFamas::Precache()
{
	PRECACHE_MODEL( "models/v_famas_r.mdl" );
	PRECACHE_MODEL( "models/w_famas.mdl" );
	PRECACHE_SOUND( "weapons/famas-1.wav" );
	PRECACHE_SOUND( "weapons/famas-2.wav" );
	PRECACHE_SOUND( "weapons/famas_clipout.wav" );
	PRECACHE_SOUND( "weapons/famas_clipin.wav" );
	PRECACHE_SOUND( "weapons/famas_boltpull.wav" );
	PRECACHE_SOUND( "weapons/famas_boltslap.wav" );
	PRECACHE_SOUND( "weapons/famas_forearm.wav" );
	PRECACHE_SOUND( "weapons/famas-burst.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireFamas = PRECACHE_EVENT( 1, "events/famas.sc" );
}

int CFamas::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = 75;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 25;
	p->iSlot = 0;
	p->iPosition = 18;
	p->iFlags = 0;
	m_iId = p->iId = WEAPON_FAMAS;
	p->iWeight = 75;
	p->bIsAccessory = FALSE;
	return 1;
}

int CFamas::iItemSlot()
{
	return 1;
}

float CFamas::GetMaxSpeed()
{
	return 216;
}

BOOL CFamas::UseDecrement()
{
	return TRUE;
}

BOOL CFamas::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	return DefaultDeploy( "models/v_famas_r.mdl", NULL, 2, "carbine", UseDecrement() );
}

void CFamas::SecondaryAttack()
{
	if ( m_iWeaponState & FAMAS_BURST )
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_FullAuto" );
		m_iWeaponState &= ~FAMAS_BURST;
	}
	else
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_BurstFire" );
		m_iWeaponState |= FAMAS_BURST;
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CFamas::PrimaryAttack()
{
	if ( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15;
		return;
	}
	BOOL burst = ( m_iWeaponState & FAMAS_BURST ) != 0;
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		FamasFire( 0.3 * m_flAccuracy + 0.035, 0.0825f, FALSE, burst );
	else if ( m_pPlayer->pev->velocity.Length2D() <= 140 )
		FamasFire( 0.025 * m_flAccuracy, 0.0825f, FALSE, burst );
	else
		FamasFire( 0.07 * m_flAccuracy + 0.035, 0.0825f, FALSE, burst );
}

void CFamas::FamasFire( float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL bFireBurst )
{
	(void)fUseAutoAim;
	if ( bFireBurst )
	{
		m_iFamasShotsFired = 0;
		flCycleTime = 0.55f;
	}
	else
		flSpread = flSpread + 0.01;
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = m_iShotsFired * m_iShotsFired * m_iShotsFired / 215 + 0.3;
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
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = 1500;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_556MM,
		bFireBurst ? 34 : 30, 0.96f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 10000000;
	float punchPitch = m_pPlayer->pev->punchangle.x * 10000000;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireFamas, 0, (float*)&g_vecZero, (float*)&g_vecZero,
		vecDir.x, vecDir.y, (int)punchPitch, (int)punchYaw, 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.1;
	if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1, 0.45f, 0.275f, 0.05f, 4, 2.5f, 7 );
	else if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.25f, 0.45f, 0.22f, 0.18f, 5.5f, 4, 5 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.575f, 0.325f, 0.2f, 0.011f, 3.25f, 2, 8 );
	else
		KickBack( 0.625f, 0.375f, 0.25f, 0.0125f, 3.5f, 2.25f, 8 );
	if ( bFireBurst )
	{
		m_flFamasShoot = gpGlobals->time + 0.05;
		m_iFamasShotsFired++;
		m_fBurstSpread = flSpread;
	}
}

void CFamas::Reload()
{
	if ( m_pPlayer->ammo_556nato > 0 && DefaultReload( 25, 1, 3.3f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		if ( m_pPlayer->m_iFOV != 90 )
			SecondaryAttack();
		m_flAccuracy = 0;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CFamas::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( 0, UseDecrement() );
	}
}
