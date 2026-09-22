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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "monsters.h"
#include "soundent.h"
#include "gamerules.h"

#include "player.h"

#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_PLAYER			0x0002
#define SF_TANK_HUMANS			0x0004
#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_SOUNDON			0x8000

extern DLL_GLOBAL Vector		g_vecAttackDir;

enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3
};

//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void TrackTarget( void );
	
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void EXPORT		Die( void );

	virtual void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	virtual Vector UpdateTargetPosition( CBaseEntity *pTarget )
	{
		return pTarget->BodyTarget( pev->origin );
	}

	void StartRotSound( void );
	void StopRotSound( void );

	// Bmodels don't go across transitions
	virtual int ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline BOOL IsActive( void ) { return (pev->spawnflags & SF_TANK_ACTIVE)?TRUE:FALSE; }
	inline void TankActivate( void ) { pev->spawnflags |= SF_TANK_ACTIVE; pev->nextthink = pev->ltime + 0.1f; m_fireLast = 0; }
	inline void TankDeactivate( void ) { pev->spawnflags &= ~SF_TANK_ACTIVE; m_fireLast = 0; StopRotSound(); }
	inline BOOL CanFire( void ) { return (gpGlobals->time - m_lastSightTime) < m_persist; }
	BOOL InRange( float range );

	// Acquire a target.  pPlayer is a player in the PVS
	edict_t		*FindTarget( edict_t *pPlayer );

	void		TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr );

	Vector		BarrelPosition( void )
	{
		Vector forward, right, up;
		UTIL_MakeVectorsPrivate( pev->angles, forward, right, up );
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	void		AdjustAnglesForBarrel( Vector &angles, float distance );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL OnControls( entvars_t *pevTest );
	BOOL StartControl( CBasePlayer* pController );
	void StopControl( void );
	void ControllerPostFrame( void );
	virtual void StopFire( void ){}

protected:
	CBasePlayer* m_pController;
	float		m_flNextAttack;
	Vector		m_vecControllerUsePos;
	
	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	string_t	m_iszSpriteSmoke;
	string_t	m_iszSpriteFlash;
	TANKBULLET	m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage
	
	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	string_t	m_iszMaster;	// Master entity (game_team_master or multisource)

	int			pGibName;
};

TYPEDESCRIPTION	CFuncTank::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTank, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteSmoke, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteFlash, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_sightOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spread, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTank, m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_iszMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncTank, CBaseEntity )

static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};

#define MAX_FIRING_SPREADS ARRAYSIZE( gTankSpread )

void CFuncTank::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( pev->frags != 0 )
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
		{
			pev->dmgtime = gpGlobals->time;
			UTIL_Sparks( ptr->vecEndPos );
		}
	}
	
	CBaseEntity::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CFuncTank :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pev->frags == 0)
		return 0;

	if(bitsDamageType == DMG_NERVEGAS)
		return 0;

	if ( (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_DARK)
	|| (bitsDamageType & DMG_BULLET))
		flDamage *= 0.8;

	Vector	vecTemp;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin). 
	if ( pevAttacker == pevInflictor )	
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( pev->absmin + ( pev->size * 0.5 ) );
	}
	

	if( bitsDamageType & DMG_BLAST )
	{
		flDamage *= 2.5;
	}
	else if( bitsDamageType & DMG_ENERGYBLAST)
	{
		flDamage *= 2.0;
	}
	else if( (bitsDamageType & DMG_MORTAR) || (bitsDamageType & DMG_VALVE_SWORD) )
	{
		flDamage *= 1.2;
	}

// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();
		
// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->deadflag = DEAD_DEAD;
		pev->takedamage = DAMAGE_NO;
		Die();
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	int pitch;
	float fvol;
	fvol = RANDOM_FLOAT(0.75, 1.0);
	if (RANDOM_LONG(0,2))
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG(0,34);

	char *rgpsz[3];
	rgpsz[0] = "debris/metal1.wav";
	rgpsz[1] = "debris/metal3.wav";
	rgpsz[2] = "debris/metal2.wav";
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, rgpsz[RANDOM_LONG(0,2)], fvol, ATTN_NORM, 0, pitch);

	return 1;
}

void CFuncTank::Die( void )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	CBaseEntity *pEntity = NULL;
	char cFlag = 0;
	int pitch;
	float fvol;
	
	pitch = 95 + RANDOM_LONG(0,29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT(0.85, 1.0) + (fabs(pev->health) / 100.0);

	if (fvol > 1.0)
		fvol = 1.0;

	switch ( RANDOM_LONG(0,1) )
	{
		case 0:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
		case 1:	
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_NORM, 0, pitch);	
			break;
	}

	vecVelocity.x = 0;
	vecVelocity.y = 0;
	vecVelocity.z = 0;

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL);

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x);
		WRITE_COORD( pev->size.y);
		WRITE_COORD( pev->size.z);

		// velocity
		WRITE_COORD( vecVelocity.x ); 
		WRITE_COORD( vecVelocity.y );
		WRITE_COORD( vecVelocity.z );

		// randomization
		WRITE_BYTE( 10 ); 

		// Model
		WRITE_SHORT( pGibName );	//model id#

		// # of shards
		WRITE_BYTE( 0 );	// let client decide

		// duration
		WRITE_BYTE( 25 );// 2.5 seconds

		// flags
		WRITE_BYTE( cFlag );
	MESSAGE_END();

	float size = pev->size.x;
	if ( size < pev->size.y )
		size = pev->size.y;
	if ( size < pev->size.z )
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins; 
	Vector maxs;
	
	mins = pev->absmin;
	maxs = pev->absmax;
	mins.z = pev->absmax.z;

	maxs.z += 8;

	::RadiusDamage2( vecSpot + Vector(0,0,128), pev, pev, 320, 800, CLASS_NONE, DMG_BLAST );
	FX_Explosion( vecSpot + Vector(0,0,128), EXPLOSION_C4 );
	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

//	RadiusDamage( vecSpot, pev, pev, 320, CLASS_NONE, DMG_BLAST );

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[128];
	int count = UTIL_EntitiesInBox( pList, 128, mins, maxs, FL_ONGROUND );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			ClearBits( pList[i]->pev->flags, FL_ONGROUND );
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;

	// Fire targets on break
	SUB_UseTargets( NULL, USE_TOGGLE, 0 );

	UTIL_Remove( this );
}

void CFuncTank::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0f;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if( m_fireRate <= 0 )
		m_fireRate = 1;
	if( m_spread > (int)MAX_FIRING_SPREADS )
		m_spread = 0;

	pev->oldorigin = pev->origin;

	pev->takedamage = DAMAGE_YES;
}

void CFuncTank::Precache( void )
{
	if( m_iszSpriteSmoke )
		PRECACHE_MODEL( STRING( m_iszSpriteSmoke ) );

	if( m_iszSpriteFlash )
		PRECACHE_MODEL( STRING( m_iszSpriteFlash ) );

	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
	
	pGibName = PRECACHE_MODEL("models/metalplategibs.mdl");
	PRECACHE_SOUND( "debris/metal1.wav" );
	PRECACHE_SOUND( "debris/metal2.wav" );
	PRECACHE_SOUND( "debris/metal3.wav" );
	PRECACHE_SOUND( "debris/bustmetal1.wav" );
	PRECACHE_SOUND( "debris/bustmetal2.wav" );
}

void CFuncTank::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "yawrate" ) )
	{
		m_yawRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawrange" ) )
	{
		m_yawRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawtolerance" ) )
	{
		m_yawTolerance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrange" ) )
	{
		m_pitchRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrate" ) )
	{
		m_pitchRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchtolerance" ) )
	{
		m_pitchTolerance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "firerate" ) )
	{
		m_fireRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrel" ) )
	{
		m_barrelPos.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrely" ) )
	{
		m_barrelPos.y = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrelz" ) )
	{
		m_barrelPos.z = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spritescale" ) )
	{
		m_spriteScale = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spritesmoke" ) )
	{
		m_iszSpriteSmoke = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spriteflash" ) )
	{
		m_iszSpriteFlash = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "rotatesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "persistence" ) )
	{
		m_persist = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet" ) )
	{
		m_bulletType = (TANKBULLET)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet_damage" ) )
	{
		m_iBulletDamage = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "firespread" ) )
	{
		m_spread = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "minRange" ) )
	{
		m_minRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxRange" ) )
	{
		m_maxRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_iszMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

////////////// START NEW STUFF //////////////

//==================================================================================
// TANK CONTROLLING
BOOL CFuncTank::OnControls( entvars_t *pevTest )
{
	if( !( pev->spawnflags & SF_TANK_CANCONTROL ) )
		return FALSE;

	//Vector offset = pevTest->origin - pev->origin;

	if( ( m_vecControllerUsePos - pevTest->origin ).Length() < 30 )
		return TRUE;

	return FALSE;
}

BOOL CFuncTank::StartControl( CBasePlayer *pController )
{
	if( m_pController != NULL )
		return FALSE;

	// Team only or disabled?
	if( m_iszMaster )
	{
		if( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return FALSE;
	}

	ALERT( at_console, "using TANK!\n");

	m_pController = pController;
	m_pController->m_pTank = this;
	if( m_pController->m_pActiveItem )
	{
		m_pController->m_pActiveItem->Holster();
		m_pController->pev->weaponmodel = 0;
		m_pController->pev->viewmodel = 0;
	}

	m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;
	m_vecControllerUsePos = m_pController->pev->origin;

	pev->nextthink = pev->ltime + 0.1f;

	return TRUE;
}

void CFuncTank::StopControl()
{
	StopFire();

	// TODO: bring back the controllers current weapon
	if( !m_pController )
		return;

	if( m_pController->m_pActiveItem )
		m_pController->m_pActiveItem->Deploy();

	ALERT( at_console, "stopped using TANK\n");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;

	pev->nextthink = 0;

	m_pController->m_pTank = NULL;
	m_pController = NULL;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0f;
}

// Called each frame by the player's ItemPostFrame
void CFuncTank::ControllerPostFrame( void )
{
	ASSERT( m_pController != NULL );

	if( gpGlobals->time < m_flNextAttack )
		return;

	if( m_pController->pev->button & IN_ATTACK )
	{
		Vector vecForward;
		UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );

		m_fireLast = gpGlobals->time - ( 1.0f / m_fireRate ) - 0.01f;  // to make sure the gun doesn't fire too many bullets

		Fire( BarrelPosition(), vecForward, m_pController->pev );
		
		// HACKHACK -- make some noise (that the AI can hear)
		if( m_pController && m_pController->IsPlayer() )
			( (CBasePlayer *)m_pController )->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + ( 1.0f / m_fireRate );
	}
}
////////////// END NEW STUFF //////////////

void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_TANK_CANCONTROL )
	{
		// player controlled turret
		if( pActivator->Classify() != CLASS_PLAYER )
			return;

		if( value == 2 && useType == USE_SET )
		{
			ControllerPostFrame();
		}
		else if( !m_pController && useType != USE_OFF )
		{
			StartControl( (CBasePlayer*)pActivator );
		}
		else
		{
			StopControl();
		}
	}
	else
	{
		if( !ShouldToggle( useType, IsActive() ) )
			return;

		if( IsActive() )
			TankDeactivate();
		else
			TankActivate();
	}
}

edict_t *CFuncTank::FindTarget( edict_t *pPlayer )
{
	return pPlayer;
}

BOOL CFuncTank::InRange( float range )
{
	if( range < m_minRange )
		return FALSE;
	if( m_maxRange > 0 && range > m_maxRange )
		return FALSE;

	return TRUE;
}

void CFuncTank::Think( void )
{
	pev->avelocity = g_vecZero;
	TrackTarget();

	if( fabs( pev->avelocity.x ) > 1 || fabs( pev->avelocity.y ) > 1 )
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::TrackTarget( void )
{
	TraceResult tr;
	edict_t *pPlayer = FIND_CLIENT_IN_PVS( edict() );
	BOOL updateTime = FALSE, lineOfSight;
	Vector angles, direction, targetPosition, barrelEnd;
	edict_t *pTarget = NULL;

	// Get a position to aim for
	if( m_pController )
	{
		// Tanks attempt to mirror the player's angles
		angles = m_pController->pev->v_angle;
		angles[0] = 0 - angles[0];
		pev->nextthink = pev->ltime + 0.05f;
	}
	else
	{
		if( IsActive() )
			pev->nextthink = pev->ltime + 0.1f;
		else
			return;

		if( FNullEnt( pPlayer ) )
		{
			if( IsActive() )
				pev->nextthink = pev->ltime + 2.0f;	// Wait 2 secs
			return;
		}
		pTarget = FindTarget( pPlayer );
		if( !pTarget|| FBitSet( pPlayer->v.flags, FL_NOTARGET ) )
			return;

		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->v.origin + pTarget->v.view_ofs;
		float range = ( targetPosition - barrelEnd ).Length();

		if( !InRange( range ) )
			return;

		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );

		lineOfSight = FALSE;
		if( tr.flFraction == 1.0f || tr.pHit == pTarget )
		{
			lineOfSight = TRUE;
			CBaseEntity *pInstance = CBaseEntity::Instance(pTarget);
			if( InRange( range ) && pInstance && pInstance->IsAlive() )
			{
				updateTime = TRUE;
				m_sightOrigin = UpdateTargetPosition( pInstance );
			}
		}

		// Track sight origin
		// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles( direction );

		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center) 
		AdjustAnglesForBarrel( angles, direction.Length() );
	}

	angles.x = -angles.x;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if( angles.y > m_yawCenter + m_yawRange )
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = FALSE;	// Don't update if you saw the player, but out of range
	}
	else if( angles.y < ( m_yawCenter - m_yawRange ) )
	{
		angles.y = ( m_yawCenter - m_yawRange );
		updateTime = FALSE; // Don't update if you saw the player, but out of range
	}

	if( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if( pev->avelocity.y > m_yawRate )
		pev->avelocity.y = m_yawRate;
	else if( pev->avelocity.y < -m_yawRate )
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if( angles.x > m_pitchCenter + m_pitchRange )
		angles.x = m_pitchCenter + m_pitchRange;
	else if( angles.x < m_pitchCenter - m_pitchRange )
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	pev->avelocity.x = distX  * 10;

	if( pev->avelocity.x > m_pitchRate )
		pev->avelocity.x = m_pitchRate;
	else if( pev->avelocity.x < -m_pitchRate )
		pev->avelocity.x = -m_pitchRate;

	if( m_pController )
		return;

	if( CanFire() && ( ( fabs( distX ) < m_pitchTolerance && fabs( distY ) < m_yawTolerance ) || ( pev->spawnflags & SF_TANK_LINEOFSIGHT ) ) )
	{
		BOOL fire = FALSE;
		Vector forward;
		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

		if( pev->spawnflags & SF_TANK_LINEOFSIGHT )
		{
			float length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if( tr.pHit == pTarget )
				fire = TRUE;
		}
		else
			fire = TRUE;

		if( fire )
		{
			Fire( BarrelPosition(), forward, pev );
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}

// If barrel is offset, add in additional rotation
void CFuncTank::AdjustAnglesForBarrel( Vector &angles, float distance )
{
	float r2, d2;

	if( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			if( d2 > r2 )
				angles.y += ( 180.0f / M_PI_F ) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			if( d2 > r2 )
				angles.x += ( 180.0f / M_PI_F ) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}

// Fire targets and spawn sprites
void CFuncTank::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if( m_fireLast != 0 )
	{
		if( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteSmoke ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0f, 20.0f ) );
			pSprite->SetTransparency( kRenderTransAlpha, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, 255, kRenderFxNone );
			pSprite->pev->velocity.z = RANDOM_FLOAT( 40.0f, 80.0f );
			pSprite->SetScale( m_spriteScale );
		}
		if( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteFlash ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 60 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->pev->nextthink += 0.1f;
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}
	m_fireLast = gpGlobals->time;
}

void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr )
{
	// get circular gaussian spread
	float x, y, z;
	do
	{
		x = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
		y = RANDOM_FLOAT( -0.5f, 0.5f ) + RANDOM_FLOAT( -0.5f, 0.5f );
		z = x * x + y * y;
	} while( z > 1 );
	Vector vecDir = vecForward +
		x * vecSpread.x * gpGlobals->v_right +
		y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd;

	vecEnd = vecStart + vecDir * 4096;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
}
	
void CFuncTank::StartRotSound( void )
{
	if( !pev->noise || ( pev->spawnflags & SF_TANK_SOUNDON ) )
		return;
	pev->spawnflags |= SF_TANK_SOUNDON;
	EMIT_SOUND( edict(), CHAN_STATIC, STRING( pev->noise ), 0.85, ATTN_NORM );
}

void CFuncTank::StopRotSound( void )
{
	if( pev->spawnflags & SF_TANK_SOUNDON )
		STOP_SOUND( edict(), CHAN_STATIC, STRING( pev->noise ) );
	pev->spawnflags &= ~SF_TANK_SOUNDON;
}

class CFuncTankGun : public CFuncTank
{
public:
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun )

void CFuncTankGun::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if( m_fireLast != 0 )
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				switch( m_bulletType )
				{
				case TANK_BULLET_9MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_9MM, 1, m_iBulletDamage, pevAttacker );
					break;
				case TANK_BULLET_MP5:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_MP5, 1, m_iBulletDamage, pevAttacker );
					break;
				case TANK_BULLET_12MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_12MM, 1, m_iBulletDamage, pevAttacker );
					break;
				default:
				case TANK_BULLET_NONE:
					break;
				}
			}
			CFuncTank::Fire( barrelEnd, forward, pevAttacker );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pevAttacker );
}

class CFuncTankLaser : public CFuncTank
{
public:
	void Activate( void );
	void KeyValue( KeyValueData *pkvd );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	void Think( void );
	CLaser *GetLaser( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	virtual void StopFire( void );

private:
	CLaser *m_pLaser;
	float m_laserTime;
};

LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser )

TYPEDESCRIPTION	CFuncTankLaser::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTankLaser, m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankLaser, m_laserTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFuncTankLaser, CFuncTank )

void CFuncTankLaser::Activate( void )
{
	if( !GetLaser() )
	{
		UTIL_Remove( this );
		ALERT( at_error, "Laser tank with no env_laser!\n" );
	}
	else
	{
		m_pLaser->TurnOff();
	}
}

void CFuncTankLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "laserentity" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

CLaser *CFuncTankLaser::GetLaser( void )
{
	if( m_pLaser )
		return m_pLaser;

	edict_t	*pentLaser;

	pentLaser = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->message ) );
	while( !FNullEnt( pentLaser ) )
	{
		// Found the landmark
		if( FClassnameIs( pentLaser, "env_laser" ) )
		{
			m_pLaser = (CLaser *)CBaseEntity::Instance( pentLaser );
			break;
		}
		else
			pentLaser = FIND_ENTITY_BY_TARGETNAME( pentLaser, STRING( pev->message ) );
	}

	return m_pLaser;
}

void CFuncTankLaser::Think( void )
{
	if( m_pLaser && (gpGlobals->time > m_laserTime) )
		m_pLaser->TurnOff();

	CFuncTank::Think();
}

void CFuncTankLaser::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;
	TraceResult tr;

	if( m_fireLast != 0 && GetLaser() )
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0f;
				m_pLaser->FireAtPoint( tr );
				m_pLaser->pev->nextthink = 0;
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
	{
		CFuncTank::Fire( barrelEnd, forward, pev );
	}
}

void CFuncTankLaser::StopFire( void )
{
	if( m_pLaser )
		m_pLaser->TurnOff();
}

class CFuncTankRocket : public CFuncTank
{
public:
	void Precache( void );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket )

void CFuncTankRocket::Precache( void )
{
	UTIL_PrecacheOther( "rpg_rocket" );
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if( m_fireLast != 0 )
	{
		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				CBaseEntity::Create( "rpg_rocket", barrelEnd, pev->angles, edict() );
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

class CFuncTankMortar : public CFuncTank
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};

LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar )

void CFuncTankMortar::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

void CFuncTankMortar::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if( m_fireLast != 0 )
	{
		int bulletCount = (int)( ( gpGlobals->time - m_fireLast ) * m_fireRate );
		// Only create 1 explosion
		if( bulletCount > 0 )
		{
			TraceResult tr;

			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors( pev->angles );

			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

			ExplosionCreate( tr.vecEndPos, pev->angles, edict(), pev->impulse, TRUE );

			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

//============================================================================
// FUNC TANK CONTROLS
//============================================================================
class CFuncTankControls : public CBaseEntity
{
public:
	virtual int ObjectCaps( void );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	CFuncTank *m_pTank;
};

LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls )

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTankControls, m_pTank, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity )

int CFuncTankControls::ObjectCaps( void ) 
{ 
	return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_IMPULSE_USE; 
}

void CFuncTankControls::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// pass the Use command onto the controls
	if( m_pTank )
		m_pTank->Use( pActivator, pCaller, useType, value );

	ASSERT( m_pTank != NULL );	// if this fails,  most likely means save/restore hasn't worked properly
}

void CFuncTankControls::Think( void )
{
	edict_t *pTarget = NULL;

	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING( pev->target ) );
	} while( !FNullEnt( pTarget ) && strncmp( STRING( pTarget->v.classname ), "func_tank", 9 ) );

	if( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No tank %s\n", STRING( pev->target ) );
		return;
	}

	m_pTank = (CFuncTank*)Instance( pTarget );
}

void CFuncTankControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->nextthink = gpGlobals->time + 0.3f;	// After all the func_tank's have spawned

	CBaseEntity::Spawn();
}
