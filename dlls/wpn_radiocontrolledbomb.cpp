#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "triggers.h"
#include "buildslave_assert.h"

#define ASSERT_FILE BUILDSLAVE_PATH( L"czero\\dlls\\wpn_radiocontrolledbomb.cpp" )

enum radiocontrolledbomb_e
{
	RCBOMB_IDLE = 0,
	RCBOMB_DETONATOR_IDLE,
	RCBOMB_DRAW,
	RCBOMB_DETONATOR_DRAW,
	RCBOMB_BOMB_TO_DETONATOR,
	RCBOMB_DETONATOR_TO_BOMB,
	RCBOMB_PLANT,
	RCBOMB_DETONATE,
};

LINK_ENTITY_TO_CLASS( weapon_radiocontrolledbomb, CRadioControlledBomb );
#ifndef CLIENT_DLL
LINK_ENTITY_TO_CLASS( bomb, CBomb );
TYPEDESCRIPTION CRadioControlledBomb::m_SaveData[] =
{
	DEFINE_ARRAY( CRadioControlledBomb, m_BombList, FIELD_CLASSPTR, 20 ),
};
IMPLEMENT_SAVERESTORE( CRadioControlledBomb, CBasePlayerWeapon );
#endif

void CRadioControlledBomb::ClearBombArray()
{
	memset( m_BombList, 0, sizeof( m_BombList ) );
}

BOOL CRadioControlledBomb::BombArrayIsEmpty()
{
	for ( int i = 0; i < ARRAYSIZE( m_BombList ); ++i )
	{
		if ( m_BombList[i] )
			return FALSE;
	}
	return TRUE;
}

BOOL CRadioControlledBomb::AddToBombArray( CBomb* bomb )
{
	if ( !bomb )
		return FALSE;
	for ( int i = 0; i < ARRAYSIZE( m_BombList ); ++i )
	{
		if ( !m_BombList[i] )
		{
			m_BombList[i] = bomb;
			return TRUE;
		}
	}
	return FALSE;
}

void CRadioControlledBomb::DetonateBombs()
{
#ifndef CLIENT_DLL
	for ( int i = 0; i < ARRAYSIZE( m_BombList ); ++i )
	{
		if ( m_BombList[i] )
		{
			m_BombList[i]->SetThink( &CGrenade::Detonate3 );
			m_BombList[i]->pev->nextthink = gpGlobals->time + 0.1;
		}
	}
	ClearBombArray();
#endif
}

void CRadioControlledBomb::Spawn()
{
	SET_MODEL( ENT( pev ), "models/w_rcontrolbomb.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	pev->classname = MAKE_STRING( "weapon_radiocontrolledbomb" );
	Precache();
	m_iId = ACCESSORY_RADIOCONTROLLEDBOMB;
	m_iDefaultAmmo = 5;
	if ( pev->targetname )
	{
		pev->effects |= EF_NODRAW;
		DROP_TO_FLOOR( ENT( pev ) );
	}
	else
	{
		m_bIsAccessory = TRUE;
		FallInit();
		SetThink( &CBasePlayerItem::FallThink );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	}
}

void CRadioControlledBomb::Precache()
{
	PRECACHE_MODEL( "models/v_rcontrolbomb.mdl" );
	PRECACHE_MODEL( "models/w_rcontrolbomb.mdl" );
	UTIL_PrecacheOther( "bomb" );
	PRECACHE_SOUND( "weapons/rc_detonate.wav" );
	PRECACHE_SOUND( "weapons/rc_draw_detonator.wav" );
}

int CRadioControlledBomb::GetItemInfo( ItemInfo* p )
{
	p->pszAmmo1 = "bomb";
	p->iMaxAmmo1 = 5;
	p->pszName = STRING( pev->classname );
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 6;
	p->iId = m_iId = ACCESSORY_RADIOCONTROLLEDBOMB;
	p->iWeight = 3;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = TRUE;
	m_iPlayEmptySound = TRUE;
	return 1;
}

int CRadioControlledBomb::iItemSlot()
{
	return 5;
}

float CRadioControlledBomb::GetMaxSpeed()
{
	float speed = 0;
	switch ( m_WeaponState )
	{
	case RCBOMB_IDLE:
	case RCBOMB_DETONATOR_IDLE:
	case RCBOMB_DRAW:
	case RCBOMB_DETONATOR_DRAW:
	case RCBOMB_BOMB_TO_DETONATOR:
	case RCBOMB_DETONATOR_TO_BOMB:
	case RCBOMB_DETONATE:
		speed = 225;
		break;
	case RCBOMB_PLANT:
		speed = 1;
		break;
	default:
		ASSERT_AT( false, 386 );
		break;
	}
	return speed;
}

void CRadioControlledBomb::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "detonatedelay" ) )
	{
		pev->speed = atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "detonatetarget" ) )
	{
		pev->noise1 = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "defusetarget" ) )
	{
		pev->target = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else
		data->fHandled = FALSE;
}

void CRadioControlledBomb::Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
}

BOOL CRadioControlledBomb::Deploy()
{
	if ( BombArrayIsEmpty() )
		return DefaultDeploy( "models/v_rcontrolbomb.mdl", NULL, RCBOMB_DRAW, "radiocontrolledbomb", UseDecrement() );
	SetWeaponState( RCBOMB_DETONATOR_IDLE );
	return DefaultDeploy( "models/v_rcontrolbomb.mdl", NULL, RCBOMB_DETONATOR_DRAW, "radiocontrolledbomb", UseDecrement() );
}

void CRadioControlledBomb::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 && BombArrayIsEmpty() )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_RADIOCONTROLLEDBOMB );
		DestroyItem();
	}
	SetWeaponState( RCBOMB_DRAW );
	EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM );
}

void CRadioControlledBomb::SetWeaponState( const unsigned short weaponState )
{
	float nextAttack = 0;
	float duration = 0;
	switch ( weaponState )
	{
	case RCBOMB_IDLE:
	case RCBOMB_DETONATOR_IDLE:
		duration = 20;
		break;
	case RCBOMB_DRAW:
		nextAttack = duration = 1;
		break;
	case RCBOMB_DETONATOR_DRAW:
		nextAttack = duration = 1.333f;
		break;
	case RCBOMB_BOMB_TO_DETONATOR:
		nextAttack = duration = 0.666f;
		break;
	case RCBOMB_DETONATOR_TO_BOMB:
		nextAttack = duration = 1.2f;
		break;
	case RCBOMB_PLANT:
		nextAttack = duration = 1.06f;
		break;
	case RCBOMB_DETONATE:
		if ( BombArrayIsEmpty() )
		{
			PlayEmptySound();
			return;
		}
		nextAttack = duration = 1.5f;
		break;
	default:
		ASSERT_AT( false, 357 );
		break;
	}
	SendWeaponAnim( weaponState, UseDecrement() );
	m_WeaponState = weaponState;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + duration;
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + nextAttack;
}

void CRadioControlledBomb::PrimaryAttack()
{
	unsigned short state = GetWeaponState();
	if ( state == RCBOMB_DETONATOR_IDLE )
	{
		SetWeaponState( RCBOMB_DETONATE );
		return;
	}
	if ( state != RCBOMB_IDLE )
		return;
	if ( m_pPlayer && m_pPlayer->m_toolTrigger &&
		m_pPlayer->m_toolTrigger->IsPlayerInPositionUsingProperTool() && !m_pPlayer->m_toolTrigger->m_bombPlanted )
	{
		SetWeaponState( RCBOMB_PLANT );
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
	UTIL_HudMessageAll( text, "#RCBOMB_MESSAGE" );
}

void CRadioControlledBomb::SecondaryAttack()
{
	unsigned short state = GetWeaponState();
	if ( state == RCBOMB_IDLE )
		SetWeaponState( RCBOMB_BOMB_TO_DETONATOR );
	else if ( state == RCBOMB_DETONATOR_IDLE && m_pPlayer->m_rgAmmo[CBasePlayer::GetAmmoIndex( "bomb" )] > 0 )
		SetWeaponState( RCBOMB_DETONATOR_TO_BOMB );
}

BOOL CRadioControlledBomb::PlantBomb()
{
#ifndef CLIENT_DLL
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector forward = gpGlobals->v_forward * 16;
	Vector right = gpGlobals->v_right * 8;
	Vector up = gpGlobals->v_up * -8;
	Vector source = m_pPlayer->GetGunPosition() + forward + right + up;
	CBomb* bomb = CBomb::CreateBomb( source, m_pPlayer->pev->v_angle, m_pPlayer );
	if ( !bomb )
		return FALSE;
	AddToBombArray( bomb );
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	--m_pPlayer->m_rgAmmo[CBasePlayer::GetAmmoIndex( "bomb" )];
#endif
	if ( m_pPlayer->m_toolTrigger )
		m_pPlayer->m_toolTrigger->FireUseTarget( m_pPlayer, NULL, USE_SET, 0 );
	return TRUE;
}

void CRadioControlledBomb::WeaponIdle()
{
	if ( !( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() ) )
		return;
	switch ( GetWeaponState() )
	{
	case RCBOMB_IDLE:
	case RCBOMB_DRAW:
		SetWeaponState( RCBOMB_IDLE );
		break;
	case RCBOMB_DETONATOR_IDLE:
	case RCBOMB_DETONATOR_DRAW:
		SetWeaponState( RCBOMB_DETONATOR_IDLE );
		break;
	case RCBOMB_BOMB_TO_DETONATOR:
		SetWeaponState( RCBOMB_DETONATOR_DRAW );
		break;
	case RCBOMB_DETONATOR_TO_BOMB:
		if ( m_pPlayer->m_rgAmmo[CBasePlayer::GetAmmoIndex( "bomb" )] <= 0 )
			RetireWeapon();
		else
			SetWeaponState( RCBOMB_DRAW );
		break;
	case RCBOMB_PLANT:
		SetWeaponState( PlantBomb() ? RCBOMB_DETONATOR_DRAW : RCBOMB_IDLE );
		break;
	case RCBOMB_DETONATE:
		DetonateBombs();
		SetWeaponState( RCBOMB_DETONATOR_TO_BOMB );
		break;
	}
}

#ifndef CLIENT_DLL
CBomb* CBomb::CreateBomb( Vector origin, Vector angles, CBaseEntity* owner )
{
	CBomb* bomb = GetClassPtr( (CBomb*)NULL );
	BOOL player = owner->IsPlayer();
	if ( !owner || !player || !( (CBasePlayer*)owner )->m_toolTrigger )
		return NULL;
	CBaseEntity* target = ( (CBasePlayer*)owner )->m_toolTrigger->GetRCBombTarget();
	if ( target )
	{
		bomb->Spawn();
		bomb->SetTouch( NULL );
		bomb->pev->owner = owner->edict();
		bomb->pev->movetype = MOVETYPE_FLY;
		bomb->pev->angles = target->pev->angles;
		bomb->pev->origin = target->pev->origin;
		target->SUB_Remove();
	}
	else
	{
		UTIL_SetOrigin( bomb->pev, origin );
		bomb->pev->angles = angles;
		bomb->Spawn();
		bomb->SetTouch( &CBomb::BombTouch );
		bomb->pev->owner = owner->edict();
	}
	return bomb;
}

void CBomb::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/w_rcontrolbomb_bomb.mdl" );
	UTIL_SetSize( pev, Vector( -6, -6, 0 ), Vector( 6, 6, 4 ) );
	pev->classname = MAKE_STRING( "bomb" );
	SetThink( &CBomb::BombThink );
	SetTouch( &CBomb::BombTouch );
	UTIL_MakeVectors( pev->angles );
	pev->origin.x += gpGlobals->v_forward.x * 4.0;
	pev->origin.y += gpGlobals->v_forward.y * 4.0;
	pev->origin.z += gpGlobals->v_forward.z * 4.0;
	pev->origin.z -= 16.0;
	UTIL_SetOrigin( pev, pev->origin );
	pev->angles = g_vecZero;
	pev->gravity = 1;
	pev->nextthink = gpGlobals->time;
	pev->dmg = 100;
}

void CBomb::Precache()
{
	PRECACHE_MODEL( "models/w_rcontrolbomb_bomb.mdl" );
}

void CBomb::BombThink()
{
}

void CBomb::BombTouch( CBaseEntity* other )
{
	BounceSound();
}
#endif
