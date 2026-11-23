#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_camera.cpp" )

enum camera_e
{
	CAMERA_IDLE,
	CAMERA_USE,
	CAMERA_DRAW,
	CAMERA_HOLSTER
};

LINK_ENTITY_TO_CLASS( weapon_camera, CCamera );

void CCamera::Spawn()
{
	SET_MODEL( edict(), "models/w_camera.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_camera" );
	Precache();
	m_iId = ACCESSORY_CAMERA;
	m_iDefaultAmmo = -1;
	m_iClip = -1;
	m_bIsAccessory = true;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CCamera::Precache()
{
	PRECACHE_MODEL( "models/v_camera.mdl" );
	PRECACHE_MODEL( "models/w_camera.mdl" );
	PRECACHE_SOUND( "weapons/camera-1.wav" );
	PRECACHE_SOUND( "weapons/camera_clipout.wav" );
	PRECACHE_SOUND( "weapons/camera_clipin.wav" );
	PRECACHE_SOUND( "weapons/camera_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/camera_slidepull.wav" );
	m_usFireCamera = PRECACHE_EVENT( 1, "events/camera.sc" );
}

int CCamera::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 20;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iFlags = 0;
	p->iId = m_iId = ACCESSORY_CAMERA;
	p->iWeight = 5;
	p->bIsAccessory = true;
	m_iPlayEmptySound = 1;
	return 1;
}

BOOL CCamera::Deploy()
{
	return DefaultDeploy( "models/v_camera.mdl", NULL, CAMERA_DRAW, "onehanded", UseDecrement() );
}

void CCamera::Holster( int )
{
	SetWeaponState( CAMERA_DRAW );
}

void CCamera::PrimaryAttack()
{
	if ( GetWeaponState() != CAMERA_IDLE )
		return;

	if ( m_pPlayer->m_toolTrigger && m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() )
	{
		SetWeaponState( CAMERA_USE );
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
		UTIL_HudMessageAll( text, "#CAMERA_MESSAGE" );
	}
}

void CCamera::SecondaryAttack()
{
}

void CCamera::Reload()
{
}

void CCamera::WeaponIdle()
{
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		switch ( GetWeaponState() )
		{
		case CAMERA_USE:
			if ( m_pPlayer->m_toolTrigger )
				m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
			SetWeaponState( CAMERA_IDLE );
			m_pPlayer->ResetMaxSpeed();
			break;
		case CAMERA_IDLE:
		case CAMERA_DRAW:
			SetWeaponState( CAMERA_IDLE );
			break;
		case CAMERA_HOLSTER:
			RetireWeapon();
			break;
		default:
			ASSERT_AT( false, 243 );
			break;
		}
	}
}

void CCamera::SetWeaponState( unsigned short state )
{
	float duration = 0;
	float nextAttack = 0;
	switch ( state )
	{
	case CAMERA_IDLE:
		duration = 20;
		nextAttack = 0;
		break;
	case CAMERA_USE:
		g_engfuncs.pfnSetClientMaxspeed( m_pPlayer->edict(), 1 );
		duration = nextAttack = 1.794f;
		break;
	case CAMERA_DRAW:
		duration = nextAttack = 1;
		break;
	case CAMERA_HOLSTER:
		duration = nextAttack = 0.7f;
		break;
	default:
		ASSERT_AT( false, 139 );
		break;
	}
	SendWeaponAnim( state, UseDecrement() );
	m_WeaponState = state;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

int CCamera::iItemSlot()
{
	return 5;
}

float CCamera::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case CAMERA_IDLE:
	case CAMERA_DRAW:
	case CAMERA_HOLSTER:
		speed = 225;
		break;
	case CAMERA_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 164 );
		break;
	}
	return speed;
}
