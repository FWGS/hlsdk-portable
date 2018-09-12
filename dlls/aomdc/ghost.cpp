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
// Spider ghost monster
//=========================================================

#include	"zombie.h"
#include	"shake.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		GHOST_AE_CLAW1		( 1 )
#define		GHOST_AE_CLAW2		( 2 )
#define		GHOST_AE_CLAW3		( 3 )

class CGhost : public CZombie
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAlertSounds[];
	static const char *pDeathSounds[];
};

LINK_ENTITY_TO_CLASS( monster_ghost, CGhost );

const char *CGhost::pAlertSounds[] =
{
	"aslave/slv_alert1.wav",
	"aslave/slv_alert2.wav",
	"aslave/slv_alert3.wav",
	"aslave/slv_alert4.wav"
};

const char *CGhost::pDeathSounds[] =
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

void CGhost :: PainSound( void )
{
}

void CGhost :: AlertSound( void )
{
	int pitch = PITCH_LOW + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGhost :: IdleSound( void )
{
}

void CGhost :: AttackSound( void )
{
}

//=========================================================
// DieSound
//=========================================================
void CGhost :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_LOW + RANDOM_LONG(0,9) );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGhost :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GHOST_AE_CLAW1:
		case GHOST_AE_CLAW2:
		case GHOST_AE_CLAW3:
		{
			if (m_flNextAttack < gpGlobals->time)
			{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.slaveDmgClaw, DMG_POISON );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						UTIL_ScreenFade( pHurt, Vector( 255, 0, 0 ), 1.0, 11.0, 100, FFADE_IN );
						EMIT_SOUND( ENT( pHurt->pev), CHAN_ITEM, "ghost/ear_ringing.wav", 1.0, ATTN_NORM );
						m_flNextAttack = gpGlobals->time + 12.0f;
					}
				}
			}
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
void CGhost :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/ghost.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= gSkillData.slaveHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGhost :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL( STRING(pev->model) ); //LRC
	else
		PRECACHE_MODEL("models/ghost.mdl");

	PRECACHE_SOUND("ghost/ear_ringing.wav");

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( pAlertSounds[i] );

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
                PRECACHE_SOUND( pDeathSounds[i] );
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
