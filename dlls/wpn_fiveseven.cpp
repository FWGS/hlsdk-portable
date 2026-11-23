#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum fiveseven_e
{
	FIVESEVEN_IDLE,
	FIVESEVEN_SHOOT1,
	FIVESEVEN_SHOOT2,
	FIVESEVEN_SHOOT_EMPTY,
	FIVESEVEN_RELOAD,
	FIVESEVEN_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_fiveseven, CFiveSeven );

void CFiveSeven::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_fiveseven" );
	Precache();
	m_iId = WEAPON_FIVESEVEN;
	SET_MODEL( edict(), "models/w_fiveseven.mdl" );
	m_iDefaultAmmo = 20;
	m_flAccuracy = 0.92f;
	m_bIsAccessory = false;
	FallInit();
}

void CFiveSeven::Precache()
{
	PRECACHE_MODEL( "models/v_fiveseven.mdl" );
	PRECACHE_MODEL( "models/w_fiveseven.mdl" );
	PRECACHE_MODEL( "models/p_fiveseven.mdl" );
	PRECACHE_SOUND( "weapons/fiveseven-1.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_clipout.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_clipin.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/fiveseven_slidepull.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireFiveSeven = PRECACHE_EVENT( 1, "events/fiveseven.sc" );
}

int CFiveSeven::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "57mm";
	p->iMaxAmmo1 = 100;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 1;
	p->iPosition = 6;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_FIVESEVEN;
	p->iWeight = 5;
	p->bIsAccessory = false;
	return 1;
}

BOOL CFiveSeven::Deploy()
{
	m_flAccuracy = 0.92f;
	return DefaultDeploy( "models/v_fiveseven.mdl", NULL, FIVESEVEN_DRAW, "onehanded", UseDecrement() );
}

void CFiveSeven::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		FiveSevenFire( ( 1 - m_flAccuracy ) * 1.5, 0.2f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		FiveSevenFire( ( 1 - m_flAccuracy ) * 0.255, 0.2f, FALSE );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		FiveSevenFire( ( 1 - m_flAccuracy ) * 0.075, 0.2f, FALSE );
	else
		FiveSevenFire( ( 1 - m_flAccuracy ) * 0.15, 0.2f, FALSE );
}

void CFiveSeven::FiveSevenFire( float spread, float cycleTime, BOOL )
{
	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.275 - ( gpGlobals->time - m_flLastFire ) ) * 0.25;
		if ( m_flAccuracy > 0.92 )
			m_flAccuracy = 0.92f;
		else if ( m_flAccuracy < 0.725 )
			m_flAccuracy = 0.725f;
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
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 4096, 1, BULLET_PLAYER_57MM, 20,
		0.885f, m_pPlayer->pev, false, m_pPlayer->random_seed );
	float yaw = m_pPlayer->pev->punchangle.y * 100;
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireFiveSeven, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, direction.x, direction.y, (int)pitch, (int)yaw, m_iClip == 0, 0 );

	cycleTime -= 0.05;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CFiveSeven::Reload()
{
	if ( m_pPlayer->ammo_57mm > 0 && DefaultReload( 20, FIVESEVEN_RELOAD, 2.7f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.92f;
	}
}

void CFiveSeven::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
		SendWeaponAnim( FIVESEVEN_IDLE, UseDecrement() );
	}
}

int CFiveSeven::iItemSlot()
{
	return 2;
}

float CFiveSeven::GetMaxSpeed()
{
	return 225;
}

BOOL CFiveSeven::UseDecrement()
{
	return TRUE;
}
