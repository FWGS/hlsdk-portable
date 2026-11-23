#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_briefcase.cpp" )

enum briefcase_e
{
	BRIEFCASE_IDLE = 0,
	BRIEFCASE_DRAW,
	BRIEFCASE_HOLSTER,
	BRIEFCASE_USE,
};

LINK_ENTITY_TO_CLASS( weapon_briefcase, CBriefcase );

void CBriefcase::Spawn()
{
	SET_MODEL( ENT( pev ), "models/w_briefcase.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_briefcase" );
	Precache();
	m_iId = ACCESSORY_BRIEFCASE;
	m_iDefaultAmmo = -1;
	m_iClip = WEAPON_NOCLIP;
	m_bIsAccessory = TRUE;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CBriefcase::Precache()
{
	PRECACHE_MODEL( "models/v_briefcase.mdl" );
	PRECACHE_MODEL( "models/w_briefcase.mdl" );
	PRECACHE_SOUND( "weapons/briefcase_use.wav" );
	PRECACHE_SOUND( "weapons/briefcase_deploy.wav" );
	m_usFireBriefcase = PRECACHE_EVENT( 1, "events/briefcase.sc" );
}

int CBriefcase::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 8;
	p->iId = m_iId = ACCESSORY_BRIEFCASE;
	p->iWeight = 5;
	p->iFlags = 0;
	p->bIsAccessory = TRUE;
	m_iPlayEmptySound = TRUE;
	return 1;
}

int CBriefcase::iItemSlot()
{
	return 5;
}

float CBriefcase::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case BRIEFCASE_IDLE:
	case BRIEFCASE_DRAW:
	case BRIEFCASE_HOLSTER:
		speed = 225;
		break;
	case BRIEFCASE_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 170 );
		break;
	}
	return speed;
}

BOOL CBriefcase::Deploy()
{
	int incognito = m_pPlayer->m_iIncognito;
	BOOL result = DefaultDeploy( "models/v_briefcase.mdl", NULL, BRIEFCASE_DRAW, "onehanded", UseDecrement() );
	m_pPlayer->m_iIncognito = incognito;
	return result;
}

void CBriefcase::Holster( int skiplocal )
{
	SetWeaponState( BRIEFCASE_DRAW );
}

void CBriefcase::SetWeaponState( const unsigned short weaponState )
{
	float nextAttack = 0;
	float duration = 0;
	switch ( weaponState )
	{
	case BRIEFCASE_IDLE:
		duration = 20;
		break;
	case BRIEFCASE_DRAW:
		nextAttack = duration = 0.833f;
		break;
	case BRIEFCASE_HOLSTER:
		nextAttack = duration = 0.533f;
		break;
	case BRIEFCASE_USE:
		{
			int z = (int)m_pPlayer->GetGunPosition().z;
			int y = (int)m_pPlayer->GetGunPosition().y;
			int x = (int)m_pPlayer->GetGunPosition().x;
			PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireBriefcase, 0,
				(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, x, y, z, 0 );
			g_engfuncs.pfnSetClientMaxspeed( m_pPlayer->edict(), 1 );
			nextAttack = duration = 5.733f;
		}
		break;
	default:
		ASSERT_AT( false, 145 );
		break;
	}
	SendWeaponAnim( weaponState, UseDecrement() );
	m_WeaponState = weaponState;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

void CBriefcase::PrimaryAttack()
{
	if ( GetWeaponState() != BRIEFCASE_IDLE )
		return;
	if ( m_pPlayer->m_toolTrigger && m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() )
	{
		SetWeaponState( BRIEFCASE_USE );
		return;
	}

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
	UTIL_HudMessageAll( text, "#BRIEFCASE_MESSAGE" );
}

void CBriefcase::SecondaryAttack()
{
}

void CBriefcase::Reload()
{
}

void CBriefcase::WeaponIdle()
{
	switch ( GetWeaponState() )
	{
	case BRIEFCASE_IDLE:
	case BRIEFCASE_DRAW:
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
			SetWeaponState( BRIEFCASE_IDLE );
		break;
	case BRIEFCASE_HOLSTER:
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
			RetireWeapon();
		break;
	case BRIEFCASE_USE:
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			if ( m_pPlayer->m_toolTrigger )
				m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
			RetireWeapon();
			m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_BRIEFCASE );
			DestroyItem();
			m_pPlayer->ResetMaxSpeed();
		}
		break;
	default:
		ASSERT_AT( false, 258 );
		break;
	}
}
