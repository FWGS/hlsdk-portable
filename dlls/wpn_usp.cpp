#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum usp_e
{
	USP_IDLE,
	USP_SHOOT1,
	USP_SHOOT2,
	USP_SHOOT3,
	USP_SHOOT_EMPTY,
	USP_RELOAD,
	USP_DRAW,
	USP_ATTACH_SILENCER,
	USP_UNSIL_IDLE,
	USP_UNSIL_SHOOT1,
	USP_UNSIL_SHOOT2,
	USP_UNSIL_SHOOT3,
	USP_UNSIL_SHOOT_EMPTY,
	USP_UNSIL_RELOAD,
	USP_UNSIL_DRAW,
	USP_DETACH_SILENCER
};

LINK_ENTITY_TO_CLASS( weapon_usp, CUSP );

void CUSP::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_usp" );
	Precache();
	m_iId = WEAPON_USP;
	SET_MODEL( edict(), "models/w_usp.mdl" );
	m_iDefaultAmmo = 12;
	m_flAccuracy = 0.92f;
	m_bIsAccessory = false;
	FallInit();
}

void CUSP::Precache()
{
	PRECACHE_MODEL( "models/v_usp.mdl" );
	PRECACHE_MODEL( "models/w_usp.mdl" );
	PRECACHE_SOUND( "weapons/usp1.wav" );
	PRECACHE_SOUND( "weapons/usp2.wav" );
	PRECACHE_SOUND( "weapons/usp_unsil-1.wav" );
	PRECACHE_SOUND( "weapons/usp_clipout.wav" );
	PRECACHE_SOUND( "weapons/usp_clipin.wav" );
	PRECACHE_SOUND( "weapons/usp_silencer_on.wav" );
	PRECACHE_SOUND( "weapons/usp_silencer_off.wav" );
	PRECACHE_SOUND( "weapons/usp_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/usp_slideback.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireUSP = PRECACHE_EVENT( 1, "events/usp.sc" );
}

int CUSP::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "45ACP";
	p->iMaxAmmo1 = 100;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 12;
	p->iSlot = 1;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_USP;
	p->iWeight = 5;
	p->bIsAccessory = false;
	return 1;
}

BOOL CUSP::Deploy()
{
	m_flAccuracy = 0.92f;
	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
		return DefaultDeploy( "models/v_usp.mdl", NULL, USP_DRAW, "onehanded", UseDecrement() != FALSE );
	else
		return DefaultDeploy( "models/v_usp.mdl", NULL, USP_UNSIL_DRAW, "onehanded", UseDecrement() != FALSE );
}

void CUSP::SecondaryAttack()
{
	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
	{
		m_iWeaponState &= ~WPNSTATE_USP_SILENCED;
		SendWeaponAnim( USP_DETACH_SILENCER, UseDecrement() );
	}
	else
	{
		m_iWeaponState |= WPNSTATE_USP_SILENCED;
		SendWeaponAnim( USP_ATTACH_SILENCER, UseDecrement() );
	}
	strcpy( m_pPlayer->m_szAnimExtention, "onehanded" );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3;
}

void CUSP::PrimaryAttack()
{
	if ( m_iWeaponState & WPNSTATE_USP_SILENCED )
	{
		if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
			USPFire( ( 1 - m_flAccuracy ) * 1.3, 0.225f, FALSE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			USPFire( ( 1 - m_flAccuracy ) * 0.25, 0.225f, FALSE );
		else if ( m_pPlayer->pev->flags & FL_DUCKING )
			USPFire( ( 1 - m_flAccuracy ) * 0.125, 0.225f, FALSE );
		else
			USPFire( ( 1 - m_flAccuracy ) * 0.15, 0.225f, FALSE );
	}
	else
	{
		if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
			USPFire( ( 1 - m_flAccuracy ) * 1.2, 0.225f, FALSE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			USPFire( ( 1 - m_flAccuracy ) * 0.225, 0.225f, FALSE );
		else if ( m_pPlayer->pev->flags & FL_DUCKING )
			USPFire( ( 1 - m_flAccuracy ) * 0.08, 0.225f, FALSE );
		else
			USPFire( ( 1 - m_flAccuracy ) * 0.1, 0.225f, FALSE );
	}
}

void CUSP::USPFire( float spread, float cycleTime, BOOL )
{
	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.3 - ( gpGlobals->time - m_flLastFire ) ) * 0.275;
		if ( m_flAccuracy > 0.92 )
			m_flAccuracy = 0.92f;
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

	cycleTime -= 0.075;
	float nextAttack = UTIL_WeaponTimeBase() + cycleTime;
	m_iClip--;
	m_flNextSecondaryAttack = m_flNextPrimaryAttack = nextAttack;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_iWeaponVolume = ( m_iWeaponState & WPNSTATE_USP_SILENCED ) ? 100 : 500;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	if ( !( m_iWeaponState & WPNSTATE_USP_SILENCED ) )
		m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 4096, 1, BULLET_PLAYER_45ACP,
		( m_iWeaponState & WPNSTATE_USP_SILENCED ) ? 30 : 34, 0.79f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireUSP, 0, (float*)&g_vecZero.x, (float*)&g_vecZero.x,
		direction.x, direction.y, (int)pitch, 0, m_iClip == 0, m_iWeaponState & WPNSTATE_USP_SILENCED );

	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CUSP::Reload()
{
	if ( m_pPlayer->ammo_45acp > 0 )
	{
		int animation = ( m_iWeaponState & WPNSTATE_USP_SILENCED ) ? USP_RELOAD : USP_UNSIL_RELOAD;
		if ( DefaultReload( 12, animation, 2.7f ) )
		{
			m_pPlayer->SetAnimation( PLAYER_RELOAD );
			m_flAccuracy = 0.92f;
		}
	}
}

void CUSP::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		int animation = ( m_iWeaponState & WPNSTATE_USP_SILENCED ) ? USP_IDLE : USP_UNSIL_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0;
		SendWeaponAnim( animation, UseDecrement() );
	}
}

int CUSP::iItemSlot()
{
	return 2;
}

float CUSP::GetMaxSpeed()
{
	return 225;
}

BOOL CUSP::UseDecrement()
{
	return TRUE;
}
