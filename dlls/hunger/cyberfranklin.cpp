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

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"hgrunt.h"
#include	"hornet.h"

#define		FRANKLIN_MELEE_DIST	100

int iFranklinMuzzleFlash;

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define GRUNT_VOL						0.35		// volume of grunt sounds
#define GRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				20
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there? 
#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	HGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER		( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_AE_RELOAD		( 2 )
#define		HGRUNT_AE_KICK			( 3 )
#define		HGRUNT_AE_BURST1		( 4 )
#define		HGRUNT_AE_BURST2		( 5 ) 
#define		HGRUNT_AE_BURST3		( 6 ) 
#define		HGRUNT_AE_GREN_TOSS		( 7 )
#define		HGRUNT_AE_GREN_LAUNCH	( 8 )
#define		HGRUNT_AE_GREN_DROP		( 9 )
#define		HGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		HGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.

class CCyberFranklin : public CHGrunt
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	void CheckAmmo(void) { }
	void SetActivity(Activity NewActivity);
	void StartTask(Task_t *pTask);

	void AlertSound(void);
	void DeathSound(void);
	void PainSound(void);
	void AttackSound(void);
	void IdleSound(void) { }
	Vector GetGunPosition(void);
	void Shoot(void);

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fCanHornetAttack;
	float	m_flNextHornetAttackCheck;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
};

LINK_ENTITY_TO_CLASS(monster_th_cyberfranklin, CCyberFranklin);

TYPEDESCRIPTION	CCyberFranklin::m_SaveData[] =
{
	DEFINE_FIELD(CCyberFranklin, m_fCanHornetAttack, FIELD_BOOLEAN),
	DEFINE_FIELD(CCyberFranklin, m_flNextHornetAttackCheck, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CCyberFranklin, CHGrunt);


const char *CCyberFranklin::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CCyberFranklin::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CCyberFranklin::pAttackSounds[] =
{
	"franklin/attack1.wav",
};

const char *CCyberFranklin::pDieSounds[] =
{
	"franklin/death1.wav",
	"franklin/death2.wav",
	"franklin/death3.wav",
};

const char *CCyberFranklin::pPainSounds[] =
{
	"franklin/pain1.wav",
	"franklin/pain2.wav",
};

const char *CCyberFranklin::pAlertSounds[] =
{
	"franklin/alert1.wav",
};

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CCyberFranklin::CheckMeleeAttack1(float flDot, float flDist)
{
	CBaseMonster *pEnemy;

	if (m_hEnemy != NULL)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if (!pEnemy)
		{
			return FALSE;
		}
	}

	if (flDist <= 72 && flDot >= 0.7	&&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing. 
//=========================================================
BOOL CCyberFranklin :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( gpGlobals->time < m_flNextHornetAttackCheck )
	{
		return m_fCanHornetAttack;
	}

	if (HasConditions(bits_COND_SEE_ENEMY) && flDist >= FRANKLIN_MELEE_DIST && flDist <= 1024 && flDot >= 0.5 && NoFriendlyFire())
	{
		TraceResult	tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors( pev->angles );
		GetAttachment( 0, vecArmPos, vecArmDir );
//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextHornetAttackCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
			m_fCanHornetAttack = TRUE;
			return m_fCanHornetAttack;
		}
	}
	
	m_flNextHornetAttackCheck = gpGlobals->time + 0.2;// don't check for half second if this check wasn't successful
	m_fCanHornetAttack = FALSE;
	return m_fCanHornetAttack;
}

//=========================================================
// DieSound
//=========================================================
void CCyberFranklin::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDieSounds), 1.0, ATTN_NORM);
}

//=========================================================
// AlertSound
//=========================================================
void CCyberFranklin::AlertSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_NORM);
}

//=========================================================
// AttackSound
//=========================================================
void CCyberFranklin::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM);
}

//=========================================================
// PainSound
//=========================================================
void CCyberFranklin::PainSound(void)
{
	if (m_flNextPainTime > gpGlobals->time)
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1.0, ATTN_NORM);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCyberFranklin::Classify(void)
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CCyberFranklin::GetGunPosition()
{
	return pev->origin + Vector(0, 0, 48);
}


//=========================================================
// Shoot
//=========================================================
void CCyberFranklin::Shoot(void)
{
	// m_vecEnemyLKP should be center of enemy body
	Vector vecArmPos, vecArmDir;
	Vector vecDirToEnemy;
	Vector angDir;

	if (HasConditions( bits_COND_SEE_ENEMY))
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
	if (angDir.x > 180)
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
			
	switch ( RANDOM_LONG ( 0 , 2 ) )
	{
		case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire1.wav", 1.0, ATTN_NORM, 0, 100 );	break;
		case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire2.wav", 1.0, ATTN_NORM, 0, 100 );	break;
		case 2:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire3.wav", 1.0, ATTN_NORM, 0, 100 );	break;
	}

	CBaseMonster *pHornetMonster = pHornet->MyMonsterPointer();

	if ( pHornetMonster )
	{
		pHornetMonster->m_hEnemy = m_hEnemy;
	}
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCyberFranklin::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
		case HGRUNT_AE_RELOAD:
		case HGRUNT_AE_GREN_TOSS:
		case HGRUNT_AE_GREN_LAUNCH:
		case HGRUNT_AE_GREN_DROP:
			break;

		case HGRUNT_AE_BURST1:
		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			Shoot();
			break;

		case HGRUNT_AE_KICK:
		{
			CBaseEntity *pHurt = Kick();

			if ( pHurt )
			{	
				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );

				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
				{
					pHurt->pev->punchangle.x = 15;
				}

				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );

				// Play a random attack hit sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
			}
			else// Play a random attack miss sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
		}
		break;

		default:
			CHGrunt::HandleAnimEvent( pEvent );
			break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CCyberFranklin::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/franklin2.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.cyberfranklinHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = -1;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(24, 64, 48);

	pev->weapons = HGRUNT_9MMAR;

	m_cClipSize = HORNETGUN_MAX_CLIP;

	m_cAmmoLoaded = m_cClipSize;

	pev->body = 0;
	pev->skin = 0;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();

	m_afCapability &= ~bits_CAP_RANGE_ATTACK2;

	ClearConditions(bits_COND_NO_AMMO_LOADED);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCyberFranklin::Precache()
{
	PRECACHE_MODEL("models/franklin2.mdl");

	PRECACHE_SOUND_ARRAY(pAttackHitSounds);
	PRECACHE_SOUND_ARRAY(pAttackMissSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);

	PRECACHE_SOUND("franklin/franklin_step.wav");

	PRECACHE_SOUND("hassault/hw_shoot1.wav");

	iFranklinMuzzleFlash = PRECACHE_MODEL("sprites/muz4.spr");

	UTIL_PrecacheOther("hornet");

	m_voicePitch = 100;
}



//=========================================================
// SetActivity 
//=========================================================
void CCyberFranklin::SetActivity(Activity NewActivity)
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		{
			// get crouching shoot
			iSequence = LookupSequence("crouching_mp5");
		}
		break;
	default:
		CHGrunt::SetActivity(NewActivity);
		return;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// start task
//=========================================================
void CCyberFranklin::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK1:
		AttackSound();
		CHGrunt::StartTask(pTask);
		break;

	default:
		CHGrunt::StartTask(pTask);
		break;
	}
}
