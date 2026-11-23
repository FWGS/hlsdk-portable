#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum glock18_e
{
	GLOCK18_IDLE1,
	GLOCK18_IDLE2,
	GLOCK18_IDLE3,
	GLOCK18_SHOOT,
	GLOCK18_SHOOT2,
	GLOCK18_SHOOT3,
	GLOCK18_SHOOT_EMPTY,
	GLOCK18_RELOAD,
	GLOCK18_DRAW,
	GLOCK18_HOLSTER,
	GLOCK18_ADD_SILENCER,
	GLOCK18_DRAW2,
	GLOCK18_RELOAD2
};

LINK_ENTITY_TO_CLASS( weapon_glock18, CGLOCK18 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CGLOCK18::m_SaveData[] =
{
	DEFINE_FIELD( CGLOCK18, m_bBurstFire, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CGLOCK18, CBasePlayerWeapon );
#endif

void CGLOCK18::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_glock18" );
	Precache();
	m_iId = WEAPON_GLOCK18;
	SET_MODEL( edict(), "models/w_glock18.mdl" );
	m_iGlock18ShotsFired = 0;
	m_iDefaultAmmo = 20;
	m_bBurstFire = false;
	m_flGlock18Shoot = 0;
	m_flAccuracy = 0.9f;
	m_bIsAccessory = false;
	FallInit();
}

void CGLOCK18::Precache()
{
	PRECACHE_MODEL( "models/v_glock18.mdl" );
	PRECACHE_MODEL( "models/w_glock18.mdl" );
	PRECACHE_SOUND( "weapons/glock18-1.wav" );
	PRECACHE_SOUND( "weapons/glock18-2.wav" );
	PRECACHE_SOUND( "weapons/clipout1.wav" );
	PRECACHE_SOUND( "weapons/clipin1.wav" );
	PRECACHE_SOUND( "weapons/sliderelease1.wav" );
	PRECACHE_SOUND( "weapons/slideback1.wav" );
	PRECACHE_SOUND( "weapons/357_cock1.wav" );
	PRECACHE_SOUND( "weapons/de_clipin.wav" );
	PRECACHE_SOUND( "weapons/de_clipout.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_iShellId = m_iShell;
	m_usFireGlock18 = PRECACHE_EVENT( 1, "events/glock18.sc" );
}

int CGLOCK18::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = 120;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 1;
	p->iPosition = 2;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_GLOCK18;
	p->iWeight = 5;
	p->bIsAccessory = false;
	return 1;
}

BOOL CGLOCK18::Deploy()
{
	m_bBurstFire = false;
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0;
	m_flAccuracy = 0.9f;
	if ( RANDOM_LONG( 0, 1 ) )
		return DefaultDeploy( "models/v_glock18.mdl", NULL, GLOCK18_DRAW, "onehanded", UseDecrement() != FALSE );
	else
		return DefaultDeploy( "models/v_glock18.mdl", NULL, GLOCK18_DRAW2, "onehanded", UseDecrement() != FALSE );
}

void CGLOCK18::SecondaryAttack()
{
	if ( m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE )
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_SemiAuto" );
		m_iWeaponState &= ~WPNSTATE_GLOCK18_BURST_MODE;
	}
	else
	{
		ClientPrint( m_pPlayer->pev, HUD_PRINTCENTER, "#Switch_To_BurstFire" );
		m_iWeaponState |= WPNSTATE_GLOCK18_BURST_MODE;
	}
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CGLOCK18::PrimaryAttack()
{
	if ( m_iWeaponState & WPNSTATE_GLOCK18_BURST_MODE )
	{
		if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 1.2, 0.5f, TRUE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.185, 0.5f, TRUE );
		else if ( m_pPlayer->pev->flags & FL_DUCKING )
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.095, 0.5f, TRUE );
		else
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.3, 0.5f, TRUE );
	}
	else
	{
		if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
			GLOCK18Fire( 1 - m_flAccuracy, 0.2f, FALSE );
		else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.165, 0.2f, FALSE );
		else if ( m_pPlayer->pev->flags & FL_DUCKING )
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.075, 0.2f, FALSE );
		else
			GLOCK18Fire( ( 1 - m_flAccuracy ) * 0.1, 0.2f, FALSE );
	}
}

void CGLOCK18::GLOCK18Fire( float spread, float cycleTime, BOOL burst )
{
	if ( burst )
	{
		m_iGlock18ShotsFired = 0;
	}
	else
	{
		if ( ++m_iShotsFired > 1 )
			return;
		cycleTime -= 0.05;
	}

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.325 - ( gpGlobals->time - m_flLastFire ) ) * 0.275;
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
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 8192, 1, BULLET_PLAYER_9MM, 20,
		0.75f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	float yaw = m_pPlayer->pev->punchangle.y * 100;
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireGlock18, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, direction.x, direction.y, (int)pitch, (int)yaw, m_iClip == 0, 0 );
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
	if ( burst )
	{
		m_flGlock18Shoot = gpGlobals->time + 0.1;
		m_iGlock18ShotsFired++;
	}
}

void CGLOCK18::Reload()
{
	if ( m_pPlayer->ammo_9mm > 0 )
	{
		int animation = RANDOM_LONG( 0, 1 ) ? GLOCK18_RELOAD : GLOCK18_RELOAD2;
		if ( DefaultReload( 20, animation, 2.2f ) )
		{
			m_pPlayer->SetAnimation( PLAYER_RELOAD );
			m_flAccuracy = 0.9f;
		}
	}
}

void CGLOCK18::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		float random = RANDOM_FLOAT( 0, 1 );
		int animation;
		if ( random <= 0.3 )
		{
			animation = GLOCK18_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
		}
		else if ( random > 0.6 )
		{
			animation = GLOCK18_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
		}
		else
		{
			animation = GLOCK18_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
		}
		SendWeaponAnim( animation, UseDecrement() );
	}
}

int CGLOCK18::iItemSlot()
{
	return 2;
}

float CGLOCK18::GetMaxSpeed()
{
	return 225;
}

BOOL CGLOCK18::UseDecrement()
{
	return TRUE;
}
