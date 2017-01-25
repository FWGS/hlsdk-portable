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
#define BOLT_AIR_VELOCITY	1300
#define BOLT_WATER_VELOCITY	1300


class CCrowbar2 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void FireBolt( void );
	void FireSniperBolt( void );
	void PrimaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );

	int m_fInZoom; // don't save this

	virtual BOOL UseDecrement( void )
	{
		return FALSE;
	}

private:
	unsigned short m_usCrossbow12;
	unsigned short m_usCrossbow22;
};



// UNDONE: Save/restore this?  Don't forget to set classname and LINK_ENTITY_TO_CLASS()
// 
// OVERLOADS SOME ENTVARS:
//
// speed - the ideal magnitude of my velocity
class CCrowbar2Bolt : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void EXPORT BubbleThink( void );
	void EXPORT BoltTouch( CBaseEntity *pOther );
	void EXPORT ExplodeThink( void );

	int m_iTrail;

public:
	static CCrowbar2Bolt *BoltCreate( void );
};

LINK_ENTITY_TO_CLASS( crowbar2_bolt, CCrowbar2Bolt )

CCrowbar2Bolt *CCrowbar2Bolt::BoltCreate( void )
{
	// Create a new entity with CCrossbowBolt private data
	CCrowbar2Bolt *pBolt = GetClassPtr( (CCrowbar2Bolt *)NULL );
	pBolt->pev->classname = MAKE_STRING( "crossbow_bolt" );	// g-cont. enable save\restore
	pBolt->Spawn();

	return pBolt;
}

void CCrowbar2Bolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 300;

	SET_MODEL( ENT( pev ), "models/w_hammer.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( &CCrowbar2Bolt::BoltTouch );
	SetThink( &CCrowbar2Bolt::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CCrowbar2Bolt::Precache()
{
	PRECACHE_MODEL( "models/w_hammer.mdl" );
	PRECACHE_SOUND( "weapons/hammer_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/hammer_hitbod2.wav" );
	PRECACHE_SOUND( "weapons/g_bounce2.wav" );
	PRECACHE_SOUND( "fvox/beep.wav" );
	m_iTrail = PRECACHE_MODEL( "sprites/streak.spr" );
}

int CCrowbar2Bolt::Classify( void )
{
	return CLASS_NONE;
}

void CCrowbar2Bolt::BoltTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t *pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if( pOther->IsPlayer() )
		{
			pOther->TraceAttack( pevOwner, 259, pev->velocity.Normalize(), &tr, DMG_ALWAYSGIB ); 
		}
		else
		{
			pOther->TraceAttack( pevOwner, 259, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_ALWAYSGIB ); 
		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/hammer_hitbod.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/hammer_hitbod.wav", 1, ATTN_NORM );
			break;
		}

		if( !g_pGameRules->IsMultiplayer() )
		{
			Killed( pev, GIB_ALWAYS );
		}
	}
	else
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/hammer_hit.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 7 ) );

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 180,360 );
			pev->nextthink = gpGlobals->time + 10.0;
		}
		else if( pOther->pev->movetype == MOVETYPE_PUSH || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 180 );
			pev->nextthink = gpGlobals->time + 10.0;			

		}

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		{
			UTIL_Sparks( pev->origin );
		}
	}


}

void CCrowbar2Bolt::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if( pev->waterlevel == 0 )
		return;

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}

void CCrowbar2Bolt::ExplodeThink( void )
{
	int iContents = UTIL_PointContents( pev->origin );
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if( iContents != CONTENTS_WATER )
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( iScale ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	entvars_t *pevOwner;

	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );

	UTIL_Remove( this );
}
#endif

enum crowbar_e
{
	CROSSBOW_IDLE = 0,
	CROSSBOW_DRAW,	
	CROSSBOW_HOLSTER,
	CROSSBOW_ATTACK1HIT
};

LINK_ENTITY_TO_CLASS( weapon_hammer, CCrowbar2 )

void CCrowbar2::Spawn()
{
	Precache();
	m_iId = WEAPON_HAMMER;
	SET_MODEL( ENT( pev ), "models/w_hammer.mdl" );

	m_iDefaultAmmo = -1;

	FallInit();// get ready to fall down.
}

int CCrowbar2::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

void CCrowbar2::Precache( void )
{
	PRECACHE_MODEL( "models/w_hammer.mdl" );
	PRECACHE_MODEL( "models/v_hammer.mdl" );
	PRECACHE_MODEL( "models/p_hammer.mdl" );

	PRECACHE_SOUND( "g_bounce2.wav" );

	UTIL_PrecacheOther( "crossbow2_bolt" );

	m_usCrossbow12 = PRECACHE_EVENT( 1, "events/crowbar.sc" );
	m_usCrossbow22 = PRECACHE_EVENT( 1, "events/crowbar.sc" );
}

int CCrowbar2::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = -1;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = WEAPON_HAMMER;
	p->iFlags = 0;
	p->iWeight = -10;
	return 1;
}

BOOL CCrowbar2::Deploy()
{
	if( m_iClip )
		return DefaultDeploy( "models/v_hammer.mdl", "models/p_hammer.mdl", CROSSBOW_DRAW, "bow" );
	return DefaultDeploy( "models/v_hammer.mdl", "models/p_hammer.mdl", CROSSBOW_DRAW, "bow" );
}

void CCrowbar2::Holster( int skiplocal /* = 0 */ )
{
	m_fInReload = FALSE;// cancel any reload in progress.

	if( m_fInZoom )
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	if( m_iClip )
{
		SendWeaponAnim( CROSSBOW_HOLSTER );
}

}

void CCrowbar2::PrimaryAttack( void )
{
#ifdef CLIENT_DLL
	if( m_fInZoom && bIsMultiplayer() )
#else
	if( m_fInZoom && g_pGameRules->IsMultiplayer() )
#endif
	{
		FireSniperBolt();
		return;
	}

	FireBolt();
}

// this function only gets called in multiplayer
void CCrowbar2::FireSniperBolt()
{
	m_flNextPrimaryAttack = gpGlobals->time + 0.35;


	TraceResult tr;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow22, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, m_pPlayer->edict(), &tr );

#ifndef CLIENT_DLL
	if( tr.pHit->v.takedamage )
	{
		ClearMultiDamage();
		CBaseEntity::Instance( tr.pHit )->TraceAttack( m_pPlayer->pev, 120, vecDir, &tr, DMG_BULLET | DMG_ALWAYSGIB ); 
		ApplyMultiDamage( pev, m_pPlayer->pev );
	}
#endif
}

void CCrowbar2::FireBolt()
{
	TraceResult tr;

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;


	int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usCrossbow12, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, m_iClip, m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType], 0, 0 );

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );

	anglesAim.x	= -anglesAim.x;
	Vector vecSrc	= m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir	= gpGlobals->v_forward;

#ifndef CLIENT_DLL
	CCrowbar2Bolt *pBolt = CCrowbar2Bolt::BoltCreate();
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();

	if( m_pPlayer->pev->waterlevel == 3 )
	{
		pBolt->pev->velocity = vecDir * BOLT_WATER_VELOCITY;
		pBolt->pev->speed = BOLT_WATER_VELOCITY;
	}
	else
	{
		pBolt->pev->velocity = vecDir * BOLT_AIR_VELOCITY;
		pBolt->pev->speed = BOLT_AIR_VELOCITY;
	}
	pBolt->pev->avelocity.z = 10;
#endif

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = gpGlobals->time + 0.35;

	m_flNextSecondaryAttack = gpGlobals->time + 0.35;

	if( m_iClip != 0 )
		m_flTimeWeaponIdle = gpGlobals->time + 5.0;
	else
		m_flTimeWeaponIdle = gpGlobals->time + 0.75;
}


void CCrowbar2::WeaponIdle( void )
{
	m_pPlayer->GetAutoaimVector( AUTOAIM_2DEGREES );  // get the autoaim vector but ignore it;  used for autoaim crosshair in DM

	ResetEmptySound();
	
	if( m_flTimeWeaponIdle < gpGlobals->time )
	{
		float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
		if( flRand <= 0.75 )
		{
			if( m_iClip )
			{
				SendWeaponAnim( CROSSBOW_IDLE );
			}
			else
			{
				SendWeaponAnim( CROSSBOW_IDLE );
			}
			m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
		else
		{
			if( m_iClip )
			{
				m_flTimeWeaponIdle = gpGlobals->time + 90.0 / 30.0;
			}
			else
			{
				m_flTimeWeaponIdle = gpGlobals->time + 80.0 / 30.0;
			}
			m_flTimeWeaponIdle = gpGlobals->time + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		}
	}
}

#endif
