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
//=========================================================
// Hornets
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"
#include	"gamerules.h"

int iHornetTrail;
int iHornetPuff;

LINK_ENTITY_TO_CLASS( hornet, CHornet )

//=========================================================
// Save/Restore
//=========================================================
TYPEDESCRIPTION	CHornet::m_SaveData[] =
{
	DEFINE_FIELD( CHornet, m_flStopAttack, FIELD_TIME ),
	DEFINE_FIELD( CHornet, m_iHornetType, FIELD_INTEGER ),
	DEFINE_FIELD( CHornet, m_flFlySpeed, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CHornet, CBaseMonster )

//=========================================================
// don't let hornets gib, ever.
//=========================================================
int CHornet::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// filter these bits a little.
	bitsDamageType &= ~( DMG_ALWAYSGIB );
	bitsDamageType |= DMG_NEVERGIB;

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
//=========================================================
void CHornet::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->health = 1;// weak!

	if( g_pGameRules->IsMultiplayer() )
	{
		// hornets don't live as long in multiplayer
		m_flStopAttack = gpGlobals->time + 3.5f;
	}
	else
	{
		m_flStopAttack	= gpGlobals->time + 5.0f;
	}

	m_flFieldOfView = 0.9f; // +- 25 degrees

	if( RANDOM_LONG( 1, 5 ) <= 2 )
	{
		m_iHornetType = HORNET_TYPE_RED;
		m_flFlySpeed = HORNET_RED_SPEED;
	}
	else
	{
		m_iHornetType = HORNET_TYPE_ORANGE;
		m_flFlySpeed = HORNET_ORANGE_SPEED;
	}

	SET_MODEL( ENT( pev ), "models/hornet.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetTouch( &CHornet::DieTouch );
	SetThink( &CHornet::StartTrack );

	/*edict_t *pSoundEnt = pev->owner;
	if( !pSoundEnt )
		pSoundEnt = edict();*/

	if( !FNullEnt( pev->owner ) && ( pev->owner->v.flags & FL_CLIENT ) )
	{
		pev->dmg = gSkillData.plrDmgHornet;
	}
	else
	{
		// no real owner, or owner isn't a client. 
		pev->dmg = gSkillData.monDmgHornet;
	}

	SetNextThink( 0.1f );
	ResetSequenceInfo();
}

void CHornet::Precache()
{
	PRECACHE_MODEL( "models/hornet.mdl" );

	PRECACHE_SOUND( "agrunt/ag_fire1.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire2.wav" );
	PRECACHE_SOUND( "agrunt/ag_fire3.wav" );

	PRECACHE_SOUND( "hornet/ag_buzz1.wav" );
	PRECACHE_SOUND( "hornet/ag_buzz2.wav" );
	PRECACHE_SOUND( "hornet/ag_buzz3.wav" );

	PRECACHE_SOUND( "hornet/ag_hornethit1.wav" );
	PRECACHE_SOUND( "hornet/ag_hornethit2.wav" );
	PRECACHE_SOUND( "hornet/ag_hornethit3.wav" );

	iHornetPuff = PRECACHE_MODEL( "sprites/muz1.spr" );
	iHornetTrail = PRECACHE_MODEL( "sprites/laserbeam.spr" );
}

//=========================================================
// hornets will never get mad at each other, no matter who the owner is.
//=========================================================
int CHornet::IRelationship( CBaseEntity *pTarget )
{
	if( pTarget->pev->modelindex == pev->modelindex )
	{
		return R_NO;
	}

	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// ID's Hornet as their owner
//=========================================================
int CHornet::Classify( void )
{
	if (m_iClass) return m_iClass;
	if( pev->owner && pev->owner->v.flags & FL_CLIENT )
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return	CLASS_ALIEN_BIOWEAPON;
}

//=========================================================
// StartTrack - starts a hornet out tracking its target
//=========================================================
void CHornet::StartTrack( void )
{
	IgniteTrail();

	SetTouch( &CHornet::TrackTouch );
	SetThink( &CHornet::TrackTarget );

	SetNextThink( 0.1f );
}

//=========================================================
// StartDart - starts a hornet out just flying straight.
//=========================================================
void CHornet::StartDart( void )
{
	IgniteTrail();

	SetTouch( &CHornet::DartTouch );

	SetThink(&CHornet :: SUB_Remove );
	SetNextThink( 4.0f );
}

void CHornet::IgniteTrail( void )
{
/*

  ted's suggested trail colors:

r161
g25
b97

r173
g39
b14

old colors
		case HORNET_TYPE_RED:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		case HORNET_TYPE_ORANGE:
			WRITE_BYTE( 0 );   // r, g, b
			WRITE_BYTE( 100 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			break;

*/
	// trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );	// entity
		WRITE_SHORT( iHornetTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 2 );  // width

		switch( m_iHornetType )
		{
		case HORNET_TYPE_RED:
			WRITE_BYTE( 179 );   // r, g, b
			WRITE_BYTE( 39 );   // r, g, b
			WRITE_BYTE( 14 );   // r, g, b
			break;
		case HORNET_TYPE_ORANGE:
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 128 );   // r, g, b
			WRITE_BYTE( 0 );   // r, g, b
			break;
		}

		WRITE_BYTE( 128 );	// brightness
	MESSAGE_END();
}

//=========================================================
// Hornet is flying, gently tracking target
//=========================================================
void CHornet::TrackTarget( void )
{
	Vector	vecFlightDir;
	Vector	vecDirToEnemy;
	float	flDelta;

	StudioFrameAdvance();

	if( gpGlobals->time > m_flStopAttack )
	{
		SetTouch( NULL );
		SetThink(&CHornet::SUB_Remove );
		SetNextThink( 0.1f );
		return;
	}

	// UNDONE: The player pointer should come back after returning from another level
	if( m_hEnemy == 0 )
	{
		// enemy is dead.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy();
	}

	if( m_hEnemy != 0 && FVisible( m_hEnemy ) )
	{
		m_vecEnemyLKP = m_hEnemy->BodyTarget( pev->origin );
	}
	else
	{
		m_vecEnemyLKP = m_vecEnemyLKP + pev->velocity * m_flFlySpeed * 0.1f;
	}

	vecDirToEnemy = ( m_vecEnemyLKP - pev->origin ).Normalize();

	if( pev->velocity.Length() < 0.1f )
		vecFlightDir = vecDirToEnemy;
	else 
		vecFlightDir = pev->velocity.Normalize();

	// measure how far the turn is, the wider the turn, the slow we'll go this time.
	flDelta = DotProduct( vecFlightDir, vecDirToEnemy );

	if( flDelta < 0.5f )
	{
		// hafta turn wide again. play sound
		switch( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
			break;
		}
	}

	if( flDelta <= 0 && m_iHornetType == HORNET_TYPE_RED )
	{
		// no flying backwards, but we don't want to invert this, cause we'd go fast when we have to turn REAL far.
		flDelta = 0.25f;
	}

	pev->velocity = ( vecFlightDir + vecDirToEnemy ).Normalize();

	if( pev->owner && ( pev->owner->v.flags & FL_MONSTER ) )
	{
		// random pattern only applies to hornets fired by monsters, not players. 
		pev->velocity.x += RANDOM_FLOAT( -0.10f, 0.10f );// scramble the flight dir a bit.
		pev->velocity.y += RANDOM_FLOAT( -0.10f, 0.10f );
		pev->velocity.z += RANDOM_FLOAT( -0.10f, 0.10f );
	}

	switch( m_iHornetType )
	{
		case HORNET_TYPE_RED:
			pev->velocity = pev->velocity * ( m_flFlySpeed * flDelta );// scale the dir by the ( speed * width of turn )
			SetNextThink( RANDOM_FLOAT( 0.1f, 0.3f ) );
			break;
		case HORNET_TYPE_ORANGE:
			pev->velocity = pev->velocity * m_flFlySpeed;// do not have to slow down to turn.
			SetNextThink( 0.1f );// fixed think time
			break;
	}

	pev->angles = UTIL_VecToAngles( pev->velocity );

	pev->solid = SOLID_BBOX;

	// if hornet is close to the enemy, jet in a straight line for a half second.
	// (only in the single player game)
	if( m_hEnemy != 0 && !g_pGameRules->IsMultiplayer() )
	{
		if( flDelta >= 0.4f && ( pev->origin - m_vecEnemyLKP ).Length() <= 300 )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( pev->origin.x );	// pos
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( iHornetPuff );		// model
				// WRITE_BYTE( 0 );				// life * 10
				WRITE_BYTE( 2 );				// size * 10
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

			switch( RANDOM_LONG( 0, 2 ) )
			{
			case 0:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz1.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz2.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_buzz3.wav", HORNET_BUZZ_VOLUME, ATTN_NORM );
				break;
			}
			pev->velocity = pev->velocity * 2.0f;
			SetNextThink( 1.0f );

			// don't attack again
			m_flStopAttack = gpGlobals->time;
		}
	}
}

//=========================================================
// Tracking Hornet hit something
//=========================================================
void CHornet::TrackTouch( CBaseEntity *pOther )
{
	if( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{
		// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		return;
	}

	if( IRelationship( pOther ) <= R_NO )
	{
		// hit something we don't want to hurt, so turn around.

		pev->velocity = pev->velocity.Normalize();

		pev->velocity.x *= -1.0f;
		pev->velocity.y *= -1.0f;

		pev->origin = pev->origin + pev->velocity * 4.0f; // bounce the hornet off a bit.
		pev->velocity = pev->velocity * m_flFlySpeed;

		return;
	}

	DieTouch( pOther );
}

void CHornet::DartTouch( CBaseEntity *pOther )
{
	DieTouch( pOther );
}

void CHornet::DieTouch( CBaseEntity *pOther )
{
	if( pOther && pOther->pev->takedamage && pev->owner )
	{
		// do the damage
		switch( RANDOM_LONG( 0, 2 ) )
		{
			// buzz when you plug someone
			case 0:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_hornethit1.wav", 1, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_hornethit2.wav", 1, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( pev ), CHAN_VOICE, "hornet/ag_hornethit3.wav", 1, ATTN_NORM );
				break;
		}

		pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_BULLET );
	}

	pev->modelindex = 0;// so will disappear for the 0.1 secs we wait until NEXTTHINK gets rid
	pev->solid = SOLID_NOT;

	SetThink( &CHornet::SUB_Remove );
	SetNextThink( 1.0f );// stick around long enough for the sound to finish!
}

// HORNET HIVE

#define SF_HORNETNEST_OFF 1

#define HORNETHIVE_RANGE 640.0f // Player detection range
#define HORNETHIVE_FIRE_RATE 0.5f // Fire hornets every n seconds

class CHornetHive : public CPointEntity
{
public:
	void Precache();
	void Spawn();
	void Think();
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	STATE GetState() {
		if (pev->spawnflags & SF_HORNETNEST_OFF)
			return STATE_OFF;
		return STATE_ON;
	}
	bool IsActive() {
		return GetState() == STATE_ON;
	}
};

LINK_ENTITY_TO_CLASS(env_hornethive, CHornetHive)

void CHornetHive::Precache()
{
	UTIL_PrecacheOther("hornet");

	PRECACHE_SOUND("agrunt/ag_fire1.wav");
	PRECACHE_SOUND("agrunt/ag_fire2.wav");
	PRECACHE_SOUND("agrunt/ag_fire3.wav");
}

void CHornetHive::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetNextThink(0.3f);
}

void CHornetHive::Think()
{
	if (!IsActive())
		return;

	SetNextThink(0.5f);

	edict_t* pPlayer = FIND_CLIENT_IN_PVS(edict());
	if (FNullEnt(pPlayer))
	{
		SetNextThink(2.0f);
		return;
	}

	const Vector nestPosition = pev->origin;
	const Vector targetPosition = pPlayer->v.origin + pPlayer->v.view_ofs;
	const float range = (targetPosition - nestPosition).Length();
	if (range > HORNETHIVE_RANGE)
		return;

	TraceResult tr;
	UTIL_TraceLine(nestPosition, targetPosition, dont_ignore_monsters, edict(), &tr);

	if (tr.flFraction == 1.0f || tr.pHit == pPlayer)
	{
		Vector enemyPosition = pPlayer->v.origin;
		if (pPlayer->v.velocity != g_vecZero)
		{
			enemyPosition = enemyPosition - pPlayer->v.velocity * RANDOM_FLOAT(-0.05f, 0.0f);
		}

		Vector vecDirToEnemy = (enemyPosition - nestPosition).Normalize();

		CHornet* pHornet = (CHornet*)CBaseEntity::Create("hornet", nestPosition, UTIL_VecToAngles(vecDirToEnemy), edict());
		pHornet->pev->velocity = vecDirToEnemy * 300.0f;

		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "agrunt/ag_fire1.wav", 1.0, ATTN_NORM, 0, 100);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "agrunt/ag_fire2.wav", 1.0, ATTN_NORM, 0, 100);
			break;
		case 2:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "agrunt/ag_fire3.wav", 1.0, ATTN_NORM, 0, 100);
			break;
		}

		pHornet->m_hEnemy = CBaseEntity::Instance(pPlayer);

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(pev->origin.x);	// pos
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(iHornetPuff);		// model
		WRITE_BYTE(6);				// size * 10
		WRITE_BYTE(128);			// brightness
		MESSAGE_END();

		SetNextThink(HORNETHIVE_FIRE_RATE);
	}
}

void CHornetHive::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType))
		return;
	if (IsActive())
	{
		pev->spawnflags |= SF_HORNETNEST_OFF;
	}
	else
	{
		pev->spawnflags &= SF_HORNETNEST_OFF;
		SetNextThink(0.1f);
	}
}

#define HORNET_COMFORT_DIST 16
#define HORNET_FLY_MAXSPEED 120.0f
#define HORNET_FLY_ACCEL 40.0f
#define HORNET_FRAMETIME 0.1f
#define HORNET_FLY_VERTICALSPEED (HORNET_COMFORT_DIST * 0.5f)

class CEnvHornet : public CBaseAnimating
{
public:
	void Precache();
	void Spawn();

	void EXPORT FlyThink();
	void EXPORT FallThink();

	void EXPORT HornetUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

	STATE GetState() {
		if (pev->movetype == MOVETYPE_FLY)
			return STATE_ON;
		return STATE_OFF;
	}

	void PlayBuzzSound();

	Vector m_center;
	Vector m_wanderTarget;
	int m_iHornetType;
	float m_verticalSpeed;
	BOOL m_wanderingOff;

	float m_nextTrailTime;

	int iHornetTrail;
	int iHornetPuff;

	virtual int Save(CSave& save);
	virtual int Restore(CRestore& restore);
	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS(env_hornet, CEnvHornet)

TYPEDESCRIPTION	CEnvHornet::m_SaveData[] =
{
	DEFINE_FIELD(CEnvHornet, m_center, FIELD_VECTOR),
	DEFINE_FIELD(CEnvHornet, m_wanderTarget, FIELD_VECTOR),
	DEFINE_FIELD(CEnvHornet, m_iHornetType, FIELD_INTEGER),
	DEFINE_FIELD(CEnvHornet, m_verticalSpeed, FIELD_FLOAT),
	DEFINE_FIELD(CEnvHornet, m_wanderingOff, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CEnvHornet, CBaseAnimating)

void CEnvHornet::Precache()
{
	PRECACHE_MODEL("models/hornet.mdl");

	PRECACHE_SOUND("hornet/ag_buzz1.wav");
	PRECACHE_SOUND("hornet/ag_buzz2.wav");
	PRECACHE_SOUND("hornet/ag_buzz3.wav");

	iHornetTrail = PRECACHE_MODEL("sprites/laserbeam.spr");
	iHornetPuff = PRECACHE_MODEL("sprites/muz1.spr");
	m_nextTrailTime = gpGlobals->time + 0.1f;
}

void CEnvHornet::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	SET_MODEL(ENT(pev), "models/hornet.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	m_center = pev->origin;
	pev->speed = HORNET_FLY_MAXSPEED;
	m_verticalSpeed = RANDOM_LONG(0, 1) ? HORNET_FLY_VERTICALSPEED : -HORNET_FLY_VERTICALSPEED;

	m_iHornetType = RANDOM_LONG(0, 1);

	SetUse(&CEnvHornet::HornetUse);
	SetThink(&CEnvHornet::FlyThink);
	SetNextThink(0.1f);
	ResetSequenceInfo();
}

void CEnvHornet::FlyThink()
{
	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
	{
		SetNextThink(1.0f);
		return;
	}

	if (RANDOM_LONG(0, 15) == 0)
		PlayBuzzSound();

	UTIL_MakeVectors(pev->angles);

	const Vector vecToCenter = (m_center - pev->origin);
	const float wanderingDist = vecToCenter.Length2D();

	if (wanderingDist < HORNET_COMFORT_DIST)
	{
		if (m_wanderingOff)
		{
			pev->avelocity.y = 0;
			m_wanderingOff = FALSE;
		}
		pev->speed = UTIL_Approach(HORNET_FLY_MAXSPEED, pev->speed, HORNET_FLY_ACCEL * HORNET_FRAMETIME);
		pev->velocity = gpGlobals->v_forward * pev->speed;
	}
	else
	{
		if (!m_wanderingOff)
		{
			m_wanderingOff = TRUE;
			m_wanderTarget = m_center;

			pev->speed = HORNET_FLY_MAXSPEED * RANDOM_FLOAT(0.5f, 0.6f);
			pev->yaw_speed = RANDOM_FLOAT(250, 300);
		}

		const Vector vecToWanderTarget = (m_wanderTarget - pev->origin);
		const float targetYaw = UTIL_VecToYaw(vecToWanderTarget);

		const float currentYaw = UTIL_AngleMod(pev->angles.y);
		const float yawDiff = UTIL_AngleDiff(targetYaw, currentYaw);
		if (yawDiff < 0)
		{
			pev->avelocity.y = Q_min(-pev->yaw_speed, yawDiff);
		}
		else
		{
			pev->avelocity.y = Q_max(pev->yaw_speed, yawDiff);
		}
		pev->velocity = gpGlobals->v_forward * pev->speed;

		//ALERT(at_console, "Speed :%f. Dist to target: %f\n", pev->velocity.Length2D(), vecToWanderTarget.Length2D());
	}

	float targetVerticalSpeed;
	if (pev->origin.z >= m_center.z + HORNET_COMFORT_DIST)
	{
		targetVerticalSpeed = -HORNET_FLY_VERTICALSPEED;
	}
	else if (pev->origin.z < m_center.z - HORNET_COMFORT_DIST)
	{
		targetVerticalSpeed = HORNET_FLY_VERTICALSPEED;
	}
	else if (m_verticalSpeed >= 0)
	{
		targetVerticalSpeed = HORNET_FLY_VERTICALSPEED;
	}
	else
	{
		targetVerticalSpeed = -HORNET_FLY_VERTICALSPEED;
	}

	m_verticalSpeed = UTIL_Approach(targetVerticalSpeed, m_verticalSpeed, HORNET_FLY_VERTICALSPEED * HORNET_FRAMETIME);

	pev->velocity.z = m_verticalSpeed;

	if (gpGlobals->time > m_nextTrailTime)
	{
		const int life = 10;
		m_nextTrailTime = gpGlobals->time + life * 0.1f;

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT(iHornetTrail);	// model
		WRITE_BYTE(life); // life
		WRITE_BYTE(2);  // width

		switch (m_iHornetType)
		{
		case HORNET_TYPE_RED:
			WRITE_BYTE(179);   // r, g, b
			WRITE_BYTE(39);   // r, g, b
			WRITE_BYTE(14);   // r, g, b
			break;
		case HORNET_TYPE_ORANGE:
			WRITE_BYTE(255);   // r, g, b
			WRITE_BYTE(128);   // r, g, b
			WRITE_BYTE(0);   // r, g, b
			break;
		}

		WRITE_BYTE(128);	// brightness
		MESSAGE_END();
	}

	StudioFrameAdvance();
	SetNextThink(HORNET_FRAMETIME);
}

void CEnvHornet::HornetUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType))
		return;
	if (GetState() == STATE_ON)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_KILLBEAM);
		WRITE_SHORT(entindex());
		MESSAGE_END();

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPRITE);
		WRITE_COORD(pev->origin.x);	// pos
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(iHornetPuff);		// model
		WRITE_BYTE(2);				// size * 10
		WRITE_BYTE(128);			// brightness
		MESSAGE_END();

		pev->gravity = 0.5f;
		pev->movetype = MOVETYPE_TOSS;

		pev->frame = 0;
		ResetSequenceInfo();
		pev->framerate = 0;

		pev->velocity = g_vecZero;

		UTIL_SetOrigin(this, pev->origin);
		UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

		SetNextThink(0.0f);
		SetThink(&CEnvHornet::FallThink);

		SetUse(NULL);
	}
}

void CEnvHornet::FallThink()
{
	SetNextThink(0.1f);

	if (pev->flags & FL_ONGROUND)
	{
		SetThink(&CBaseEntity::SUB_StartFadeOut);
	}
}

void CEnvHornet::PlayBuzzSound()
{
	const float volume = 0.6f;
	const int pitch = RANDOM_LONG(85, 95);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), volume, "hornet/ag_buzz1.wav", volume, ATTN_STATIC, 0, pitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), volume, "hornet/ag_buzz2.wav", volume, ATTN_STATIC, 0, pitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), volume, "hornet/ag_buzz3.wav", volume, ATTN_STATIC, 0, pitch);
		break;
	}
}