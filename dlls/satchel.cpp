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

enum satchel_state
{
	SATCHEL_IDLE = 0,
	SATCHEL_READY,
	SATCHEL_RELOAD
};

enum satchel_e
{
	SATCHEL_IDLE1 = 0,
	SATCHEL_FIDGET1,
	SATCHEL_DRAW,
	SATCHEL_DROP
};

enum satchel_radio_e
{
	SATCHEL_RADIO_IDLE1 = 0,
	SATCHEL_RADIO_FIDGET1,
	SATCHEL_RADIO_DRAW,
	SATCHEL_RADIO_FIRE,
	SATCHEL_RADIO_HOLSTER
};

LINK_ENTITY_TO_CLASS( monster_satchel, CPipebombCharge )

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CPipebombCharge::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	UTIL_Remove( this );
}

void CPipebombCharge::PipebombUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->owner = m_hOwner->edict();

	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CPipebombCharge::Spawn( void )
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_pipebomb.mdl" );
	//UTIL_SetSize( pev, Vector( -16, -16, -4 ), Vector( 16, 16, 32 ) );	// Old box -- size of headcrab monsters/players get blocked by this
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );	// Uses point-sized, and can be stepped over
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CPipebombCharge::PipebombSlide );
	SetUse( &CPipebombCharge::PipebombUse );
	SetThink( &CPipebombCharge::PipebombThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->gravity = 0.5f;
	pev->friction = 0.5f;

	pev->dmg = gSkillData.plrDmgSatchel;
	// ResetSequenceInfo();
	pev->sequence = 1;
	m_flDropTime = gpGlobals->time;
}

void CPipebombCharge::PipebombSlide( CBaseEntity *pOther )
{
	//entvars_t *pevOther = pOther->pev;

	// don't hit the guy that launched this grenade
	if( pOther->edict() == m_hOwner->edict() )
	{
		if( pev->velocity.Length2D() < 10.0f
			&& m_flDropTime + 0.5f <= gpGlobals->time
			&& pOther->GiveAmmo( SATCHEL_DEFAULT_GIVE, "Satchel Charge", SATCHEL_MAX_CARRY ) != -1 )
		{
			CBasePlayer *pPlayer = static_cast<CBasePlayer*>( pOther );

			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );

			UTIL_Remove( this );

			CPipebomb *pPipebomb = static_cast<CPipebomb*>( pPlayer->GiveNamedPlayerItem( "weapon_pipebomb" ) );
			if( pPipebomb )
				pPipebomb->PipebombReload();
		}
		return;
	}
	// pev->avelocity = Vector( 300, 300, 300 );
	pev->gravity = 0.7f;// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 10 ), ignore_monsters, edict(), &tr );

	if( tr.flFraction < 1.0f )
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.7;
		pev->avelocity = pev->avelocity * 0.9;
		// play sliding sound, volume based on velocity
	}
	if( !( pev->flags & FL_ONGROUND ) && pev->velocity.Length2D() > 10.0f )
	{
		// Fix for a bug in engine: when object isn't moving, but its speed isn't 0 and on ground isn't set
		if( pev->origin != m_lastBounceOrigin )
		BounceSound();
	}
	m_lastBounceOrigin = pev->origin;
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
}

void CPipebombCharge::PipebombThink( void )
{
	// There is no model animation so commented this out to prevent net traffic
	// StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->owner && m_flDropTime + 0.5f <= gpGlobals->time )
		pev->owner = 0;

	if( !IsInWorld() )
	{
		UTIL_Remove( this );
		return;
	}

	if( pev->waterlevel == 3 )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8f;
		pev->avelocity = pev->avelocity * 0.9f;
		pev->velocity.z += 8;
	}
	else if( pev->waterlevel == 0 )
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}
	else
	{
		pev->velocity.z -= 8.0f;
	}	
}

void CPipebombCharge::Precache( void )
{
	PRECACHE_SOUND( "weapons/pb_bounce1.wav" );
	PRECACHE_SOUND( "weapons/pb_bounce2.wav" );
	PRECACHE_SOUND( "weapons/pb_bounce3.wav" );
	m_iTrail = PRECACHE_MODEL( "sprites/white.spr" );
}

void CPipebombCharge::BounceSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/pb_bounce1.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/pb_bounce2.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/pb_bounce3.wav", 1, ATTN_NORM );
		break;
	}
}

LINK_ENTITY_TO_CLASS( weapon_pipebomb, CPipebomb )

//=========================================================
// CALLED THROUGH the newly-touched weapon's instance. The existing player weapon is pOriginal
//=========================================================
int CPipebomb::AddDuplicate( CBasePlayerItem *pOriginal )
{
	CPipebomb *pPipebomb;

#ifdef CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		pPipebomb = (CPipebomb *)pOriginal;

		if( pPipebomb->m_chargeReady != SATCHEL_IDLE )
		{
			// player has some satchels deployed. Refuse to add more.
			return FALSE;
		}
	}

	return CBasePlayerWeapon::AddDuplicate( pOriginal );
}

//=========================================================
//=========================================================
int CPipebomb::AddToPlayer( CBasePlayer *pPlayer )
{
	int bResult = CBasePlayerItem::AddToPlayer( pPlayer );

	pPlayer->pev->weapons |= ( 1 << m_iId );
	m_chargeReady = SATCHEL_IDLE;// this satchel charge weapon now forgets that any satchels are deployed by it.

	if( bResult )
	{
		return AddWeapon();
	}
	return FALSE;
}

void CPipebomb::Spawn()
{
	Precache();
	m_iId = WEAPON_SATCHEL;
	SET_MODEL( ENT( pev ), "models/w_pipebomb.mdl" );

	m_iDefaultAmmo = SATCHEL_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CPipebomb::Precache( void )
{
	PRECACHE_MODEL( "models/v_pipebomb.mdl" );
	PRECACHE_MODEL( "models/v_pipebomb_watch.mdl" );
	PRECACHE_MODEL( "models/w_pipebomb.mdl" );
	PRECACHE_MODEL( "models/p_pipebomb.mdl" );
	PRECACHE_MODEL( "models/p_pipebomb_watch.mdl" );

	UTIL_PrecacheOther( "monster_satchel" );
}

int CPipebomb::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Satchel Charge";
	p->iMaxAmmo1 = SATCHEL_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iFlags = ITEM_FLAG_SELECTONEMPTY | ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;
	p->iId = m_iId = WEAPON_SATCHEL;
	p->iWeight = SATCHEL_WEIGHT;

	return 1;
}

//=========================================================
//=========================================================
BOOL CPipebomb::IsUseable( void )
{
	return CanDeploy();
}

BOOL CPipebomb::CanDeploy( void )
{
	if( m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] > 0 ) 
	{
		// player is carrying some satchels
		return TRUE;
	}

	if( m_chargeReady )
	{
		// player isn't carrying any satchels, but has some out
		return TRUE;
	}

	return FALSE;
}

BOOL CPipebomb::Deploy()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );

	if( m_chargeReady )
		return DefaultDeploy( "models/v_pipebomb_watch.mdl", "models/p_pipebomb_watch.mdl", SATCHEL_RADIO_DRAW, "hive" );
	else
		return DefaultDeploy( "models/v_pipebomb.mdl", "models/p_pipebomb.mdl", SATCHEL_DRAW, "trip" );
	
	return TRUE;
}

void CPipebomb::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;

	if( m_chargeReady )
	{
		SendWeaponAnim( SATCHEL_RADIO_HOLSTER );
	}
	else
	{
		SendWeaponAnim( SATCHEL_DROP );
	}
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0f, ATTN_NORM );

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] && m_chargeReady != SATCHEL_READY )
	{
		m_pPlayer->pev->weapons &= ~( 1 << WEAPON_SATCHEL );
		DestroyItem();
	}
}

void CPipebomb::PrimaryAttack()
{
	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		{
			Throw();
		}
		break;
	case SATCHEL_READY:
		{
			SendWeaponAnim( SATCHEL_RADIO_FIRE );

			edict_t *pPlayer = m_pPlayer->edict();

			CBaseEntity *pEnt = NULL;

			while( ( pEnt = UTIL_FindEntityInSphere( pEnt, m_pPlayer->pev->origin, 4096 ) ) != NULL )
			{
				if( FClassnameIs( pEnt->pev, "monster_satchel" ) )
				{
					CPipebombCharge *pPipebomb = (CPipebombCharge *)pEnt;
					if( pPipebomb->m_hOwner->edict() == pPlayer )
					{
						pPipebomb->Use( m_pPlayer, m_pPlayer, USE_ON, 0 );
					}
				}
			}

			m_chargeReady = SATCHEL_RELOAD;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
			break;
		}
	case SATCHEL_RELOAD:
		// we're reloading, don't allow fire
		break;
	}
}

void CPipebomb::SecondaryAttack( void )
{
	if( m_chargeReady != SATCHEL_RELOAD )
	{
		Throw();
	}
}

void CPipebomb::Throw( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
#ifndef CLIENT_DLL
		Vector vecSrc = m_pPlayer->pev->origin;

		Vector vecThrow = gpGlobals->v_forward * 274 + m_pPlayer->pev->velocity;

		CPipebombCharge *pPipebomb = (CPipebombCharge *)Create( "monster_satchel", vecSrc, g_vecZero, m_pPlayer->edict() );
		pPipebomb->pev->velocity = vecThrow;
		pPipebomb->pev->avelocity.y = 400;
		pPipebomb->m_hOwner = m_pPlayer;
		pPipebomb->pev->owner = pPipebomb->m_hOwner->edict();

		m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_pipebomb_watch.mdl" );
		m_pPlayer->pev->weaponmodel = MAKE_STRING( "models/p_pipebomb_watch.mdl" );
#else
		LoadVModel( "models/v_pipebomb_watch.mdl", m_pPlayer );
#endif

		SendWeaponAnim( SATCHEL_RADIO_DRAW );

		// player "shoot" animation
		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		m_chargeReady = SATCHEL_READY;

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
	}
}

void CPipebomb::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	switch( m_chargeReady )
	{
	case SATCHEL_IDLE:
		SendWeaponAnim( SATCHEL_FIDGET1 );
		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );
		break;
	case SATCHEL_READY:
		SendWeaponAnim( SATCHEL_RADIO_FIDGET1 );
		// use hivehand animations
		strcpy( m_pPlayer->m_szAnimExtention, "hive" );
		break;
	case SATCHEL_RELOAD:
		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			m_chargeReady = 0;
			RetireWeapon();
			return;
		}

#ifndef CLIENT_DLL
		m_pPlayer->pev->viewmodel = MAKE_STRING( "models/v_pipebomb.mdl" );
		m_pPlayer->pev->weaponmodel = MAKE_STRING( "models/p_pipebomb.mdl" );
#else
		LoadVModel( "models/v_pipebomb.mdl", m_pPlayer );
#endif
		SendWeaponAnim( SATCHEL_DRAW );

		// use tripmine animations
		strcpy( m_pPlayer->m_szAnimExtention, "trip" );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_chargeReady = SATCHEL_IDLE;
		break;
	}
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );// how long till we do this again.
}

void CPipebomb::PipebombReload()
{
	int i = 0;

	edict_t *pPlayer = m_pPlayer->edict();

	CBaseEntity *pEntity = NULL;

	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, m_pPlayer->pev->origin, 4096 ) ) != NULL )
	{
		if( FClassnameIs( pEntity->pev, "monster_satchel" ) )
		{
			CPipebombCharge *pPipebomb = (CPipebombCharge *)pEntity; 
			if( pPipebomb->m_hOwner->edict() == pPlayer )
			{
				++i;
			}
		}
	}

	if( i < 2 )
	{
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
		m_chargeReady = SATCHEL_RELOAD;
	}
}

//=========================================================
// DeactivatePipebombs - removes all pipebombs owned by
// the provided player. Should only be used upon death.
//
// Made this global on purpose.
//=========================================================
void DeactivatePipebombs( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_satchel" );

	while( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CPipebombCharge *pPipebomb = (CPipebombCharge *)pEnt;

		if( pPipebomb )
		{
			if( pPipebomb->m_hOwner->edict() == pOwner->edict() )
			{
				pPipebomb->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_satchel" );
	}
}
#endif
