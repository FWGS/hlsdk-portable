#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum hegrenade_e
{
	HEGRENADE_IDLE,
	HEGRENADE_PINPULL,
	HEGRENADE_THROW,
	HEGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_hegrenade, CHEGrenade );

void CHEGrenade::Spawn()
{
	Precache();
	m_iId = ACCESSORY_HEGRENADE;
	SET_MODEL( edict(), "models/w_hegrenade.mdl" );
	pev->dmg = 4;
	m_iDefaultAmmo = 1;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CHEGrenade::Precache()
{
	PRECACHE_MODEL( "models/v_hegrenade.mdl" );
	PRECACHE_MODEL( "models/w_hegrenade.mdl" );
	PRECACHE_SOUND( "weapons/hegrenade-1.wav" );
	PRECACHE_SOUND( "weapons/hegrenade-2.wav" );
	PRECACHE_SOUND( "weapons/he_bounce-1.wav" );
	PRECACHE_SOUND( "weapons/pinpull.wav" );
	m_usCreateExplosion = PRECACHE_EVENT( 1, "events/createexplo.sc" );
}

int CHEGrenade::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "HEGrenade";
	p->iMaxAmmo1 = 3;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 1;
	p->iId = m_iId = ACCESSORY_HEGRENADE;
	p->iWeight = 2;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = true;
	return 1;
}

BOOL CHEGrenade::Deploy()
{
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_hegrenade.mdl", NULL, HEGRENADE_DRAW, "grenade", UseDecrement() );
}

BOOL CHEGrenade::CanHolster()
{
	return m_flStartThrow == 0;
}

void CHEGrenade::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_HEGRENADE );
		DestroyItem();
	}
	m_flReleaseThrow = 0;
	m_flStartThrow = 0;
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CHEGrenade::PrimaryAttack()
{
	if ( m_flStartThrow == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flReleaseThrow = 0;
		m_flStartThrow = gpGlobals->time;
		SendWeaponAnim( HEGRENADE_PINPULL, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CHEGrenade::WeaponIdle()
{
	if ( m_flReleaseThrow == 0 && m_flStartThrow != 0 )
		m_flReleaseThrow = gpGlobals->time;
	if ( !( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() ) )
		return;

	if ( m_flStartThrow != 0 )
	{
		Vector angles = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
		if ( angles.x < 0 )
			angles.x = angles.x * ( 80.0 / 90.0 ) - 10.0;
		else
			angles.x = angles.x * ( 100.0 / 90.0 ) - 10.0;
		float speed = ( 90 - angles.x ) * 6;
		if ( speed > 750 )
			speed = 750;
		UTIL_MakeVectors( angles );
		Vector start = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
		Vector velocity = gpGlobals->v_forward * speed + m_pPlayer->pev->velocity;
		CGrenade::ShootTimed2( m_pPlayer->pev, start, velocity, 1.5f, m_pPlayer->m_iTeam, m_usCreateExplosion );
		SendWeaponAnim( HEGRENADE_THROW, UseDecrement() );
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		m_flStartThrow = 0;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			m_flNextPrimaryAttack = m_flNextSecondaryAttack = m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
	else if ( m_flReleaseThrow > 0 )
	{
		m_flStartThrow = 0;
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( HEGRENADE_DRAW, UseDecrement() );
			float time = UTIL_WeaponTimeBase();
			float delay = RANDOM_FLOAT( 10, 15 );
			m_flReleaseThrow = -1;
			m_flTimeWeaponIdle = delay + time;
		}
		else
		{
			RetireWeapon();
		}
	}
	else if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( HEGRENADE_IDLE, UseDecrement() );
		float time = UTIL_WeaponTimeBase();
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10, 15 ) + time;
	}
}

BOOL CHEGrenade::CanDrop()
{
	return FALSE;
}

BOOL CHEGrenade::CanDeploy()
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
}

int CHEGrenade::iItemSlot()
{
	return 4;
}

float CHEGrenade::GetMaxSpeed()
{
	return 225;
}

BOOL CHEGrenade::UseDecrement()
{
	return TRUE;
}
