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
#define BOLT_AIR_VELOCITY	800
#define BOLT_WATER_VELOCITY	450

extern cvar_t scipgvel;

int TestSpray;

// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CSciPGBolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );
	void SprayTest( const Vector &position, const Vector &direction, int spriteModel, int count );

	//int m_iTrail;

public:
	static CSciPGBolt *BoltCreate( void );
};
LINK_ENTITY_TO_CLASS( scientist_bolt, CSciPGBolt );

CSciPGBolt *CSciPGBolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CSciPGBolt *pBolt = GetClassPtr( (CSciPGBolt *)NULL );
	pBolt->pev->classname = MAKE_STRING("scibolt");
	pBolt->Spawn();

	return pBolt;
}

void CSciPGBolt::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL(	ENT(pev), "models/sci_rocket.mdl");
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/sci_scream.wav", 1, ATTN_NORM );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch( &CSciPGBolt::BoltTouch );
	SetThink( &CSciPGBolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}


void CSciPGBolt::Precache( )
{
	PRECACHE_MODEL("models/sci_rocket.mdl");
	PRECACHE_SOUND("weapons/sci_scream.wav");
	PRECACHE_SOUND("misc/kill.wav");
	TestSpray = PRECACHE_MODEL("sprites/spinning_coin.spr");// client side spittle of mario coins!
}


int	CSciPGBolt :: Classify ( void )
{
	return	CLASS_NONE;
}

void CSciPGBolt::BoltTouch( CBaseEntity *pOther )
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

		pOther->TraceAttack(pevOwner, gSkillData.plrDmgBow, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_ALWAYSGIB ); 

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );

		if ( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_ALWAYS ); //Smokey ;D
		}
		STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/sci_scream.wav" ); //Often times this works on certain maps, others not. Fix this if you want -- GOAHEAD
		SetThink( &CSciPGBolt::ExplodeThink );
		SprayTest( pev->origin, Vector(0,0,1), TestSpray, 24 );
		UTIL_Remove( this );
	}
	else
	{
		SetThink( &CBaseEntity::SUB_Remove );
		SetThink( &CSciPGBolt::ExplodeThink );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/sci_scream.wav" );
			SetThink( &CSciPGBolt::ExplodeThink );
			SprayTest( pev->origin, Vector(0,0,1), TestSpray, 24 );
			SetThink( &CBaseEntity::SUB_Remove );
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/sci_scream.wav" );
			SetThink( &CSciPGBolt::ExplodeThink );
			SprayTest( pev->origin, Vector(0,0,1), TestSpray, 24 );
			UTIL_Remove(this);
		}
	}
}

void CSciPGBolt::SprayTest( const Vector &position, const Vector &direction, int spriteModel, int count )
{
	EMIT_SOUND( ENT(pev), CHAN_VOICE, "misc/kill.wav", 1, ATTN_NORM );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( position.x);	// pos
		WRITE_COORD( position.y);	
		WRITE_COORD( position.z);	
		WRITE_COORD( direction.x);	// dir
		WRITE_COORD( direction.y);	
		WRITE_COORD( direction.z);	
		WRITE_SHORT( spriteModel );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( 130 );			// speed
		WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
	MESSAGE_END();
}

void CSciPGBolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 20 );
}

void CSciPGBolt::ExplodeThink( void )
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

enum sci_e {
	RPG_IDLE = 0,
	RPG_FIDGET,
	RPG_RELOAD,		// to reload
	RPG_FIRE2,		// to empty
	RPG_HOLSTER1,	// loaded
	RPG_DRAW1,		// loaded
	RPG_HOLSTER2,	// unloaded
	RPG_DRAW_UL,	// unloaded
	RPG_IDLE_UL,	// unloaded idle
	RPG_FIDGET_UL,	// unloaded fidget
};

LINK_ENTITY_TO_CLASS( weapon_scientist, CSciPG );
LINK_ENTITY_TO_CLASS( weapon_egon, CSciPG );

void CSciPG::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SCIENTIST;
	SET_MODEL(ENT(pev), "models/w_rpg.mdl");

	m_iDefaultAmmo = BOW_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

int CSciPG::AddToPlayer( CBasePlayer *pPlayer )
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

void CSciPG::Precache( void )
{
	PRECACHE_MODEL("models/w_rpg.mdl");
	PRECACHE_MODEL("models/v_rpg.mdl");
	PRECACHE_MODEL("models/p_rpg.mdl");

	UTIL_PrecacheOther( "scientist_bolt" );

	m_usScientist = PRECACHE_EVENT( 1, "events/scientist.sc" );
}


int CSciPG::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "scientist";
	p->iMaxAmmo1 = BOW_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = BOW_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = WEAPON_SCIENTIST;
	p->iFlags = 0;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}


BOOL CSciPG::Deploy( )
{
	if (m_iClip)
		return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW1, "scientist" );
	return DefaultDeploy( "models/v_rpg.mdl", "models/p_rpg.mdl", RPG_DRAW_UL, "scientist" );
}

void CSciPG::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	if (m_iClip)
		SendWeaponAnim( RPG_HOLSTER1 );
	else
		SendWeaponAnim( RPG_HOLSTER2 );
}

void CSciPG::PrimaryAttack( void )
{
	FireBolt();
}

void CSciPG::FireBolt()
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

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usScientist, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	
	anglesAim.x		= -anglesAim.x;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( ) - gpGlobals->v_up * 2;
	Vector vecDir	 = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CSciPGBolt *pBolt = CSciPGBolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if (m_pPlayer->pev->waterlevel == 3)
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		if (g_pGameRules->IsTest())
		{
		pBolt->pev->velocity = vecDir * scipgvel.value;
		pBolt->pev->speed = scipgvel.value;
		}
		else
		{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
		}
	}
	pBolt->pev->avelocity.z = 10;
#endif

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.75f;

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.75f;

	if (m_iClip != 0)
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75f;
}

void CSciPG::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound( );
	
	if ( m_flTimeWeaponIdle < UTIL_WeaponTimeBase() )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if (flRand <= 0.75f)
		{
			if (m_iClip)
			{
				SendWeaponAnim( RPG_IDLE );
			}
			else
			{
				SendWeaponAnim( RPG_IDLE_UL );
			}
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			if (m_iClip)
			{
				SendWeaponAnim( RPG_FIDGET );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 90.0f / 30.0f;
			}
			else
			{
				SendWeaponAnim( RPG_FIDGET_UL );
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0f / 30.0f;
			}
		}
	}
}



class CSciPGAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_bow_clip.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_bow_clip.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( 30, "scientist", BOW_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_scientist, CSciPGAmmo );

#endif
