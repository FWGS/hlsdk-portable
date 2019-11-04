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
// Priest
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"animation.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

#define	PRIEST_SENTENCE_VOLUME			(float)0.70 // volume of priest sentences
#define PRIEST_VOL						0.35		// volume of priest sounds
#define PRIEST_ATTN						ATTN_NORM	// attenutation of priest sentences

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		PRIEST_AE_KNIFE				( 1 )
#define		PRIEST_AE_SUMMON_POWERUP	( 2 )
#define		PRIEST_AE_SUMMON_DO			( 3 )
#define		PRIEST_AE_SUMMON_DONE		( 4 )
#define		PRIEST_AE_DROP_KNIFE		( 5 )

#define		PRIEST_MAX_BEAMS		8
#define		PRIEST_LIMP_HEALTH		20

#define GUN_GROUP					1
#define GUN_KNIFE					0
#define GUN_NONE					1

#include "Priest.h"


LINK_ENTITY_TO_CLASS( monster_priest, CPriest );


TYPEDESCRIPTION	CPriest::m_SaveData[] = 
{
	DEFINE_ARRAY( CPriest, m_pBeam, FIELD_CLASSPTR, PRIEST_MAX_BEAMS ),
	DEFINE_FIELD( CPriest, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CPriest, m_flNextAttack, FIELD_TIME ),

	DEFINE_FIELD( CPriest, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPriest, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CPriest, m_iSentence, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CPriest, CSquadMonster );




const char *CPriest::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CPriest::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CPriest::pPainSounds[] = 
{
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
};

const char *CPriest::pDeathSounds[] = 
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

const char *CPriest::pPriestSentences[] = 
{
	"PR_GREN", // grenade scared PRIEST
	"PR_ALERT", // sees player
	"PR_CAST", // casts a spell
	"PR_COVER", // running to cover
	"PR_CHARGE",  // running out to get the enemy
	"PR_TAUNT", // say rude things
};

enum
{
	PRIEST_SENT_NONE = -1,
	PRIEST_SENT_GREN = 0,
	PRIEST_SENT_ALERT,
	PRIEST_SENT_CAST,
	PRIEST_SENT_COVER,
	PRIEST_SENT_CHARGE,
	PRIEST_SENT_TAUNT,
} PRIEST_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some priest sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a priest says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the PRIEST has 
// started moving.
//=========================================================
void CPriest :: SpeakSentence( void )
{
	if ( m_iSentence == PRIEST_SENT_NONE )
	{
		// no sentence cued up.
		return; 
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pPriestSentences[ m_iSentence ], PRIEST_SENTENCE_VOLUME, PRIEST_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_PRIEST_SPEAK_SENTENCE = LAST_COMMON_TASK + 1,
};

//=========================================================
// SetActivity 
//=========================================================
void CPriest :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_MELEE_ATTACK1:

		// a short enemy...probably a snake...
		if ((m_hEnemy != NULL) && (m_hEnemy->pev->maxs.z < 36))
		{
			m_fStanding = 0;
		}
		else
		{
			m_fStanding = 1;
		}

		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "ref_shoot_crowbar" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouch_shoot_crowbar" );
		}
		break;
	case ACT_RUN:
		if ( pev->health <= PRIEST_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_RUN );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= PRIEST_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
			if ( iSequence == ACTIVITY_NOT_AVAILABLE )
			{
				iSequence = LookupActivity ( ACT_WALK );
			}
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
//			NewActivity = ACT_IDLE_ANGRY;
			NewActivity = ACT_IDLE;
		}
		iSequence = LookupActivity ( NewActivity );
		if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		{
			iSequence = LookupActivity ( ACT_IDLE );
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
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPriest :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_HUMAN_CULTIST;
}


int CPriest::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( (pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! (m_afMemory & bits_MEMORY_PROVOKED ))
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}


void CPriest :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
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
void CPriest :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		SENTENCEG_PlayRndSz(ENT(pev), "PR_ALERT", PRIEST_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
		JustSpoke();

		CallForHelp( "monster_cultist", 512, m_hEnemy, m_vecEnemyLKP );
	}
}

//=========================================================
// IdleSound
//=========================================================
void CPriest :: IdleSound( void )
{
	//if (RANDOM_LONG( 0, 2 ) == 0)
	//{
	//	SENTENCEG_PlayRndSz(ENT(pev), "PRIEST_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch);
	//}
}

//=========================================================
// PainSound
//=========================================================
void CPriest :: PainSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================

void CPriest :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CPriest :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CPriest :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}
	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;
	
	return TRUE;
}

//=========================================================
//=========================================================
void CPriest :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = PRIEST_SENT_NONE;
}

//========================================================
// Killed
//========================================================
void CPriest::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams( );
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CPriest :: GibMonster ( void )
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if ( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{// throw a knife if the priest has one
		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;

		pGun = DropItem( "weapon_knife", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}

	CBaseMonster :: GibMonster();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPriest :: SetYawSpeed ( void )
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
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CPriest :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case PRIEST_AE_DROP_KNIFE:
			{
			Vector	vecGunPos;
			Vector	vecGunAngles;

			GetAttachment( 0, vecGunPos, vecGunAngles );

			// switch to body group with no gun.
			SetBodygroup( GUN_GROUP, GUN_NONE );

			DropItem( "weapon_knife", vecGunPos, vecGunAngles );

			}
			break;

		case PRIEST_AE_KNIFE:
		{
			Vector oldorig = pev->origin;
			CBaseEntity *pHurt = NULL;
			// check down below in stages for snakes...
			for (int dz = 0; dz >= -3; dz--)
			{
				pev->origin = oldorig;
				pev->origin.z += dz * 12;
				pHurt = CheckTraceHullAttack( 70, gSkillData.priestDmgKnife, DMG_SLASH );
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
		}
		break;

		case PRIEST_AE_SUMMON_POWERUP:
		{
			// speed up attack when on hard
			if (g_iSkillLevel == SKILL_HARD)
				pev->framerate = 1.5;

			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 255 );		// r
					WRITE_BYTE( 180 );		// g
					WRITE_BYTE( 96 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );

			}

			ArmBeam( -1 );
			ArmBeam( 1 );
			BeamGlow( );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
		break;

		case PRIEST_AE_SUMMON_DO:
		{
			ClearBeams( );

			// I WAS GOING TO GET THE PRIEST TO SUMMON MORE MONSTERS, BUT DECIDED AGAINST IT...
			// find a clear spot nearby
			// create a monster
			//CBaseEntity *pNew = Create( "monster_ghoul", pev->origin + (pev->angles*64), pev->angles );
			//CBaseMonster *pNewMonster = pNew->MyMonsterPointer( );
			//pNew->pev->spawnflags |= 1;
			//EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );

			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( -1 );
			ZapBeam( 1 );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );

			ApplyMultiDamage(pev, pev);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
		}
		break;

		case PRIEST_AE_SUMMON_DONE:
		{
			ClearBeams( );
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CPriest :: ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= PRIEST_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.01;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 1024, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
		pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CPriest :: GetGunPosition( )
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
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CPriest :: CheckMeleeAttack1 ( float flDot, float flDist )
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
BOOL CPriest :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	return CSquadMonster::CheckRangeAttack1( flDot, flDist );
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CPriest :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}


//=========================================================
// StartTask
//=========================================================
void CPriest :: StartTask ( Task_t *pTask )
{
	ClearBeams( );

	switch ( pTask->iTask )
	{
	case TASK_PRIEST_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	default:
		CSquadMonster :: StartTask ( pTask );
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CPriest :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/priest.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-16,-16,-36), Vector(16,16,36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.priestHealth;
	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_SQUAD | bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 85, 110 );
	m_iSentence			= PRIEST_SENT_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPriest :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/priest.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	UTIL_PrecacheOther( "test_effect" );
}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CPriest :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// don't slash one of your own
	if ((bitsDamageType & DMG_SLASH) && pevAttacker && IRelationship( Instance(pevAttacker) ) < R_DL)
		return 0;

	m_afMemory |= bits_MEMORY_PROVOKED;
	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CPriest::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	SCHED_PRIEST_ESTABLISH_LINE_OF_FIRE = LAST_COMMON_SCHEDULE + 1,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_PRIEST_ATTACK1,
	//SCHED_PRIEST_SUPPRESS,
	//SCHED_PRIEST_COVER_AND_RELOAD,
	//SCHED_PRIEST_SWEEP,
	//SCHED_PRIEST_FOUND_ENEMY,
	//SCHED_PRIEST_WAIT_FACE_ENEMY,
	//SCHED_PRIEST_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_PRIEST_ELOF_FAIL,
};

// primary range attack
Task_t	tlPriestAttack1[] =
{
	{ TASK_STOP_MOVING,				0				},
	{ TASK_FACE_IDEAL,				(float)0		},
	{ TASK_PRIEST_SPEAK_SENTENCE,	(float)0		},
	{ TASK_RANGE_ATTACK1,			(float)0		},
};

Schedule_t	slPriestAttack1[] =
{
	{ 
		tlPriestAttack1,
		ARRAYSIZE ( tlPriestAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAR_SOUND |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Priest Range Attack1"
	},
};


//=========================================================
// Establish line of fire - move to a position that allows
// the priest to attack.
//=========================================================
Task_t tlPriestEstablishLineOfFire[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_PRIEST_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_PRIEST_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slPriestEstablishLineOfFire[] =
{
	{ 
		tlPriestEstablishLineOfFire,
		ARRAYSIZE ( tlPriestEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"PriestEstablishLineOfFire"
	},
};

DEFINE_CUSTOM_SCHEDULES( CPriest )
{
	slPriestAttack1,
	slPriestEstablishLineOfFire,
};

IMPLEMENT_CUSTOM_SCHEDULES( CPriest, CSquadMonster );


//=========================================================
//=========================================================
Schedule_t *CPriest :: GetSchedule( void )
{
	m_iSentence = PRIEST_SENT_NONE;

	ClearBeams( );

/*
	if (pev->spawnflags)
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/

	// priests place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				// dangerous sound nearby!
				
				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause 
				// this may only affect a single individual in a squad. 
				
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "PR_GREN", PRIEST_SENTENCE_VOLUME, PRIEST_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			if ( pSound->m_iType & bits_SOUND_COMBAT )
				m_afMemory |= bits_MEMORY_PROVOKED;
		}
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
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "PR_COVER", PRIEST_SENTENCE_VOLUME, PRIEST_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				if ( HasConditions ( bits_COND_SEE_ENEMY ) && HasConditions ( bits_COND_ENEMY_FACING_ME ) )
				{
					// ALERT( at_console, "exposed\n");
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
		}
// can't see enemy
		else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
		{
			if ( OccupySlot( bits_SLOTS_PRIEST_ENGAGE ) )
			{
				//!!!KELLY - priest cannot see the enemy and has just decided to 
				// charge the enemy's position. 
				if (FOkToSpeak() && RANDOM_LONG(0,1))
				{
					m_iSentence = PRIEST_SENT_CHARGE;
				}

				return GetScheduleOfType( SCHED_PRIEST_ESTABLISH_LINE_OF_FIRE );
			}
			else
			{
				//!!!KELLY - priest is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// priest's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0,1))
				{
					//m_iSentence = PRIEST_SENT_TAUNT;
					SENTENCEG_PlayRndSz( ENT(pev), "PR_TAUNT", PRIEST_SENTENCE_VOLUME, PRIEST_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_STANDOFF );
			}
		}
		break;
	}
	return CSquadMonster::GetSchedule( );
}


Schedule_t *CPriest :: GetScheduleOfType ( int Type ) 
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
		m_iSentence = PRIEST_SENT_CAST;
		return slPriestAttack1;
	case SCHED_RANGE_ATTACK2:
		m_iSentence = PRIEST_SENT_CAST;
		return slPriestAttack1;
	case SCHED_PRIEST_ESTABLISH_LINE_OF_FIRE:
		{
			return &slPriestEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_PRIEST_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	}
	return CSquadMonster :: GetScheduleOfType( Type );
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CPriest :: ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;
	
	if (m_iBeams >= PRIEST_MAX_BEAMS)
		return;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CPriest :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CPriest :: WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;
	
	if (m_iBeams >= PRIEST_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side < 0 ? 2 : 1 );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CPriest :: ClearBeams( )
{
	for (int i = 0; i < PRIEST_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}
