#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CMAC10::m_SaveData[] =
{
	DEFINE_FIELD( CMAC10, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CMAC10, CBasePlayerWeapon );
#endif

enum mac10_e
{
	MAC10_IDLE1,
	MAC10_RELOAD,
	MAC10_DRAW,
	MAC10_SHOOT1,
	MAC10_SHOOT2,
	MAC10_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_mac10, CMAC10 );

void CMAC10::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_mac10" );
	Precache();
	m_iId = WEAPON_MAC10;
	SET_MODEL( ENT( pev ), "models/w_mac10.mdl" );
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.15f;
	m_bDelayFire = false;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CMAC10::Precache()
{
	PRECACHE_MODEL( "models/v_mac10.mdl" );
	PRECACHE_MODEL( "models/w_mac10.mdl" );
	PRECACHE_SOUND( "weapons/mac10-1.wav" );
	PRECACHE_SOUND( "weapons/mac10_clipout.wav" );
	PRECACHE_SOUND( "weapons/mac10_clipin.wav" );
	PRECACHE_SOUND( "weapons/mac10_boltpull.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireMAC10 = PRECACHE_EVENT( 1, "events/mac10.sc" );
}

int CMAC10::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "45acp";
	info->iMaxAmmo1 = 100;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = 30;
	info->iSlot = 0;
	info->iPosition = 13;
	info->iFlags = 0;
	info->iId = m_iId = WEAPON_MAC10;
	info->iWeight = 25;
	info->bIsAccessory = FALSE;
	return 1;
}

BOOL CMAC10::Deploy()
{
	iShellOn = 1;
	m_flAccuracy = 0.15f;
	m_bDelayFire = false;
	return DefaultDeploy( "models/v_mac10.mdl", NULL, MAC10_DRAW, "onehanded", UseDecrement() );
}

void CMAC10::PrimaryAttack()
{
	if ( m_pPlayer->pev->flags & FL_ONGROUND )
		MAC10Fire( 0.03 * m_flAccuracy, 0.07f, FALSE );
	else
		MAC10Fire( 0.375 * m_flAccuracy, 0.07f, FALSE );
}

void CMAC10::MAC10Fire( float spread, float cycleTime, BOOL useAutoAim )
{
	m_bDelayFire = true;
	++m_iShotsFired;
	float accuracy = ( m_iShotsFired * m_iShotsFired * m_iShotsFired / 200 ) + 0.6;
	m_flAccuracy = accuracy <= 1.65 ? accuracy : 1.65f;
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
	m_pPlayer->m_iWeaponVolume = 800;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	Vector direction = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	direction = m_pPlayer->FireBullets3( source, direction, spread, 8192, 1,
		BULLET_PLAYER_45ACP, 29, 0.82f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireMAC10, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, (int)punchPitch, (int)punchYaw, 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.3f, 0.55f, 0.4f, 0.05f, 4.75f, 3.75f, 5 );
	else if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 0.9f, 0.45f, 0.25f, 0.035f, 3.5f, 2.75f, 7 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.75f, 0.4f, 0.175f, 0.03f, 2.75f, 2.5f, 10 );
	else
		KickBack( 0.775f, 0.425f, 0.2f, 0.03f, 3, 2.75f, 9 );
}

void CMAC10::Reload()
{
	if ( m_pPlayer->ammo_45acp > 0 && DefaultReload( 30, MAC10_RELOAD, 3.15f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0;
		m_iShotsFired = 0;
	}
}

void CMAC10::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( MAC10_IDLE1, UseDecrement() );
	}
}

int CMAC10::iItemSlot()
{
	return 1;
}

float CMAC10::GetMaxSpeed()
{
	return 225;
}

BOOL CMAC10::UseDecrement()
{
	return TRUE;
}
