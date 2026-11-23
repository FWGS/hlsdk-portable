#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_laws.cpp" )

enum laws_e
{
	LAWS_IDLE = 0,
	LAWS_DRAW,
	LAWS_FIRE,
	LAWS_HOLSTER,
};

LINK_ENTITY_TO_CLASS( weapon_laws, CLaws );
#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( laws_rocket, CLawsRocket );

CLawsRocket* CLawsRocket::CreateLawsRocket( Vector origin, Vector angles, CBaseEntity* owner, CLaws* launcher )
{
	CLawsRocket* rocket = GetClassPtr( (CLawsRocket*)NULL );
	UTIL_SetOrigin( rocket->pev, origin );
	rocket->pev->angles = angles;
	rocket->Spawn();
	rocket->SetTouch( &CLawsRocket::RocketTouch );
	rocket->m_pLauncher = launcher;
	rocket->pev->owner = owner->edict();
	return rocket;
}

void CLawsRocket::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/lawrocket.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "laws_rocket" );
	SetThink( &CLawsRocket::IgniteThink );
	SetTouch( &CGrenade::ExplodeTouch );
	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30 );
	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5f;
	pev->nextthink = gpGlobals->time;
	pev->dmg = 125;
}

void CLawsRocket::Precache()
{
	PRECACHE_MODEL( "models/lawrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CLawsRocket::RocketTouch( CBaseEntity* pOther )
{
	STOP_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav" );
	SetThink( &CGrenade::Detonate3 );
}

void CLawsRocket::IgniteThink()
{
	pev->movetype = MOVETYPE_FLY;
	pev->effects |= EF_LIGHT;
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav", 1, 0.5 );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( m_iTrail );
		WRITE_BYTE( 40 );
		WRITE_BYTE( 5 );
		WRITE_BYTE( 224 );
		WRITE_BYTE( 224 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
	m_flIgniteTime = gpGlobals->time;
	SetThink( &CLawsRocket::FollowThink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CLawsRocket::FollowThink()
{
	UTIL_MakeAimVectors( pev->angles );
	Vector target = gpGlobals->v_forward;
	pev->angles = UTIL_VecToAngles( target );
	float speed = pev->velocity.Length();
	if ( gpGlobals->time - m_flIgniteTime < 1 )
	{
		pev->velocity = pev->velocity * 0.2f + target * ( speed * 0.8 + 400 );
		if ( pev->waterlevel == 3 )
		{
			if ( pev->velocity.Length() > 300 )
				pev->velocity = pev->velocity.Normalize() * 300;
			UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 4 );
		}
		else if ( pev->velocity.Length() > 2000 )
			pev->velocity = pev->velocity.Normalize() * 2000;
	}
	else
	{
		if ( pev->effects & EF_LIGHT )
		{
			pev->effects = 0;
			STOP_SOUND( ENT( pev ), CHAN_VOICE, "weapons/rocket1.wav" );
		}
		pev->velocity = target * speed * 0.798f + pev->velocity * 0.2f;
		if ( pev->waterlevel == 0 && pev->velocity.Length() < 1500 )
			Detonate();
	}
	pev->nextthink = gpGlobals->time + 0.1;
}
#endif

void CLaws::Spawn()
{
	SET_MODEL( ENT( pev ), "models/w_law.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = LAWS_IDLE;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_laws" );
	Precache();
	m_iId = ACCESSORY_LAWS;
	m_iDefaultAmmo = 1;
	m_iClip = WEAPON_NOCLIP;
	m_bIsAccessory = TRUE;
	FallInit();
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
}

void CLaws::Precache()
{
	PRECACHE_MODEL( "models/w_law.mdl" );
	PRECACHE_MODEL( "models/v_law.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
	UTIL_PrecacheOther( "laws_rocket" );
	PRECACHE_SOUND( "weapons/rocketfire1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	m_usFireLaws = PRECACHE_EVENT( 1, "events/laws.sc" );
}

int CLaws::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "66mm";
	p->iMaxAmmo1 = 1;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 0;
	p->iSlot = 4;
	p->iPosition = 7;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = ACCESSORY_LAWS;
	p->iWeight = 2;
	p->bIsAccessory = TRUE;
	m_iPlayEmptySound = TRUE;
	return 1;
}

int CLaws::iItemSlot()
{
	return 5;
}

float CLaws::GetMaxSpeed()
{
	return 199;
}

BOOL CLaws::Deploy()
{
	BOOL result = DefaultDeploy( "models/v_law.mdl", NULL, LAWS_DRAW, "laws", UseDecrement() );
	SetWeaponState( LAWS_DRAW );
	return result;
}

void CLaws::Holster( int skiplocal )
{
	SetWeaponState( LAWS_DRAW );
}

void CLaws::SetWeaponState( const unsigned short weaponState )
{
	float nextAttack = 0;
	float duration = 0;
	switch ( weaponState )
	{
	case LAWS_IDLE:
		duration = 20;
		break;
	case LAWS_DRAW:
		nextAttack = duration = 3.266f;
		break;
	case LAWS_FIRE:
		nextAttack = duration = 1.1f;
		break;
	case LAWS_HOLSTER:
		nextAttack = duration = 1.666f;
		break;
	default:
		ASSERT_AT( false, 292 );
		break;
	}
	SendWeaponAnim( weaponState, UseDecrement() );
	m_WeaponState = weaponState;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = nextAttack;
}

void CLaws::PrimaryAttack()
{
	if ( GetWeaponState() != LAWS_IDLE )
		return;
	SetWeaponState( LAWS_FIRE );
	m_pPlayer->m_iWeaponVolume = 2250;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector forward = gpGlobals->v_forward * 16;
	Vector right = gpGlobals->v_right * 8;
	Vector up = gpGlobals->v_up * -8;
	Vector source = m_pPlayer->GetGunPosition() + forward + right + up;
	CLawsRocket* rocket = CLawsRocket::CreateLawsRocket( source, m_pPlayer->pev->v_angle, m_pPlayer, this );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	rocket->pev->velocity = rocket->pev->velocity + gpGlobals->v_forward * DotProduct( m_pPlayer->pev->velocity, gpGlobals->v_forward );
#endif
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usFireLaws, 0,
		(float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->m_rgAmmo[CBasePlayer::GetAmmoIndex( "66mm" )] = 0;
	m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_LAWS );
}

void CLaws::SecondaryAttack()
{
}

void CLaws::WeaponIdle()
{
	if ( !( m_flTimeWeaponIdle <= gpGlobals->time ) )
		return;
	switch ( GetWeaponState() )
	{
	case LAWS_IDLE:
	case LAWS_DRAW:
		SetWeaponState( LAWS_IDLE );
		break;
	case LAWS_FIRE:
		SetWeaponState( LAWS_HOLSTER );
		break;
	case LAWS_HOLSTER:
		RetireWeapon();
		DestroyItem();
		break;
	default:
		ASSERT_AT( false, 366 );
		break;
	}
}
