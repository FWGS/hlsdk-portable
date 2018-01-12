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
#include	"headcrab.h"
#include	"game.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		RAT_AE_JUMPATTACK	( 2 )

class CRat : public CHeadCrab
{
public:
	void Spawn( void );
	void Precache( void );
	int Classify( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void EXPORT RatLeapTouch( CBaseEntity *pOther );
	void StartTask( Task_t *pTask );

	void PainSound( void );
	void DeathSound( void );
	void IdleSound( void );
	void AlertSound( void );

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
};

LINK_ENTITY_TO_CLASS( monster_rat, CRat )

const char *CRat::pIdleSounds[] =
{
	"rat/rat_idle.wav"
};
 
const char *CRat::pAlertSounds[] =
{
	"rat/rat_alert.wav"
};
  
const char *CRat::pPainSounds[] =
{
	"rat/rat_pain.wav"
};
        
const char *CRat::pAttackSounds[] =
{
	"rat/rat_attack.wav"
};

const char *CRat::pDeathSounds[] =
{
	"rat/rat_die.wav"
};

const char *CRat::pBiteSounds[] =
{
	"rat/rat_headbite.wav"
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CRat::Classify( void )
{
	return CLASS_HUMAN_MILITARY;
}

//=========================================================
// Spawn
//=========================================================
void CRat::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/bigrat.mdl" );
	if( !strncmp( STRING( gpGlobals->mapname ), "asmap", 5 ) )
		UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 24 ) );
	else
		UTIL_SetSize( pev, g_vecZero, g_vecZero );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	pev->view_ofs = Vector( 0, 0, 6 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRat::Precache()
{
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );

	PRECACHE_MODEL( "models/bigrat.mdl" );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CRat::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case RAT_AE_JUMPATTACK:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors( pev->angles );

			Vector vecJumpDir;
			if( m_hEnemy != 0 )
			{
				float gravity = g_psv_gravity->value;
				if( gravity <= 1 )
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z;
				if( height < 16 )
					height = 16;
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin;
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();

				if( distance > 650 )
				{
					vecJumpDir = vecJumpDir * ( 650.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			EMIT_SOUND_DYN( edict(), CHAN_VOICE, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// LeapTouch - this is the rat's touch function when it
// is in the air
//=========================================================
void CRat::RatLeapTouch( CBaseEntity *pOther )
{
        if( !pOther->pev->takedamage )
        {
                return;
        }

        if( pOther->Classify() == Classify() )
        {
                return;
        }
        
        // Don't hit if back on ground
        if( !FBitSet( pev->flags, FL_ONGROUND ) )
        {
                EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pBiteSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
 
                pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_SLASH );
        }

        SetTouch( NULL );
}

void CRat::StartTask( Task_t *pTask )
{
        m_iTaskStatus = TASKSTATUS_RUNNING;

        switch( pTask->iTask )
        {
        case TASK_RANGE_ATTACK1:
                {
                        EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackSounds[0], GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
                        m_IdealActivity = ACT_RANGE_ATTACK1;
                        SetTouch( &CRat::RatLeapTouch );
                        break;
                }
        default:
                {
                        CBaseMonster::StartTask( pTask );
                }
        }
}

//=========================================================
// IdleSound
//=========================================================
void CRat::IdleSound( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CRat::AlertSound( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AlertSound 
//=========================================================
void CRat::PainSound( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// DeathSound 
//=========================================================
void CRat::DeathSound( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDeathSounds ), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
