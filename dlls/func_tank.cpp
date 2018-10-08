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
#include "movewith.h"

#include "player.h"

#define SF_TANK_ACTIVE			0x0001
//#define SF_TANK_PLAYER			0x0002
//#define SF_TANK_HUMANS			0x0004
//#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_LASERSPOT		0x0040 //LRC
#define SF_TANK_MATCHTARGET		0x0080 //LRC
#define SF_TANK_SOUNDON			0x8000
#define SF_TANK_SEQFIRE			0x10000 //LRC - a TankSequence is telling me to fire

enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3,
};

class CFuncTank;
class CTankSequence;

// declare Controls up here to stop the compiler complaining. (come back Java, all is forgiven...)
class CFuncTankControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps( void );
	void Spawn( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
//	void Think( void );
	void	KeyValue( KeyValueData *pkvd );
	STATE GetState(void) { return m_active?STATE_ON:STATE_OFF; }

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
	BOOL OnControls( entvars_t *pevTest );

	BOOL m_active; // am I being used to control tanks right now?
	Vector		m_vecControllerUsePos; // where was the player standing when he used me?
										// for a 'movewith' controls entity, this is relative to the movewith ent.
	CBasePlayer* m_pController;
	int			m_iCrosshair;	//LRC - show a crosshair while in use. (currently this is just yes or no,
								// but in future it will be the id of the weapon whose crosshair should be used.)
//	CFuncTank *m_pTank;
};


#define SF_TSEQ_DUMPPLAYER	1
#define SF_TSEQ_REPEATABLE	2

#define TSEQ_UNTIL_NONE		0
#define TSEQ_UNTIL_FACING	1
#define TSEQ_UNTIL_DEATH	2

#define TSEQ_TURN_NO		0
#define TSEQ_TURN_ANGLE		1
#define TSEQ_TURN_FACE		2
#define TSEQ_TURN_ENEMY		3

#define TSEQ_SHOOT_NO		0
#define TSEQ_SHOOT_ONCE		1
#define TSEQ_SHOOT_ALWAYS	2
#define TSEQ_SHOOT_FACING	3

#define TSEQ_FLAG_NOCHANGE	0
#define TSEQ_FLAG_ON		1
#define TSEQ_FLAG_OFF		2
#define TSEQ_FLAG_TOGGLE	3

class CTankSequence : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EndThink( void );
	void TimeOutThink( void );
	void	KeyValue( KeyValueData *pkvd );
	STATE	GetState( void ) { return m_pTank?STATE_ON:STATE_OFF; }
	virtual int	ObjectCaps( void );

	void StopSequence( void );
	void FacingNotify( void );
	void DeadEnemyNotify( void );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	string_t m_iszEntity;
	string_t m_iszEnemy;
	int m_iUntil;
	float m_fDuration;
	int m_iTurn;
	int m_iShoot;
	int m_iActive;
	int m_iControllable;
	int m_iLaserSpot;
	CFuncTank *m_pTank; // the sequence can only control one tank at a time, for the moment
};

//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{
public:
	void Spawn( void );
	void	PostSpawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void Think( void );
	void TrackTarget( void );
	CBaseEntity* BestVisibleEnemy( void );
	int		IRelationship( CBaseEntity* pTarget );

	int		Classify( void ) { return m_iTankClass; }

	void TryFire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
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
	inline void TankActivate( void ) { pev->spawnflags |= SF_TANK_ACTIVE; SetNextThink(0.1); m_fireLast = 0; }
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

//	BOOL OnControls( entvars_t *pevTest );
	BOOL StartControl( CBasePlayer* pController, CFuncTankControls* pControls );
	void StopControl( CFuncTankControls* pControls );
//	void ControllerPostFrame( void );

	CFuncTankControls* m_pControls; //LRC - tankcontrols is used as a go-between.

	void StartSequence( CTankSequence *pSequence);
	void StopSequence();

	CTankSequence *m_pSequence; //LRC - if set, then this is the sequence the tank is currently performing
	CBaseEntity *m_pSequenceEnemy; //LRC - the entity that our sequence wants us to attack
	CLaserSpot*  m_pSpot;		// Laser spot entity

	//LRC - unprotected these, so that TankSequence can look at them
	float		m_maxRange;		// Max range to aim/track
	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second

protected:
//	CBasePlayer* m_pController;
	float		m_flNextAttack;
//LRC	Vector		m_vecControllerUsePos;
	
	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	string_t	m_iszSpriteSmoke;
	string_t	m_iszSpriteFlash;
	TANKBULLET	m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage
	
	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	string_t		m_iszMaster;	// Master entity
	int			m_iszFireMaster;//LRC - Fire-Master entity (prevents firing when inactive)

	int			m_iTankClass;	// Behave As

	void UpdateSpot( void );
//	CLaserSpot*  m_pViewTarg;	// Player view indicator

	CPointEntity *m_pFireProxy; //LRC - locus position for custom shots
	int			m_iszLocusFire;
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
	DEFINE_FIELD( CFuncTank, m_pControls, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTank, m_pSequence, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTank, m_pSequenceEnemy, FIELD_CLASSPTR ), //LRC
	DEFINE_FIELD( CFuncTank, m_pSpot, FIELD_CLASSPTR ), //LRC
//LRC	DEFINE_FIELD( CFuncTank, m_pController, FIELD_CLASSPTR ),
//LRC	DEFINE_FIELD( CFuncTank, m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_iszMaster, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_iszFireMaster, FIELD_STRING ), //LRC
	DEFINE_FIELD( CFuncTank, m_iszLocusFire, FIELD_STRING ), //LRC
	DEFINE_FIELD( CFuncTank, m_pFireProxy, FIELD_CLASSPTR ), //LRC
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

void CFuncTank::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if( IsActive() )
	{
		SetNextThink(1.0);
	}

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if( m_fireRate <= 0 )
		m_fireRate = 1;
	if( m_spread > (int)MAX_FIRING_SPREADS )
		m_spread = 0;

	pev->oldorigin = pev->origin;

	if (m_iszLocusFire) //LRC - locus trigger
	{
		m_pFireProxy = GetClassPtr( (CPointEntity*)NULL );
	}
}

void CFuncTank::PostSpawn( void )
{
	if (m_pMoveWith)
	{
		m_yawCenter = pev->angles.y - m_pMoveWith->pev->angles.y;
		m_pitchCenter = pev->angles.x - m_pMoveWith->pev->angles.x;
	}
	else
	{
		m_yawCenter = pev->angles.y;
		m_pitchCenter = pev->angles.x;
	}
}

void CFuncTank::Precache( void )
{
	if( m_iszSpriteSmoke )
		PRECACHE_MODEL( STRING( m_iszSpriteSmoke ) );

	if( m_iszSpriteFlash )
		PRECACHE_MODEL( STRING( m_iszSpriteFlash ) );

	if( pev->noise )
		PRECACHE_SOUND( STRING( pev->noise ) );
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
	else if (FStrEq(pkvd->szKeyName, "firemaster"))
	{
		m_iszFireMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iClass"))
	{
		m_iTankClass = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszLocusFire"))
	{
		m_iszLocusFire = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

//==================================================================================
// TANK CONTROLLING
/*LRC- TankControls checks this instead
BOOL CFuncTank::OnControls( entvars_t *pevTest )
{
	if( !( pev->spawnflags & SF_TANK_CANCONTROL ) )
		return FALSE;

	//Vector offset = pevTest->origin - pev->origin;

	if( ( m_vecControllerUsePos - pevTest->origin ).Length() < 30 )
		return TRUE;

	return FALSE;
} */

BOOL CFuncTank :: StartControl( CBasePlayer* pController, CFuncTankControls *pControls )
{
//	ALERT(at_console, "StartControl\n");
	// we're already being controlled or playing a sequence
	if ( m_pControls != NULL || m_pSequence != NULL )
{
//		ALERT(at_debug,"StartControl failed, already in use\n");
		return FALSE;
	}

	// Team only or disabled?
	if( m_iszMaster )
	{
		if( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
		{
//			ALERT(at_debug,"StartControl failed, locked\n");
			return FALSE;
	}
	}

//	ALERT( at_console, "using TANK!\n");

	m_pControls = pControls;

	if (m_pSpot) m_pSpot->Revive();
//	if (m_pViewTarg) m_pViewTarg->Revive();

	SetNextThink(0.1);
//	ALERT(at_debug,"StartControl succeeded\n");
	return TRUE;
}

void CFuncTank :: StopControl( CFuncTankControls* pControls)
{
//LRC- various commands moved from here to FuncTankControls
	if ( !m_pControls || m_pControls != pControls)
	{
		//ALERT(at_debug,"StopControl failed, not in use\n");
		return;
	}

//	ALERT(at_debug,"StopControl succeeded\n");

//	ALERT( at_debug, "stopped using TANK\n");

	if (m_pSpot) m_pSpot->Suspend(-1);
//	if (m_pViewTarg) m_pViewTarg->Suspend(-1);
	StopRotSound(); //LRC

	DontThink();
	UTIL_SetAvelocity(this, g_vecZero);
	m_pControls = NULL;

	if ( IsActive() )
	{
		SetNextThink(1.0);
	}
}

void CFuncTank::UpdateSpot( void )
{
	if ( pev->spawnflags & SF_TANK_LASERSPOT )
	{
		if (!m_pSpot)
{
			m_pSpot = CLaserSpot::CreateSpot();
		}

		Vector vecAiming;
		UTIL_MakeVectorsPrivate( pev->angles, vecAiming, NULL, NULL );
		Vector vecSrc = BarrelPosition( );

		TraceResult tr;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(pev), &tr );

		// ALERT( "%f %f\n", gpGlobals->v_forward.y, vecAiming.y );

		/*
		float a = gpGlobals->v_forward.y * vecAiming.y + gpGlobals->v_forward.x * vecAiming.x;
		m_pPlayer->pev->punchangle.y = acos( a ) * (180 / M_PI);

		ALERT( at_console, "%f\n", a );
		*/

		UTIL_SetOrigin( m_pSpot, tr.vecEndPos );
	}
}

// Called each frame by PostThink, via Use.
// all we do here is handle firing.
// LRC- this is now never called. Think functions are handling it all.
/*void CFuncTank :: ControllerPostFrame( void )
{
	ASSERT( m_pController != NULL );

	if( gpGlobals->time < m_flNextAttack )
		return;

	if( m_pController->pev->button & IN_ATTACK )
	{
		Vector vecForward;
		UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );

		m_fireLast = gpGlobals->time - ( 1 / m_fireRate ) - 0.01;  // to make sure the gun doesn't fire too many bullets

		Fire( BarrelPosition(), vecForward, m_pController->pev );
		
		// HACKHACK -- make some noise (that the AI can hear)
		if( m_pController && m_pController->IsPlayer() )
			( (CBasePlayer *)m_pController )->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + ( 1 / m_fireRate );
	}
}*/
////////////// END NEW STUFF //////////////

void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_TANK_CANCONTROL )
	{
		// player controlled turret
		if( pActivator->Classify() != CLASS_PLAYER )
			return;

		// from Player::PostThink. ("try fire the gun")
		if( value == 2 && useType == USE_SET )
		{
// LRC- actually, we handle firing with TrackTarget, to support multitank.
//			ControllerPostFrame();
		}

// LRC- tankcontrols handles all this
//		else if ( !m_pController && useType != USE_OFF )
//		{
//			// LRC- add one more tank to the ones the player's using
//			((CBasePlayer*)pActivator)->m_pTank = this;
//			StartControl( (CBasePlayer*)pActivator );
//		}
//		else
//		{
//		// probably from Player::PostThink- player stopped using tank.
//			StopControl();
//		}
	}
	else
	{
		if( !ShouldToggle( useType, IsActive() ) )
			return;

		if( IsActive() )
		{
			TankDeactivate();
			if (m_pSpot) m_pSpot->Suspend(-1);
		}
		else
		{
			TankActivate();
			if (m_pSpot) m_pSpot->Revive();
		}
	}
}

edict_t *CFuncTank::FindTarget( edict_t *pPlayer )
{
	return pPlayer;
}

CBaseEntity *CFuncTank:: BestVisibleEnemy ( void )
{
	CBaseEntity	*pReturn;
	int			iNearest;
	int			iDist;
	int			iBestRelationship;
	int			iLookDist = m_maxRange?m_maxRange:512; //thanks to Waldo for this.

	iNearest = 8192;// so first visible entity will become the closest.
	pReturn = NULL;
	iBestRelationship = R_DL;

	CBaseEntity *pList[100];

	Vector delta = Vector( iLookDist, iLookDist, iLookDist );

	// Find only monsters/clients in box, NOT limited to PVS
	int count = UTIL_EntitiesInBox( pList, 100, pev->origin - delta, pev->origin + delta, FL_CLIENT|FL_MONSTER );
	int i;

	for (i = 0; i < count; i++ )
	{
		if ( pList[i]->IsAlive() )
		{
			if ( IRelationship( pList[i] ) > iBestRelationship )
			{
				// this entity is disliked MORE than the entity that we 
				// currently think is the best visible enemy. No need to do 
				// a distance check, just get mad at this one for now.
				iBestRelationship = IRelationship ( pList[i] );
				iNearest = ( pList[i]->pev->origin - pev->origin ).Length();
				pReturn = pList[i];
			}
			else if ( IRelationship( pList[i] ) == iBestRelationship )
			{
				// this entity is disliked just as much as the entity that
				// we currently think is the best visible enemy, so we only
				// get mad at it if it is closer.
				iDist = ( pList[i]->pev->origin - pev->origin ).Length();
				
				if ( iDist <= iNearest )
				{
					iNearest = iDist;
					//these are guaranteed to be the same! iBestRelationship = IRelationship ( pList[i] ); 
					pReturn = pList[i];
				}
			}
		}
	}

//	if (pReturn)
//		ALERT(at_debug, "Tank's best enemy is %s\n", STRING(pReturn->pev->classname));
//	else
//		ALERT(at_debug, "Tank has no best enemy\n");
	return pReturn;
}


int	CFuncTank::IRelationship( CBaseEntity* pTarget )
{
	int iOtherClass = pTarget->Classify();
	if (iOtherClass == CLASS_NONE) return R_NO;

	if (!m_iTankClass)
	{
		if (iOtherClass == CLASS_PLAYER)
			return R_HT;
		else
			return R_NO;
	}
	else if (m_iTankClass == CLASS_PLAYER_ALLY)
	{
		switch (iOtherClass)
		{
		case CLASS_HUMAN_MILITARY:
		case CLASS_MACHINE:
		case CLASS_ALIEN_MILITARY:
		case CLASS_ALIEN_MONSTER:
		case CLASS_ALIEN_PREDATOR:
		case CLASS_ALIEN_PREY:
			return R_HT;
		default:
			return R_NO;
		}
	}
	else if (m_iTankClass == CLASS_HUMAN_MILITARY)
	{
		switch (iOtherClass)
		{
		case CLASS_PLAYER:
		case CLASS_PLAYER_ALLY:
		case CLASS_ALIEN_MILITARY:
		case CLASS_ALIEN_MONSTER:
		case CLASS_ALIEN_PREDATOR:
		case CLASS_ALIEN_PREY:
			return R_HT;
		case CLASS_HUMAN_PASSIVE:
			return R_DL;
		default:
			return R_NO;
		}
	}
	else if (m_iTankClass == CLASS_ALIEN_MILITARY)
	{
		switch (iOtherClass)
		{
		case CLASS_PLAYER:
		case CLASS_PLAYER_ALLY:
		case CLASS_HUMAN_MILITARY:
			return R_HT;
		case CLASS_HUMAN_PASSIVE:
			return R_DL;
		default:
			return R_NO;
		}
	}
	else
		return R_NO;
}


BOOL CFuncTank::InRange( float range )
{
	if( range < m_minRange )
		return FALSE;
	if( m_maxRange > 0 && range > m_maxRange )
		return FALSE;

	return TRUE;
}

//LRC
void CFuncTank :: StartSequence(CTankSequence *pSequence)
{
	m_pSequence = pSequence;
	SetNextThink(1.0);
}

//LRC
void CFuncTank :: StopSequence( )
{
	StopRotSound();
	DontThink();
	pev->avelocity = g_vecZero;
	m_pSequence = NULL;
	m_pSequenceEnemy = NULL;
}

// NB: tracktarget updates nextthink
void CFuncTank::Think( void )
{
//	pev->avelocity = g_vecZero;
	TrackTarget();

	if( fabs( pev->avelocity.x ) > 1 || fabs( pev->avelocity.y ) > 1 )
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::TrackTarget( void )
{
	TraceResult tr;
//	edict_t *pPlayer;
	BOOL updateTime = FALSE, lineOfSight;
	Vector angles, direction, targetPosition, barrelEnd;
	Vector v_right, v_up;
	CBaseEntity *pTarget;
	CBasePlayer* pController = NULL;

//	ALERT(at_console,"TrackTarget\n");

	// update the barrel position
	if (m_pFireProxy)
	{
		m_pFireProxy->pev->origin = BarrelPosition();
		UTIL_MakeVectorsPrivate( pev->angles, m_pFireProxy->pev->velocity, NULL, NULL );
	}

	// Get a position to aim for
	if (m_pSequence)
	{
		UpdateSpot();
		SetNextThink(0.05, FALSE);

		if (m_pSequence->m_iTurn == TSEQ_TURN_ENEMY)
		{
			CBaseMonster *pMonst = m_pSequenceEnemy->MyMonsterPointer();
			if (pMonst && !pMonst->IsAlive())
				m_pSequence->DeadEnemyNotify();

			// Work out what angle we need to face to look at the enemy
			targetPosition = m_pSequenceEnemy->pev->origin + m_pSequenceEnemy->pev->view_ofs;
			direction = targetPosition - pev->origin;
			angles = UTIL_VecToAngles( direction );
			AdjustAnglesForBarrel( angles, direction.Length() );
		}
		else if (m_pSequence->m_iTurn == TSEQ_TURN_ANGLE)
		{
			angles = m_pSequence->pev->angles;
		}
		else if (m_pSequence->m_iTurn == TSEQ_TURN_FACE)
		{
			// Work out what angle we need to face to look at the sequence
			direction = m_pSequence->pev->origin - pev->origin;
			angles = UTIL_VecToAngles( direction );
			AdjustAnglesForBarrel( angles, direction.Length() );
		}
	}
	else if (m_pControls && m_pControls->m_pController)
	{
//		ALERT( at_console, "TANK has controller\n");
		UpdateSpot();
		pController = m_pControls->m_pController;
		SetNextThink(0.05, FALSE);

		// LRC- changed here to allow "match target" as well as "match angles" mode.
		if (pev->spawnflags & SF_TANK_MATCHTARGET)
		{
			// "Match target" mode:
			// first, get the player's angles
			angles = pController->pev->v_angle;
			// Work out what point the player is looking at
			UTIL_MakeVectorsPrivate(angles, direction,NULL,NULL);

			targetPosition = pController->EyePosition() + direction * 1000;

			edict_t *ownerTemp = pev->owner; //LRC store the owner, so we can put it back after the check
			pev->owner = pController->edict(); //LRC when doing the matchtarget check, don't hit the player or the tank.

			UTIL_TraceLine(
				pController->EyePosition(),
				targetPosition,
				missile, //the opposite of ignore_monsters: target them if we go anywhere near!
				ignore_glass,
				edict(), &tr
			);

			pev->owner = ownerTemp; //LRC put the owner back

//			if (!m_pViewTarg)
//			{
//				m_pViewTarg = CLaserSpot::CreateSpot("sprites/mommablob.spr");
//			}
//			UTIL_SetOrigin( m_pViewTarg, tr.vecEndPos );

			// Work out what angle we need to face to look at that point
			direction = tr.vecEndPos - pev->origin;
			angles = UTIL_VecToAngles( direction );
			targetPosition = tr.vecEndPos;

//			ALERT( at_console, "TANK: look at pos %.0f %.0f %.0f; target angle %.0f %.0f %.0f\n",
//				targetPosition.x,targetPosition.y,targetPosition.z,angles.x,angles.y,angles.z);

			// Calculate the additional rotation to point the end of the barrel at the target
			// (instead of the gun's center)
			AdjustAnglesForBarrel( angles, direction.Length() );
		}
		else
	{
			// "Match angles" mode
			// just get the player's angles
			angles = pController->pev->v_angle;
		angles[0] = 0 - angles[0];
		}
	}
	else
	{
//		ALERT( at_console, "TANK has no controller\n");
		if( IsActive() )
		{
			SetNextThink(0.1);
		}
		else
		{
			DontThink();
			UTIL_SetAvelocity(this, g_vecZero);
			return;
		}

		UpdateSpot();

		// if we can't see any players
		//pPlayer = FIND_CLIENT_IN_PVS( edict() );
		pTarget = BestVisibleEnemy();
		if ( FNullEnt( pTarget ) )
		{
			if( IsActive() )
				SetNextThink(2);	// No enemies visible, wait 2 secs
			return;
		}

		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
		float range = ( targetPosition - barrelEnd ).Length();

		if( !InRange( range ) )
			return;

		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );

		if ( tr.flFraction == 1.0 || tr.pHit == ENT(pTarget->pev) )
		{
			lineOfSight = TRUE;

			if ( InRange( range ) && pTarget->IsAlive() )
			{
				updateTime = TRUE; // I think I saw him, pa!
				m_sightOrigin = UpdateTargetPosition( pTarget );
			}
			}
		else
		{
			// No line of sight, don't track
			lineOfSight = FALSE;
		}

		// Track sight origin

// !!! I'm not sure what i changed (cuh, these Valve cowboys... --LRC)
// m_sightOrigin is the last known location of the player.

		direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles( direction );

		// Calculate the additional rotation to point the end of the barrel at the target
		// (not the gun's center)
		AdjustAnglesForBarrel( angles, direction.Length() );
	}

	angles.x = -angles.x;

	float currentYawCenter, currentPitchCenter;

	// Force the angles to be relative to the center position
	if (m_pMoveWith)
	{
		currentYawCenter = m_yawCenter + m_pMoveWith->pev->angles.y;
		currentPitchCenter = m_pitchCenter + m_pMoveWith->pev->angles.x;
	}
	else
	{
		currentYawCenter = m_yawCenter;
		currentPitchCenter = m_pitchCenter;
	}

	angles.y = currentYawCenter + UTIL_AngleDistance( angles.y, currentYawCenter );
	angles.x = currentPitchCenter + UTIL_AngleDistance( angles.x, currentPitchCenter );

	// Limit against range in y
	if (m_yawRange < 360)
	{
		if ( angles.y > currentYawCenter + m_yawRange )
	{
			angles.y = currentYawCenter + m_yawRange;
			updateTime = FALSE;	// If player is outside fire arc, we didn't really see him
	}
		else if ( angles.y < (currentYawCenter - m_yawRange) )
	{
			angles.y = (currentYawCenter - m_yawRange);
			updateTime = FALSE; // If player is outside fire arc, we didn't really see him
		}
	}
	// we can always 'see' the whole vertical arc, so it's just the yaw we needed to check.

	if( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
//	ALERT(at_console, "%f -> %f: dist= %f\n", angles.y, pev->angles.y, distY);
	Vector setAVel = g_vecZero;

	setAVel.y = distY * 10;
	if ( setAVel.y > m_yawRate )
		setAVel.y = m_yawRate;
	else if ( setAVel.y < -m_yawRate )
		setAVel.y = -m_yawRate;

	// Limit against range in x
	if ( angles.x > currentPitchCenter + m_pitchRange )
		angles.x = currentPitchCenter + m_pitchRange;
	else if ( angles.x < currentPitchCenter - m_pitchRange )
		angles.x = currentPitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	setAVel.x = distX  * 10;

	if ( setAVel.x > m_pitchRate )
		setAVel.x = m_pitchRate;
	else if ( setAVel.x < -m_pitchRate )
		setAVel.x = -m_pitchRate;

	UTIL_SetAvelocity(this, setAVel);

	// notify the TankSequence if we're (pretty close to) facing the target
	if( m_pSequence && fabs( distY ) < 0.1 && fabs( distX ) < 0.1 )
		m_pSequence->FacingNotify();

	// firing in tanksequences:
	if ( m_pSequence )
	{
		if ( gpGlobals->time < m_flNextAttack )	return;

		if ( pev->spawnflags & SF_TANK_SEQFIRE ) // does the sequence want me to fire?
		{
			Vector forward;
			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

			// to make sure the gun doesn't fire too many bullets
			m_fireLast = gpGlobals->time - (1/m_fireRate) - 0.01;

			TryFire( BarrelPosition(), forward, pev );

			m_flNextAttack = gpGlobals->time + (1/m_fireRate);
		}
		return;
	}
	// firing with player-controlled tanks:
	else if ( pController )
	{
		if ( gpGlobals->time < m_flNextAttack )
		return;

		// FIXME- use m_???Tolerance to fire in the desired direction,
		// instead of the one we're facing.

		if ( pController->pev->button & IN_ATTACK )
		{
			Vector forward;
			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

			// to make sure the gun doesn't fire too many bullets
			m_fireLast = gpGlobals->time - (1/m_fireRate) - 0.01;

			TryFire( BarrelPosition(), forward, pController->pev );
		
			// HACKHACK -- make some noise (that the AI can hear)
			if ( pController && pController->IsPlayer() )
				((CBasePlayer *)pController)->m_iWeaponVolume = LOUD_GUN_VOLUME;

			m_flNextAttack = gpGlobals->time + (1/m_fireRate);
		}
	}
	// firing with automatic guns:
	else if ( CanFire() && ( (fabs(distX) < m_pitchTolerance && fabs(distY) < m_yawTolerance) || (pev->spawnflags & SF_TANK_LINEOFSIGHT) ) )
	{
		BOOL fire = FALSE;
		Vector forward;
		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

		if( pev->spawnflags & SF_TANK_LINEOFSIGHT )
		{
			float length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if ( tr.pHit == ENT(pTarget->pev) )
				fire = TRUE;
		}
		else
			fire = TRUE;

		if( fire )
		{
			TryFire( BarrelPosition(), forward, pev );
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
			angles.y += ( 180.0 / M_PI ) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			angles.x += ( 180.0 / M_PI ) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}

// Check the FireMaster before actually firing
void CFuncTank::TryFire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
//	ALERT(at_console, "TryFire\n");
	if (UTIL_IsMasterTriggered(m_iszFireMaster, NULL))
	{
//		ALERT(at_console, "Fire is %p, rocketfire %p, tankfire %p\n", this->Fire, CFuncTankRocket::Fire, CFuncTank::Fire);
		Fire( barrelEnd, forward, pevAttacker );
	}
//	else
//		m_fireLast = 0;
}

// Fire targets and spawn sprites
void CFuncTank::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
//	ALERT(at_console, "FuncTank::Fire\n");
	if( m_fireLast != 0 )
	{
		if( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteSmoke ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0, 20.0 ) );
			pSprite->SetTransparency( kRenderTransAlpha, (int)pev->rendercolor.x, (int)pev->rendercolor.y, (int)pev->rendercolor.z, 255, kRenderFxNone );
			pSprite->pev->velocity.z = RANDOM_FLOAT( 40, 80 );
			pSprite->SetScale( m_spriteScale );
		}
		if( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteFlash ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 60 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->AbsoluteNextThink( pSprite->m_fNextThink + 0.1 );
		}

		//LRC
		if (m_iszLocusFire)
		{
			FireTargets( STRING(m_iszLocusFire), m_pFireProxy, this, USE_TOGGLE, 0 );
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
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
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
	CFuncTank::Activate();
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

	CBaseEntity	*pEntity;

	pEntity = UTIL_FindEntityByTargetname( NULL, STRING(pev->message) );
	while ( pEntity )
	{
		// Found the laser
		if ( FClassnameIs( pEntity->pev, "env_laser" ) )
		{
			m_pLaser = (CLaser *)pEntity;
			break;
		}
		else
			pEntity = UTIL_FindEntityByTargetname( pEntity, STRING(pev->message) );
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
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0;
				m_pLaser->FireAtPoint( barrelEnd, tr );

				//LRC - tripbeams
				CBaseEntity* pTrip;
				if (!FStringNull(m_pLaser->pev->target) && (pTrip = m_pLaser->GetTripEntity( &tr )) != NULL)
					FireTargets(STRING(m_pLaser->pev->target), pTrip, m_pLaser, USE_TOGGLE, 0);

				m_pLaser->DontThink();
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
	virtual void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
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
#define SF_TANKCONTROLS_NO_USE	1
#define SF_TANKCONTROLS_VISIBLE 2

LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls );

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] = 
{
//	DEFINE_FIELD( CFuncTankControls, m_pTank, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankControls, m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTankControls, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankControls, m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTankControls, m_iCrosshair, FIELD_INTEGER ), //LRC
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity );


void CFuncTankControls :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "crosshair"))
{
		m_iCrosshair = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

int CFuncTankControls::ObjectCaps( void ) 
{ 
	if (pev->spawnflags & SF_TANKCONTROLS_NO_USE)
		return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
	else
	return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_IMPULSE_USE; 
}

//LRC- copied here from FuncTank.
BOOL CFuncTankControls :: OnControls( entvars_t *pevTest )
{
//	if ( !(pev->spawnflags & SF_TANK_CANCONTROL) )
//		return FALSE;

	Vector offset = pevTest->origin - pev->origin;

	if (pev->frags == -1)
	{
//		ALERT(at_console, "TANK OnControls: TRUE(full tolerance)\n");
		return TRUE;
	}

	if (m_pMoveWith)
	{
		if ( ((m_vecControllerUsePos + m_pMoveWith->pev->origin) - pevTest->origin).Length() <= pev->frags )
		{
//			ALERT(at_console, "TANK OnControls: TRUE(movewith)\n");
			return TRUE;
		}
	}
	else if ( (m_vecControllerUsePos - pevTest->origin).Length() <= pev->frags )
	{
//		ALERT(at_console, "TANK OnControls: TRUE\n");
		return TRUE;
	}

//	ALERT(at_console, "TANK OnControls: FALSE\n");

	return FALSE;
}

void CFuncTankControls::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
// LRC- rewritten to allow TankControls to be the thing that handles the relationship
// between the player and one or more faithful tanks.
	CBaseEntity *tryTank = NULL;

//	ALERT(at_console,"controls %p triggered by \"%s\" %p\n", this, STRING(pCaller->pev->classname), pCaller);

	if ( !m_pController && useType != USE_OFF )
	{
		// if not activated by a player, don't work.
		if (!pActivator || !(pActivator->IsPlayer()))
			return;
		// if I've already got a controller, or the player's already using
		// another controls, then forget it.
		if (m_active != FALSE || ((CBasePlayer*)pActivator)->m_pTank != 0)
			return;

		//LRC- Now uses FindEntityByTargetname, so that aliases work.
		while( ( tryTank = UTIL_FindEntityByTargetname( tryTank, STRING( pev->target ) ) ) )
		{
			if (!strncmp( STRING(tryTank->pev->classname), "func_tank", 9 ))
			{
				if (((CFuncTank*)tryTank)->StartControl((CBasePlayer*)pActivator, this))
				{
					//ALERT(at_console,"started controlling tank %s\n",STRING(tryTank->pev->targetname));
					// here's a tank we can control. Phew.
					m_active = TRUE;
				}
			}
		}
		if (m_active)
		{
			// we found at least one tank to use, so holster player's weapon
			m_pController = (CBasePlayer*)pActivator;
			m_pController->m_pTank = this;
			if ( m_pController->m_pActiveItem )
			{
				m_pController->m_pActiveItem->Holster();
				m_pController->pev->weaponmodel = 0;
				m_pController->pev->viewmodel = 0; 
			}

			//LRC - allow tank crosshairs
			if (m_iCrosshair)
				m_pController->m_iHideHUD |= ( HIDEHUD_CUSTOMCROSSHAIR | HIDEHUD_WEAPONS );
			else
				m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;

			// remember where the player's standing, so we can tell when he walks away
			if (m_pMoveWith)
				m_vecControllerUsePos = m_pController->pev->origin - m_pMoveWith->pev->origin;
			else
				m_vecControllerUsePos = m_pController->pev->origin;
			//ALERT( at_console, "TANK controls activated\n");
		}
	}
	else if (m_pController && useType != USE_ON)
	{
	// player stepped away or died, most likely.
		//ALERT(at_console, "TANK controls deactivated\n");

		//LRC- Now uses FindEntityByTargetname, so that aliases work.
		while( ( tryTank = UTIL_FindEntityByTargetname( tryTank, STRING( pev->target ) ) ) )
		{
			if( FClassnameIs( tryTank->pev, "func_tank" ) ||
			FClassnameIs( tryTank->pev, "func_tanklaser" ) ||
			FClassnameIs( tryTank->pev, "func_tankmortar" ) ||
			FClassnameIs( tryTank->pev, "func_tankrocket" ) )
			{
				// this is a tank we're controlling.
				((CFuncTank*)tryTank)->StopControl(this);
			}
		}
//		if (!m_pController)
//			return;

		// bring back player's weapons
		if ( m_pController->m_pActiveItem )
			m_pController->m_pActiveItem->Deploy();

		m_pController->m_iHideHUD &= ~ (HIDEHUD_CUSTOMCROSSHAIR | HIDEHUD_WEAPONS);
		m_pController->m_pTank = NULL;				

		m_pController = NULL;
		m_active = false;
	}


//	if ( m_pTank )
//		m_pTank->Use( pActivator, pCaller, useType, value );

//	ASSERT( m_pTank != NULL );	// if this fails,  most likely means save/restore hasn't worked properly
}


/* LRC- no need to set up m_pTank any more...
void CFuncTankControls::Think( void )
{
	CBaseEntity *pTarget = NULL;

	do
	{
		pTarget = UTIL_FindEntityByClassname( pTarget, STRING(pev->target) );
	} while ( pTarget && strncmp( STRING(pTarget->v.classname), "func_tank", 9 ) );


	if ( !pTarget )
	{
		ALERT( at_console, "No tank %s\n", STRING( pev->target ) );
		return;
	}
	else
	{
		m_pTank = (CFuncTank*)pTarget;
		do 
		{
			pTarget = UTIL_FindEntityByClassname( pTarget, STRING(pev->target) );
		} while ( pTarget && strncmp( STRING(pTarget->v.classname), "func_tank", 9 ) );

		if ( pTarget )
		{
			m_pTank2 = (CFuncTank*)pTarget;
			ALERT( at_console, "Got second tank %s\n", STRING(pev->target) );
		}
}
}*/

void CFuncTankControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	if (!(pev->spawnflags & SF_TANKCONTROLS_VISIBLE))
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	if (pev->frags == 0) //LRC- in case the level designer didn't set it.
		pev->frags = 30;

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( this, pev->origin );

//LRC	SetNextThink( 0.3 );	// After all the func_tanks have spawned

	CBaseEntity::Spawn();
}

//============================================================================
//LRC - Scripted Tank Sequence
//============================================================================

LINK_ENTITY_TO_CLASS( scripted_tanksequence, CTankSequence );

TYPEDESCRIPTION	CTankSequence::m_SaveData[] = 
{
	DEFINE_FIELD( CTankSequence, m_iszEntity, FIELD_STRING ),
	DEFINE_FIELD( CTankSequence, m_iszEnemy, FIELD_STRING ),
	DEFINE_FIELD( CTankSequence, m_iUntil, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_fDuration, FIELD_FLOAT ),
	DEFINE_FIELD( CTankSequence, m_iTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_iShoot, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_iActive, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_iControllable, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_iLaserSpot, FIELD_INTEGER ),
	DEFINE_FIELD( CTankSequence, m_pTank, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE( CTankSequence, CBaseEntity );

int	CTankSequence :: ObjectCaps( void ) 
{
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION);
}

void CTankSequence :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "m_iUntil"))
	{
		m_iUntil = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iTurn"))
	{
		m_iTurn = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iShoot"))
	{
		m_iShoot = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iActive"))
	{
		m_iActive = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iControllable"))
	{
		m_iControllable = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iLaserSpot"))
	{
		m_iLaserSpot = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEntity"))
	{
		m_iszEntity = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "m_iszEnemy"))
	{
		m_iszEnemy = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CTankSequence :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (!ShouldToggle(useType)) return;

	if (GetState() == STATE_OFF)
	{
		// take control of the tank, start the sequence

		CBaseEntity* pEnt = UTIL_FindEntityByTargetname(NULL, STRING(m_iszEntity));
		if (!pEnt || !FStrEq(STRING(pEnt->pev->classname), "func_tank"))
		{
			ALERT(at_error, "Invalid or missing tank \"%s\" for scripted_tanksequence!\n", STRING(m_iszEntity));
			return;
		}
		CFuncTank* pTank = (CFuncTank*)pEnt;

		// check whether it's being controlled by another sequence
		if (pTank->m_pSequence)
			return;

		// check whether it's being controlled by the player
		if (pTank->m_pControls)
		{
			if (pev->spawnflags & SF_TSEQ_DUMPPLAYER)
			{
				pTank->StopControl( pTank->m_pControls );
			}
			else if (!FBitSet(pev->spawnflags, SF_TSEQ_DUMPPLAYER))
			{
				//ALERT(at_debug, "scripted_tanksequence can't take tank away from player\n");
				return;
			}
		}

		//passed all the tests, so we can now take control of it.
		if (m_iTurn == TSEQ_TURN_ENEMY)
		{
			CBaseEntity *pEnemy;
			if (m_iszEnemy)
				pEnemy = UTIL_FindEntityGeneric(STRING(m_iszEnemy), pTank->pev->origin, pTank->m_maxRange );
			else
				pEnemy = pTank->BestVisibleEnemy();

			if (pEnemy)
			{
				pTank->m_pSequenceEnemy = pEnemy;
				pTank->StartSequence(this);
			}
		}
		else
		{
			pTank->StartSequence(this);
		}

		if (m_iShoot == TSEQ_SHOOT_ALWAYS)
			pTank->pev->spawnflags |= SF_TANK_SEQFIRE;
		else
			pTank->pev->spawnflags &= ~SF_TANK_SEQFIRE;

		m_pTank = pTank;
		if (m_fDuration)
		{
			SetThink(&CTankSequence :: TimeOutThink );
			SetNextThink( m_fDuration );
		}
	}
	else // don't check UNTIL_TRIGGER - any UNTIL value can be prematurely ended.
	{
		//disable any other end conditions
		DontThink();

		// release control of the tank
		StopSequence();
	}
}

void CTankSequence :: FacingNotify()
{
	if (m_iUntil == TSEQ_UNTIL_FACING)
	{
		SetThink(&CTankSequence :: EndThink );
		SetNextThink( 0 );
	}
	else if (m_iShoot == TSEQ_SHOOT_FACING)
		m_pTank->pev->spawnflags |= SF_TANK_SEQFIRE;
}

void CTankSequence :: DeadEnemyNotify()
{
	if (m_iUntil == TSEQ_UNTIL_DEATH)
	{
		SetThink(&CTankSequence :: EndThink );
		SetNextThink( 0 );
	}
//	else
//		m_pTank->pev->spawnflags &= ~SF_TANK_SEQFIRE; // if the enemy's dead, stop firing
}

void CTankSequence :: EndThink()
{
	//the sequence has expired. Release control of the tank.
	StopSequence();
	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
}

void CTankSequence :: TimeOutThink()
{
	//the sequence has timed out. Release control of the tank.
	StopSequence();
	if (!FStringNull(pev->netname))
		FireTargets(STRING(pev->netname), this, this, USE_TOGGLE, 0);
}

void CTankSequence :: StopSequence()
{
	if (!m_pTank)
	{
		ALERT(at_error, "TankSeq: StopSequence with no tank!\n");
		return; // this shouldn't happen. Just insurance...
	}
	
	// if we're doing "shoot at end", fire that shot now.
	if (m_iShoot == TSEQ_SHOOT_ONCE)
	{
		m_pTank->m_fireLast = gpGlobals->time - 1/m_pTank->m_fireRate; // exactly one shot.
		Vector forward;
		UTIL_MakeVectorsPrivate( m_pTank->pev->angles, forward, NULL, NULL );	
		m_pTank->TryFire( m_pTank->BarrelPosition(), forward, m_pTank->pev );
	}

	if (m_iLaserSpot)
	{
		if (m_pTank->pev->spawnflags & SF_TANK_LASERSPOT && m_iLaserSpot != TSEQ_FLAG_ON)
		{
			m_pTank->pev->spawnflags &= ~SF_TANK_LASERSPOT;
		}
		else if (!FBitSet(m_pTank->pev->spawnflags, SF_TANK_LASERSPOT) && m_iLaserSpot != TSEQ_FLAG_OFF)
		{
			m_pTank->pev->spawnflags |= SF_TANK_LASERSPOT;
		}
	}

	if (m_iControllable)
	{
		if (m_pTank->pev->spawnflags & SF_TANK_CANCONTROL && m_iControllable != TSEQ_FLAG_ON)
		{
			m_pTank->pev->spawnflags &= ~SF_TANK_CANCONTROL;
		}
		else if (!(m_pTank->pev->spawnflags & SF_TANK_CANCONTROL) && m_iControllable != TSEQ_FLAG_OFF)
		{
			m_pTank->pev->spawnflags |= SF_TANK_CANCONTROL;
		}
	}

	m_pTank->StopSequence();

	if (!FBitSet(pev->spawnflags, SF_TSEQ_REPEATABLE))
		UTIL_Remove( this );

	if (m_pTank->IsActive() && (m_iActive == TSEQ_FLAG_OFF || m_iActive == TSEQ_FLAG_TOGGLE))
	{
		m_pTank->TankDeactivate();
		if (m_pTank->m_pSpot) m_pTank->m_pSpot->Suspend(-1);
	}
	else if (!m_pTank->IsActive() && (m_iActive == TSEQ_FLAG_ON || m_iActive == TSEQ_FLAG_TOGGLE))
	{
		m_pTank->TankActivate();
		if (m_pTank->m_pSpot) m_pTank->m_pSpot->Revive();
	}

	m_pTank = NULL;
}
