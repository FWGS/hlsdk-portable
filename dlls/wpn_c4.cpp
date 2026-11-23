#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"

enum c4_e
{
	C4_IDLE1,
	C4_DRAW,
	C4_DROP,
	C4_ARM
};

LINK_ENTITY_TO_CLASS( weapon_c4, CC4 );

#ifndef CLIENT_DLL
TYPEDESCRIPTION CC4::m_SaveData[] =
{
	DEFINE_FIELD( CC4, m_bStartedArming, FIELD_BOOLEAN ),
	DEFINE_FIELD( CC4, m_bBombPlacedAnimation, FIELD_BOOLEAN ),
	DEFINE_FIELD( CC4, m_fArmedTime, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CC4, CBasePlayerWeapon );
#endif

void CC4::Spawn()
{
	SET_MODEL( edict(), "models/w_backpack.mdl" );
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = 0;
	pev->framerate = 0;
	m_iDefaultAmmo = 1;
	m_bStartedArming = FALSE;
	m_iId = ACCESSORY_C4;
	m_fArmedTime = 0;
	if ( pev->targetname )
	{
		pev->effects |= EF_NODRAW;
		DROP_TO_FLOOR( edict() );
	}
	else
	{
		m_bIsAccessory = true;
		FallInit();
		SetThink( &CC4::FallThink );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.1;
	}
}

void CC4::Precache()
{
	PRECACHE_MODEL( "models/v_c4.mdl" );
	PRECACHE_MODEL( "models/w_backpack.mdl" );
	PRECACHE_SOUND( "weapons/c4_click.wav" );
}

void CC4::KeyValue( KeyValueData* data )
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
	{
		data->fHandled = FALSE;
	}
}

int CC4::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "C4";
	p->iMaxAmmo1 = 1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iId = m_iId = ACCESSORY_C4;
	p->iWeight = 3;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->bIsAccessory = true;
	return 1;
}

BOOL CC4::Deploy()
{
	pev->body = 0;
	m_bStartedArming = FALSE;
	m_fArmedTime = 0;
	return DefaultDeploy( "models/v_c4.mdl", NULL, C4_DRAW, "c4", UseDecrement() );
}

void CC4::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	m_bStartedArming = FALSE;
	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->m_iAccessoryBits &= ~( 1 << ACCESSORY_C4 );
		DestroyItem();
	}
	SendWeaponAnim( C4_DRAW, FALSE );
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "common/null.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CC4::PrimaryAttack()
{
}

void CC4::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
}

void CC4::WeaponIdle()
{
	if ( m_bStartedArming )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		SendWeaponAnim( m_bBombPlacedAnimation ? C4_DRAW : C4_IDLE1, UseDecrement() );
	}
	if ( m_flTimeWeaponIdle <= UTIL_WeaponTimeBase() )
	{
		if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		{
			RetireWeapon();
		}
		else
		{
			SendWeaponAnim( C4_DRAW, UseDecrement() );
			SendWeaponAnim( C4_IDLE1, UseDecrement() );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20;
		}
	}
}

int CC4::iItemSlot()
{
	return 5;
}

float CC4::GetMaxSpeed()
{
	return 225;
}
