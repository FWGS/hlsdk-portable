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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// modif de Julien
extern int gmsgClientDecal;

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask( void );
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGenericMonster::Classify( void )
{
	//modif de Julien
	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
		return CLASS_NONE;

	return CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;

	// modif de Julien
	case 1444:
		Vector vecBout;
		Vector vZero0(0,0,0); //Just init a couple of Vectors to shut down some weird compiler errors.
		Vector vZero1(0,0,0);
		GetAttachment( 1, vecBout, vZero0 ); //put vector outside 
		Vector vecDir;
		GetAttachment( 0, vecDir, vZero1 ); //same

		MESSAGE_BEGIN( MSG_ALL, gmsgClientDecal );

			WRITE_COORD( vecBout.x );			// xyz source
			WRITE_COORD( vecBout.y );
			WRITE_COORD( vecBout.z );
			WRITE_COORD( vecDir.x );						// xyz norme
			WRITE_COORD( vecDir.y );
			WRITE_COORD( vecDir.z );
			WRITE_CHAR ( 'a' );						// type de texture
			WRITE_BYTE ( 6 );						//  6 == outromuzzle

		MESSAGE_END();
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask( void )
{
	return 0;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
/*
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) )
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/
	if( FStrEq( STRING( pev->model ), "models/player.mdl" ) || FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	//modif de Julien
	else if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
		UTIL_SetSize(pev, Vector(0,0,0),Vector(0,0,0) );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	// modif de Julien
	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid			= SOLID_NOT;
		pev->movetype		= MOVETYPE_NOCLIP;
	}

	MonsterInit();

	if( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster::Precache()
{
	PRECACHE_MODEL( STRING( pev->model ) );
	PRECACHE_MODEL( "sprites/outro_muzzle.spr" );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
