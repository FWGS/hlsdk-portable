/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Chicken
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"decals.h"

#define		CHICKEN_IDLE				0
#define		CHICKEN_BORED				1
#define		CHICKEN_SCARED_BY_ENT		2
#define		CHICKEN_EAT					3
#define		CHICKEN_CROW				4

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#include "Chicken.h"

//=========================================================
// Furniture - this is the cool comment I cut-and-pasted
//=========================================================

LINK_ENTITY_TO_CLASS( monster_chicken_feathers, CChickenFeathers );


//=========================================================
// Furniture is killed
//=========================================================
void CChickenFeathers :: Die ( void )
{
	SetThink ( SUB_Remove );
	SetNextThink(0);
}

//=========================================================
// This used to have something to do with bees flying, but 
// now it only initializes moving furniture in scripted sequences
//=========================================================
void CChickenFeathers :: Spawn( )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/feathers.mdl");
	
	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev),	"models/feathers.mdl");

	pev->movetype	= MOVETYPE_NONE;
	pev->solid		= SOLID_NOT;
	pev->health		= 80000;
	pev->takedamage = DAMAGE_AIM;
	pev->effects		= 0;
	pev->yaw_speed		= 0;
	pev->sequence		= 0;
	pev->frame			= 0;

//	pev->nextthink += 1.0;
//	SetThink (WalkMonsterDelay);

	ResetSequenceInfo( );
	pev->frame = 0;
	MonsterInit();
	// so it doesn't get affected by things...
	pev->deadflag		= DEAD_DEAD;
}

//=========================================================
// ID's Furniture as neutral (noone will attack it)
//=========================================================
int CChickenFeathers::Classify ( void )
{
	return m_iClass?m_iClass:CLASS_NONE;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////


LINK_ENTITY_TO_CLASS( monster_chicken, CChicken );

TYPEDESCRIPTION	CChicken::m_SaveData[] = 
{
	DEFINE_FIELD( CChicken, m_iszGibModel, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CChicken, CBaseMonster );

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CChicken :: ISoundMask ( void )
{
	return	bits_SOUND_COMBAT | bits_SOUND_DANGER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CChicken :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_MUNDANE_PREY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChicken :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CChicken :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/chicken.mdl");
	UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8, 8, 8 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	pev->health			= 4;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity ( ACT_IDLE );

	pev->view_ofs		= Vector ( 0, 0, 1 );// position of the eyes relative to monster's origin.
	pev->takedamage		= DAMAGE_YES;
	m_iMode				= CHICKEN_IDLE;
	m_flNextSmellTime	= gpGlobals->time;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChicken :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/chicken.mdl");

	PRECACHE_SOUND("chicken/chick_cluck1.wav");
	PRECACHE_SOUND("chicken/chick_cluck2.wav");
	PRECACHE_SOUND("chicken/chick_cluck3.wav");
	PRECACHE_SOUND("chicken/chick_scream.wav");

	m_iszGibModel = ALLOC_STRING("models/chickengibs.mdl");
	PRECACHE_MODEL( "models/chickengibs.mdl" ); //LRC
	PRECACHE_MODEL( "models/feathers.mdl" ); //LRC
}	


//=========================================================
// Killed.
//=========================================================
void CChicken :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->solid = SOLID_NOT;

	CSoundEnt::InsertSound ( bits_SOUND_WORLD, pev->origin, 128, 1 );

	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
	if ( pOwner )
	{
		pOwner->DeathNotice( pev );
	}
	CBaseMonster::Killed( pevAttacker, GIB_ALWAYS );

	// send some faethers flying...
	CBaseEntity *pNew = Create( "monster_chicken_feathers", pev->origin, pev->angles );
	CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
	pNew->pev->spawnflags |= 1;

	UTIL_Remove( this );
}

//=========================================================
// MonsterThink, overridden for chickens.
//=========================================================
void CChicken :: MonsterThink( void  )
{
	if ( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		SetNextThink(RANDOM_FLOAT(1,1.5));
	else
	{
		SetNextThink(0.l);// keep monster thinking
	}

	float flInterval = StudioFrameAdvance( ); // animate

	switch ( m_iMode )
	{
	case	CHICKEN_CROW:
		if (m_fSequenceFinished)
		{
			SetActivity ( ACT_IDLE );
			m_iMode = CHICKEN_IDLE;
		}
		break;
	case	CHICKEN_BORED:
		Look( 100 );
		if (HasConditions(bits_COND_SEE_FEAR | bits_COND_SEE_CLIENT))
		{
			// if see something scary
			//ALERT ( at_aiconsole, "Scared\n" );
			Eat( 30 +  ( RANDOM_LONG(0,14) ) );// chicken will ignore food for 30 to 45 seconds
			PickNewDest( CHICKEN_SCARED_BY_ENT );
			SetActivity ( ACT_RUN );
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_scream.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
		}
		break;
	case	CHICKEN_IDLE:
	case	CHICKEN_EAT:
		{
			// Cower when you hear something scary
			if ( HasConditions( bits_COND_HEAR_SOUND ) )
			{
				CSound *pSound;
				pSound = PBestSound();

				ASSERT( pSound != NULL );
				if ( pSound )
				{
					if ( pSound->m_iType & bits_SOUND_COMBAT )
					{
						PickNewDest( CHICKEN_SCARED_BY_ENT );
						SetActivity ( ACT_RUN );
						EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_scream.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
						break;
					}
				}
			}
			// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
			if ( RANDOM_LONG(0,3) < 5 )
			{
				Look( 150 );
				if (HasConditions(bits_COND_SEE_FEAR | bits_COND_SEE_CLIENT))
				{
					// if see something scary
					//ALERT ( at_aiconsole, "Scared\n" );
					Eat( 30 +  ( RANDOM_LONG(0,14) ) );// chicken will ignore food for 30 to 45 seconds
					PickNewDest( CHICKEN_SCARED_BY_ENT );
					SetActivity ( ACT_RUN );
					EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_scream.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
				}
				else if ( RANDOM_LONG(0,100) < 5 )
				{
					// if chicken doesn't see anything, there's still a good chance that it will move. (boredom)
					//ALERT ( at_aiconsole, "Bored\n" );
					PickNewDest( CHICKEN_BORED );
					SetActivity ( ACT_WALK );

					if ( m_iMode == CHICKEN_EAT )
					{
						// chicken will ignore food for 30 to 45 seconds if it got bored while eating. 
						Eat( 30 +  ( RANDOM_LONG(0,14) ) );
					}
				}
			}
			// don't do this stuff if eating!
			else if ( m_iMode == CHICKEN_IDLE )
			{
				float f = RANDOM_FLOAT(0,1.0);
				if ( f < 0.1 )
				{
					// crow
					SetActivity ( ACT_EXCITED );
					m_iMode = CHICKEN_CROW;
				}
				else if ( f < 0.4 )
				{
					// peck at ground...
					SetActivity ( ACT_EAT );
					m_iMode = CHICKEN_EAT;
				}
				else
				{
					SetActivity ( ACT_IDLE );
				}
			}

			break;
		}
	}
	
	if (( m_iMode != CHICKEN_SCARED_BY_ENT ) && ( m_iMode != CHICKEN_CROW ))
	{
		if ( RANDOM_FLOAT(0,1.0) < 0.05 )
		{
			switch (RANDOM_LONG(0,2))
			{
			case 0:
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_cluck1.wav", 0.4, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
				break;
			case 1:
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_cluck2.wav", 0.4, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
				break;
			default:
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chicken/chick_cluck3.wav", 0.4, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );
				break;
			}
		}
	}

	if ( m_flGroundSpeed != 0 )
	{
		Move( flInterval );
	}
}

//=========================================================
// Picks a new spot for chicken to run to.(
//=========================================================
void CChicken :: PickNewDest ( int iCondition )
{
	Vector	vecNewDir;
	Vector	vecDest;
	float	flDist;

	m_iMode = iCondition;

	do 
	{
		// picks a random spot, requiring that it be at least 128 units away
		// else, the chicken will pick a spot too close to itself and run in 
		// circles. this is a hack but buys me time to work on the real monsters.
		vecNewDir.x = RANDOM_FLOAT( -1, 1 );
		vecNewDir.y = RANDOM_FLOAT( -1, 1 );
		flDist		= 256 + ( RANDOM_LONG(0,255) );
		vecDest = pev->origin + vecNewDir * flDist;

	} while ( ( vecDest - pev->origin ).Length2D() < 64 );

	m_Route[ 0 ].vecLocation.x = vecDest.x;
	m_Route[ 0 ].vecLocation.y = vecDest.y;
	m_Route[ 0 ].vecLocation.z = pev->origin.z;
	m_Route[ 0 ].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[ 0 ].iType );
}

//=========================================================
// chickens's move function
//=========================================================
void CChicken :: Move ( float flInterval ) 
{
	float		flWaypointDist;
	Vector		vecApex;

	// local move to waypoint.
	flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length2D();
	MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );

	ChangeYaw ( pev->yaw_speed );
	UTIL_MakeVectors( pev->angles );

	if ( RANDOM_LONG(0,7) == 1 )
	{
		// randomly check for blocked path.(more random load balancing)
		if ( !WALK_MOVE( ENT(pev), pev->ideal_yaw, 4, WALKMOVE_NORMAL ) )
		{
			// stuck, so just pick a new spot to run off to
			PickNewDest( m_iMode );
		}
	}
	
	WALK_MOVE( ENT(pev), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );

	// if the waypoint is closer than step size, then stop after next step (ok for chicken to overshoot)
	if ( flWaypointDist <= m_flGroundSpeed * flInterval )
	{
		// take truncated step and stop

		SetActivity ( ACT_IDLE );

		m_iMode = CHICKEN_IDLE;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

