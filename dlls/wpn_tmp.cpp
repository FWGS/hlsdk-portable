#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

#ifndef CLIENT_DLL
TYPEDESCRIPTION CTMP::m_SaveData[] =
{
	DEFINE_FIELD( CTMP, iShellOn, FIELD_INTEGER )
};
IMPLEMENT_SAVERESTORE( CTMP, CBasePlayerWeapon );
#endif

enum tmp_e
{
	TMP_IDLE1,
	TMP_RELOAD,
	TMP_DRAW,
	TMP_SHOOT1,
	TMP_SHOOT2,
	TMP_SHOOT3,
};

LINK_ENTITY_TO_CLASS( weapon_tmp, CTMP );

void CTMP::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_tmp" );
	Precache();
	m_iId = WEAPON_TMP;
	SET_MODEL( ENT( pev ), "models/w_tmp.mdl" );
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.2f;
	m_iShotsFired = 0;
	m_bDelayFire = false;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CTMP::Precache()
{
	PRECACHE_MODEL( "models/v_tmp.mdl" );
	PRECACHE_MODEL( "models/w_tmp.mdl" );
	PRECACHE_SOUND( "weapons/tmp-1.wav" );
	PRECACHE_SOUND( "weapons/tmp-2.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireTMP = PRECACHE_EVENT( 1, "events/tmp.sc" );
}

int CTMP::GetItemInfo( ItemInfo* info )
{
	info->pszAmmo1 = "9mm";
	info->iMaxAmmo1 = 120;
	info->pszName = STRING( pev->classname );
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = 30;
	info->iSlot = 0;
	info->iPosition = 11;
	info->iFlags = 0;
	info->iId = m_iId = WEAPON_TMP;
	info->iWeight = 25;
	info->bIsAccessory = FALSE;
	return 1;
}

BOOL CTMP::Deploy()
{
	m_iShotsFired = 0;
	iShellOn = 1;
	m_flAccuracy = 0.2f;
	m_bDelayFire = false;
	return DefaultDeploy( "models/v_tmp.mdl", NULL, TMP_DRAW, "onehanded", UseDecrement() );
}

void CTMP::PrimaryAttack()
{
	if ( m_pPlayer->pev->flags & FL_ONGROUND )
		TMPFire( 0.03 * m_flAccuracy, 0.07f, FALSE );
	else
		TMPFire( 0.25 * m_flAccuracy, 0.07f, FALSE );
}

void CTMP::TMPFire( float spread, float cycleTime, BOOL useAutoAim )
{
	m_bDelayFire = true;
	++m_iShotsFired;
	float accuracy = ( m_iShotsFired * m_iShotsFired * m_iShotsFired / 200 ) + 0.55;
	m_flAccuracy = accuracy <= 1.4 ? accuracy : 1.4f;
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
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->m_iWeaponVolume = 500;
	Vector direction = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	direction = m_pPlayer->FireBullets3( source, direction, spread, 8192, 1,
		BULLET_PLAYER_9MM, 20, 0.85f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float punchYaw = m_pPlayer->pev->punchangle.y * 100;
	float punchPitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireTMP, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, (int)punchPitch, (int)punchYaw, 5, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", 0, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		KickBack( 1.1f, 0.5f, 0.35f, 0.045f, 4.5f, 3.5f, 6 );
	else if ( !( m_pPlayer->pev->velocity.Length2D() <= 0 ) )
		KickBack( 0.8f, 0.4f, 0.2f, 0.03f, 3, 2.5f, 7 );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		KickBack( 0.7f, 0.35f, 0.125f, 0.025f, 2.5f, 2, 10 );
	else
		KickBack( 0.725f, 0.375f, 0.15f, 0.025f, 2.75f, 2.25f, 9 );
}

void CTMP::Reload()
{
	if ( m_pPlayer->ammo_9mm > 0 && DefaultReload( 30, TMP_RELOAD, 2.12f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.2f;
		m_iShotsFired = 0;
	}
}

void CTMP::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		SendWeaponAnim( TMP_IDLE1, UseDecrement() );
	}
}

int CTMP::iItemSlot()
{
	return 1;
}

float CTMP::GetMaxSpeed()
{
	return 225;
}

BOOL CTMP::UseDecrement()
{
	return TRUE;
}
