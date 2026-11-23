#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum elite_e
{
	ELITE_IDLE,
	ELITE_IDLE_LEFTEMPTY,
	ELITE_SHOOTLEFT1,
	ELITE_SHOOTLEFT2,
	ELITE_SHOOTLEFT3,
	ELITE_SHOOTLEFT4,
	ELITE_SHOOTLEFT5,
	ELITE_SHOOTLEFTLAST,
	ELITE_SHOOTRIGHT1,
	ELITE_SHOOTRIGHT2,
	ELITE_SHOOTRIGHT3,
	ELITE_SHOOTRIGHT4,
	ELITE_SHOOTRIGHT5,
	ELITE_SHOOTRIGHTLAST,
	ELITE_RELOAD,
	ELITE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_elite, CELITE );

void CELITE::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_elite" );
	Precache();
	m_iId = WEAPON_ELITE;
	SET_MODEL( edict(), "models/w_elite.mdl" );
	m_iDefaultAmmo = 30;
	m_flAccuracy = 0.88f;
	m_bIsAccessory = false;
	FallInit();
}

void CELITE::Precache()
{
	PRECACHE_MODEL( "models/v_elite.mdl" );
	PRECACHE_MODEL( "models/w_elite.mdl" );
	PRECACHE_SOUND( "weapons/elite_fire.wav" );
	PRECACHE_SOUND( "weapons/elite_reloadstart.wav" );
	PRECACHE_SOUND( "weapons/elite_leftclipin.wav" );
	PRECACHE_SOUND( "weapons/elite_clipout.wav" );
	PRECACHE_SOUND( "weapons/elite_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/elite_rightclipin.wav" );
	PRECACHE_SOUND( "weapons/elite_deploy.wav" );
	m_iShell = PRECACHE_MODEL( "models/pshell.mdl" );
	m_usFireELITE_LEFT = PRECACHE_EVENT( 1, "events/elite_left.sc" );
	m_usFireELITE_RIGHT = PRECACHE_EVENT( 1, "events/elite_right.sc" );
}

int CELITE::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = 120;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 30;
	p->iSlot = 1;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_ELITE;
	p->iWeight = 5;
	p->bIsAccessory = false;
	return 1;
}

BOOL CELITE::Deploy()
{
	m_flAccuracy = 0.88f;
	if ( !( m_iClip & 1 ) )
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;
	return DefaultDeploy( "models/v_elite.mdl", NULL, ELITE_DRAW, "dualpistols", UseDecrement() );
}

void CELITE::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		ELITEFire( ( 1 - m_flAccuracy ) * 1.3, 0.2f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 0 )
		ELITEFire( ( 1 - m_flAccuracy ) * 0.175, 0.2f, FALSE );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		ELITEFire( ( 1 - m_flAccuracy ) * 0.08, 0.2f, FALSE );
	else
		ELITEFire( ( 1 - m_flAccuracy ) * 0.1, 0.2f, FALSE );
}

void CELITE::ELITEFire( float spread, float cycleTime, BOOL )
{
	if ( ++m_iShotsFired > 1 )
		return;

	float timeDiff = gpGlobals->time - m_flLastFire;
	if ( m_flLastFire != 0 )
	{
		m_flAccuracy -= ( 0.325 - timeDiff ) * 0.275;
		if ( m_flAccuracy > 0.88 )
			m_flAccuracy = 0.88f;
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

	float time = UTIL_WeaponTimeBase();
	m_iClip--;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = time + cycleTime - 0.125;
	m_pPlayer->m_iWeaponVolume = 500;
	m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
	Vector aiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	Vector source = m_pPlayer->GetGunPosition();
	bool right = ( m_iClip & 1 ) != 0;
	if ( right )
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK2 );
		m_iWeaponState |= WPNSTATE_ELITE_LEFT;
		source = source + gpGlobals->v_right * 5;
	}
	else
	{
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		m_iWeaponState &= ~WPNSTATE_ELITE_LEFT;
		source = source - gpGlobals->v_right * 5;
	}
	Vector direction = m_pPlayer->FireBullets3( source, aiming, spread, 8192, 1, BULLET_PLAYER_9MM,
		right ? 27 : 20, 0.75f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	float yaw = direction.y * 100;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), right ? m_usFireELITE_RIGHT : m_usFireELITE_LEFT, 0,
		(float*)&g_vecZero.x, (float*)&g_vecZero.x, timeDiff, direction.x, (int)yaw, m_iClip, 0, 0 );
	if ( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CELITE::Reload()
{
	if ( m_pPlayer->ammo_9mm > 0 && DefaultReload( 30, ELITE_RELOAD, 4.5f ) )
	{
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
		m_flAccuracy = 0.88f;
	}
}

void CELITE::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		int animation = m_iClip == 1 ? ELITE_IDLE_LEFTEMPTY : ELITE_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60.0;
		SendWeaponAnim( animation, UseDecrement() );
	}
}

int CELITE::iItemSlot()
{
	return 2;
}

float CELITE::GetMaxSpeed()
{
	return 225;
}

BOOL CELITE::UseDecrement()
{
	return TRUE;
}
