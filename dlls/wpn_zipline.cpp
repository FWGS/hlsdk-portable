#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_zipline.cpp" )

enum zipline_e
{
	ZIPLINE_DRAW = 0,
	ZIPLINE_USE,
	ZIPLINE_RELEASE,
	ZIPLINE_HOLSTER,
	ZIPLINE_IDLE,
};

LINK_ENTITY_TO_CLASS( weapon_zipline, CZipline );

void CZipline::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_zipline.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = ZIPLINE_IDLE;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_zipline" );
	m_iId = ACCESSORY_ZIPLINE;
	m_iDefaultAmmo = -1;
	m_iClip = WEAPON_NOCLIP;
	m_bIsAccessory = TRUE;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CZipline::Precache()
{
	PRECACHE_MODEL( "models/v_zipline.mdl" );
	PRECACHE_MODEL( "models/w_zipline.mdl" );
	m_usFireZipline = PRECACHE_EVENT( 1, "events/zipline.sc" );
}

int CZipline::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 13;
	p->iId = m_iId = ACCESSORY_ZIPLINE;
	p->iWeight = 2;
	p->iFlags = 0;
	p->bIsAccessory = TRUE;
	m_iPlayEmptySound = TRUE;
	return 1;
}

int CZipline::iItemSlot()
{
	return 5;
}

float CZipline::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case ZIPLINE_DRAW:
	case ZIPLINE_RELEASE:
	case ZIPLINE_HOLSTER:
	case ZIPLINE_IDLE:
		speed = 225;
		break;
	case ZIPLINE_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 168 );
		break;
	}
	return speed;
}

BOOL CZipline::CanDrop()
{
	return FALSE;
}

BOOL CZipline::UseDecrement()
{
	return TRUE;
}

BOOL CZipline::Deploy()
{
	return DefaultDeploy( "models/v_zipline.mdl", NULL, ZIPLINE_DRAW, "zipline", UseDecrement() );
}

void CZipline::Holster( int skiplocal )
{
	m_pPlayer->EnableControl( TRUE );
	m_pPlayer->RemovePlayerItem( this );
}

void CZipline::SetWeaponState( const unsigned short weaponState )
{
	float delay = 0;
	switch ( weaponState )
	{
	case ZIPLINE_DRAW:
		delay = 1.033f;
		break;
	case ZIPLINE_USE:
		delay = 2;
		break;
	case ZIPLINE_RELEASE:
	case ZIPLINE_HOLSTER:
		delay = 0.766f;
		break;
	case ZIPLINE_IDLE:
		delay = 0;
		break;
	default:
		ASSERT_AT( false, 140 );
		break;
	}
	SendWeaponAnim( weaponState, UseDecrement() );
	m_WeaponState = weaponState;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + delay;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + delay;
}

void CZipline::PrimaryAttack()
{
	if ( GetWeaponState() != ZIPLINE_IDLE )
		return;
	int z = (int)m_pPlayer->GetGunPosition().z;
	int y = (int)m_pPlayer->GetGunPosition().y;
	int x = (int)m_pPlayer->GetGunPosition().x;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireZipline, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, x, y, z, 0 );
	m_pPlayer->pev->flags |= FL_FROZEN;
	m_pPlayer->pev->flags |= FL_IMMUNE_LAVA;
	SetWeaponState( ZIPLINE_USE );
}

void CZipline::SecondaryAttack()
{
	if ( GetWeaponState() != ZIPLINE_USE )
		return;
	m_pPlayer->pev->flags &= ~FL_IMMUNE_LAVA;
	m_pPlayer->pev->flags &= ~FL_FROZEN;
	SetWeaponState( ZIPLINE_RELEASE );
}

CBaseEntity* CZipline::FindZipLineEntity()
{
	return NULL;
}

void CZipline::WeaponIdle()
{
	if ( !( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() ) )
		return;
	switch ( GetWeaponState() )
	{
	case ZIPLINE_DRAW:
		SetWeaponState( ZIPLINE_IDLE );
		break;
	case ZIPLINE_USE:
		m_pPlayer->ResetMaxSpeed();
		break;
	case ZIPLINE_RELEASE:
	case ZIPLINE_HOLSTER:
		RetireWeapon();
		break;
	case ZIPLINE_IDLE:
		PrimaryAttack();
		break;
	default:
		ASSERT_AT( false, 243 );
		break;
	}
}
