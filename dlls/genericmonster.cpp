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
#include 	"soundent.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#define	SF_GENERICMONSTER_SCIENTIST					32
#define	SF_GENERICMONSTER_BARNEY					64

const int SF_GENERICMONSTER_CONTROLLER = 8;

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

	void PlayScriptedSentence( const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener );

	void MonsterThink( void );
	void IdleHeadTurn( Vector& vecFriend );

	void DeathSound( void );


	float m_talkTime;
	EHANDLE m_hTalkTarget;
	float m_flIdealYaw;
	float m_flCurrentYaw;
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster )

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CGenericMonster::Classify( void )
{
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
		ys = 100;
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
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster::ISoundMask( void )
{
	return bits_SOUND_NONE;
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
	if ( FStrEq( STRING( pev->model ), "models/player.mdl" ) 
		|| FStrEq( STRING( pev->model ), "models/holo.mdl" )
		|| FStrEq( STRING( pev->model ), "models/cross.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

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
	PRECACHE_MODEL( ( char* ) STRING( pev->model ) );

	PRECACHE_SOUND( "scientist/sci_pain1.wav" );
	PRECACHE_SOUND( "scientist/sci_pain2.wav" );
	PRECACHE_SOUND( "scientist/sci_pain3.wav" );
	PRECACHE_SOUND( "scientist/sci_pain4.wav" );
	PRECACHE_SOUND( "scientist/sci_pain5.wav" );

	PRECACHE_SOUND( "barney/ba_die1.wav" );
	PRECACHE_SOUND( "barney/ba_die2.wav" );
	PRECACHE_SOUND( "barney/ba_die3.wav" );

	PRECACHE_SOUND( "ambience/loader_hydra1.wav" );
	PRECACHE_SOUND( "ambience/loader_step1.wav" );

	PRECACHE_SOUND( "gonarch/gon_step1.wav" );
	PRECACHE_SOUND( "gonarch/gon_step2.wav" );
	PRECACHE_SOUND( "gonarch/gon_step3.wav" );
}

void CGenericMonster::PlayScriptedSentence( const char* pszSentence, float duration, float volume, float attenuation, bool bConcurrent, CBaseEntity* pListener )
{
	m_talkTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation );

	m_hTalkTarget = pListener;
}

void CGenericMonster::MonsterThink()
{
	if ( m_afCapability & bits_CAP_TURN_HEAD )
	{
		if ( m_hTalkTarget )
		{
			if ( gpGlobals->time > m_talkTime )
			{
				m_flIdealYaw = 0;
				m_hTalkTarget = NULL;
			}
			else
			{
				IdleHeadTurn( m_hTalkTarget->pev->origin );
			}
		}

		if ( m_flCurrentYaw != m_flIdealYaw )
		{
			if ( m_flCurrentYaw <= m_flIdealYaw )
			{
				m_flCurrentYaw += Q_min( 20.0, m_flIdealYaw - m_flCurrentYaw );
			}
			else
			{
				m_flCurrentYaw -= Q_min( 20.0, m_flCurrentYaw - m_flIdealYaw );
			}

			SetBoneController( 0, m_flCurrentYaw );
		}
	}

	CBaseMonster::MonsterThink();
}

// turn head towards supplied origin
void CGenericMonster::IdleHeadTurn( Vector& vecFriend )
{
	// turn head in desired direction only if ent has a turnable head
	if ( m_afCapability & bits_CAP_TURN_HEAD )
	{
		float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;

		if ( yaw > 180 ) 
			yaw -= 360;
		if ( yaw < -180 ) 
			yaw += 360;

		// turn towards vector
		m_flIdealYaw = yaw;
	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

void CGenericMonster::DeathSound()
{
	// for some reason, scripting the death sound inside
	// the maps doesn't work so here we go.
	if ( pev->spawnflags & SF_GENERICMONSTER_SCIENTIST )
	{
		switch ( RANDOM_LONG( 0, 4 ) )
		{
		case 0:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 2:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 3:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 4:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, 100 );
			break;
		}
	}
	else if (pev->spawnflags & SF_GENERICMONSTER_BARNEY)
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 1:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, 100 );
			break;
		case 2:
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, 100 );
			break;
		}
	}
}