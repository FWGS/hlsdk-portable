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
#include	"animation.h"
#include	"weapons.h"
// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CLaoba : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	int m_iTrail;

	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_laoba, CLaoba );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CLaoba :: Classify ( void )
{
	return	CLASS_PLAYER;
}

void CLaoba :: RunAI( void )
{
	CBaseMonster :: RunAI();
	
}

void CLaoba :: SetActivity ( Activity NewActivity )
{
	
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CLaoba :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CLaoba :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
		pev->body = 1;
		}
		break;

	case 2:
		{
		pev->body = 2;
		}
		break;

	case 3:
		{
		pev->skin = 1;
		}
		break;

	case 4:
		{
		Vector vecArmPos,vecArmDir;
		GetAttachment( 0, vecArmPos, vecArmDir );
		SpawnBlood(vecArmPos, BLOOD_COLOR_YELLOW, 40);
		}
		break;
	
	case 5:
		{
		pev->skin = 3;
		}
		break;

	case 6:
		{
		pev->skin = 4;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CLaoba :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CLaoba :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/laoba.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 80;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CLaoba :: Precache()
{
	PRECACHE_MODEL("models/laoba.mdl");

}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
