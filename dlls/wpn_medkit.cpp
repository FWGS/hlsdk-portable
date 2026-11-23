#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum medkit_e
{
	MEDKIT_IDLE,
	MEDKIT_PINPULL,
	MEDKIT_THROW,
	MEDKIT_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_medkit, CMedkit );

void CMedkit::Spawn()
{
	Precache();
	m_iId = ACCESSORY_MEDKIT;
	SET_MODEL( edict(), "models/w_medkit.mdl" );
	pev->dmg = 80;
	m_iDefaultAmmo = 1;
	m_flStartThrow = 0;
	m_flReleaseThrow = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CMedkit::Precache()
{
	PRECACHE_MODEL( "models/v_medkit.mdl" );
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "weapons/medkit-1.wav" );
	m_usFireMedkit = PRECACHE_EVENT( 1, "events/medkit.sc" );
}

int CMedkit::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Medkit";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 9;
	p->iId = m_iId = ACCESSORY_MEDKIT;
	p->iWeight = 2;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = true;
	return 1;
}

BOOL CMedkit::Deploy()
{
	m_flReleaseThrow = -1;
	return DefaultDeploy( "models/v_medkit.mdl", NULL, MEDKIT_DRAW, "grenade", UseDecrement() );
}

BOOL CMedkit::CanHolster()
{
	return m_flStartThrow == 0;
}

void CMedkit::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_MEDKIT );
		DestroyItem();
	}
}

void CMedkit::PrimaryAttack()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 )
	{
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
		m_pPlayer->TakeHealth( pev->dmg, DMG_GENERIC );
		SendWeaponAnim( MEDKIT_PINPULL, UseDecrement() );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
	}
}

void CMedkit::WeaponIdle()
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( MEDKIT_IDLE, UseDecrement() );
		float time = UTIL_WeaponTimeBase();
		m_flTimeWeaponIdle = RANDOM_FLOAT( 10, 15 ) + time;
	}
}

BOOL CMedkit::CanDrop()
{
	return FALSE;
}

BOOL CMedkit::CanDeploy()
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 0;
}

int CMedkit::iItemSlot()
{
	return 5;
}

float CMedkit::GetMaxSpeed()
{
	return 225;
}
