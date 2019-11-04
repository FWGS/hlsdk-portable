/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// rat - environmental monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"decals.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	RAT_IDLE		0
#define	RAT_BORED		1
#define	RAT_SCARED_BY_ENT	2
#define	RAT_SCARED_BY_LIGHT	3
#define	RAT_SMELL_FOOD		4
#define	RAT_EAT			5
#define	RAT_DEAD		6

class CRat : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void Look ( int iDistance );
	int ISoundMask( void );
	void EXPORT MonsterThink( void );
	void Move( float flInterval );
	void PickNewDest( int iCondition );
	void EXPORT Touch( CBaseEntity *pOther );
	void Killed( entvars_t *pevAttacker, int iGib );

	float	m_flLastLightLevel;             
	float	m_flNextSmellTime;
	BOOL	m_fLightHacked;
	int	m_iMode;
	float	m_flNextTouchTime;
};

LINK_ENTITY_TO_CLASS( monster_rat, CRat )

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CRat::ISoundMask( void )
{
	return	bits_SOUND_CARCASS | bits_SOUND_MEAT;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CRat::Classify( void )
{
	return m_iClass?m_iClass:CLASS_INSECT; //LRC- maybe someone needs to give them a basic biology lesson...
}

//=========================================================
// Touch
//=========================================================
void CRat::Touch( CBaseEntity *pOther )
{
	Vector          vecSpot;
	TraceResult     tr;

	if( m_flNextTouchTime > gpGlobals->time )
		return;

	if( pOther->pev->velocity == g_vecZero || !pOther->IsPlayer() )
	{
		return;
	}

	vecSpot = pev->origin + Vector( 0, 0, 8 ); // move up a bit, and trace down.

	TakeDamage( pOther->pev, pOther->pev, 1, DMG_CRUSH );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rat/rat_squeak.wav", 0.3, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );

	m_flNextTouchTime = gpGlobals->time + 1.0f;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRat::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 45;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CRat::Spawn()
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL( ENT( pev ), "models/bigrat.mdl" );
	UTIL_SetSize( pev, Vector( -6, -6, 0 ), Vector( 6, 6, 6 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health = 8;
	pev->view_ofs = Vector( 0, 0, 6 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity( ACT_IDLE );

	pev->view_ofs		= Vector( 0, 0, 1 );// position of the eyes relative to monster's origin.
	pev->takedamage		= DAMAGE_YES;
	m_fLightHacked		= FALSE;
	m_flLastLightLevel	= -1;
	m_iMode			= RAT_IDLE;
	m_flNextSmellTime	= gpGlobals->time;
	m_flNextTouchTime	= gpGlobals->time;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRat::Precache()
{
	if (pev->model)
		PRECACHE_MODEL(STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL( "models/bigrat.mdl" );

	PRECACHE_SOUND( "roach/rch_walk.wav" );
	PRECACHE_SOUND( "rat/rat_squeak.wav" );
	PRECACHE_SOUND( "rat/rat_die.wav" );
}

//=========================================================
// Killed.
//=========================================================
void CRat::Killed( entvars_t *pevAttacker, int iGib )
{
	// random sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rat/rat_die.wav", 0.3, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );

	CSoundEnt::InsertSound ( bits_SOUND_WORLD, pev->origin, 128, 1 );

	// no gibs
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// MonsterThink, overridden for rats.
//=========================================================
void CRat::MonsterThink( void )
{
	if( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		SetNextThink( RANDOM_FLOAT( 1.0f, 1.5f ) );
	else
		SetNextThink( 0.1f );// keep monster thinking

	RunAI();

	if( !IsAlive() )
		m_iMode = RAT_DEAD;

	float flInterval = StudioFrameAdvance(); // animate

	if( !m_fLightHacked )
	{
		// if light value hasn't been collection for the first time yet,
		// suspend the creature for a second so the world finishes spawning, then we'll collect the light level.
		SetNextThink( 1.0f );
		m_fLightHacked = TRUE;
		return;
	}
	else if( m_flLastLightLevel < 0 )
	{
		// collect light level for the first time, now that all of the lightmaps in the rat's area have been calculated.
		m_flLastLightLevel = GETENTITYILLUM( ENT( pev ) );
	}

	switch( m_iMode )
	{
	case RAT_IDLE:
	case RAT_EAT:
		{
			// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
			if( RANDOM_LONG( 0, 3 ) == 1 )
			{
				Look( 150 );
				if( HasConditions( bits_COND_SEE_FEAR ) )
				{
					// if see something scary
					// ALERT ( at_aiconsole, "Scared\n" );
					Eat( 30 + ( RANDOM_LONG( 0, 14 ) ) );// rat will ignore food for 30 to 45 seconds
					PickNewDest( RAT_SCARED_BY_ENT );
					SetActivity( ACT_RUN );
				}
				else if( RANDOM_LONG( 0, 149 ) == 1 )
				{
					// if rat doesn't see anything, there's still a chance that it will move. (boredom)
					// ALERT( at_aiconsole, "Bored\n" );
					PickNewDest( RAT_BORED );
					SetActivity( ACT_WALK );

					if( m_iMode == RAT_EAT )
					{
						// rat will ignore food for 30 to 45 seconds if it got bored while eating.
						Eat( 30 + ( RANDOM_LONG( 0, 14 ) ) );
					}
				}
			}

			// don't do this stuff if eating!
			if( m_iMode == RAT_IDLE )
			{
				if( FShouldEat() )
				{
					Listen();
				}

				if( GETENTITYILLUM( ENT( pev ) ) > m_flLastLightLevel )
				{
					// someone turned on lights!
					// ALERT( at_console, "Lights!\n" );
					PickNewDest( RAT_SCARED_BY_LIGHT );
					SetActivity( ACT_RUN );
				}
				else if( HasConditions(bits_COND_SMELL_FOOD) )
				{
					CSound *pSound;

					pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );

					// rat smells food and is just standing around. Go to food unless food isn't on same z-plane.
					if( pSound && abs( pSound->m_vecOrigin.z - pev->origin.z ) <= 3 )
					{
						PickNewDest( RAT_SMELL_FOOD );
						SetActivity( ACT_WALK );
					}
				}
			}

			break;
		}
	case	RAT_SCARED_BY_LIGHT:
		{
			// if rat was scared by light, then stop if we're over a spot at least as dark as where we started!
			if( GETENTITYILLUM( ENT( pev ) ) <= m_flLastLightLevel )
			{
				SetActivity( ACT_IDLE );
				m_flLastLightLevel = GETENTITYILLUM( ENT ( pev ) );// make this our new light level.
			}
			break;
		}
	case	RAT_DEAD:
		return;
		break;
	}

	if( m_flGroundSpeed != 0 )
	{
		Move( flInterval );
	}
}

//=========================================================
// Picks a new spot for rat to run to.(
//=========================================================
void CRat::PickNewDest( int iCondition )
{
	Vector	vecNewDir;
	Vector	vecDest;
	float	flDist;

	m_iMode = iCondition;

	if( m_iMode == RAT_SMELL_FOOD )
	{
		// find the food and go there.
		CSound *pSound;

		pSound = CSoundEnt::SoundPointerForIndex( m_iAudibleList );

		if( pSound )
		{
			m_Route[0].vecLocation.x = pSound->m_vecOrigin.x + ( 3 - RANDOM_LONG( 0, 5 ) );
			m_Route[0].vecLocation.y = pSound->m_vecOrigin.y + ( 3 - RANDOM_LONG( 0, 5 ) );
			m_Route[0].vecLocation.z = pSound->m_vecOrigin.z;
			m_Route[0].iType = bits_MF_TO_LOCATION;
			m_movementGoal = RouteClassify( m_Route[0].iType );
			return;
		}
	}

	do
	{
		// picks a random spot, requiring that it be at least 128 units away
		// else, the rat will pick a spot too close to itself and run in
		// circles. this is a hack but buys me time to work on the real monsters.
		vecNewDir.x = RANDOM_FLOAT( -1, 1 );
		vecNewDir.y = RANDOM_FLOAT( -1, 1 );
		flDist = 256 + ( RANDOM_LONG( 0, 255 ) );
		vecDest = pev->origin + vecNewDir * flDist;
	} while( ( vecDest - pev->origin ).Length2D() < 128 );

	m_Route[0].vecLocation.x = vecDest.x;
	m_Route[0].vecLocation.y = vecDest.y;
	m_Route[0].vecLocation.z = pev->origin.z;
	m_Route[0].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify( m_Route[0].iType );
	if( RANDOM_LONG( 0, 9 ) == 1 )
	{
		// every once in a while, a rat will play a skitter sound when they decide to run
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "roach/rch_walk.wav", 1, ATTN_NORM, 0, 80 + RANDOM_LONG( 0, 39 ) );
	}
}

//=========================================================
// rat's move function
//=========================================================
void CRat::Move( float flInterval )
{
	float           flWaypointDist;
	Vector          vecApex;

	// local move to waypoint.
	flWaypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length2D();
	MakeIdealYaw ( m_Route[m_iRouteIndex].vecLocation );

	ChangeYaw( pev->yaw_speed );
	UTIL_MakeVectors( pev->angles );

	if( RANDOM_LONG( 0, 7 ) == 1 )
	{
		// randomly check for blocked path.(more random load balancing)
		if( !WALK_MOVE( ENT( pev ), pev->ideal_yaw, 4, WALKMOVE_NORMAL ) )
		{
			// stuck, so just pick a new spot to run off to
			PickNewDest( m_iMode );
		}
	}

	WALK_MOVE( ENT( pev ), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );

	// if the waypoint is closer than step size, then stop after next step (ok for rat to overshoot)
	if( flWaypointDist <= m_flGroundSpeed * flInterval )
	{
		// take truncated step and stop
		SetActivity( ACT_IDLE );
		m_flLastLightLevel = GETENTITYILLUM( ENT( pev ) );// this is rat's new comfortable light level

		if( m_iMode == RAT_SMELL_FOOD )
		{
			m_iMode = RAT_EAT;
		}
		else
		{
			m_iMode = RAT_IDLE;
		}
	}

	// if( RANDOM_LONG( 0, 149 ) == 1 && m_iMode != RAT_SCARED_BY_LIGHT && m_iMode != RAT_SMELL_FOOD )
	// {
		// random skitter while moving as long as not on a b-line to get out of light or going to food
	//	PickNewDest( FALSE );
	// }
}

//=========================================================
// Look - overriden for the rat, which can virtually see
// 360 degrees.
//=========================================================
void CRat::Look( int iDistance )
{
	CBaseEntity	*pSightEnt = NULL;// the current visible entity that we're dealing with
	CBaseEntity	*pPreviousEnt;// the last entity added to the link list
	int		iSighted = 0;

	// DON'T let visibility information from last frame sit around!
	ClearConditions( bits_COND_SEE_HATE |bits_COND_SEE_DISLIKE | bits_COND_SEE_ENEMY | bits_COND_SEE_FEAR );

	// don't let monsters outside of the player's PVS act up, or most of the interesting
	// things will happen before the player gets there!
	if( FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
	{
		return;
	}

	m_pLink = NULL;
	pPreviousEnt = this;

	// Does sphere also limit itself to PVS?
	// Examine all entities within a reasonable radius
	// !!!PERFORMANCE - let's trivially reject the ent list before radius searching!
	while( ( pSightEnt = UTIL_FindEntityInSphere( pSightEnt, pev->origin, iDistance ) ) != NULL )
	{
		// only consider ents that can be damaged. !!!temporarily only considering other monsters and clients
		if( pSightEnt->IsPlayer() || FBitSet( pSightEnt->pev->flags, FL_MONSTER ) )
		{
			if( /*FVisible( pSightEnt ) &&*/ !FBitSet( pSightEnt->pev->flags, FL_NOTARGET ) && pSightEnt->pev->health > 0 )
			{
				// NULL the Link pointer for each ent added to the link list. If other ents follow, the will overwrite
				// this value. If this ent happens to be the last, the list will be properly terminated.
				pPreviousEnt->m_pLink = pSightEnt;
				pSightEnt->m_pLink = NULL;
				pPreviousEnt = pSightEnt;

				// don't add the Enemy's relationship to the conditions. We only want to worry about conditions when
				// we see monsters other than the Enemy.
				switch( IRelationship( pSightEnt ) )
				{
				case	R_FR:
					iSighted |= bits_COND_SEE_FEAR;
					break;
				case	R_NO:
					break;
				default:
					ALERT( at_debug, "%s can't assess %s\n", STRING( pev->classname ), STRING( pSightEnt->pev->classname ) );
					break;
				}
			}
		}
	}
	SetConditions( iSighted );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
