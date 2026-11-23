#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum smokegrenade_e
{
	SMOKEGRENADE_IDLE,
	SMOKEGRENADE_PINPULL,
	SMOKEGRENADE_THROW,
	SMOKEGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_smokegrenade, CSmokeGrenade );

void CSmokeGrenade::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_smokegrenade" );
	Precache();
	m_iId = ACCESSORY_SMOKEGRENADE;
	SET_MODEL( edict(), "models/w_smokegrenade.mdl" );
	pev->dmg = 4;
	m_iDefaultAmmo = 1;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CSmokeGrenade::Precache()
{
	PRECACHE_MODEL( "models/v_smokegrenade.mdl" );
	PRECACHE_MODEL( "models/w_smokegrenade.mdl" );
	PRECACHE_SOUND( "weapons/pinpull.wav" );
	PRECACHE_SOUND( "weapons/sg_explode.wav" );
	m_usCreateSmoke = PRECACHE_EVENT( 1, "events/createsmoke.sc" );
}

int CSmokeGrenade::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "SmokeGrenade";
	p->iMaxAmmo1 = 3;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = ACCESSORY_SMOKEGRENADE;
	p->iWeight = 1;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = true;
	return 1;
}

BOOL CSmokeGrenade::Deploy()
{
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_smokegrenade.mdl", NULL, SMOKEGRENADE_DRAW, "grenade", UseDecrement() );
}

BOOL CSmokeGrenade::CanHolster()
{
	return TRUE;
}

void CSmokeGrenade::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_SMOKEGRENADE );
		DestroyItem();
	}
	m_flReleaseThrow = 0;
	m_flStartThrow = 0;
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CSmokeGrenade::PrimaryAttack()
{
	if ( m_flStartThrow == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flReleaseThrow = 0;
		m_flStartThrow = gpGlobals->time;
		SendWeaponAnim( SMOKEGRENADE_PINPULL, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CSmokeGrenade::WeaponIdle()
{
	if ( m_flReleaseThrow == 0 )
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
		CGrenade::ShootSmokeGrenade( m_pPlayer->pev, start, velocity, 1.5f, m_usCreateSmoke );
		SendWeaponAnim( SMOKEGRENADE_THROW, UseDecrement() );
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
			SendWeaponAnim( SMOKEGRENADE_DRAW, UseDecrement() );
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
		if ( RANDOM_FLOAT( 0, 1 ) > 0.75 )
		{
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
		}
		else
		{
			float time = UTIL_WeaponTimeBase();
			m_flTimeWeaponIdle = RANDOM_FLOAT( 10, 15 ) + time;
		}
		SendWeaponAnim( SMOKEGRENADE_IDLE, UseDecrement() );
	}
}

BOOL CSmokeGrenade::CanDrop()
{
	return FALSE;
}

BOOL CSmokeGrenade::CanDeploy()
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
}

int CSmokeGrenade::iItemSlot()
{
	return 4;
}

float CSmokeGrenade::GetMaxSpeed()
{
	return 225;
}

BOOL CSmokeGrenade::UseDecrement()
{
	return TRUE;
}
