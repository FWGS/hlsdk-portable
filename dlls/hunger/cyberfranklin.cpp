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

#include	"agrunt.h"

#define		FRANKLIN_MELEE_DIST	100

int iFranklinMuzzleFlash;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		CF_AE_KICK		( 3 )
#define		CF_AE_SHOOT1		( 4 )
#define		CF_AE_SHOOT2		( 5 ) 
#define		CF_AE_SHOOT3		( 6 ) 
#define		CF_AE_DROP_GUN		( 11 )

class CEinarCyberFranklin : public CAGrunt
{
public:
	void Spawn();
	void Precache();
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void AlertSound();
	void DeathSound();
	void PainSound();
	void AttackSound();
	void IdleSound() {}

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
};

LINK_ENTITY_TO_CLASS( monster_th_cyberfranklin, CEinarCyberFranklin )

const char *CEinarCyberFranklin::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CEinarCyberFranklin::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CEinarCyberFranklin::pAttackSounds[] =
{
	"franklin/attack1.wav",
};

const char *CEinarCyberFranklin::pDieSounds[] =
{
	"franklin/death1.wav",
	"franklin/death2.wav",
	"franklin/death3.wav",
};

const char *CEinarCyberFranklin::pPainSounds[] =
{
	"franklin/pain1.wav",
	"franklin/pain2.wav",
};

const char *CEinarCyberFranklin::pAlertSounds[] =
{
	"franklin/alert1.wav",
};

//=========================================================
// DieSound
//=========================================================
void CEinarCyberFranklin::DeathSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDieSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// AlertSound
//=========================================================
void CEinarCyberFranklin::AlertSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// AttackSound
//=========================================================
void CEinarCyberFranklin::AttackSound()
{
	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// PainSound
//=========================================================
void CEinarCyberFranklin::PainSound()
{
	if( m_flNextPainTime > gpGlobals->time )
		return;

	m_flNextPainTime = gpGlobals->time + 2.0f;

	EMIT_SOUND( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0, ATTN_NORM );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CEinarCyberFranklin::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case CF_AE_SHOOT1:
	case CF_AE_SHOOT2:
	case CF_AE_SHOOT3:
		{
			// m_vecEnemyLKP should be center of enemy body
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			pev->effects = EF_MUZZLEFLASH;

			// make angles +-180
			if( angDir.x > 180 )
			{
				angDir.x = angDir.x - 360;
			}

			SetBlending( 0, angDir.x );
			GetAttachment( 0, vecArmPos, vecArmDir );

			vecArmPos = vecArmPos + vecDirToEnemy * 32;
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecArmPos );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( vecArmPos.x );	// pos
				WRITE_COORD( vecArmPos.y );	
				WRITE_COORD( vecArmPos.z );	
				WRITE_SHORT( iFranklinMuzzleFlash );		// model
				WRITE_BYTE( 6 );				// size * 10
				WRITE_BYTE( 128 );			// brightness
			MESSAGE_END();

			CBaseEntity *pHornet = CBaseEntity::Create( "hornet", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
			UTIL_MakeVectors ( pHornet->pev->angles );
			pHornet->pev->velocity = gpGlobals->v_forward * 300;

			switch( RANDOM_LONG( 0, 2 ) )
			{
				case 0:
					EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "agrunt/ag_fire1.wav", 1.0, ATTN_NORM, 0, 100 );
					break;
				case 1:
					EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "agrunt/ag_fire2.wav", 1.0, ATTN_NORM, 0, 100 );
					break;
				case 2:
					EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "agrunt/ag_fire3.wav", 1.0, ATTN_NORM, 0, 100 );
					break;
			}

			CBaseMonster *pHornetMonster = pHornet->MyMonsterPointer();

			if( pHornetMonster )
			{
				pHornetMonster->m_hEnemy = m_hEnemy;
			}
		}
		break;
	case CF_AE_KICK:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( FRANKLIN_MELEE_DIST, gSkillData.agruntDmgPunch, DMG_CLUB );

			if( pHurt )
			{
				pHurt->pev->punchangle.y = RANDOM_LONG( -5, 5 );
				pHurt->pev->punchangle.x = 8;

				// OK to use gpGlobals without calling MakeVectors, cause CheckTraceHullAttack called it above.
				if( pHurt->IsPlayer() )
				{
					// this is a player. Knock him around.
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_right * RANDOM_LONG( -50, 50 );
				}

				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );
				SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );// a little surface blood.
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
		}
		break;
	case CF_AE_DROP_GUN:
		break;
	default:
		CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CEinarCyberFranklin::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/franklin2.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.agruntHealth * 6;
	m_flFieldOfView = -1.0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = 0;
	SetBits( m_afCapability, bits_CAP_SQUAD | bits_CAP_TURN_HEAD );

	m_HackedGunPos = Vector( 24, 64, 48 );

	// m_flNextSpeakTime = m_flNextWordTime = gpGlobals->time + 10 + RANDOM_LONG( 0, 10 );

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEinarCyberFranklin::Precache()
{
	PRECACHE_MODEL( "models/franklin2.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );

	PRECACHE_SOUND( "player/pl_tile1.wav" );
	PRECACHE_SOUND( "franklin/franklin_step.wav" );

	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );

	iFranklinMuzzleFlash = PRECACHE_MODEL( "sprites/muz4.spr" );

	UTIL_PrecacheOther( "hornet" );
}
