#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum p228_e
{
	P228_IDLE,
	P228_SHOOT1,
	P228_SHOOT2,
	P228_SHOOT3,
	P228_SHOOT_EMPTY,
	P228_RELOAD,
	P228_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_p228, CP228 );

void CP228::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_p228" );
	Precache();
	m_iId = WEAPON_P228;
	SET_MODEL( edict(), "models/w_p228.mdl" );
	m_iDefaultAmmo = 13;
	m_flAccuracy = 0.9f;
	m_bIsAccessory = false;
	FallInit();
}

void CP228::Precache()
{
	PRECACHE_MODEL( "models/v_p228.mdl" );
	PRECACHE_MODEL( "models/w_p228.mdl" );
	PRECACHE_SOUND( "weapons/p228-1.wav" );
	PRECACHE_SOUND( "weapons/p228_clipout.wav" );
	PRECACHE_SOUND( "weapons/p228_clipin.wav" );
	PRECACHE_SOUND( "weapons/p228_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/p228_slidepull.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireP228 = PRECACHE_EVENT( 1, "events/p228.sc" );
}

int CP228::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "357SIG";
	p->iMaxAmmo1 = 52;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 13;
	p->iSlot = 1;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_P228;
	p->iWeight = 5;
	p->bIsAccessory = false;
	return 1;
}

BOOL CP228::Deploy()
{
	m_flAccuracy = 0.9f;
	return DefaultDeploy( "models/v_p228.mdl", NULL, P228_DRAW, "onehanded", UseDecrement() );
}

void CP228::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		P228Fire( ( 1 - m_flAccuracy ) * 1.5, 0.2f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		P228Fire( ( 1 - m_flAccuracy ) * 0.255, 0.2f, FALSE );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		P228Fire( ( 1 - m_flAccuracy ) * 0.075, 0.2f, FALSE );
	else
		P228Fire( ( 1 - m_flAccuracy ) * 0.15, 0.2f, FALSE );
}

void CP228::P228Fire( float spread, float cycleTime, BOOL )
{
	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.325 - ( gpGlobals->time - m_flLastFire ) ) * 0.3;
		if ( m_flAccuracy > 0.9 )
			m_flAccuracy = 0.9f;
		else if ( m_flAccuracy < 0.6 )
			m_flAccuracy = 0.6f;
	}
	m_flLastFire = gpGlobals->time;

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
	m_pPlayer->m_iWeaponVolume = 500;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 4096, 1, BULLET_PLAYER_357SIG, 32,
		0.8f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	float yaw = m_pPlayer->pev->punchangle.y * 100;
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireP228, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, direction.x, direction.y, (int)pitch, (int)yaw, m_iClip == 0, 0 );

	cycleTime -= 0.05;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CP228::Reload()
{
	if ( m_pPlayer->ammo_357sig > 0 && DefaultReload( 13, P228_RELOAD, 2.7f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.9f;
	}
}

void CP228::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
		SendWeaponAnim( P228_IDLE, UseDecrement() );
	}
}

int CP228::iItemSlot()
{
	return 2;
}

float CP228::GetMaxSpeed()
{
	return 225;
}

BOOL CP228::UseDecrement()
{
	return TRUE;
}
