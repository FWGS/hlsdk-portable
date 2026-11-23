#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_fiberopticcamera.cpp" )

enum fiberopticcamera_e
{
	FIBER_OPTIC_CAMERA_IDLE,
	FIBER_OPTIC_CAMERA_DRAW,
	FIBER_OPTIC_CAMERA_USE,
	FIBER_OPTIC_CAMERA_IN_USE
};

LINK_ENTITY_TO_CLASS( weapon_fiberopticcamera, CFiberOpticCamera );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CFiberOpticCamera::m_SaveData[] =
{
	DEFINE_FIELD( CFiberOpticCamera, m_bIsZoomed, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CFiberOpticCamera, CBasePlayerWeapon );
#endif

void CFiberOpticCamera::Spawn()
{
	SET_MODEL( edict(), "models/w_focamera.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_fiberopticcamera" );
	Precache();
	m_iId = ACCESSORY_FIBEROPTICCAMERA;
	m_iDefaultAmmo = -1;
	m_iClip = -1;
	m_bIsAccessory = true;
	FallInit();
}

void CFiberOpticCamera::Precache()
{
	PRECACHE_MODEL( "models/p_focamera.mdl" );
	PRECACHE_MODEL( "models/v_focamera.mdl" );
	PRECACHE_MODEL( "models/v_nothing.mdl" );
	PRECACHE_MODEL( "models/w_focamera.mdl" );
	PRECACHE_SOUND( "weapons/camera-1.wav" );
	PRECACHE_SOUND( "weapons/camera_clipout.wav" );
	PRECACHE_SOUND( "weapons/camera_clipin.wav" );
	PRECACHE_SOUND( "weapons/camera_sliderelease.wav" );
	PRECACHE_SOUND( "weapons/camera_slidepull.wav" );
	m_usFireFiberOpticCamera = PRECACHE_EVENT( 1, "events/fiberopticcamera.sc" );
}

int CFiberOpticCamera::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 4;
	p->iPosition = 12;
	p->iId = m_iId = ACCESSORY_FIBEROPTICCAMERA;
	p->iWeight = 5;
	p->iFlags = 0;
	p->bIsAccessory = true;
	m_iPlayEmptySound = 1;
	return 1;
}

BOOL CFiberOpticCamera::Deploy()
{
	return DefaultDeploy( "models/v_focamera.mdl", "models/p_focamera.mdl", FIBER_OPTIC_CAMERA_DRAW,
		"onehanded", UseDecrement() );
}

void CFiberOpticCamera::Holster( int )
{
	m_pPlayer->m_iFOV = 90;
	if ( GetWeaponState() == FIBER_OPTIC_CAMERA_IN_USE && m_pPlayer->m_toolTrigger )
		m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
	m_pPlayer->pev->iuser3 = 0;
	SetWeaponState( FIBER_OPTIC_CAMERA_DRAW );
}

void CFiberOpticCamera::PrimaryAttack()
{
	switch ( GetWeaponState() )
	{
	case FIBER_OPTIC_CAMERA_IN_USE:
		if ( m_pPlayer->m_toolTrigger )
			m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
		SetWeaponState( FIBER_OPTIC_CAMERA_IDLE );
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		break;
	case FIBER_OPTIC_CAMERA_IDLE:
		if ( m_pPlayer->m_toolTrigger && m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() )
		{
			SetWeaponState( FIBER_OPTIC_CAMERA_USE );
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
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
			UTIL_HudMessageAll( text, "#FIBERCAMERA_MESSAGE" );
		}
		break;
	}
}

void CFiberOpticCamera::SecondaryAttack()
{
	if ( GetWeaponState() != FIBER_OPTIC_CAMERA_IN_USE )
		return;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.2;
	if ( m_bIsZoomed )
	{
		m_bIsZoomed = FALSE;
		m_pPlayer->m_iFOV = 120;
	}
	else
	{
		m_bIsZoomed = TRUE;
		m_pPlayer->m_iFOV = 60;
	}
}

void CFiberOpticCamera::Reload()
{
}

void CFiberOpticCamera::WeaponIdle()
{
	switch ( GetWeaponState() )
	{
	case FIBER_OPTIC_CAMERA_IDLE:
	case FIBER_OPTIC_CAMERA_DRAW:
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
			SetWeaponState( FIBER_OPTIC_CAMERA_IDLE );
		break;
	case FIBER_OPTIC_CAMERA_USE:
		if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
		{
			m_pPlayer->pev->iuser3 = 1;
			m_pPlayer->pev->flags |= FL_DUCKING;
			SetWeaponState( FIBER_OPTIC_CAMERA_IN_USE );
		}
		break;
	case FIBER_OPTIC_CAMERA_IN_USE:
		break;
	default:
		ASSERT_AT( false, 293 );
		break;
	}
}

void CFiberOpticCamera::SetWeaponState( unsigned short state )
{
	float duration = 0;
	float nextAttack = 0;
	switch ( state )
	{
	case FIBER_OPTIC_CAMERA_IDLE:
		m_pPlayer->m_iFOV = 90;
		m_pPlayer->ResetMaxSpeed();
		m_pPlayer->pev->iuser3 = 0;
		DefaultDeploy( "models/v_focamera.mdl", "models/p_focamera.mdl", FIBER_OPTIC_CAMERA_DRAW,
			"onehanded", UseDecrement() );
		duration = 20;
		nextAttack = 0;
		break;
	case FIBER_OPTIC_CAMERA_DRAW:
		duration = nextAttack = 1;
		break;
	case FIBER_OPTIC_CAMERA_USE:
		m_pPlayer->SetMaxSpeed( 1 );
		duration = nextAttack = 1.166f;
		break;
	case FIBER_OPTIC_CAMERA_IN_USE:
		m_bIsZoomed = FALSE;
		if ( m_pPlayer->m_toolTrigger )
		{
			DefaultDeploy( "models/v_nothing.mdl", "models/v_nothing.mdl", 0, "onehanded", UseDecrement() );
			m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
		}
		m_pPlayer->m_iFOV = 120;
		duration = 20;
		nextAttack = 0.1f;
		break;
	default:
		ASSERT_AT( false, 166 );
		break;
	}
	SendWeaponAnim( state, UseDecrement() );
	m_WeaponState = state;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = nextAttack;
}

int CFiberOpticCamera::iItemSlot()
{
	return 5;
}

float CFiberOpticCamera::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case FIBER_OPTIC_CAMERA_IDLE:
	case FIBER_OPTIC_CAMERA_DRAW:
		speed = 225;
		break;
	case FIBER_OPTIC_CAMERA_USE:
	case FIBER_OPTIC_CAMERA_IN_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 191 );
		break;
	}
	return speed;
}
