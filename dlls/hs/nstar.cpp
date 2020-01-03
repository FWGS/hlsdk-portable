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

#ifndef CLIENT_DLL
#define STAR_AIR_VELOCITY	1500
#define STAR_WATER_VELOCITY	150

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CNinjaStar : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT StarTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

	int m_iTrail;

public:
	static CNinjaStar *StarCreate( void );
};
LINK_ENTITY_TO_CLASS( ninja_star, CNinjaStar );

CNinjaStar *CNinjaStar::StarCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CNinjaStar *pStar = GetClassPtr( (CNinjaStar *)NULL );
	pStar->pev->classname = MAKE_STRING("star");
	pStar->Spawn();

	return pStar;
}

void CNinjaStar::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(ENT(pev), "models/nstar.mdl");

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( &CNinjaStar::StarTouch );
	SetThink( &CNinjaStar::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}


void CNinjaStar::Precache( )
{
	PRECACHE_MODEL ("models/nstar.mdl");
	PRECACHE_SOUND("weapons/nstar_hitbod.wav");
	PRECACHE_SOUND("weapons/nstar_hit.wav");
	PRECACHE_SOUND("weapons/nstar_fire.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CNinjaStar :: Classify ( void )
{
	return	CLASS_NONE;
}

void CNinjaStar::StarTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

		pOther->TraceAttack(pevOwner, gSkillData.plrDmgStar, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "errgh" sound
		EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/nstar_hitbod.wav", 1, ATTN_NORM);

		if ( !g_pGameRules->IsMultiplayer() )
		{
		switch (RANDOM_LONG(1,2)) 
			{
	case 1: 
		Killed( pev, GIB_ALWAYS ); // Victim is lucky to have a heart at this point
		break;
	case 2: 
		Killed( pev, GIB_NEVER ); // Victim is lucky and keeps his organs in his corpse
		break;
			}
		}

		UTIL_Remove( this );
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/nstar_hit.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize( );
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0,360);
			pev->nextthink = gpGlobals->time + 10.0f;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

void CNinjaStar::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 20 );
}

void CNinjaStar::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);		
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove(this);
}
#endif

enum star_e { // Do this!@
	NSTAR_IDLE = 0,	// full
	NSTAR_FIRE,		// full
	NSTAR_DRAW,		// full
};

LINK_ENTITY_TO_CLASS( weapon_nstar, CNStar );

void CNStar::Spawn( )
{
	Precache( );
	m_iId = WEAPON_NSTAR;
	SET_MODEL(ENT(pev), "models/w_nstar.mdl");

	m_iDefaultAmmo = NSTAR_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CNStar::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CNStar::Precache( void )
{
	PRECACHE_MODEL("models/w_nstar.mdl");
	PRECACHE_MODEL("models/v_nstar.mdl");
	PRECACHE_MODEL("models/p_nstar.mdl");

	UTIL_PrecacheOther( "ninja_star" );

	m_usNStar = PRECACHE_EVENT( 1, "events/nstar.sc" );
}


int CNStar::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "stars";
	p->iMaxAmmo1 = NSTAR_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = NSTAR_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = WEAPON_NSTAR;
	p->iFlags = 0;
	p->iWeight = NSTAR_WEIGHT;
	return 1;
}


BOOL CNStar::Deploy( )
{
	return DefaultDeploy( "models/v_nstar.mdl", "models/p_nstar.mdl", NSTAR_DRAW, "nstar" );
}

void CNStar::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
}

void CNStar::PrimaryAttack( void )
{
	if (m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] > 0)
	{
		FireStar();
	}
}

void CNStar::FireStar()
{
	TraceResult tr;

	if (m_iClip == 0)
	{
		PlayEmptySound( );
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;

	//m_iClip--;
	m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usNStar, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	
	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir	 = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CNinjaStar *pStar = CNinjaStar::StarCreate();
	pStar->pev->origin = vecSrc;
	pStar->pev->angles = anglesAim;
	pStar->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pStar->pev->velocity = vecDir * STAR_WATER_VELOCITY;
		pStar->pev->speed = STAR_WATER_VELOCITY;
	}
	else
	{
		pStar->pev->velocity = vecDir * STAR_AIR_VELOCITY;
		pStar->pev->speed = STAR_AIR_VELOCITY;
	}
	pStar->pev->avelocity.z = 10;
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3f;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
}

void CNStar::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	SendWeaponAnim( NSTAR_IDLE );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0f / 30.0f;
}



class CNStarAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_nstar.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_nstar.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 30, "stars", NSTAR_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_nstar, CNStarAmmo );

#endif
