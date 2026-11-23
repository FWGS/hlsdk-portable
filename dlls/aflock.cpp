/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

#define AFLOCK_MAX_RECRUIT_RADIUS 1024
#define AFLOCK_FLY_SPEED 125
#define AFLOCK_TURN_RATE 75
#define AFLOCK_ACCELERATE 10
#define AFLOCK_CHECK_DIST 192
#define AFLOCK_TOO_CLOSE 100
#define AFLOCK_TOO_FAR 256

//=========================================================
//=========================================================
class CFlockingFlyerFlock : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void SpawnFlock();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	// Sounds are shared by the flock
	static void PrecacheFlockSounds();
	int m_cFlockSize;
	float m_flFlockRadius;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

//=========================================================
//=========================================================
class CFlockingFlyer : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SpawnCommonCode();
	void EXPORT IdleThink();
	void BoidAdvanceFrame();
	void EXPORT FormFlock();
	void EXPORT Start();
	void EXPORT FlockLeaderThink();
	void EXPORT FlockFollowerThink();
	void EXPORT FallHack();
	void MakeSound();
	void AlertFlock();
	void SpreadFlock();
	void SpreadFlock2();
	void Killed( entvars_t* attacker, int gib ) override;
	void Poop();
	BOOL FPathBlocked();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int IsLeader() { return m_pSquadLeader == this; }
	int InSquad() { return m_pSquadLeader != NULL; }
	int SquadCount();
	void SquadRemove( CFlockingFlyer* remove );
	void SquadUnlink();
	void SquadAdd( CFlockingFlyer* add );
	void SquadDisband();
	CFlockingFlyer* m_pSquadLeader;
	CFlockingFlyer* m_pSquadNext;
	BOOL m_fTurning; // is this boid turning?
	BOOL m_fCourseAdjust; // followers set this flag TRUE to override flocking while they avoid something
	BOOL m_fPathBlocked; // TRUE if there is an obstacle ahead
	Vector m_vecReferencePoint; // last place we saw leader
	Vector m_vecAdjustedVelocity; // adjusted velocity (used when fCourseAdjust is TRUE)
	float m_flGoalSpeed;
	float m_flLastBlockedTime;
	float m_flFakeBlockedTime;
	float m_flAlertTime;
	float m_flFlockNextSoundTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_flyer, CFlockingFlyer );
LINK_ENTITY_TO_CLASS( monster_flyer_flock, CFlockingFlyerFlock );

TYPEDESCRIPTION CFlockingFlyerFlock::m_SaveData[] =
{
	DEFINE_FIELD( CFlockingFlyerFlock, m_cFlockSize, FIELD_INTEGER ),
	DEFINE_FIELD( CFlockingFlyerFlock, m_flFlockRadius, FIELD_FLOAT )
};

TYPEDESCRIPTION CFlockingFlyer::m_SaveData[] =
{
	DEFINE_FIELD( CFlockingFlyer, m_pSquadLeader, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFlockingFlyer, m_pSquadNext, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFlockingFlyer, m_fTurning, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFlockingFlyer, m_fCourseAdjust, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFlockingFlyer, m_fPathBlocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFlockingFlyer, m_vecReferencePoint, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CFlockingFlyer, m_vecAdjustedVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CFlockingFlyer, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFlockingFlyer, m_flLastBlockedTime, FIELD_TIME ),
	DEFINE_FIELD( CFlockingFlyer, m_flFakeBlockedTime, FIELD_TIME ),
	DEFINE_FIELD( CFlockingFlyer, m_flAlertTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CFlockingFlyerFlock, CBaseMonster );
IMPLEMENT_SAVERESTORE( CFlockingFlyer, CBaseMonster );

//=========================================================
//=========================================================
void CFlockingFlyerFlock::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "iflocksize" ) )
	{
		m_cFlockSize = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "flflockradius" ) )
	{
		m_flFlockRadius = (float)atof( data->szValue );
		data->fHandled = TRUE;
	}
}

void CFlockingFlyerFlock::PrecacheFlockSounds()
{
	PRECACHE_SOUND( "boid/boid_alert1.wav" );
	PRECACHE_SOUND( "boid/boid_alert2.wav" );
	PRECACHE_SOUND( "boid/boid_idle1.wav" );
	PRECACHE_SOUND( "boid/boid_idle2.wav" );
}

//=========================================================
//=========================================================
void CFlockingFlyerFlock::Precache()
{
	//PRECACHE_MODEL("models/aflock.mdl");		
	PRECACHE_MODEL( "models/pigeon.mdl" );
	PrecacheFlockSounds();
}

//=========================================================
//=========================================================
void CFlockingFlyerFlock::Spawn()
{
	Precache();
	SpawnFlock();
	REMOVE_ENTITY( edict() );		// dump the spawn ent
}

void CFlockingFlyerFlock::SpawnFlock()
{
	float radius = m_flFlockRadius;
	CFlockingFlyer* leader = NULL;
	for ( int i = 0; i < m_cFlockSize; ++i )
	{
		CFlockingFlyer* boid = GetClassPtr( (CFlockingFlyer*)NULL );
		if ( !leader )
		{
			leader = boid;
			leader->m_pSquadLeader = leader;
			leader->m_pSquadNext = NULL;
		}
		Vector spot;
		spot.x = RANDOM_FLOAT( -radius, radius );
		spot.y = RANDOM_FLOAT( -radius, radius );
		spot.z = RANDOM_FLOAT( 0, 16 );
		spot = pev->origin + spot;
		UTIL_SetOrigin( boid->pev, spot );
		boid->pev->movetype = MOVETYPE_FLY;
		boid->SpawnCommonCode();
		boid->pev->flags &= ~FL_ONGROUND;
		boid->pev->velocity = g_vecZero;
		boid->pev->angles = pev->angles;
		boid->pev->frame = 0;
		boid->pev->nextthink = gpGlobals->time + 0.2;
		boid->SetThink( &CFlockingFlyer::IdleThink );
		if ( boid != leader )
			leader->SquadAdd( boid );
	}
}

//=========================================================
//=========================================================
void CFlockingFlyer::Precache()
{
	//PRECACHE_MODEL("models/aflock.mdl");
	PRECACHE_MODEL( "models/pigeon.mdl" );
	CFlockingFlyerFlock::PrecacheFlockSounds();
}

//=========================================================
//=========================================================
void CFlockingFlyer::SpawnCommonCode()
{
	pev->deadflag = DEAD_NO;
	pev->classname = MAKE_STRING( "monster_flyer" );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->takedamage = DAMAGE_NO;
	pev->health = 1;
	m_fPathBlocked = FALSE; // obstacles will be detected
	m_flFieldOfView = 0.2f;
	SET_MODEL( edict(), "models/pigeon.mdl" );

//	UTIL_SetSize(pev, Vector(0,0,0), Vector(0,0,0));
	UTIL_SetSize( pev, Vector( -5, -5, 0 ), Vector( 5, 5, 2 ) );
}

//=========================================================
//=========================================================
void CFlockingFlyer::Spawn()
{
	Precache();
	SpawnCommonCode();
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CFlockingFlyer::IdleThink );
}

void CFlockingFlyer::MakeSound()
{
	const char* sound;
	if ( m_flAlertTime > gpGlobals->time )
		sound = RANDOM_LONG( 0, 1 ) ? "boid/boid_alert2.wav" : "boid/boid_alert1.wav";
	else
		sound = RANDOM_LONG( 0, 1 ) ? "boid/boid_idle2.wav" : "boid/boid_idle1.wav";
	EMIT_SOUND( edict(), CHAN_WEAPON, sound, 1, ATTN_NORM );
}

//=========================================================
//=========================================================
void CFlockingFlyer::Killed( entvars_t*, int )
{
	for ( CFlockingFlyer* boid = m_pSquadLeader; boid; boid = boid->m_pSquadNext )
		boid->m_flAlertTime = gpGlobals->time + 15;
	if ( m_pSquadLeader )
		m_pSquadLeader->SquadRemove( this );
	pev->deadflag = DEAD_DEAD;
	pev->framerate = 0;
	pev->effects = EF_NOINTERP;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->movetype = MOVETYPE_TOSS;
	SetThink( &CFlockingFlyer::FallHack );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CFlockingFlyer::FallHack()
{
	if ( pev->flags & FL_ONGROUND )
	{
		if ( FClassnameIs( pev->groundentity, "worldspawn" ) )
		{
			pev->velocity = g_vecZero;
			SetThink( NULL );
		}
		else
		{
			pev->flags &= ~FL_ONGROUND;
			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
}

//=========================================================
//=========================================================
void CFlockingFlyer::BoidAdvanceFrame()
{
	float flapSpeed = (pev->speed - pev->armorvalue) / AFLOCK_ACCELERATE;
	pev->armorvalue = pev->armorvalue * 0.8 + pev->speed * 0.2;
	if ( flapSpeed < 0 )
		flapSpeed = -flapSpeed;
	if ( flapSpeed < 0.25f )
		flapSpeed = 0.25f;
	if ( flapSpeed > 1.9 )
		flapSpeed = 1.9f;
	pev->framerate = flapSpeed;
	pev->avelocity.x = -(pev->angles.x + flapSpeed * 5);

	// bank
	pev->avelocity.z = -(pev->angles.z + pev->avelocity.y);

	// pev->framerate		= flapspeed;
	StudioFrameAdvance( 0.1f );
}

//=========================================================
//=========================================================
void CFlockingFlyer::IdleThink()
{
	pev->nextthink = gpGlobals->time + 0.2;

	// see if there's a client in the same pvs as the monster
	if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		SetThink( &CFlockingFlyer::Start );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

//=========================================================
// Start - player enters the pvs, so get things going.
//=========================================================
void CFlockingFlyer::Start()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( IsLeader() )
		SetThink( &CFlockingFlyer::FlockLeaderThink );
	else
		SetThink( &CFlockingFlyer::FlockFollowerThink );

/*
	Vector	vecTakeOff;
	vecTakeOff = Vector ( 0 , 0 , 0 );

	vecTakeOff.z = 50 + RANDOM_FLOAT ( 0, 100 );
	vecTakeOff.x = 20 - RANDOM_FLOAT ( 0, 40);
	vecTakeOff.y = 20 - RANDOM_FLOAT ( 0, 40);

	pev->velocity = vecTakeOff;


	pev->speed = pev->velocity.Length();
	pev->sequence = 0;
*/
	SetActivity( ACT_FLY );
	ResetSequenceInfo();
	BoidAdvanceFrame();
	pev->speed = AFLOCK_FLY_SPEED; // no delay!
}

//=========================================================
// Leader boid calls this to form a flock from surrounding boids
//=========================================================
void CFlockingFlyer::FormFlock()
{
	if ( !InSquad() )
	{
		// I am my own leader
		m_pSquadLeader = this;
		m_pSquadNext = NULL;
		CBaseEntity* entity = NULL;
		while ( (entity = UTIL_FindEntityInSphere( entity, pev->origin, AFLOCK_MAX_RECRUIT_RADIUS )) != NULL )
		{
			CBaseMonster* recruit = entity->MyMonsterPointer();
			if ( recruit && recruit != this && recruit->IsAlive() && !recruit->m_pCine
				&& FClassnameIs( recruit->pev, "monster_flyer" ) )
				SquadAdd( (CFlockingFlyer*)recruit );
		}
	}
	SetThink( &CFlockingFlyer::IdleThink ); // now that flock is formed, go to idle and wait for a player to come along.
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Searches for boids that are too close and pushes them away
//=========================================================
void CFlockingFlyer::SpreadFlock()
{
	for ( CFlockingFlyer* boid = m_pSquadLeader; boid; boid = boid->m_pSquadNext )
	{
		if ( boid != this && (pev->origin - boid->pev->origin).Length() <= AFLOCK_TOO_CLOSE )
		{
			Vector direction = (boid->pev->origin - pev->origin).Normalize();
			float speed = boid->pev->velocity.Length();
			boid->pev->velocity = boid->pev->velocity.Normalize();
			boid->pev->velocity = (boid->pev->velocity + direction) * 0.5f;
			boid->pev->velocity = boid->pev->velocity * speed;
		}
	}
}

void CFlockingFlyer::SpreadFlock2()
{
	for ( CFlockingFlyer* boid = m_pSquadLeader; boid; boid = boid->m_pSquadNext )
	{
		if ( boid != this && (pev->origin - boid->pev->origin).Length() <= AFLOCK_TOO_CLOSE )
			pev->velocity = pev->velocity + (pev->origin - boid->pev->origin).Normalize();
	}
}

//=========================================================
// FBoidPathBlocked - returns TRUE if there is an obstacle ahead
//=========================================================
BOOL CFlockingFlyer::FPathBlocked()
{
	if ( m_flFakeBlockedTime > gpGlobals->time )
	{
		m_flLastBlockedTime = gpGlobals->time;
		return TRUE;
	}
	UTIL_MakeVectors( pev->angles );
	BOOL blocked = FALSE;
	TraceResult trace;
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * AFLOCK_CHECK_DIST,
		ignore_monsters, edict(), &trace );
	if ( trace.flFraction != 1 )
	{
		m_flLastBlockedTime = gpGlobals->time;
		blocked = TRUE;
	}
	UTIL_TraceLine( pev->origin + gpGlobals->v_right * 12,
		pev->origin + gpGlobals->v_right * 12 + gpGlobals->v_forward * AFLOCK_CHECK_DIST,
		ignore_monsters, edict(), &trace );
	if ( trace.flFraction != 1 )
	{
		m_flLastBlockedTime = gpGlobals->time;
		blocked = TRUE;
	}
	UTIL_TraceLine( pev->origin - gpGlobals->v_right * 12,
		pev->origin - gpGlobals->v_right * 12 + gpGlobals->v_forward * AFLOCK_CHECK_DIST,
		ignore_monsters, edict(), &trace );
	if ( trace.flFraction != 1 )
	{
		m_flLastBlockedTime = gpGlobals->time;
		blocked = TRUE;
	}
	if ( !blocked && gpGlobals->time - m_flLastBlockedTime > 6 )
		m_flFakeBlockedTime = gpGlobals->time + (float)RANDOM_LONG( 1, 3 );
	return blocked;
}


//=========================================================
// Leader boids use this think every tenth
//=========================================================
void CFlockingFlyer::FlockLeaderThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	UTIL_MakeVectors( pev->angles );

	// is the way ahead clear?
	if ( !FPathBlocked() )
	{
		// if the boid is turning, stop the trend.
		if ( m_fTurning )
		{
			m_fTurning = FALSE;
			pev->avelocity.y = 0;
		}
		m_fPathBlocked = FALSE;
		if ( pev->speed <= AFLOCK_FLY_SPEED )
			pev->speed += 5;
		pev->velocity = gpGlobals->v_forward * pev->speed;
		BoidAdvanceFrame();
		return;
	}

	// IF we get this far in the function, the leader's path is blocked!
	m_fPathBlocked = TRUE;
	TraceResult trace;
	if ( !m_fTurning ) // something in the way and boid is not already turning to avoid
	{
		UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_right * AFLOCK_CHECK_DIST,
			ignore_monsters, edict(), &trace );
		float right = (trace.vecEndPos - pev->origin).Length();
		UTIL_TraceLine( pev->origin, pev->origin - gpGlobals->v_right * AFLOCK_CHECK_DIST,
			ignore_monsters, edict(), &trace );
		float left = (trace.vecEndPos - pev->origin).Length();
		if ( right > left )
			pev->avelocity.y = -AFLOCK_TURN_RATE;
		else if ( left > right )
			pev->avelocity.y = AFLOCK_TURN_RATE;
		else
			pev->avelocity.y = RANDOM_LONG( 0, 1 ) ? -AFLOCK_TURN_RATE : AFLOCK_TURN_RATE;
		m_fTurning = TRUE;
	}
	SpreadFlock();
	pev->velocity = gpGlobals->v_forward * pev->speed;
	UTIL_TraceLine( pev->origin, pev->origin - gpGlobals->v_up * 16, ignore_monsters, edict(), &trace );
	if ( trace.flFraction != 1 && pev->velocity.z < 0 )
		pev->velocity.z = 0;
	if ( pev->flags & FL_ONGROUND )
	{
		UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );
		pev->velocity.z = 0;
	}
	if ( m_flFlockNextSoundTime < gpGlobals->time )
	{
		MakeSound();
		m_flFlockNextSoundTime = gpGlobals->time + RANDOM_FLOAT( 1, 3 );
	}
	BoidAdvanceFrame();
}

//=========================================================
// follower boids execute this code when flocking
//=========================================================
void CFlockingFlyer::FlockFollowerThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( IsLeader() || !InSquad() )
	{
		// the leader has been killed and this flyer suddenly finds himself the leader. 
		SetThink( &CFlockingFlyer::FlockLeaderThink );
		return;
	}
	Vector direction = m_pSquadLeader->pev->origin - pev->origin;
	float distance = direction.Length();

	// match heading with leader
	pev->angles = m_pSquadLeader->pev->angles;

	//
	// We can see the leader, so try to catch up to it
	//
	if ( FInViewCone( m_pSquadLeader ) )
	{
		if ( distance > AFLOCK_TOO_FAR )
			m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 1.5;
		else if ( distance < AFLOCK_TOO_CLOSE )
			m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 0.5;
	}
	else
		m_flGoalSpeed = m_pSquadLeader->pev->velocity.Length() * 0.5;
	SpreadFlock2();
	pev->speed = pev->velocity.Length();
	pev->velocity = pev->velocity.Normalize();
	if ( distance > AFLOCK_TOO_FAR )
		pev->velocity = (pev->velocity + direction.Normalize()) * 0.5;

	// clamp speeds and handle acceleration
	if ( m_flGoalSpeed > AFLOCK_FLY_SPEED * 2 )
		m_flGoalSpeed = AFLOCK_FLY_SPEED * 2;
	if ( pev->speed < m_flGoalSpeed )
		pev->speed += AFLOCK_ACCELERATE;
	else if ( pev->speed > m_flGoalSpeed )
		pev->speed -= AFLOCK_ACCELERATE;
	pev->velocity = pev->velocity * pev->speed;
	BoidAdvanceFrame();
}

/*	
	// Is this boid's course blocked?
	if ( FBoidPathBlocked (pev) )
	{
		// course is still blocked from last time. Just keep flying along adjusted 
		// velocity
		if ( m_fCourseAdjust )
		{
			pev->velocity = m_vecAdjustedVelocity * pev->speed;
			return;
		}
		else // set course adjust flag and calculate adjusted velocity
		{
			m_fCourseAdjust = TRUE;
			
			// use VELOCITY, not angles, not all boids point the direction they are flying
			//vecDir = UTIL_VecToAngles( pev->velocity );
			//UTIL_MakeVectors ( vecDir );

			UTIL_MakeVectors ( pev->angles );

			// measure clearance on left and right to pick the best dir to turn
			UTIL_TraceLine(pev->origin, pev->origin + gpGlobals->v_right * AFLOCK_CHECK_DIST, ignore_monsters, ENT(pev), &tr);
			vecDist = (tr.vecEndPos - pev->origin);
			flRightSide = vecDist.Length();

			UTIL_TraceLine(pev->origin, pev->origin - gpGlobals->v_right * AFLOCK_CHECK_DIST, ignore_monsters, ENT(pev), &tr);
			vecDist = (tr.vecEndPos - pev->origin);
			flLeftSide = vecDist.Length();

			// slide right if more clearance on right side
			if ( flRightSide > flLeftSide )
			{
				m_vecAdjustedVelocity = gpGlobals->v_right;
			}
			// else slide left
			else
			{
				m_vecAdjustedVelocity = gpGlobals->v_right * -1;
			}
		}
		return;
	}

	// if we make it this far, boids path is CLEAR!
	m_fCourseAdjust = FALSE;
*/


//=========================================================
//
// SquadUnlink(), Unlink the squad pointers.
//
//=========================================================
void CFlockingFlyer::SquadUnlink()
{
	m_pSquadLeader = NULL;
	m_pSquadNext = NULL;
}

void CFlockingFlyer::SquadAdd( CFlockingFlyer* add )
{
	add->m_pSquadNext = m_pSquadNext;
	m_pSquadNext = add;
	add->m_pSquadLeader = this;
}

int CFlockingFlyer::SquadCount()
{
	int count = 0;
	for ( CFlockingFlyer* boid = m_pSquadLeader; boid; boid = boid->m_pSquadNext )
		++count;
	return count;
}

//=========================================================
//
// SquadDisband(), Unlink all squad members
//
//=========================================================
void CFlockingFlyer::SquadDisband()
{
	CFlockingFlyer* boid = m_pSquadLeader;
	while ( boid )
	{
		CFlockingFlyer* next = boid->m_pSquadNext;
		boid->SquadUnlink();
		boid = next;
	}
}

void CFlockingFlyer::SquadRemove( CFlockingFlyer* remove )
{
	if ( !m_pSquadLeader )
		return;
	if ( SquadCount() <= 2 )
	{
		SquadDisband();
		return;
	}
	if ( remove == this )
	{
		CFlockingFlyer* leader = m_pSquadNext;
		leader->m_vecEnemyLKP = m_vecEnemyLKP;
		for ( CFlockingFlyer* boid = leader; boid; boid = boid->m_pSquadNext )
			boid->m_pSquadLeader = leader;
		SquadUnlink();
	}
	else
	{
		CFlockingFlyer* boid = this;
		while ( boid->m_pSquadNext != remove )
			boid = boid->m_pSquadNext;
		boid->m_pSquadNext = remove->m_pSquadNext;
		remove->SquadUnlink();
	}
}
