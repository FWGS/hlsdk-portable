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
// Zombie
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
#define	EVILSCI_AE_ATTACK_RIGHT		0x01
#define	EVILSCI_AE_ATTACK_LEFT		0x02
#define	EVILSCI_AE_ATTACK_BOTH		0x03

#define EVILSCI_FLINCH_DELAY		2		// at most one flinch every n secs
#define EVILSCI_ONESLASH_DAMAGE		50
#define EVILSCI_BOTHSLASH_DAMAGE	80

class CEvilsci : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_evilsci, CEvilsci )

const char *CEvilsci::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CEvilsci::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CEvilsci::pAttackSounds[] =
{
	"scientist/evil_die.wav",
	"scientist/c3a2_sci_fool.wav",
};

const char *CEvilsci::pIdleSounds[] =
{
	"scientist/getoutalive.wav",
	"scientist/hearsomething.wav",
	"scientist/ihearsomething.wav",
	"scientist/peculiarmarks.wav",
	"scientist/smellburn.wav",
	"scientist/cantbeworse.wav",
};

const char *CEvilsci::pAlertSounds[] =
{
	"scientist/startle4.wav",
	"scientist/sci_fear2.wav",
	"scientist/heal5.wav",
	"scientist/letsgo.wav",
	"scientist/sci_fear14.wav",
	"scientist/startle2.wav",
};

const char *CEvilsci::pPainSounds[] =
{
	"scientist/sci_pain1.wav",
	"scientist/sci_pain2.wav",
	"scientist/sci_pain3.wav",
	"scientist/sci_pain4.wav",
	"scientist/sci_pain5.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CEvilsci::Classify( void )
{
	return CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEvilsci::SetYawSpeed( void )
{
	int ys;

	ys = 160;
#if 0
	switch ( m_Activity )
	{
	}
#endif
	pev->yaw_speed = ys;
}

int CEvilsci::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5f;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3f;
	}

	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CEvilsci::PainSound( void )
{
        int pitch = 95 + RANDOM_LONG( 0, 9 );

        if( RANDOM_LONG( 0, 5 ) < 2 )
                EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CEvilsci::AlertSound( void )
{
        int pitch = 95 + RANDOM_LONG( 0, 9 );

        EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CEvilsci::IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	// Play a random idle sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CEvilsci::AttackSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	// Play a random attack sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0, ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CEvilsci::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case EVILSCI_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, EVILSCI_ONESLASH_DAMAGE, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case EVILSCI_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, EVILSCI_ONESLASH_DAMAGE, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case EVILSCI_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, EVILSCI_BOTHSLASH_DAMAGE, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
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
void CEvilsci::Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), "models/evilsci.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health		= 800;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.8f;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEvilsci::Precache()
{
	PRECACHE_MODEL( "models/evilsci.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CEvilsci::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) )
	{
#if 0
		if( pev->health < 20 )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE| bits_COND_HEAVY_DAMAGE );
		else
#endif
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + EVILSCI_FLINCH_DELAY;
	}

	return iIgnore;
}
