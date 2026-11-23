#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"

LINK_ENTITY_TO_CLASS( weapon_scout, CSCOUT );

void CSCOUT::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_scout" );
	Precache();
	m_iId = WEAPON_SCOUT;
	SET_MODEL( ENT( pev ), "models/w_scout.mdl" );
	m_iDefaultAmmo = 10;
	m_bIsAccessory = FALSE;
	FallInit();
}

void CSCOUT::Precache()
{
	PRECACHE_MODEL( "models/v_scout.mdl" );
	PRECACHE_MODEL( "models/w_scout.mdl" );
	PRECACHE_SOUND( "weapons/scout_fire-1.wav" );
	PRECACHE_SOUND( "weapons/scout_bolt.wav" );
	PRECACHE_SOUND( "weapons/scout_clipin.wav" );
	PRECACHE_SOUND( "weapons/scout_clipout.wav" );
	PRECACHE_SOUND( "weapons/zoom.wav" );
	m_iShellId = m_iShell = PRECACHE_MODEL( "models/rshell_big.mdl" );
	m_usFireScout = PRECACHE_EVENT( 1, "events/scout.sc" );
}

int CSCOUT::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "762Nato";
	p->iMaxAmmo1 = 90;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 10;
	p->iSlot = 0;
	p->iPosition = 9;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SCOUT;
	p->iWeight = 30;
	p->bIsAccessory = FALSE;
	return 1;
}

float CSCOUT::GetMaxSpeed()
{
	return m_pPlayer->m_iFOV == 90 ? 225.0f : 198.0f;
}

BOOL CSCOUT::Deploy()
{
	return DefaultDeploy( "models/v_scout.mdl", NULL, 4, "rifle", UseDecrement() );
}

void CSCOUT::SecondaryAttack()
{
	switch ( m_pPlayer->m_iFOV )
	{
	case 90:
		m_pPlayer->m_iFOV = 40;
		m_pPlayer->pev->fov = 40;
		break;
	case 40:
		m_pPlayer->m_iFOV = 15;
		m_pPlayer->pev->fov = 15;
		break;
	case 15:
		m_pPlayer->m_iFOV = 90;
		m_pPlayer->pev->fov = 90;
		break;
	}
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_ITEM, "weapons/zoom.wav", 0.2f, 2.4f, 0, PITCH_NORM );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3;
}

void CSCOUT::PrimaryAttack()
{
	if ( !( m_pPlayer->pev->flags & FL_ONGROUND ) )
		SCOUTFire( 0.2f, 1.25f, FALSE );
	else if ( m_pPlayer->pev->velocity.Length2D() > 170 )
		SCOUTFire( 0.075f, 1.25f, FALSE );
	else if ( m_pPlayer->pev->flags & FL_DUCKING )
		SCOUTFire( 0, 1.25f, FALSE );
	else
		SCOUTFire( 0.007f, 1.25f, FALSE );
}

void CSCOUT::SCOUTFire( float flSpread, float flCycleTime, BOOL )
{
	if ( m_pPlayer->m_iFOV == 90 )
	{
		flSpread = (float)( flSpread + 0.025 );
	}
	else
	{
		m_pPlayer->m_bResumeZoom = true;
		m_pPlayer->m_iLastZoom = m_pPlayer->m_iFOV;
		m_pPlayer->m_iFOV = 90;
	}
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
	m_pPlayer->m_flEjectBrass = (float)( gpGlobals->time + 0.56 );
	m_pPlayer->m_iWeaponVolume = 800;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	Vector vecDir = m_pPlayer->FireBullets3( m_pPlayer->GetGunPosition(), gpGlobals->v_forward,
		flSpread, 8192, 3, BULLET_PLAYER_762MM, 75, 0.98f, m_pPlayer->pev, true, m_pPlayer->random_seed );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireScout, 0, (float*)&g_vecZero, m_pPlayer->pev->angles,
		vecDir.x * 1000.0, vecDir.y * 1000.0, (int)( m_pPlayer->pev->punchangle.x * 100 ),
		(int)( m_pPlayer->pev->punchangle.y * 100 ), 0, 0 );
	float flDelay = UTIL_WeaponTimeBase() + flCycleTime;
	m_flNextSecondaryAttack = flDelay;
	m_flNextPrimaryAttack = flDelay;
	if ( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.8;
	m_pPlayer->pev->punchangle.x -= 2;
}

void CSCOUT::Reload()
{
	if ( DefaultReload( 10, 3, 2 ) )
	{
		if ( m_pPlayer->m_iFOV != 90 )
		{
			m_pPlayer->m_iFOV = 15;
			m_pPlayer->pev->fov = 15;
			SecondaryAttack();
		}
		m_pPlayer->SetAnimation( PLAYER_RELOAD );
	}
}

void CSCOUT::WeaponIdle()
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip != 0 )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 60;
		SendWeaponAnim( 0, UseDecrement() );
	}
}

int CSCOUT::iItemSlot()
{
	return 1;
}

BOOL CSCOUT::UseDecrement()
{
	return TRUE;
}
