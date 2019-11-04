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
// alien_egg.cpp - spawns headcrabs
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

#include	"alien_egg.h"

const int DEAD_EGG_Z = 8;

LINK_ENTITY_TO_CLASS( monster_alienegg, CAlienEgg );

TYPEDESCRIPTION	CAlienEgg::m_SaveData[] = 
{
	DEFINE_FIELD( CAlienEgg, m_iOrientation, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CAlienEgg, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CAlienEgg :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREY;
}

//=========================================================
// Center - returns the real center of the headcrab.  The 
// bounding box is much larger than the actual creature so 
// this is needed for targeting
//=========================================================
Vector CAlienEgg :: Center ( void )
{
	if (m_iOrientation == 0)
	{
		return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 8 );
	}
	else
	{
		return Vector( pev->origin.x, pev->origin.y, pev->origin.z - 8 );
	}
}


Vector CAlienEgg :: BodyTarget( const Vector &posSrc ) 
{
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CAlienEgg :: SetYawSpeed ( void )
{
}

void CAlienEgg::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "orientation"))
	{
		m_iOrientation = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;

	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Spawn
//=========================================================
void CAlienEgg :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/alien_egg.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 48));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_NONE;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.headcrabHealth;
	pev->yaw_speed		= 0;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	if (m_iOrientation == 1)
	{
		UTIL_SetSize(pev, Vector(-16, -16, -48), Vector(16, 16, 0));

		pev->idealpitch = 180;
		pev->flags |= FL_FLY;
		SetBits( pev->spawnflags, SF_MONSTER_FALL_TO_GROUND);
		pev->effects |= EF_INVLIGHT;

		pev->angles.x = 180;
		pev->angles.y = pev->angles.y + 180;
		if (pev->angles.y > 360)
			pev->angles.y = pev->angles.y - 360;
	}

	MonsterInit();

	SetUse(EggUse);

	if (m_iOrientation == 0)
	{
		pev->view_ofs	= Vector ( 0, 0, 32 );// position of the eyes relative to monster's origin.
	}
	else
	{
		pev->view_ofs	= Vector ( 0, 0, -32 );// position of the eyes relative to monster's origin.
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAlienEgg :: Precache()
{
	PRECACHE_SOUND("gonarch/gon_birth3.wav");
	PRECACHE_SOUND("common/bodysplat.wav");

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/alien_egg.mdl");

	UTIL_PrecacheOther( "monster_headcrab" );
}	


//=========================================================
// RunTask 
//=========================================================
void CAlienEgg :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
		{
			TaskComplete();
			break;
		}
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();

				Burst();

				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}


void CAlienEgg :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_FACE_ENEMY:
		{
			TaskComplete();
			break;
		}
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			CBaseMonster :: StartTask( pTask );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CAlienEgg :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 128 + (128 * m_iOrientation) )
	{
		return TRUE;
	}
	return FALSE;
}

int CAlienEgg :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CAlienEgg :: Killed( entvars_t *pevAttacker, int iGib )
{
	// change body
	pev->body = 1;
	// spray gibs
	CGib::SpawnRandomGibs( pev, 6, 0 );	// throw some human gibs.
	// make sound
	EMIT_SOUND_DYN( edict(), CHAN_BODY, "common/bodysplat.wav", 1.0, ATTN_IDLE, 0, 100 );
	// cannot use now
	SetUse(NULL);

	pev->deadflag = DEAD_DEAD;
	SetThink(NULL);
	StopAnimation();
	if (m_iOrientation == 0)
	{
		UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->mins.z ), Vector ( pev->maxs.x, pev->maxs.y, pev->mins.z + DEAD_EGG_Z ) );
	}
	else
	{
		UTIL_SetSize ( pev, Vector ( pev->mins.x, pev->mins.y, pev->maxs.z - DEAD_EGG_Z), Vector ( pev->maxs.x, pev->maxs.y, pev->maxs.z ) );
	}
	pev->solid = SOLID_NOT;
}

void CAlienEgg::Burst()
{
	pev->health = 0;
	Killed(NULL, GIB_NEVER);
	
	// spawn a headcrab
	edict_t* pent = CREATE_NAMED_ENTITY( ALLOC_STRING("monster_headcrab") );

	if ( FNullEnt( pent ) )
	{
		ALERT ( at_debug, "NULL Ent in Alien Egg!\n" );
		return;
	}

	entvars_t* pevCreate = VARS( pent );
	pevCreate->origin = Center();
	if (m_iOrientation == 0)
	{
	}
	else
	{
		pevCreate->origin.z -= 32;
	}
	pevCreate->angles.y = pev->angles.y;
	pevCreate->angles.z = pev->angles.z;
	SetBits( pevCreate->spawnflags, SF_MONSTER_FALL_TO_GROUND );

	DispatchSpawn( ENT( pevCreate ) );
}

void CAlienEgg::EggUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (IsAlive())
	{
		Burst();
	}
}

Schedule_t *CAlienEgg :: GetSchedule ( void )
{
	// we can see the enemy
	if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
	{
		return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
	}
	
	return GetScheduleOfType ( SCHED_IDLE_STAND );
}


