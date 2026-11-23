#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum deagle_e
{
	DEAGLE_IDLE1,
	DEAGLE_SHOOT1,
	DEAGLE_SHOOT2,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_deagle, CDEAGLE );

void CDEAGLE::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_deagle" );
	Precache();
	m_iId = WEAPON_DEAGLE;
	SET_MODEL( edict(), "models/w_deagle.mdl" );
	m_iDefaultAmmo = 7;
	m_flAccuracy = 0.9f;
	m_bIsAccessory = false;
	FallInit();
}

void CDEAGLE::Precache()
{
	PRECACHE_MODEL( "models/v_deagle.mdl" );
	PRECACHE_MODEL( "models/w_deagle.mdl" );
	PRECACHE_SOUND( "weapons/deagle-1.wav" );
	PRECACHE_SOUND( "weapons/deagle-2.wav" );
	PRECACHE_SOUND( "weapons/de_clipout.wav" );
	PRECACHE_SOUND( "weapons/de_clipin.wav" );
	PRECACHE_SOUND( "weapons/de_deploy.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireDeagle = PRECACHE_EVENT( 1, "events/deagle.sc" );
}

int CDEAGLE::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "50AE";
	p->iMaxAmmo1 = 35;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 7;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_DEAGLE;
	p->iWeight = 7;
	p->bIsAccessory = false;
	return 1;
}

BOOL CDEAGLE::Deploy()
{
	m_flAccuracy = 0.9f;
	return DefaultDeploy( "models/v_deagle.mdl", NULL, DEAGLE_DRAW, "onehanded", UseDecrement() );
}

void CDEAGLE::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 1.5, 0.3f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.25, 0.3f, FALSE );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.115, 0.3f, FALSE );
	else
		DEAGLEFire( ( 1 - m_flAccuracy ) * 0.13, 0.3f, FALSE );
}

void CDEAGLE::DEAGLEFire( float spread, float cycleTime, BOOL )
{
	if ( ++m_iShotsFired > 1 )
		return;

	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.4 - ( gpGlobals->time - m_flLastFire ) ) * 0.35;
		if ( m_flAccuracy > 0.9 )
			m_flAccuracy = 0.9f;
		else if ( m_flAccuracy < 0.55 )
			m_flAccuracy = 0.55f;
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
	m_pPlayer->m_iWeaponVolume = 1500;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 8192, 2, BULLET_PLAYER_50AE, 54,
		0.81f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	float yaw = m_pPlayer->pev->punchangle.y * 100;
	float pitch = m_pPlayer->pev->punchangle.x * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireDeagle, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, direction.x, direction.y, (int)pitch, (int)yaw, m_iClip == 0, 0 );

	cycleTime -= 0.075;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + cycleTime;
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.8;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CDEAGLE::Reload()
{
	if ( m_pPlayer->ammo_50ae > 0 && DefaultReload( 7, DEAGLE_RELOAD, 2.2f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.9f;
	}
}

void CDEAGLE::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
		SendWeaponAnim( DEAGLE_IDLE1, UseDecrement() );
	}
}

int CDEAGLE::iItemSlot()
{
	return 2;
}

float CDEAGLE::GetMaxSpeed()
{
	return 225;
}

BOOL CDEAGLE::UseDecrement()
{
	return TRUE;
}
