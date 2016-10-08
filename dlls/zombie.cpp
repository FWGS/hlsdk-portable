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
#include	"zombie.h"

//
// Spawn Flags
//
#define SF_ZOMBIE_FASTMODE	1024

#define NUM_ZOMBIE1_BODIES	11
#define NUM_ZOMBIE2_BODIES	6
#define NUM_ZOMBIE3_BODIES	5

enum
{
	ZOMBIE1_FUNERAL = 0,
	ZOMBIE1_FUNERAL_HEADLESS,
	ZOMBIE1_CIVILIAN,
	ZOMBIE1_CIVILIAN_HEADLESS,
	ZOMBIE1_COP,
	ZOMBIE1_FEMALE,
	ZOMBIE1_BIOHAZARD_SUIT,
	ZOMBIE1_ECHELON_OFFICER,
	ZOMBIE1_EINSTEIN,
	ZOMBIE1_DOCTOR,
	ZOMBIE1_PATIENT
};

enum
{
	ZOMBIE2_COP_HEADOPEN = 0,
	ZOMBIE2_COP_CROWBAR,
	ZOMBIE2_DOCTOR,
	ZOMBIE2_DOCTOR_BUTCHER,
	ZOMBIE2_DOCTOR_SHOTGUN,
	ZOMBIE2_CIVILIAN_CHEST_GUN
};

enum
{
	ZOMBIE3_NEIL = 0,
	ZOMBIE3_BROOM,
	ZOMBIE3_OLD,
	ZOMBIE3_MECHANIC,
	ZOMBIE3_HAMMER
};

enum
{
	LPZOMBIE_STANDARD = 0,
	LPZOMBIE_COP,
	LPZOMBIE_BURNT,
	LPZOMBIE_FLESH
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY		2		// at most one flinch every n secs

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie )

TYPEDESCRIPTION	CZombie::m_SaveData[] =
{
	DEFINE_FIELD( CZombie, m_iZombieFlags, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CZombie, CBaseMonster )

const char *CZombie::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

const char *CZombie::pCopAttackSounds[] =
{
	"zombiecop/zo_attack1.wav",
	"zombiecop/zo_attack2.wav",
};

const char *CZombie::pCopIdleSounds[] =
{
	"zombiecop/zo_idle1.wav",
	"zombiecop/zo_idle2.wav",
	"zombiecop/zo_idle3.wav",
	"zombiecop/zo_idle4.wav",
};

const char *CZombie::pCopAlertSounds[] =
{
	"zombiecop/zo_alert10.wav",
	"zombiecop/zo_alert20.wav",
	"zombiecop/zo_alert30.wav",
};

const char *CZombie::pCopPainSounds[] =
{
	"zombiecop/zo_pain1.wav",
	"zombiecop/zo_pain2.wav",
};

const char *CZombie::pFemaleAttackSounds[] =
{
	"zfemale/zo_attack1.wav",
	"zfemale/zo_attack2.wav",
};

const char *CZombie::pFemaleIdleSounds[] =
{
	"zfemale/zo_idle1.wav",
	"zfemale/zo_idle2.wav",
	"zfemale/zo_idle3.wav",
	"zfemale/zo_idle4.wav",
};

const char *CZombie::pFemaleAlertSounds[] =
{
	"zfemale/zo_alert10.wav",
	"zfemale/zo_alert20.wav",
	"zfemale/zo_alert30.wav",
};

const char *CZombie::pFemalePainSounds[] =
{
	"zfemale/zo_pain1.wav",
	"zfemale/zo_pain2.wav",
};

const char *CZombie::pNurseAttackSounds[] =
{
	"znurse/zo_attack1.wav",
	"znurse/zo_attack2.wav",
};

const char *CZombie::pNurseIdleSounds[] =
{
	"znurse/zo_idle1.wav",
	"znurse/zo_idle2.wav",
	"znurse/zo_idle3.wav",
	"znurse/zo_idle4.wav",
};

const char *CZombie::pNurseAlertSounds[] =
{
	"znurse/zo_alert10.wav",
	"znurse/zo_alert20.wav",
	"znurse/zo_alert30.wav",
};

const char *CZombie::pNursePainSounds[] =
{
	"znurse/zo_pain1.wav",
	"znurse/zo_pain2.wav",
};

const char *CZombie::pNewAttackSounds[] =
{
	"zombienew/zo_attack1.wav",
	"zombienew/zo_attack2.wav",
};

const char *CZombie::pNewIdleSounds[] =
{
	"zombienew/zo_idle1.wav",
	"zombienew/zo_idle2.wav",
	"zombienew/zo_idle3.wav",
	"zombienew/zo_idle4.wav",
};

const char *CZombie::pNewAlertSounds[] =
{
	"zombienew/zo_alert10.wav",
	"zombienew/zo_alert20.wav",
	"zombienew/zo_alert30.wav",
};

const char *CZombie::pNewPainSounds[] =
{
	"zombienew/zo_pain1.wav",
	"zombienew/zo_pain2.wav",
	"zombienew/zo_pain3.wav",
};

BOOL CZombie::IsFemale() const
{
	return ( m_iZombieFlags & ZF_FEMALE );
}

BOOL CZombie::IsNurse() const
{
	return ( m_iZombieFlags & ZF_NURSE );
}

BOOL CZombie::IsCop() const
{
	return ( m_iZombieFlags & ZF_COP );
}

BOOL CZombie::UseNewSounds() const
{
	return ( m_iZombieFlags & ZF_NEWSOUNDS );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CZombie::Classify( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed( void )
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

int CZombie::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie::PainSound( void )
{
	if( RANDOM_LONG( 0, 5 ) < 2 )
	{
		int pitch = 95 + RANDOM_LONG( 0, 9 );

		if( IsFemale() )
		{
			if( IsNurse() )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNursePainSounds[RANDOM_LONG( 0, ARRAYSIZE( pNursePainSounds) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pFemalePainSounds[RANDOM_LONG( 0, ARRAYSIZE( pFemalePainSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
		}
		else
		{
			if( IsCop() )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pCopPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pCopPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
			else
			{
				if( UseNewSounds() )
				{
					EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNewPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pNewPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
				}
				else
				{
					EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
				}
			}
		}
	}
}

void CZombie::AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	if( IsFemale() )
	{
		if( IsNurse() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNurseAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pNurseAlertSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
		else
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pFemaleAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pFemaleAlertSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
	}
	else
	{
		if( IsCop() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pCopAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pCopAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		}
		else
		{
			if( UseNewSounds() )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNewAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pNewAlertSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pAlertSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
		}
	}
}

void CZombie::IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	if( IsFemale() )
	{
		if( IsNurse() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNurseIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pNurseIdleSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
		else
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pFemaleIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pFemaleIdleSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
	}
	else
	{
		if( IsCop() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pCopIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pCopIdleSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
		}
		else
		{
			if( UseNewSounds() )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNewIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pNewIdleSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pIdleSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
		}
	}
}

void CZombie::AttackSound( void )
{
	int pitch = 100 + RANDOM_LONG( -5, 5 );

	// Play a random attack sound
	if( IsFemale() )
	{
		if( IsNurse() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNurseAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pNurseAttackSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
		else
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pFemaleAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pFemaleAttackSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
	}
	else
	{
		if( IsCop() )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pCopAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pCopAttackSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
		}
		else
		{
			if( UseNewSounds() )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pNewAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pNewAttackSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
			else
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackSounds ) - 1 )], 1.0, ATTN_NORM, 0, pitch );
			}
		}
}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5 , 5 ) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgBothSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

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
void CZombie::Spawn()
{
	Precache();

	char* szModel = (char*)STRING( pev->model );
	if( !szModel || !*szModel )
	{
		szModel = "models/zombie.mdl";
		pev->model = ALLOC_STRING( szModel );
	}

	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health		= gSkillData.zombieHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_iZombieFlags = 0;

	if( FStrEq( STRING( pev->model ), "models/zombie.mdl" ) )
	{
		switch( pev->body )
		{
		case ZOMBIE1_COP:
			m_iZombieFlags |= ZF_COP;
			break;
		case ZOMBIE1_FEMALE:
			m_iZombieFlags |= ZF_FEMALE;
			break;
		default:
			break;
		}
	}
	else if( FStrEq( STRING( pev->model ), "models/zombie2.mdl" ) )
	{
		switch( pev->body )
		{
		case ZOMBIE2_COP_HEADOPEN:
		case ZOMBIE2_COP_CROWBAR:
			m_iZombieFlags |= ZF_COP;
			break;
		default:
			break;
		}
	}
	else if( FStrEq( STRING( pev->model ), "models/zombie3.mdl" ) )
	{
		// Third zombie model always use new sounds.
		m_iZombieFlags |= ZF_NEWSOUNDS;

		switch( pev->body )
		{
		case ZOMBIE3_BROOM:
			m_iZombieFlags |= ZF_FEMALE;
			break;
		default:
			break;
		}
	}
	else if( FStrEq( STRING( pev->model ), "models/nursezombie.mdl" ) )
	{
		m_iZombieFlags |= ( ZF_FEMALE | ZF_NURSE );
	}
	else if( FStrEq( STRING( pev->model ), "models/lpzombie.mdl" ) ) // Special zombie type with few polygons.
	{
		// Low polygon zombie model always use new sounds.
		m_iZombieFlags |= ZF_NEWSOUNDS;

		switch( pev->skin )
		{
		case LPZOMBIE_COP:
			m_iZombieFlags |= ZF_COP;
			break;
		case LPZOMBIE_FLESH:
			// Unable to differentiate genders so set this flag randomly.
			if( RANDOM_LONG( 0, 1 ) )
				m_iZombieFlags |= ZF_FEMALE;
			break;
		default:
			break;
		}
	}
	else
	{
		ALERT( at_warning, "Unsupported zombie model %s\n", STRING( pev->model ) );
}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	int i;

	PRECACHE_MODEL( "models/zombie.mdl" );
	PRECACHE_MODEL( "models/zombie2.mdl" );
	PRECACHE_MODEL( "models/zombie3.mdl" );
	PRECACHE_MODEL( "models/nursezombie.mdl" );
	PRECACHE_MODEL( "models/lpzombie.mdl" );

	for( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackHitSounds[i] );

	for( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackMissSounds[i] );

	for( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char *)pAttackSounds[i] );

	for( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char *)pIdleSounds[i] );

	for( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char *)pAlertSounds[i] );

	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char *)pPainSounds[i] );

	PRECACHE_SOUND_ARRAY( pCopAttackSounds );
	PRECACHE_SOUND_ARRAY( pCopIdleSounds );
	PRECACHE_SOUND_ARRAY( pCopAlertSounds );
	PRECACHE_SOUND_ARRAY( pCopPainSounds );

	PRECACHE_SOUND_ARRAY( pFemaleAttackSounds );
	PRECACHE_SOUND_ARRAY( pFemaleIdleSounds );
	PRECACHE_SOUND_ARRAY( pFemaleAlertSounds );
	PRECACHE_SOUND_ARRAY( pFemalePainSounds );

	PRECACHE_SOUND_ARRAY( pNurseAttackSounds );
	PRECACHE_SOUND_ARRAY( pNurseIdleSounds );
	PRECACHE_SOUND_ARRAY( pNurseAlertSounds );
	PRECACHE_SOUND_ARRAY( pNursePainSounds );

	PRECACHE_SOUND_ARRAY( pNewAttackSounds );
	PRECACHE_SOUND_ARRAY( pNewIdleSounds );
	PRECACHE_SOUND_ARRAY( pNewAlertSounds );
	PRECACHE_SOUND_ARRAY( pNewPainSounds );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CZombie::IgnoreConditions( void )
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
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

//========================================================
// RunAI - overridden for zombie because there are things
// that need to be checked every think.
//========================================================
void CZombie::RunAI( void )
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if( pev->spawnflags & SF_ZOMBIE_FASTMODE )
	{
		if( m_Activity == ACT_WALK || m_Activity == ACT_RUN )
		{
			pev->framerate = 1.5;
		}
	}
}
