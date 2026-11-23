#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_syringe.cpp" )

enum syringe_e
{
	SYRINGE_IDLE,
	SYRINGE_USE,
	SYRINGE_DRAW,
	SYRINGE_DISCARD,
	SYRINGE_HOLSTER
};

void CSyringe::Spawn()
{
	SET_MODEL( edict(), "models/w_syringegun.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_syringe" );
	Precache();
	m_iId = ACCESSORY_SYRINGE;
	m_iDefaultAmmo = 3;
	m_iClip = -1;
	m_bIsAccessory = true;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CSyringe::Precache()
{
	PRECACHE_MODEL( "models/v_syringegun.mdl" );
	PRECACHE_MODEL( "models/w_syringegun.mdl" );
	m_usFireSyringe = PRECACHE_EVENT( 1, "events/syringe.sc" );
}

int CSyringe::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "syringe";
	p->iMaxAmmo1 = 3;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 3;
	p->iSlot = 4;
	p->iPosition = 10;
	p->iId = m_iId = ACCESSORY_SYRINGE;
	p->iWeight = 2;
	p->iFlags = 0;
	p->bIsAccessory = true;
	m_iPlayEmptySound = 1;
	return 1;
}

BOOL CSyringe::Deploy()
{
	return DefaultDeploy( "models/v_syringegun.mdl", NULL, SYRINGE_DRAW, "syringe", UseDecrement() );
}

void CSyringe::Holster( int )
{
	SetWeaponState( SYRINGE_DRAW );
}

CBaseEntity* CSyringe::FindInjectionTarget()
{
	Vector offset = gpGlobals->v_forward * 12;
	Vector start = m_pPlayer->GetGunPosition() + offset;
	Vector end = start + gpGlobals->v_forward * 36;
	TraceResult trace;
	trace.pHit = NULL;
	UTIL_TraceLine( start, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	CBaseEntity* entity = CBaseEntity::Instance( trace.pHit );
	if ( entity )
	{
		CBaseMonster* monster = entity->MyMonsterPointer();
		if ( monster && monster->HasHealthModifier() )
			return monster;
	}
	return NULL;
}

void CSyringe::Inject( CBaseEntity* entity )
{
	int z = (int)m_pPlayer->GetGunPosition().z;
	int y = (int)m_pPlayer->GetGunPosition().y;
	int x = (int)m_pPlayer->GetGunPosition().x;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireSyringe, 0, (float*)&g_vecZero.x,
		(float*)&g_vecZero.x, 0, 0, x, y, z, 0 );
#ifndef CLIENT_DLL
	entity->Use( m_pPlayer, m_pPlayer, USE_SET, 1 );
	m_pPlayer->m_rgAmmo[CBasePlayer::GetAmmoIndex( "syringe" )]--;
#endif
}

void CSyringe::PrimaryAttack()
{
	if ( GetWeaponState() == SYRINGE_IDLE )
	{
		CBaseEntity* target = FindInjectionTarget();
		if ( target )
		{
			Inject( target );
			SetWeaponState( SYRINGE_USE );
		}
	}
}

void CSyringe::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		switch ( GetWeaponState() )
		{
		case SYRINGE_IDLE:
		case SYRINGE_DRAW:
			SetWeaponState( SYRINGE_IDLE );
			break;
		case SYRINGE_USE:
			if ( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] <= 0 )
				RetireWeapon();
			else
				SetWeaponState( SYRINGE_IDLE );
#ifndef CLIENT_DLL
			m_pPlayer->ResetMaxSpeed();
#endif
			break;
		case SYRINGE_DISCARD:
		case SYRINGE_HOLSTER:
			RetireWeapon();
			break;
		default:
			ASSERT_AT( false, 260 );
			return;
		}
	}
}

void CSyringe::SetWeaponState( unsigned short state )
{
	float duration;
	float nextAttack;
	switch ( state )
	{
	case SYRINGE_IDLE:
		duration = 20;
		nextAttack = 0;
		break;
	case SYRINGE_USE:
		g_engfuncs.pfnSetClientMaxspeed( m_pPlayer->edict(), 1 );
		duration = nextAttack = 2.9f;
		break;
	case SYRINGE_DRAW:
		duration = nextAttack = 1.033f;
		break;
	case SYRINGE_DISCARD:
	case SYRINGE_HOLSTER:
		duration = nextAttack = 0.766f;
		break;
	default:
		ASSERT_AT( false, 169 );
		return;
	}
	SendWeaponAnim( state, UseDecrement() );
	m_WeaponState = state;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

BOOL CSyringe::CanDrop()
{
	return FALSE;
}

int CSyringe::iItemSlot()
{
	return 5;
}

float CSyringe::GetMaxSpeed()
{
	float speed;
	switch ( m_WeaponState )
	{
	case SYRINGE_IDLE:
	case SYRINGE_DRAW:
	case SYRINGE_DISCARD:
	case SYRINGE_HOLSTER:
		speed = 225;
		break;
	case SYRINGE_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 195 );
		speed = 0;
		break;
	}
	return speed;
}
