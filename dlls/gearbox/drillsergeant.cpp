/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// Drill sergeant
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"

class CDrillSergeant : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int ISoundMask(void);
	virtual int ObjectCaps( void ) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int Classify() { return CLASS_PLAYER_ALLY_MILITARY; }
	void DeathSound( void );
	void PainSound( void );

	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);

	void DeclineFollowing();

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void TalkInit();

	float m_painTime;
};

LINK_ENTITY_TO_CLASS( monster_drillsergeant, CDrillSergeant )

TYPEDESCRIPTION	CDrillSergeant::m_SaveData[] =
{
	DEFINE_FIELD( CDrillSergeant, m_painTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDrillSergeant, CTalkMonster )

void CDrillSergeant::Precache()
{
	PRECACHE_MODEL("models/drill.mdl");
	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");
	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	TalkInit();
	CTalkMonster::Precache();
}

void CDrillSergeant::Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), "models/drill.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( &CTalkMonster::FollowerUse );
}

void CDrillSergeant::DeclineFollowing()
{
	PlaySentence( "DR_POK", 2, VOL_NORM, ATTN_NORM );
}

void CDrillSergeant::SetYawSpeed( void )
{
	int ys = 0;
	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

int CDrillSergeant::ISoundMask( void)
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

void CDrillSergeant::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CDrillSergeant::DeathSound( void )
{
	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CDrillSergeant::TalkInit()
{
	CTalkMonster::TalkInit();

 	m_szGrp[TLK_ANSWER] = "DR_ANSWER";
	m_szGrp[TLK_QUESTION] = "DR_QUESTION";
	m_szGrp[TLK_IDLE] = "DR_IDLE";
	m_szGrp[TLK_STARE] = "DR_STARE";
	m_szGrp[TLK_USE] = "DR_OK";
	m_szGrp[TLK_UNUSE] = "DR_WAIT";
	m_szGrp[TLK_STOP] = "DR_STOP";

 	m_szGrp[TLK_NOSHOOT] = "DR_SCARED";
	m_szGrp[TLK_HELLO] = "DR_HELLO";

 	m_szGrp[TLK_PLHURT1] = "!DR_CUREA";
	m_szGrp[TLK_PLHURT2] = "!DR_CUREB";
	m_szGrp[TLK_PLHURT3] = "!DR_CUREC";

 	m_szGrp[TLK_PHELLO] = NULL;// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;// UNDONE
	m_szGrp[TLK_PQUESTION] = "DR_PQUEST";

 	m_szGrp[TLK_SMELL] = "DR_SMELL";

 	m_szGrp[TLK_WOUND] = "DR_WOUND";
	m_szGrp[TLK_MORTAL] = "DR_MORTAL";
}

extern Schedule_t slBaFaceTarget[];
extern Schedule_t slBaFollow[];
extern Schedule_t slIdleBaStand[];

Schedule_t *CDrillSergeant::GetScheduleOfType( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used'
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;
	case SCHED_TARGET_CHASE:
		return slBaFollow;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CDrillSergeant::GetSchedule( void )
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if( m_hEnemy == 0 && IsFollowing() )
		{
			if( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	default:
		break;
	}

	return CTalkMonster::GetSchedule();
}
