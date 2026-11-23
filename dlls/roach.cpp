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
// cockroach
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "soundent.h"
#include "decals.h"

#define ROACH_IDLE 0
#define ROACH_BORED 1
#define ROACH_SCARED_BY_ENT 2
#define ROACH_SCARED_BY_LIGHT 3
#define ROACH_SMELL_FOOD 4
#define ROACH_EAT 5

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
class CRoach : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	void EXPORT MonsterThink() override;
	void Move( float interval ) override;
	void PickNewDest( int condition );
	void EXPORT Touch( CBaseEntity* other ) override;
	void Killed( entvars_t* attacker, int gib ) override;
	float m_flLastLightLevel;
	float m_flNextSmellTime;
	int Classify() override;
	void Look( int distance ) override;
	int ISoundMask() override;

	// UNDONE: These don't necessarily need to be save/restored, but if we add more data, it may
	BOOL m_fLightHacked;
	int m_iMode;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_cockroach, CRoach );

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CRoach::ISoundMask()
{
	return bits_SOUND_CARCASS | bits_SOUND_MEAT;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CRoach::Classify()
{
	return CLASS_INSECT;
}

void CRoach::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoach::Precache()
{
	PRECACHE_MODEL( "models/roach.mdl" );
	PRECACHE_SOUND( "roach/rch_walk.wav" );
	PRECACHE_SOUND( "roach/rch_smash.wav" );
}

//=========================================================
// Spawn
//=========================================================
void CRoach::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/roach.mdl" );
	UTIL_SetSize( pev, Vector( -1, -1, 0 ), Vector( 1, 1, 2 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = 1;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.5f;
	MonsterInit();
	SetActivity( ACT_IDLE );
	pev->view_ofs = Vector( 0, 0, 1 ); // position of the eyes relative to monster's origin.
	pev->takedamage = DAMAGE_YES;
	m_iMode = ROACH_IDLE;
	m_fLightHacked = FALSE;
	m_flLastLightLevel = -1;
	m_flNextSmellTime = gpGlobals->time;
}

void CRoach::Touch( CBaseEntity* other )
{
	if ( other->pev->velocity == g_vecZero || !other->IsPlayer() )
		return;
	Vector spot = pev->origin + Vector( 0, 0, 8 );
	TraceResult trace;
	UTIL_TraceLine( spot, spot - Vector( 0, 0, 24 ), ignore_monsters, edict(), &trace );
	UTIL_DecalTrace( &trace, DECAL_YBLOOD1 + RANDOM_LONG( 0, 5 ) );
	TakeDamage( other->pev, other->pev, pev->health, DMG_CRUSH );
}

void CRoach::Killed( entvars_t*, int )
{
	pev->solid = SOLID_NOT;
	EMIT_SOUND_DYN( edict(), CHAN_BODY, "roach/rch_smash.wav", 0.7f, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
	CSoundEnt::InsertSound( bits_SOUND_WORLD, pev->origin, 128, 1 );
	CBaseEntity* owner = CBaseEntity::Instance( pev->owner );
	if ( owner )
		owner->DeathNotice( pev );
	UTIL_Remove( this );
}

//=========================================================
// MonsterThink, overridden for roaches.
//=========================================================
void CRoach::MonsterThink()
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 1, 1.5f );
	else
		pev->nextthink = gpGlobals->time + 0.1; // keep monster thinking
	float interval = StudioFrameAdvance(); // animate
	if ( !m_fLightHacked )
	{
		// if light value hasn't been collection for the first time yet, 
		// suspend the creature for a second so the world finishes spawning, then we'll collect the light level.
		pev->nextthink = gpGlobals->time + 1;
		m_fLightHacked = TRUE;
		return;
	}
	if ( m_flLastLightLevel < 0 )
		m_flLastLightLevel = (float)GETENTITYILLUM( edict() );
	if ( m_iMode == ROACH_IDLE || m_iMode == ROACH_EAT )
	{
		// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
		if ( RANDOM_LONG( 0, 3 ) == 1 )
		{
			Look( 150 );
			if ( HasConditions( bits_COND_SEE_FEAR ) )
			{
				// if see something scary
				//ALERT ( at_aiconsole, "Scared\n" );
				Eat( (float)(30 + RANDOM_LONG( 0, 14 )) ); // roach will ignore food for 30 to 45 seconds
				PickNewDest( ROACH_SCARED_BY_ENT );
				SetActivity( ACT_WALK );
			}
			else if ( RANDOM_LONG( 0, 149 ) == 1 )
			{
				// if roach doesn't see anything, there's still a chance that it will move. (boredom)
				//ALERT ( at_aiconsole, "Bored\n" );
				PickNewDest( ROACH_BORED );
				SetActivity( ACT_WALK );
				if ( m_iMode == ROACH_EAT )
					Eat( (float)(30 + RANDOM_LONG( 0, 14 )) );
			}
		}

		// don't do this stuff if eating!
		if ( m_iMode == ROACH_IDLE )
		{
			if ( FShouldEat() )
				Listen();
			if ( GETENTITYILLUM( edict() ) > m_flLastLightLevel )
			{
				// someone turned on lights!
				//ALERT ( at_console, "Lights!\n" );
				PickNewDest( ROACH_SCARED_BY_LIGHT );
				SetActivity( ACT_WALK );
			}
			else if ( HasConditions( bits_COND_SMELL_FOOD ) )
			{
				CSound* sound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
				if ( sound && fabsf( sound->m_vecOrigin.z - pev->origin.z ) <= 3 )
				{
					PickNewDest( ROACH_SMELL_FOOD );
					SetActivity( ACT_WALK );
				}
			}
		}
	}
	else if ( m_iMode == ROACH_SCARED_BY_LIGHT && GETENTITYILLUM( edict() ) <= m_flLastLightLevel )
	{
		SetActivity( ACT_IDLE );
		m_flLastLightLevel = (float)GETENTITYILLUM( edict() );
	}
	if ( m_flGroundSpeed != 0 )
		Move( interval );
}

//=========================================================
// Picks a new spot for roach to run to.(
//=========================================================
void CRoach::PickNewDest( int condition )
{
	m_iMode = condition;
	if ( condition == ROACH_SMELL_FOOD )
	{
		CSound* sound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );
		if ( sound )
		{
			m_Route[0].vecLocation.x = sound->m_vecOrigin.x + (3 - RANDOM_LONG( 0, 5 ));
			m_Route[0].vecLocation.y = sound->m_vecOrigin.y + (3 - RANDOM_LONG( 0, 5 ));
			m_Route[0].vecLocation.z = sound->m_vecOrigin.z;
			m_Route[0].iType = bits_MF_TO_LOCATION;
			m_movementGoal = RouteClassify( bits_MF_TO_LOCATION );
			return;
		}
	}
	Vector destination;
	do
	{
		float x = RANDOM_FLOAT( -1, 1 );
		float y = RANDOM_FLOAT( -1, 1 );
		int distance = 256 + RANDOM_LONG( 0, 255 );
		destination = pev->origin + Vector( x, y, 0 ) * (float)distance;
	}
	while ( (destination - pev->origin).Length2D() < 128 );
	m_Route[0].vecLocation = Vector( destination.x, destination.y, pev->origin.z );
	m_Route[0].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( bits_MF_TO_LOCATION );
	if ( RANDOM_LONG( 0, 9 ) == 1 )
		EMIT_SOUND_DYN( edict(), CHAN_BODY, "roach/rch_walk.wav", 1, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
}

void CRoach::Move( float interval )
{
	float distance = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Length2D();
	MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
	ChangeYaw( (int)pev->yaw_speed );
	UTIL_MakeVectors( pev->angles );
	if ( RANDOM_LONG( 0, 7 ) == 1 && !WALK_MOVE( edict(), pev->ideal_yaw, 4, WALKMOVE_NORMAL ) )
		PickNewDest( m_iMode );
	WALK_MOVE( edict(), pev->ideal_yaw, m_flGroundSpeed * interval, WALKMOVE_NORMAL );
	if ( m_flGroundSpeed * interval >= distance )
	{
		SetActivity( ACT_IDLE );
		m_flLastLightLevel = (float)GETENTITYILLUM( edict() );
		m_iMode = m_iMode == ROACH_SMELL_FOOD ? ROACH_EAT : ROACH_IDLE;
	}
	if ( RANDOM_LONG( 0, 149 ) == 1 && m_iMode != ROACH_SCARED_BY_LIGHT && m_iMode != ROACH_SMELL_FOOD )
		PickNewDest( ROACH_IDLE );
}

void CRoach::Look( int distance )
{

	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR );
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		return;
	m_pLink = NULL;
	CBaseEntity* previous = this;
	CBaseEntity* entity = NULL;
	int sighted = 0;
	while ( (entity = UTIL_FindEntityInSphere( entity, pev->origin, (float)distance )) != NULL )
	{
		if ( !(entity->IsPlayer() || (entity->pev->flags & FL_MONSTER)) )
			continue;
		if ( (entity->pev->flags & FL_NOTARGET) || entity->pev->health <= 0 )
			continue;
		previous->m_pLink = entity;
		previous = entity;
		entity->m_pLink = NULL;
		int relationship = IRelationship( entity );
		if ( relationship == R_FR )
			sighted = bits_COND_SEE_FEAR;
		else if ( relationship != R_NO )
			ALERT( at_console, "%s can't asses %s\n", STRING( pev->classname ), STRING( entity->pev->classname ) );
	}
	SetConditions( sighted );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

