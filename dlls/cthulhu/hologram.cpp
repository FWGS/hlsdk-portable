/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"

enum hologram_e {
	HOLOGRAM_IDLE1 = 0,
	HOLOGRAM_FIDGET1,
	HOLOGRAM_DRAW,
	HOLOGRAM_DROP
};

enum hologram_radio_e {
	HOLOGRAM_RADIO_IDLE1 = 0,
	HOLOGRAM_RADIO_FIDGET1,
	HOLOGRAM_RADIO_DRAW,
	HOLOGRAM_RADIO_FIRE,
	HOLOGRAM_RADIO_HOLSTER
};



class CHologramCharge : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	void BounceSound( void );

	void EXPORT HologramSlide( CBaseEntity *pOther );
	void EXPORT HologramThink( void );

	CBaseEntity*	m_pHologram;
	float			m_fDeactivateTime;

public:

#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Deactivate( void );
	void MakeHologram( void );
};

TYPEDESCRIPTION	CHologramCharge::m_SaveData[] = 
{
	DEFINE_FIELD( CHologramCharge, m_pHologram, FIELD_CLASSPTR ),
	DEFINE_FIELD( CHologramCharge, m_fDeactivateTime, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CHologramCharge, CGrenade );

LINK_ENTITY_TO_CLASS( monster_hologram, CHologramCharge );

void CHologramCharge::MakeHologram( void )
{
	// create a hologram (monster_furniture)
	edict_t* pent = CREATE_NAMED_ENTITY( ALLOC_STRING("monster_furniture") );

	if ( FNullEnt( pent ) )
	{
		ALERT ( at_debug, "NULL Ent in Hologram!\n" );
		return;
	}

	entvars_t* pevCreate = VARS( pent );
	pevCreate->origin = Center() + Vector(0,0,40);
	pevCreate->angles = pev->angles;
	pevCreate->model = ALLOC_STRING("models/player.mdl");

	pevCreate->renderfx = 16;	// hologram
	
	//pevCreate->rendermode = 4; // solid
	//pevCreate->renderamt = 255;

	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	DispatchSpawn( ENT( pevCreate ) );
	
	m_pHologram = CBaseEntity::Instance( pevCreate );

	SetBits(m_pHologram->pev->flags, FL_ONGROUND);
	m_pHologram->DontThink();

	((CBaseMonster*)m_pHologram)->m_iClass = CLASS_PLAYER;

	// set the destruct time
	m_fDeactivateTime = UTIL_WeaponTimeBase() + 60.0;
}

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// Hologram when we don't want it in the world anymore.
//=========================================================
void CHologramCharge::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	if (m_pHologram) UTIL_Remove(m_pHologram);
	UTIL_Remove( this );
}

void CHologramCharge :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_satchel.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, -4), Vector(16, 16, 4));
	UTIL_SetOrigin( this, pev->origin );

	SetTouch(&CHologramCharge :: HologramSlide );
	SetUse(&CHologramCharge :: DetonateUse );
	SetThink(&CHologramCharge :: HologramThink );
	SetNextThink( 0.1 );

	pev->gravity = 0.5;
	pev->friction = 0.8;

	// hardcoded small damage
	pev->dmg = 10;

	// ResetSequenceInfo( );
	pev->sequence = 1;

	m_pHologram = NULL;
	m_fDeactivateTime = 0.0;
}


void CHologramCharge::HologramSlide( CBaseEntity *pOther )
{
	entvars_t	*pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if ( pOther->edict() == pev->owner )
		return;

	// pev->avelocity = Vector (300, 300, 300);
	pev->gravity = 1;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr );

	if ( tr.flFraction < 1.0 )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.95;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if ( !(pev->flags & FL_ONGROUND) && pev->velocity.Length2D() > 10 )
	{
		BounceSound();
	}
	StudioFrameAdvance( );
}


void CHologramCharge :: HologramThink( void )
{
	StudioFrameAdvance( );
	SetNextThink( 0.1 );

	if (m_pHologram && gpGlobals->time >= m_fDeactivateTime)
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 2  ); 
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, SMALL_EXPLOSION_VOLUME, 3.0 );

		Deactivate();
	}

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	if (pev->waterlevel == 3 && pev->watertype != CONTENT_FOG)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel == 0 || pev->watertype == CONTENT_FOG)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8;
	}	
}

void CHologramCharge :: Precache( void )
{
	PRECACHE_MODEL("models/grenade.mdl");
	PRECACHE_SOUND("weapons/g_bounce1.wav");
	PRECACHE_SOUND("weapons/g_bounce2.wav");
	PRECACHE_SOUND("weapons/g_bounce3.wav");
}

void CHologramCharge :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/g_bounce3.wav", 1, ATTN_NORM);	break;
	}
}


class CHologram : public CBasePlayerWeapon
{
public:

#ifndef CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif


	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate( CBasePlayerItem *pOriginal );
	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	void Throw( void );
	
	virtual BOOL UseDecrement( void )
	{ 
#if defined( CLIENT_WEAPONS )
		return TRUE;
#else
		return FALSE;
#endif
	}
};

TYPEDESCRIPTION	CHologram::m_SaveData[] = 
{
	DEFINE_FIELD( CHologram, m_chargeReady, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CHologram, CBasePlayerWeapon );

LINK_ENTITY_TO_CLASS( weapon_hologram, CHologram );


//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CHologram::AddDuplicate( CBasePlayerItem *pOriginal )
{
	CHologram *pHologram;

#ifdef CLIENT_DLL
	if ( bIsMultiplayer() )
#else
	if ( g_pGameRules->IsMultiplayer() )
#endif
	{
		pHologram = (CHologram *)pOriginal;

		if ( pHologram->m_chargeReady != 0 )
		{
			// player has some Holograms deployed. Refuse to add more.
			return FALSE;
		}
	}

	return CBasePlayerWeapon::AddDuplicate ( pOriginal );
}

//=========================================================
//=========================================================
int CHologram::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	pPlayer->pev->weapons |= (1<<m_iId);
	m_chargeReady = 0;// this Hologram charge weapon now forgets that any Holograms are deployed by it.

	if ( bResult )
	{
		return AddWeapon( );
	}
	return FALSE;
}

void CHologram::Spawn( )
{
	Precache( );
	m_iId = WEAPON_HOLOGRAM;
	SET_MODEL(ENT(pev), "models/w_satchel.mdl");

	m_iDefaultAmmo = HOLOGRAM_DEFAULT_GIVE;
		
	FallInit();// get ready to fall down.
}


void CHologram::Precache( void )
{
	PRECACHE_MODEL("models/v_satchel.mdl");
	PRECACHE_MODEL("models/v_satchel_radio.mdl");
	PRECACHE_MODEL("models/w_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");

	UTIL_PrecacheOther( "monster_hologram" );
}


int CHologram::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Hologram Charge";
	p->iMaxAmmo1 = HOLOGRAM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 4;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_HOLOGRAM;
	p->iWeight = HOLOGRAM_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CHologram::IsUseable( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some Holograms
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any Holograms, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CHologram::CanDeploy( void )
{
	if ( m_pPlayer->m_rgAmmo[ PrimaryAmmoIndex() ] > 0 ) 
	{
		// player is carrying some Holograms
		return TRUE;
	}

	if ( m_chargeReady != 0 )
	{
		// player isn't carrying any Holograms, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CHologram::Deploy( )
{

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if ( m_chargeReady )
		return DefaultDeploy( "models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl", HOLOGRAM_RADIO_DRAW, "hive" );
	else
		return DefaultDeploy( "models/v_satchel.mdl", "models/p_satchel.mdl", HOLOGRAM_DRAW, "trip" );

	
	return TRUE;
}


void CHologram::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	
	if ( m_chargeReady )
	{
		SendWeaponAnim( HOLOGRAM_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( HOLOGRAM_DROP );
	}
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);

	if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && !m_chargeReady )
	{
		m_pPlayer->pev->weapons &= ~(1<<WEAPON_HOLOGRAM);
		SetThink(&CHologram:: DestroyItem );
		SetNextThink( 0.1 );
	}
}

void CHologram::PrimaryAttack()
{
	switch (m_chargeReady)
	{
	case 0:
		{
		Throw( );
		}
		break;
	case 1:
		{
		SendWeaponAnim( HOLOGRAM_RADIO_FIRE );

		edict_t *pPlayer = m_pPlayer->edict( );

		CBaseEntity *pHologram = NULL;

		while ((pHologram = UTIL_FindEntityInSphere( pHologram, m_pPlayer->pev->origin, 4096 )) != NULL)
		{
			if (FClassnameIs( pHologram->pev, "monster_hologram"))
			{
				if (pHologram->pev->owner == pPlayer)
				{
					((CHologramCharge*)pHologram)->MakeHologram();
					m_chargeReady = 2;
				}
			}
		}

		m_chargeReady = 2;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;
		}
		break;
	case 2:
		// we're reloading, don't allow fire
		{
		}
		break;
	}
}


void CHologram::SecondaryAttack( void )
{
	if ( m_chargeReady != 2 )
	{
		Throw( );
	}
}


void CHologram::Throw( void )
{
	if ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
		CBaseEntity *pHologram = Create( "monster_hologram", vecSrc, Vector( 0, 0, 0), m_pPlayer->edict() );
		pHologram->pev->velocity = vecThrow;
		pHologram->pev->avelocity.y = 400;

		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel_radio.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel_radio.mdl");
#else
		LoadVModel ( "models/v_satchel_radio.mdl", m_pPlayer );
#endif

		SendWeaponAnim( HOLOGRAM_RADIO_DRAW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_chargeReady = 1;
		
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	}
}


void CHologram::WeaponIdle( void )
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case 0:
		SendWeaponAnim( HOLOGRAM_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case 1:
		SendWeaponAnim( HOLOGRAM_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case 2:
		if ( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING("models/v_satchel.mdl");
		m_pPlayer->pev->weaponmodel = MAKE_STRING("models/p_satchel.mdl");
#else
		LoadVModel ( "models/v_satchel.mdl", m_pPlayer );
#endif

		SendWeaponAnim( HOLOGRAM_DRAW );

		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
		m_chargeReady = 0;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

//=========================================================
// DeactivateHolograms - removes all Holograms owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivateHolograms( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_hologram" );

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CHologramCharge *pHologram = (CHologramCharge *)pEnt;

		if ( pHologram )
		{
			if ( pHologram->pev->owner == pOwner->edict() )
			{
				pHologram->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_hologram" );
	}
}

#endif
