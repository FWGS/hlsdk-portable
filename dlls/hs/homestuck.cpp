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
// monster template
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CHomestuckFan : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	void IdleSound( void );
	void PainSound( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
};
LINK_ENTITY_TO_CLASS( monster_homestuck, CHomestuckFan );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHomestuckFan :: Classify ( void )
{
	return	CLASS_CWC;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHomestuckFan :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
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
void CHomestuckFan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CHomestuckFan :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/homosuck.mdl");
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= CBaseMonster::GetHealth( 45, 3 );
	pev->view_ofs		= Vector ( 0, 0, 0 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHomestuckFan :: Precache()
{
	PRECACHE_SOUND("homestuck/lemmetellyou.wav");

	PRECACHE_MODEL("models/homosuck.mdl");
}	

void CHomestuckFan::IdleSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "homestuck/lemmetellyou.wav", 1, ATTN_NORM, 0, 100 );
}

void CHomestuckFan::PainSound( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "homestuck/lemmetellyou.wav", 1, ATTN_NORM, 0, 100 );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
