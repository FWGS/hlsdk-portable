#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_blowtorch.cpp" )

enum blowtorch_e
{
	BLOWTORCH_IDLE = 0,
	BLOWTORCH_USE,
	BLOWTORCH_DRAW,
	BLOWTORCH_RELOAD,
	BLOWTORCH_HOLSTER,
};

LINK_ENTITY_TO_CLASS( weapon_blowtorch, CBlowtorch );

void CBlowtorch::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_blowtorch" );
	Precache();
	m_iId = ACCESSORY_BLOWTORCH;
	SET_MODEL( ENT( pev ), "models/w_btorch.mdl" );
	m_iClip = WEAPON_NOCLIP;
	m_bIsAccessory = TRUE;
	m_iDefaultAmmo = -1;
	m_playFireSound = TRUE;
	FallInit();
}

void CBlowtorch::Precache()
{
	PRECACHE_MODEL( "models/v_btorch.mdl" );
	PRECACHE_MODEL( "models/w_btorch.mdl" );
	PRECACHE_MODEL( "sprites/fire.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "weapons/blowtorch-1.wav" );
	m_usFireBlowtorch = PRECACHE_EVENT( 1, "events/blowtorch.sc" );
	m_usIdleBlowtorch = PRECACHE_EVENT( 1, "events/blowtorchidle.sc" );
	m_usHolsterBlowtorch = PRECACHE_EVENT( 1, "events/blowtorchholster.sc" );
}

int CBlowtorch::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 5;
	p->iFlags = 0;
	p->iId = m_iId = ACCESSORY_BLOWTORCH;
	p->iWeight = 5;
	p->bIsAccessory = TRUE;
	m_iPlayEmptySound = TRUE;
	return 1;
}

int CBlowtorch::iItemSlot()
{
	return 5;
}

float CBlowtorch::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case BLOWTORCH_IDLE:
	case BLOWTORCH_DRAW:
	case BLOWTORCH_RELOAD:
	case BLOWTORCH_HOLSTER:
		speed = 225;
		break;
	case BLOWTORCH_USE:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 203 );
		break;
	}
	return speed;
}

BOOL CBlowtorch::Deploy()
{
	return DefaultDeploy( "models/v_btorch.mdl", NULL, BLOWTORCH_DRAW, "onehanded", UseDecrement() );
}

void CBlowtorch::Holster( int skiplocal )
{
	int y = (int)( m_pPlayer->pev->punchangle.y * 100 );
	int x = (int)( m_pPlayer->pev->punchangle.x * 100 );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usHolsterBlowtorch, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, x, y, 0, 0 );
}

void CBlowtorch::Restart()
{
	m_playFireSound = TRUE;
}

void CBlowtorch::SecondaryAttack()
{
}

void CBlowtorch::Reload()
{
}

void CBlowtorch::PrimaryAttack()
{
	if ( GetWeaponState() != BLOWTORCH_IDLE && GetWeaponState() != BLOWTORCH_USE )
		return;
	if ( m_pPlayer->m_toolTrigger && m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() )
	{
		SetWeaponState( BLOWTORCH_USE );
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
	UTIL_HudMessageAll( text, "#BLOWTORCH_MESSAGE" );
}

Vector CBlowtorch::Use()
{
	++m_iShotsFired;
	m_bDelayFire = true;
	m_flLastFire = gpGlobals->time;
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector aiming = gpGlobals->v_forward;
	Vector source = m_pPlayer->GetGunPosition();
	m_bDelayFire = false;
	if ( m_playFireSound )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/blowtorch-1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		m_playFireSound = FALSE;
	}
	return m_pPlayer->FireBullets3( source, aiming, 0, 64, 2, BULLET_PLAYER_BLOWTORCH,
		2, 0.98f, m_pPlayer->pev, FALSE, m_pPlayer->random_seed );
}

void CBlowtorch::SetWeaponState( const unsigned short weaponState )
{
	float nextAttack = 0;
	float duration = 0;
	switch ( weaponState )
	{
	case BLOWTORCH_IDLE:
		m_playFireSound = TRUE;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/blowtorch-1.wav", 0, 0, SND_STOP, PITCH_NORM );
		duration = 20;
		break;
	case BLOWTORCH_USE:
		{
			m_pPlayer->m_iWeaponFlash = DIM_GUN_FLASH;
			Vector direction = Use();
			int y = (int)( m_pPlayer->pev->punchangle.y * 100 );
			int x = (int)( m_pPlayer->pev->punchangle.x * 100 );
			PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireBlowtorch, 0,
				(float*)&g_vecZero, (float*)&g_vecZero, direction.x, direction.y, x, y, 0, 0 );
			g_engfuncs.pfnSetClientMaxspeed( m_pPlayer->edict(), 1 );
			nextAttack = 0.066f;
			duration = 0.466f;
		}
		break;
	case BLOWTORCH_DRAW:
		nextAttack = duration = 1;
		break;
	case BLOWTORCH_RELOAD:
		nextAttack = duration = 4.7f;
		break;
	case BLOWTORCH_HOLSTER:
		nextAttack = duration = 1.066f;
		break;
	default:
		ASSERT_AT( false, 176 );
		break;
	}
	SendWeaponAnim( weaponState, UseDecrement() );
	m_WeaponState = weaponState;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

void CBlowtorch::WeaponIdle()
{
	int y = (int)( m_pPlayer->pev->punchangle.y * 100 );
	int x = (int)( m_pPlayer->pev->punchangle.x * 100 );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usIdleBlowtorch, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, x, y, 0, 0 );
	if ( !( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() ) )
		return;
	switch ( GetWeaponState() )
	{
	case BLOWTORCH_IDLE:
	case BLOWTORCH_DRAW:
	case BLOWTORCH_RELOAD:
		SetWeaponState( BLOWTORCH_IDLE );
		break;
	case BLOWTORCH_USE:
		SetWeaponState( BLOWTORCH_IDLE );
		m_pPlayer->ResetMaxSpeed();
		break;
	case BLOWTORCH_HOLSTER:
		RetireWeapon();
		break;
	default:
		ASSERT_AT( false, 295 );
		break;
	}
}
