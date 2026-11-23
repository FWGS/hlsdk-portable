#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_radio.cpp" )

enum radio_e
{
	RADIO_IDLE,
	RADIO_DRAW,
	RADIO_HOLSTER,
	RADIO_USE,
	RADIO_USE_START,
	RADIO_USE_LOOP,
	RADIO_USE_END
};

LINK_ENTITY_TO_CLASS( weapon_radio, CRadio );

void CRadio::Spawn()
{
	SET_MODEL( edict(), "models/w_radio.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_radio" );
	Precache();
	m_iId = ACCESSORY_RADIO;
	m_iDefaultAmmo = -1;
	m_iClip = -1;
	m_bIsAccessory = true;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CRadio::Precache()
{
	PRECACHE_MODEL( "models/v_radio.mdl" );
	PRECACHE_MODEL( "models/w_radio.mdl" );
	PRECACHE_SOUND( "weapons/radio_activate.wav" );
	PRECACHE_SOUND( "weapons/radio_deploy.wav" );
	m_usFireRadio = PRECACHE_EVENT( 1, "events/radio.sc" );
}

int CRadio::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 4;
	p->iPosition = 11;
	p->iId = m_iId = ACCESSORY_RADIO;
	p->iWeight = 5;
	p->iFlags = 0;
	p->bIsAccessory = true;
	m_iPlayEmptySound = 1;
	return 1;
}

BOOL CRadio::Deploy()
{
	return DefaultDeploy( "models/v_radio.mdl", NULL, RADIO_DRAW, "onehanded", UseDecrement() );
}

void CRadio::Holster( int )
{
	SetWeaponState( RADIO_DRAW );
}

void CRadio::PrimaryAttack()
{
	if ( GetWeaponState() != RADIO_IDLE )
		return;

	if ( m_pPlayer->m_toolTrigger && m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() )
	{
		SetWeaponState( RADIO_USE );
		if ( m_pPlayer && m_pPlayer->m_toolTrigger )
			m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
	}
	else
	{
		hudtextparms_t text;
		text.x = -1;
		text.y = 0.01f;
		text.effect = 0;
		text.r1 = 0;
		text.g1 = 0;
		text.b1 = 255;
		text.a1 = 255;
		text.r2 = 255;
		text.g2 = 255;
		text.b2 = 255;
		text.a2 = 255;
		text.fadeinTime = 0.25f;
		text.fadeoutTime = 0.25f;
		text.holdTime = 2;
		text.fxTime = 0;
		text.channel = 0;
		UTIL_HudMessageAll( text, "#RADIO_MESSAGE" );
	}
}

void CRadio::SecondaryAttack()
{
}

void CRadio::Reload()
{
}

void CRadio::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		switch ( GetWeaponState() )
		{
		case RADIO_IDLE:
		case RADIO_DRAW:
		case RADIO_USE:
			SetWeaponState( RADIO_IDLE );
			break;
		case RADIO_HOLSTER:
			RetireWeapon();
			break;
		default:
			ASSERT_AT( false, 254 );
			break;
		}
	}
}

void CRadio::SetWeaponState( unsigned short state )
{
	float duration = 0;
	float nextAttack = 0;
	switch ( state )
	{
	case RADIO_IDLE:
		duration = 20;
		nextAttack = 0;
		break;
	case RADIO_DRAW:
	case RADIO_USE_END:
		duration = nextAttack = 1;
		break;
	case RADIO_HOLSTER:
		duration = nextAttack = 1.266f;
		break;
	case RADIO_USE:
		duration = nextAttack = 2.733f;
		break;
	case RADIO_USE_START:
		duration = nextAttack = 1.166f;
		break;
	case RADIO_USE_LOOP:
		duration = nextAttack = 1.933f;
		break;
	default:
		ASSERT_AT( false, 150 );
		break;
	}
	SendWeaponAnim( state, UseDecrement() );
	m_WeaponState = state;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

int CRadio::iItemSlot()
{
	return 5;
}

float CRadio::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case RADIO_IDLE:
	case RADIO_DRAW:
	case RADIO_HOLSTER:
		speed = 225;
		break;
	case RADIO_USE:
	case RADIO_USE_START:
	case RADIO_USE_LOOP:
	case RADIO_USE_END:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 178 );
		break;
	}
	return speed;
}
