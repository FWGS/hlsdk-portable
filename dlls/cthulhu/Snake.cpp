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
// Snake
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SNAKE_AE_ATTACK		0x01

#define SNAKE_FLINCH_DELAY		2		// at most one flinch every n secs


#include "Snake.h"


LINK_ENTITY_TO_CLASS( monster_snake, CSnake );

TYPEDESCRIPTION	CSnake::m_SaveData[] = 
{
	DEFINE_FIELD( CSnake, m_iszGibModel, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CSnake, CBaseMonster );

const char *CSnake::pAttackSounds[] = 
{
	"snake/snake_attack1.wav",
	"snake/snake_attack2.wav",
};

const char *CSnake::pIdleSounds[] = 
{
	"snake/snake_idle1.wav",
	"snake/snake_idle2.wav",
};

const char *CSnake::pPainSounds[] = 
{
	"snake/snake_pain1.wav",
	"snake/snake_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSnake :: Classify ( void )
{
	// snakes are servants of great old ones...so treat them like aliens....if they are not "owned" by anyone else...
	if (mpOwner == NULL)
		return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;

	return mpOwner->Classify();
}

int CSnake::IRelationship( CBaseEntity *pTarget )
{
	return CBaseMonster::IRelationship( pTarget );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSnake :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

int CSnake :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSnake :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CSnake :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CSnake :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSnake :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SNAKE_AE_ATTACK:
		{
			pev->size.z += 48;
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.snakeDmgBite, DMG_POISON );
			pev->size.z -= 48;

			if (RANDOM_LONG(0,1))
				AttackSound();

			// can only bite once per second
			m_fNextBiteTime = gpGlobals->time + 1.0;
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CSnake :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/snake.mdl");
	UTIL_SetSize( pev, Vector(-16,-16,0), Vector(16,16,20) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= gSkillData.snakeHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_MELEE_ATTACK1 | bits_CAP_SWIM;
	m_fNextBiteTime		= 0.0;
	mpOwner				= NULL;

	MonsterInit();
}

void CSnake :: SetOwner ( CBaseEntity* pOwner )
{
	mpOwner = pOwner;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSnake :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/snake.mdl");
	m_iszGibModel = ALLOC_STRING("models/snakegibs.mdl");
	PRECACHE_MODEL( "models/snakegibs.mdl" ); //LRC

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

BOOL CSnake::CheckMeleeAttack1 ( float flDot, float flDist )
{
	// are we in range and other conditions, plus have we waited long enough
	// since the last bite
	return (CBaseMonster::CheckMeleeAttack1(flDot,flDist) && (m_fNextBiteTime <= gpGlobals->time));
}

BOOL CSnake::CheckMeleeAttack2 ( float flDot, float flDist )
{
	// are we in range and other conditions, plus have we waited long enough
	// since the last bite
	return (CBaseMonster::CheckMeleeAttack2(flDot,flDist) && (m_fNextBiteTime <= gpGlobals->time));
}

int CSnake::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + SNAKE_FLINCH_DELAY;
	}

	return iIgnore;
	
}