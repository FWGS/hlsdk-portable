#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum flashbang_e
{
	FLASHBANG_IDLE,
	FLASHBANG_PINPULL,
	FLASHBANG_THROW,
	FLASHBANG_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashbang );

void CFlashbang::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_flashbang" );
	Precache();
	m_iId = ACCESSORY_FLASHBANG;
	SET_MODEL( edict(), "models/w_flashbang.mdl" );
	pev->dmg = 4;
	m_iDefaultAmmo = 1;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CFlashbang::Precache()
{
	PRECACHE_MODEL( "models/v_flashbang.mdl" );
	PRECACHE_MODEL( "models/w_flashbang.mdl" );
	PRECACHE_SOUND( "weapons/flashbang-1.wav" );
	PRECACHE_SOUND( "weapons/flashbang-2.wav" );
	PRECACHE_SOUND( "weapons/pinpull.wav" );
}

int CFlashbang::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Flashbang";
	p->iMaxAmmo1 = 3;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = ACCESSORY_FLASHBANG;
	p->iWeight = 1;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = true;
	return 1;
}

BOOL CFlashbang::Deploy()
{
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_flashbang.mdl", NULL, FLASHBANG_DRAW, "grenade", UseDecrement() );
}

BOOL CFlashbang::CanHolster()
{
	return TRUE;
}

void CFlashbang::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_FLASHBANG );
		DestroyItem();
	}
	m_flReleaseThrow = 0;
	m_flStartThrow = 0;
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CFlashbang::PrimaryAttack()
{
	if ( m_flStartThrow == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_flReleaseThrow = 0;
		m_flStartThrow = gpGlobals->time;
		SendWeaponAnim( FLASHBANG_PINPULL, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CFlashbang::WeaponIdle()
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
		CGrenade::ShootTimed( m_pPlayer->pev, start, velocity, 1.5f, 35, 0 );
		SendWeaponAnim( FLASHBANG_THROW, UseDecrement() );
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
			SendWeaponAnim( FLASHBANG_DRAW, UseDecrement() );
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
		SendWeaponAnim( FLASHBANG_IDLE, UseDecrement() );
	}
}

BOOL CFlashbang::CanDrop()
{
	return FALSE;
}

BOOL CFlashbang::CanDeploy()
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
}

int CFlashbang::iItemSlot()
{
	return 4;
}

float CFlashbang::GetMaxSpeed()
{
	return 225;
}

BOOL CFlashbang::UseDecrement()
{
	return TRUE;
}
