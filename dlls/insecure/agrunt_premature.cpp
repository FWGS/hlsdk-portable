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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define AGRUNT_PRE_AE_ATTACK_RIGHT 12
#define AGRUNT_PRE_AE_ATTACK_LEFT 13

#define AGRUNT_PRE_FLINCH_DELAY 2 // at most one flinch every n secs

#define AGRUNT_MELEE_DIST 100

class CAGruntPremature : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	int IRelationship(CBaseEntity* pTarget);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	int IgnoreConditions( void);
	BOOL CheckMeleeAttack1(float flDot, float flDist);

	float m_flNextFlinch;

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAttackSounds[];
	static const char* pDieSounds[];
	static const char* pPainSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];

	void AlertSound( void );
	void DeathSound( void );
	void PainSound( void );
	void AttackSound( void);
	void PrescheduleThink( void);
	void StopTalking( void);
	bool ShouldSpeak( void);

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return false; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return false; }

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float m_flNextSpeakTime;
	float m_flNextWordTime;
	int m_iLastWord;
};

LINK_ENTITY_TO_CLASS(monster_alien_grunt_premature, CAGruntPremature);


const char* CAGruntPremature::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* CAGruntPremature::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CAGruntPremature::pAttackSounds[] =
{
	"agrunt/ag_attack1.wav",
	"agrunt/ag_attack2.wav",
	"agrunt/ag_attack3.wav",
};

const char* CAGruntPremature::pDieSounds[] =
{
	"agrunt/ag_die1.wav",
	"agrunt/ag_die4.wav",
	"agrunt/ag_die5.wav",
};

const char* CAGruntPremature::pPainSounds[] =
{
	"agrunt/ag_pain1.wav",
	"agrunt/ag_pain2.wav",
	"agrunt/ag_pain3.wav",
	"agrunt/ag_pain4.wav",
	"agrunt/ag_pain5.wav",
};

const char* CAGruntPremature::pIdleSounds[] =
{
	"agrunt/ag_idle1.wav",
	"agrunt/ag_idle2.wav",
	"agrunt/ag_idle3.wav",
	"agrunt/ag_idle4.wav",
};

const char* CAGruntPremature::pAlertSounds[] =
{
	"agrunt/ag_alert1.wav",
	"agrunt/ag_alert3.wav",
	"agrunt/ag_alert4.wav",
	"agrunt/ag_alert5.wav",
};

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CAGruntPremature::Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

int CAGruntPremature::IRelationship( CBaseEntity* pTarget )
{
	// Don't hate on the controller exterminator, yet.
	if ( FClassnameIs( pTarget->pev, "monster_controller_exterminator" ) )
	{
		return R_AL;
	}
	return CBaseMonster::IRelationship( pTarget );
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CAGruntPremature::SetYawSpeed( void )
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

//=========================================================
// TraceAttack
//=========================================================

//=========================================================
// StopTalking - won't speak again for 10-20 seconds.
//=========================================================
void CAGruntPremature::StopTalking( void )
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG( 0, 10 );
}

//=========================================================
// ShouldSpeak - Should this agrunt be talking?
//=========================================================
bool CAGruntPremature::ShouldSpeak( void )
{
	if ( m_flNextSpeakTime > gpGlobals->time )
	{
		// my time to talk is still in the future.
		return FALSE;
	}

	if ( ( pev->spawnflags & SF_MONSTER_GAG ) != 0 )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// if gagged, don't talk outside of combat.
			// if not going to talk because of this, put the talk time
			// into the future a bit, so we don't talk immediately after
			// going into combat
			m_flNextSpeakTime = gpGlobals->time + 3;
			return false;
		}
	}

	return TRUE;
}

//=========================================================
// PrescheduleThink
//=========================================================
void CAGruntPremature::PrescheduleThink( void )
{
	if ( ShouldSpeak() )
	{
		if ( m_flNextWordTime < gpGlobals->time )
		{
			int num = -1;

			do
			{
				num = RANDOM_LONG( 0, ARRAYSIZE( pIdleSounds ) - 1 );
			} while (num == m_iLastWord);

			m_iLastWord = num;

			// play a new sound
			EMIT_SOUND( ENT ( pev ), CHAN_VOICE, pIdleSounds[num], 1.0, ATTN_NORM );

			// is this word our last?
			if (RANDOM_LONG(1, 10) <= 1)
			{
				// stop talking.
				StopTalking();
			}
			else
			{
				m_flNextWordTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 1 );
			}
		}
	}
}

//=========================================================
// DieSound
//=========================================================
void CAGruntPremature::DeathSound( void )
{
	StopTalking();

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDieSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// AlertSound
//=========================================================
void CAGruntPremature::AlertSound( void )
{
	StopTalking();

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// AttackSound
//=========================================================
void CAGruntPremature::AttackSound( void )
{
	StopTalking();

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// PainSound
//=========================================================
void CAGruntPremature::PainSound()
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	StopTalking();

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CAGruntPremature::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case AGRUNT_PRE_AE_ATTACK_RIGHT:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack( AGRUNT_MELEE_DIST, gSkillData.agruntPreDmgPunch, DMG_CLUB );

		if ( pHurt )
		{
			pHurt->pev->punchangle.y = -25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if ( pHurt->IsPlayer() )
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 250;
			}

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 ); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
		}
	}
	break;

	case AGRUNT_PRE_AE_ATTACK_LEFT:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack( AGRUNT_MELEE_DIST, gSkillData.agruntPreDmgPunch, DMG_CLUB );

		if ( pHurt )
		{
			pHurt->pev->punchangle.y = 25;
			pHurt->pev->punchangle.x = 8;

			// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
			if ( pHurt->IsPlayer() )
			{
				// this is a player. Knock him around.
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * -250;
			}

			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			Vector vecArmPos, vecArmAng;
			GetAttachment( 0, vecArmPos, vecArmAng );
			SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 ); // a little surface blood.
		}
		else
		{
			// Play a random attack miss sound
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
		}
	}
	break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of
// any enemy that gets too close.
//=========================================================
BOOL CAGruntPremature::CheckMeleeAttack1(float flDot, float flDist)
{
	if ( HasConditions( bits_COND_SEE_ENEMY ) && flDist <= AGRUNT_MELEE_DIST && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// Spawn
//=========================================================
void CAGruntPremature::Spawn( void )
{
	Precache();

	SET_MODEL( ENT( pev ), "models/agrunt_premature.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.agruntPreHealth;
	pev->view_ofs = VEC_VIEW; // position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;	  // indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG(0, 10);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAGruntPremature::Precache( void )
{
	PRECACHE_MODEL( "models/agrunt_premature.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


int CAGruntPremature::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) )
	{
#if 0
		if ( pev->health < 20 )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
		else
#endif
			if ( m_flNextFlinch >= gpGlobals->time )
				iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if ( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if ( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + AGRUNT_PRE_FLINCH_DELAY;
	}

	return iIgnore;
}