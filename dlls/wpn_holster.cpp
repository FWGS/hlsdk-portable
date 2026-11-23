#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum holster_e
{
	HOLSTER_IDLE1,
	HOLSTER_IDLE2,
	HOLSTER_IDLE3,
	HOLSTER_SHOOT,
	HOLSTER_SHOOT2,
	HOLSTER_SHOOT3,
	HOLSTER_SHOOT_EMPTY,
	HOLSTER_RELOAD,
	HOLSTER_DRAW,
	HOLSTER_HOLSTER,
	HOLSTER_ADD_SILENCER,
	HOLSTER_DRAW2,
	HOLSTER_RELOAD2
};

void CHolster::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_holster" );
	Precache();
	m_iId = ACCESSORY_HOLSTER;
	SET_MODEL( edict(), "models/w_holster.mdl" );
	m_iGlock18ShotsFired = 0;
	m_iDefaultAmmo = -1;
	m_flGlock18Shoot = 0;
	m_flAccuracy = 0.9f;
	m_bIsAccessory = true;
	FallInit();
}

void CHolster::Precache()
{
	PRECACHE_MODEL( "models/v_holster.mdl" );
	PRECACHE_MODEL( "models/w_holster.mdl" );
}

int CHolster::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = ACCESSORY_HOLSTER;
	p->iWeight = 5;
	p->bIsAccessory = true;
	return 1;
}

BOOL CHolster::Deploy()
{
	m_iGlock18ShotsFired = 0;
	m_flGlock18Shoot = 0;
	m_flAccuracy = 0.9f;
	int incognito = m_pPlayer->m_iIncognito;
	BOOL deployed = DefaultDeploy( "models/v_holster.mdl", NULL, HOLSTER_DRAW, "onehanded", UseDecrement() );
	m_pPlayer->m_iIncognito = incognito;
	return deployed;
}

void CHolster::Holster( int )
{
	SetWeaponState( HOLSTER_DRAW );
}

void CHolster::PrimaryAttack()
{
}

void CHolster::SecondaryAttack()
{
}

void CHolster::Reload()
{
}

void CHolster::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() && m_iClip )
	{
		float random = RANDOM_FLOAT( 0, 1 );
		int animation;
		if ( random <= 0.3 )
		{
			animation = HOLSTER_IDLE3;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0625;
		}
		else if ( random > 0.6 )
		{
			animation = HOLSTER_IDLE2;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
		}
		else
		{
			animation = HOLSTER_IDLE1;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.75;
		}
		SendWeaponAnim( animation, UseDecrement() );
	}
}

int CHolster::iItemSlot()
{
	return 6;
}

float CHolster::GetMaxSpeed()
{
	return 225;
}
