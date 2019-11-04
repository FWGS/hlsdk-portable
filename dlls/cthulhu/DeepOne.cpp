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
// DeepOne
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"flyingmonster.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	DEEP_ONE_AE_ATTACK_RIGHT		0x01
#define	DEEP_ONE_AE_ATTACK_LEFT		0x02
#define	DEEP_ONE_AE_ATTACK_BOTH		0x03

#define DEEP_ONE_FLINCH_DELAY			2		// at most one flinch every n secs


#include "DeepOne.h"


LINK_ENTITY_TO_CLASS( monster_deepone, CDeepOne );

const char *CDeepOne::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDeepOne::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CDeepOne::pAttackSounds[] = 
{
	"deepone/do_attack1.wav",
	"deepone/do_attack2.wav",
};

const char *CDeepOne::pIdleSounds[] = 
{
	"deepone/do_idle1.wav",
	"deepone/do_idle2.wav",
};

const char *CDeepOne::pAlertSounds[] = 
{
	"deepone/do_alert1.wav",
	"deepone/do_alert2.wav",
};

const char *CDeepOne::pPainSounds[] = 
{
	"deepone/do_pain1.wav",
	"deepone/do_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDeepOne :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDeepOne :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

int CDeepOne :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	//if ( bitsDamageType == DMG_BULLET )
	//{
	//	Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
	//	vecDir = vecDir.Normalize();
	//	float flForce = DamageForce( flDamage );
	//	pev->velocity = pev->velocity + vecDir * flForce;
	//	flDamage *= 0.3;
	//}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CDeepOne :: PainSound( void )
{
	int pitch = GetVoicePitch() - 5 + RANDOM_LONG(0,10);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], GetVolume(), ATTN_NORM, 0, pitch );
}

void CDeepOne :: AlertSound( void )
{
	int pitch = GetVoicePitch() - 5 + RANDOM_LONG(0,10);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], GetVolume(), ATTN_NORM, 0, pitch );
}

void CDeepOne :: IdleSound( void )
{
	int pitch = GetVoicePitch() - 5 + RANDOM_LONG(0,10);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], GetVolume(), ATTN_NORM, 0, pitch );
}

void CDeepOne :: AttackSound( void )
{
	int pitch = GetVoicePitch() - 5 + RANDOM_LONG(0,10);

	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], GetVolume(), ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================

float CDeepOne :: GetAttackDist( void )
{
	return 70.0;
}

void CDeepOne :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	double fldist = GetAttackDist();

	switch( pEvent->event )
	{
		case DEEP_ONE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( fldist, gSkillData.deeponeDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case DEEP_ONE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( fldist, gSkillData.deeponeDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case DEEP_ONE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( fldist, gSkillData.deeponeDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
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
void CDeepOne :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/deepone.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.deeponeHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP | bits_CAP_SWIM | bits_CAP_CLIMB | bits_CAP_JUMP;

	MonsterInit();

}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDeepOne :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/deepone.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CDeepOne::IgnoreConditions ( void )
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
			m_flNextFlinch = gpGlobals->time + DEEP_ONE_FLINCH_DELAY;
	}

	return iIgnore;
	
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

LINK_ENTITY_TO_CLASS( monster_dagon, CDagon );

void CDagon :: Spawn( void )
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/dagon.mdl");

	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 216));
	
	if (pev->health == 0)
		pev->health	= gSkillData.deeponeHealth * 9;

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP | bits_CAP_SWIM | bits_CAP_CLIMB | bits_CAP_JUMP;

	MonsterInit();
}

void CDagon :: Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL( "models/dagon.mdl" );
	CDeepOne::Precache();
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CDagon :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
//	if ( flDist <= 128 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	if ( flDist <= 160 && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
float CDagon :: GetAttackDist( void )
{
	return 160.0;
}

void CDagon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case DEEP_ONE_AE_ATTACK_RIGHT:
		case DEEP_ONE_AE_ATTACK_LEFT:
		case DEEP_ONE_AE_ATTACK_BOTH:
		{
			pev->size.z *= 0.25;
		}
		break;

		default:
			break;
	}

	CDeepOne::HandleAnimEvent( pEvent );

	pev->origin.z += 64;
	
	CDeepOne::HandleAnimEvent( pEvent );

	pev->origin.z += 64;
	
	CDeepOne::HandleAnimEvent( pEvent );

	pev->origin.z -= 128;

	switch( pEvent->event )
	{
		case DEEP_ONE_AE_ATTACK_RIGHT:
		case DEEP_ONE_AE_ATTACK_LEFT:
		case DEEP_ONE_AE_ATTACK_BOTH:
		{
			pev->size.z *= 4.0;
		}
		break;

		default:
			break;
	}
}

int CDagon :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_CRUSH ) return 0;
	if ( bitsDamageType & DMG_SLASH ) return 0;
	if ( bitsDamageType & DMG_BULLET ) return 0;
	if ( bitsDamageType & DMG_CLUB ) return 0;

	return CDeepOne::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



