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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#include "Cow.h"


LINK_ENTITY_TO_CLASS( monster_cow, CCow );

TYPEDESCRIPTION	CCow::m_SaveData[] = 
{
	DEFINE_FIELD( CCow, m_iszGibModel, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CCow, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCow :: Classify ( void )
{
	return CLASS_NONE;
}

//=========================================================
// Spawn
//=========================================================
void CCow :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/cow.mdl");
	UTIL_SetSize( pev, Vector( -48, -16, 0 ), Vector( 48, 16, 72 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= 20;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	SetActivity ( ACT_IDLE );

	//pev->view_ofs		= Vector ( 0, 0, 1 );// position of the eyes relative to monster's origin.
	//pev->takedamage		= DAMAGE_YES;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCow :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/cow.mdl");

	m_iszGibModel = ALLOC_STRING("models/horsegibs.mdl");
	PRECACHE_MODEL( "models/horsegibs.mdl" );

	PRECACHE_SOUND("cow/cowmoo.wav");
	PRECACHE_SOUND("cow/cowdie.wav");
}	

//=========================================================
// Killed.
//=========================================================
void CCow :: IdleSound( void )
{
	if (RANDOM_LONG(0,5) < 2)
	{
		// Play a random idle sound
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "cow/cowmoo.wav", 1.0, ATTN_NORM, 0, 100 );
	}
}

//=========================================================
// Killed.
//=========================================================
void CCow :: Killed( entvars_t *pevAttacker, int iGib )
{
//	pev->solid = SOLID_NOT;

//	CSoundEnt::InsertSound ( bits_SOUND_WORLD, pev->origin, 128, 1 );

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cow/cowdie.wav", 0.8, ATTN_NORM, 0, 80 + RANDOM_LONG(0,39) );

//	CBaseEntity *pOwner = CBaseEntity::Instance(pev->owner);
//	if ( pOwner )
//	{
//		pOwner->DeathNotice( pev );
//	}
//	UTIL_Remove( this );
	CBaseMonster::Killed(pevAttacker, GIB_ALWAYS);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

