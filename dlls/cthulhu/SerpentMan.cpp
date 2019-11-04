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
// Serpent Man monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"animation.h"
#include	"snake.h"

extern DLL_GLOBAL int		g_iSkillLevel;

// TO DO:	
//			cast serpent staff
//			melee attack

#define GUN_GROUP					2
#define GUN_STAFF					0
#define GUN_NONE					1

#define	MAX_SERPENTS	12

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SERPENT_MAN_AE_CAST			( 1 )
#define		SERPENT_MAN_AE_CLAW			( 2 )
#define		SERPENT_MAN_AE_DROP_STAFF	( 3 )

#include "SerpentMan.h"


LINK_ENTITY_TO_CLASS( monster_serpentman, CSerpentMan );


TYPEDESCRIPTION	CSerpentMan::m_SaveData[] = 
{
	DEFINE_FIELD( CSerpentMan, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CSerpentMan, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSerpentMan, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CSerpentMan, m_iNumSerpents, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSerpentMan, CSquadMonster );




const char *CSerpentMan::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CSerpentMan::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CSerpentMan::pAttackSounds[] = 
{
	"serpentman/sm_attack1.wav",
	"serpentman/sm_attack2.wav",
	"serpentman/sm_attack3.wav",
};

const char *CSerpentMan::pPainSounds[] = 
{
	"serpentman/sm_pain1.wav",
	"serpentman/sm_pain2.wav",
	"serpentman/sm_pain3.wav",
};

const char *CSerpentMan::pDeathSounds[] = 
{
	"serpentman/sm_die1.wav",
	"serpentman/sm_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSerpentMan :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MILITARY;
}


int CSerpentMan::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( (pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! (m_afMemory & bits_MEMORY_PROVOKED ))
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}


void CSerpentMan :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	// ALERT( at_aiconsole, "help " );

	// skip ones not on my netname
	if ( FStringNull( pev->netname ))
		return;

	CBaseEntity *pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ))) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer( );
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy( hEnemy, vecLocation );
			}
		}
	}
}


//=========================================================
// ALertSound - scream
//=========================================================
void CSerpentMan :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SM_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch);

		CallForHelp( "monster_serpent_man", 512, m_hEnemy, m_vecEnemyLKP );
		CallForHelp( "monster_human_cultist", 512, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CSerpentMan :: IdleSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SM_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// AttackSound
//=========================================================
void CSerpentMan :: AttackSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// PainSound
//=========================================================
void CSerpentMan :: PainSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================

void CSerpentMan :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CSerpentMan :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


void CSerpentMan::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSerpentMan :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_WALK:		
		ys = 50;	
		break;
	case ACT_RUN:		
		ys = 70;
		break;
	case ACT_IDLE:		
		ys = 50;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CSerpentMan :: GetGunPosition( )
{
	// because origin is at the centre of the box, not at the bottom
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 24 );
//		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 12 );
//		return pev->origin + Vector( 0, 0, 48 );
	}
}
//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CSerpentMan :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{// throw a serpent staff if the serpent man has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity* pGun = DropItem( "weapon_serpentstaff", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CSerpentMan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int box = 24;
	int spawndist = 64;
	TraceResult tr;
	Vector trace_origin;
	Vector vecShootOrigin;
	Vector vecShootDir;

	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case SERPENT_MAN_AE_CAST:
			UTIL_MakeVectors( pev->v_angle );

			// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
			// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
			trace_origin = pev->origin;
			if ( !m_fStanding )
			{
				trace_origin = trace_origin - ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
			}

			// find place to toss monster - this is some horrible code I have written here...
	//		UTIL_TraceLine( trace_origin + angle * 20, trace_origin + angle * 64, dont_ignore_monsters, NULL, &tr );
			// firstly check all areas within the box to be created
			BOOL b1, b2, b3, b4;
			vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
			vecShootDir = ShootAtEnemy( vecShootOrigin );

			UTIL_TraceLine( trace_origin + vecShootDir * 20, trace_origin + vecShootDir * spawndist + Vector(box,box,0), dont_ignore_monsters, NULL, &tr );
			b1 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
			UTIL_TraceLine( trace_origin + vecShootDir * 20, trace_origin + vecShootDir * spawndist + Vector(-box,-box,0), dont_ignore_monsters, NULL, &tr );
			b2 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
			UTIL_TraceLine( trace_origin + vecShootDir * 20, trace_origin + vecShootDir * spawndist + Vector(-box,box,0), dont_ignore_monsters, NULL, &tr );
			b3 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
			UTIL_TraceLine( trace_origin + vecShootDir * 20, trace_origin + vecShootDir * spawndist + Vector(box,-box,0), dont_ignore_monsters, NULL, &tr );
			b4 = (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.99);
			UTIL_TraceLine( trace_origin + vecShootDir * 20, trace_origin + vecShootDir * spawndist, dont_ignore_monsters, NULL, &tr );

			// if we are clear at all corners of the box the snake is going to be created in
			if (b1 && b2 && b3 && b4)
			{
				Vector temp;
				temp.x = temp.y = temp.z = 0.0;
				temp.y = pev->v_angle.y;

				//CBaseEntity *pSnake = CBaseEntity::Create( "monster_snake", tr.vecEndPos, temp, m_pPlayer->edict() );
				CBaseEntity *pSnake = CBaseEntity::Create( "monster_snake", tr.vecEndPos, pev->angles );
				((CSnake*)pSnake)->SetOwner(this);
				pSnake->pev->spawnflags |= 1;
				SetBits(pSnake->pev->spawnflags,SF_MONSTER_NO_YELLOW_BLOBS);
				// drop to floor, to check if we have landed on top of an entity...
				DROP_TO_FLOOR( ENT( pSnake->pev ) );

				CBaseEntity *pList[2];
				int buf = 24;
				// if there are any no other entities nearby (particularly under it!), then...
				int count = UTIL_EntitiesInBox( pList, 2, pSnake->pev->origin - Vector(buf,buf,buf), pSnake->pev->origin + Vector(buf,buf,buf), FL_CLIENT|FL_MONSTER );
				//...put the snake there and decrement ammo
				if ( count <= 1 )
				{
					pSnake->pev->velocity = vecShootDir * 100 + pev->velocity;

					m_iNumSerpents--;
				}
				// otherwise we do not want this snake...as it will cause yellow blobs!
				else
				{
					UTIL_Remove(pSnake);
				}

				m_flNextAttack = gpGlobals->time + 2.0;
			}
			break;
		case SERPENT_MAN_AE_DROP_STAFF:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( GUN_GROUP, GUN_NONE );

			// now spawn a serpent staff.
			 DropItem( "weapon_serpentstaff", vecGunPos, vecGunAngles );

			}
			break;

		case SERPENT_MAN_AE_CLAW:
		{
			Vector oldorig = pev->origin;
			CBaseEntity *pHurt = NULL;
			// check down below in stages for snakes...
			for (int dz = 0; dz >=-3; dz--)
			{
				pev->origin = oldorig;
				pev->origin.z += dz * 12;
				pHurt = CheckTraceHullAttack( 70, gSkillData.serpentmanDmgStaff, DMG_POISON );
				if (pHurt) 
				{
					break;
				}
			}
			pev->origin = oldorig;

			if ( pHurt )
			{
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				// Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}

			// also play an attack sound
			AttackSound();
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CSerpentMan :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&& 
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CSerpentMan :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time || m_iNumSerpents <= 0)
	{
		return FALSE;
	}

	return CSquadMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CSerpentMan :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// StartTask
//=========================================================
void CSerpentMan :: StartTask ( Task_t *pTask )
{
	CSquadMonster :: StartTask ( pTask );
}


//=========================================================
// Spawn
//=========================================================
void CSerpentMan :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/serpentman.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-16,-16,-36), Vector(16,16,36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.serpentmanHealth;
	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 85, 110 );

	m_iNumSerpents = MAX_SERPENTS;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSerpentMan :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/serpentman.mdl");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	PRECACHE_SOUND("serpentman/sm_summon.wav");

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	//UTIL_PrecacheOther( "test_effect" );
}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CSerpentMan :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if ((bitsDamageType & DMG_POISON) || (bitsDamageType & DMG_NERVEGAS) || (bitsDamageType & DMG_PARALYZE))
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;
	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CSerpentMan::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ((bitsDamageType & DMG_PARALYZE) || (bitsDamageType & DMG_NERVEGAS) || (bitsDamageType & DMG_POISON))
		return;

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t	tlSerpentManAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSerpentManAttack1[] =
{
	{ 
		tlSerpentManAttack1,
		ARRAYSIZE ( tlSerpentManAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"SerpentMan Range Attack1"
	},
};


DEFINE_CUSTOM_SCHEDULES( CSerpentMan )
{
	slSerpentManAttack1,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSerpentMan, CSquadMonster );


//=========================================================
//=========================================================
// SetActivity 
//=========================================================
void CSerpentMan :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_MELEE_ATTACK1:

		// randomly stand or crouch
		if (RANDOM_LONG(0,99) == 0)
			m_fStanding = 0;
		else
			m_fStanding = 1;
		 
		// a short enemy...probably a snake...
		if ((m_hEnemy != NULL) && (m_hEnemy->pev->maxs.z < 36))
		{
			m_fStanding = 0;
		}

		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "ref_shoot_serpentstaff" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouch_shoot_serpentstaff" );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}
//=========================================================
Schedule_t *CSerpentMan :: GetSchedule( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		if ( pSound->m_iType & bits_SOUND_COMBAT )
			m_afMemory |= bits_MEMORY_PROVOKED;
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster :: GetSchedule();
		}

		if (pev->health < 20)
		{
			if (!HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
			{
				m_failSchedule = SCHED_CHASE_ENEMY;
				if (HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
				{
					// ALERT( at_console, "exposed\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
		}
		break;
	}
	return CSquadMonster::GetSchedule( );
}


Schedule_t *CSerpentMan :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_FAIL:
		if (HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
		{
			return CSquadMonster :: GetScheduleOfType( SCHED_MELEE_ATTACK1 ); ;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSerpentManAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSerpentManAttack1;
	}
	return CSquadMonster :: GetScheduleOfType( Type );
}


