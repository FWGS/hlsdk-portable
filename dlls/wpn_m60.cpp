#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

LINK_ENTITY_TO_CLASS( weapon_m60, CM60 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CM60::m_SaveData[] =
{
	DEFINE_FIELD( CM60, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CM60, CBasePlayerWeapon );
#endif

void CM60::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m60" );
	Precache();
	m_iId = WEAPON_M60;
	SET_MODEL( edict(), "models/w_m60.mdl" );
	m_iShotsFired = 0;
	m_iDefaultAmmo = 100;
	m_flAccuracy = 0.2f;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CM60::Precache()
{
	PRECACHE_MODEL( "models/v_m60.mdl" );
	PRECACHE_MODEL( "models/w_m60.mdl" );
	PRECACHE_SOUND( "weapons/m60-1.wav" );
	PRECACHE_SOUND( "weapons/m60-2.wav" );
	PRECACHE_SOUND( "weapons/m60_boxout.wav" );
	PRECACHE_SOUND( "weapons/m60_boxin.wav" );
	PRECACHE_SOUND( "weapons/m60_chain.wav" );
	PRECACHE_SOUND( "weapons/m60_coverup.wav" );
	PRECACHE_SOUND( "weapons/m60_coverdown.wav" );
	m_iShell = PRECACHE_MODEL( "models/rshell.mdl" );
	m_usFireM60 = PRECACHE_EVENT( 1, "events/m60.sc" );
}

int CM60::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "762NatoBox";
	p->iMaxAmmo1 = 200;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 100;
	p->iSlot = 0;
	p->iPosition = 21;
	p->iFlags = 0;
	m_iId = p->iId = WEAPON_M60;
	p->iWeight = 25;
	p->bIsAccessory = FALSE;
	return 1;
}

int CM60::iItemSlot()
{
	return 1;
}

float CM60::GetMaxSpeed()
{
	return 198;
}

BOOL CM60::UseDecrement()
{
	return TRUE;
}

BOOL CM60::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	return DefaultDeploy( "models/v_m60.mdl", NULL, 4, "m60", UseDecrement() );
}

void CM60::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		M60Fire( 0.5 * m_flAccuracy + 0.045, 0.1f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() <= 140 )
		M60Fire( 0.03 * m_flAccuracy, 0.1f, FALSE );
	else
		M60Fire( 0.095 * m_flAccuracy + 0.045, 0.1f, FALSE );
}

void CM60::M60Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim )
{
	(void)fUseAutoAim;
	m_bDelayFire = true;
	m_iShotsFired++;
	float accuracy = m_iShotsFired * m_iShotsFired * m_iShotsFired / 175 + 0.4;
	m_flAccuracy = accuracy <= 0.9 ? accuracy : 0.9f;
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
	m_pPlayer->m_iWeaponVolume = 2250;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3( vecSrc, vecAiming, flSpread, 8192, 2, BULLET_PLAYER_762MM,
		50, 0.97f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireM60, 0, (float*)&g_vecZero, (float*)&g_vecZero,
		vecDir.x, vecDir.y, (int)punchPitch, (int)punchYaw, 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.6;
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.8f, 0.65f, 0.45f, 0.125f, 5, 3.5f, 8 );
	else if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 1.1f, 0.5f, 0.3f, 0.06f, 4, 3, 8 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.75f, 0.325f, 0.25f, 0.025f, 3.5f, 2.5f, 9 );
	else
		KickBack( 0.8f, 0.35f, 0.3f, 0.03f, 3.75f, 3, 9 );
}

void CM60::Reload()
{
	if ( m_pPlayer->ammo_762natobox > 0 && DefaultReload( 100, 3, 5.766f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_bDelayFire = false;
		m_iShotsFired = 0;
	}
}

void CM60::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( 0, UseDecrement() );
	}
}
