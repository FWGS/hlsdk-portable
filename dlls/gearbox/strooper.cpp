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
// shocktrooper
//=========================================================

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
#include	"sporegrenade.h"

int g_fStrooperQuestion;	// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

extern Schedule_t	slGruntTakeCover[];
extern Schedule_t	slGruntGrenadeCover[];
extern Schedule_t	slGruntTossGrenadeCover[];

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	STROOPER_CLIP_SIZE					10			// how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define STROOPER_VOL						0.35		// volume of grunt sounds
#define STROOPER_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define STROOPER_LIMP_HEALTH				20
#define STROOPER_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define STROOPER_NUM_HEADS					2 // how many grunt heads are there? 
#define STROOPER_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	STROOPER_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define STROOPER_SHOCKRIFLE			(1 << 0)
#define STROOPER_HANDGRENADE		(1 << 1)

#define GUN_GROUP					1
#define GUN_SHOCKFIFLE				0
#define GUN_NONE					1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		STROOPER_AE_RELOAD			( 2 )
#define		STROOPER_AE_KICK			( 3 )
#define		STROOPER_AE_BURST1			( 4 )
#define		STROOPER_AE_BURST2			( 5 ) 
#define		STROOPER_AE_BURST3			( 6 ) 
#define		STROOPER_AE_GREN_TOSS		( 7 )
#define		STROOPER_AE_GREN_LAUNCH		( 8 )
#define		STROOPER_AE_GREN_DROP		( 9 )
#define		STROOPER_AE_CAUGHT_ENEMY	( 10 ) // shocktrooper established sight with an enemy (player only) that had previously eluded the squad.
#define		STROOPER_AE_DROP_GUN		( 11 ) // shocktrooper (probably dead) is dropping his shockrifle.


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_STROOPER_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_STROOPER_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_STROOPER_COVER_AND_RELOAD,
	SCHED_STROOPER_SWEEP,
	SCHED_STROOPER_FOUND_ENEMY,
	SCHED_STROOPER_REPEL,
	SCHED_STROOPER_REPEL_ATTACK,
	SCHED_STROOPER_REPEL_LAND,
	SCHED_STROOPER_WAIT_FACE_ENEMY,
	SCHED_STROOPER_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_STROOPER_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_STROOPER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_STROOPER_SPEAK_SENTENCE,
	TASK_STROOPER_CHECK_FIRE,
};


//=========================================================
// shocktrooper
//=========================================================
class CStrooper : public CHGrunt
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);

	void SetActivity(Activity NewActivity);

	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);
	void GibMonster(void);

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	Schedule_t	*GetSchedule(void);
	Schedule_t  *GetScheduleOfType(int Type);

	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fRightClaw;

	static const char *pGruntSentences[];
};

LINK_ENTITY_TO_CLASS(monster_shocktrooper, CStrooper);

TYPEDESCRIPTION	CStrooper::m_SaveData[] =
{
	DEFINE_FIELD(CStrooper, m_fRightClaw, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CStrooper, CHGrunt);

const char *CStrooper::pGruntSentences[] =
{
	"ST_GREN",		// grenade scared grunt
	"ST_ALERT",		// sees player
	"ST_MONST",		// sees monster
	"ST_COVER",		// running to cover
	"ST_THROW",		// about to throw grenade
	"ST_CHARGE",	// running out to get the enemy
	"ST_TAUNT",		// say rude things
};

enum
{
	STROOPER_SENT_NONE = -1,
	STROOPER_SENT_GREN = 0,
	STROOPER_SENT_ALERT,
	STROOPER_SENT_MONSTER,
	STROOPER_SENT_COVER,
	STROOPER_SENT_THROW,
	STROOPER_SENT_CHARGE,
	STROOPER_SENT_TAUNT,
} STROOPER_SENTENCE_TYPES;


//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CStrooper::GibMonster(void)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(1) != 1)
	{// throw a shockroach if the shock trooper has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity* pRoach = DropItem("monster_shockroach", vecGunPos, vecGunAngles);

		if (pRoach)
		{
			pRoach->pev->owner = edict();
	
			if (m_hEnemy)
				pRoach->pev->angles = (pev->origin - m_hEnemy->pev->origin).Normalize();

			// Remove any pitch.
			pRoach->pev->angles.x = 0;
		}
	}

	CBaseMonster::GibMonster();
}

void CStrooper::IdleSound(void)
{
	if (FOkToSpeak() && (g_fStrooperQuestion || RANDOM_LONG(0, 1)))
	{
		if (!g_fStrooperQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0, 2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ST_CHECK", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fStrooperQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "ST_QUEST", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fStrooperQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "ST_IDLE", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fStrooperQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "ST_CLEAR", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question 
				SENTENCEG_PlayRndSz(ENT(pev), "ST_ANSWER", STROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fStrooperQuestion = 0;
		}
		JustSpoke();
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CStrooper::Classify(void)
{
	return CLASS_ALIEN_MILITARY;
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CStrooper::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case STROOPER_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(GUN_GROUP, GUN_NONE);

		Vector vecDropAngles = vecGunAngles;


		if (m_hEnemy)
			vecDropAngles = (m_hEnemy->pev->origin - pev->origin).Normalize();

		// Remove any pitch.
		vecDropAngles.x = 0;

		// now spawn a shockroach.
		CBaseEntity* pRoach = DropItem("monster_shockroach", vecGunPos, vecDropAngles);
		if (pRoach)
		{
			pRoach->pev->owner = edict();

			if (m_hEnemy)
				pRoach->pev->angles = (pev->origin - m_hEnemy->pev->origin).Normalize();

			// Remove any pitch.
			pRoach->pev->angles.x = 0;
		}
	}
	break;

	case STROOPER_AE_RELOAD:
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case STROOPER_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CSporeGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case STROOPER_AE_GREN_LAUNCH:
	case STROOPER_AE_GREN_DROP:
		break;

	case STROOPER_AE_BURST1:
	{
		//Shoot();

		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		if (m_hEnemy)
		{
			vecGunAngles = (m_hEnemy->EyePosition() - pev->origin).Normalize();
		}

		CBaseEntity *pShock = CBaseEntity::Create("shock", vecGunPos, vecGunAngles, edict());
		pShock->pev->velocity = vecGunAngles * 900;

		// Play fire sound.
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/shock_fire.wav", 1, ATTN_NORM);

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case STROOPER_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->punchangle.z = (m_fRightClaw) ? -10 : 10;

			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.strooperDmgKick, DMG_CLUB);
		}

		m_fRightClaw = !m_fRightClaw;
	}
	break;

	case STROOPER_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "ST_ALERT", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
			JustSpoke();
		}

	}

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// Spawn
//=========================================================
void CStrooper::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/strooper.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.strooperHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = STROOPER_SENT_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = STROOPER_SHOCKRIFLE | STROOPER_HANDGRENADE;
	}

	m_cClipSize = SHOCKRIFLE_MAX_CLIP;

	m_cAmmoLoaded = m_cClipSize;

	m_fRightClaw = FALSE;

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CStrooper::Precache()
{
	PRECACHE_MODEL("models/strooper.mdl");

	PRECACHE_SOUND("shocktrooper/shock_trooper_die1.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die2.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die3.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_die4.wav");

	PRECACHE_SOUND("shocktrooper/shock_trooper_pain1.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain2.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain3.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain4.wav");
	PRECACHE_SOUND("shocktrooper/shock_trooper_pain5.wav");

	PRECACHE_SOUND("weapons/shock_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	UTIL_PrecacheOther("monster_shockroach");

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}


//=========================================================
// PainSound
//=========================================================
void CStrooper::PainSound(void)
{
	if (gpGlobals->time > m_flNextPainTime)
	{
#if 0
		if (RANDOM_LONG(0, 99) < 5)
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif 
		switch (RANDOM_LONG(0, 4))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain1.wav", 1, ATTN_NORM);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain2.wav", 1, ATTN_NORM);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain3.wav", 1, ATTN_NORM);
			break;
		case 3:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain4.wav", 1, ATTN_NORM);
			break;
		case 4:
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_pain5.wav", 1, ATTN_NORM);
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CStrooper::DeathSound(void)
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die1.wav", 1, ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die2.wav", 1, ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die3.wav", 1, ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "shocktrooper/shock_trooper_die4.wav", 1, ATTN_IDLE);
		break;
	}
}


//=========================================================
// SetActivity 
//=========================================================
void CStrooper::SetActivity(Activity NewActivity)
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// shocktrooper is either shooting standing or shooting crouched
		if (m_fStanding)
		{
			// get aimable sequence
			iSequence = LookupSequence("standing_mp5");
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence("crouching_mp5");
		}
		break;
	case ACT_RANGE_ATTACK2:
		// shocktrooper is going to throw a grenade.

		// get toss anim
		iSequence = LookupSequence("throwgrenade");
		break;

	case ACT_RUN:
		if (pev->health <= STROOPER_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_RUN_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_WALK:
		if (pev->health <= STROOPER_LIMP_HEALTH)
		{
			// limp!
			iSequence = LookupActivity(ACT_WALK_HURT);
		}
		else
		{
			iSequence = LookupActivity(NewActivity);
		}
		break;
	case ACT_IDLE:
		if (m_MonsterState == MONSTERSTATE_COMBAT)
		{
			NewActivity = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
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
// Get Schedule!
//=========================================================
Schedule_t *CStrooper::GetSchedule(void)
{

	// clear old sentence
	m_iSentence = STROOPER_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_STROOPER_REPEL_LAND);
		}
		else
		{
			// repel down a rope, 
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_STROOPER_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_STROOPER_REPEL);
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound)
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
					SENTENCEG_PlayRndSz(ENT(pev), "ST_GREN", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
			MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}

		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			if (InSquad())
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;

				if (!IsLeader())
				{
					return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
				}
				else
				{
					//!!!KELLY - the leader of a squad of grunts has just seen the player or a 
					// monster and has made it the squad's enemy. You
					// can check pev->flags for FL_CLIENT to determine whether this is the player
					// or a monster. He's going to immediately start
					// firing, though. If you'd like, we can make an alternate "first sight" 
					// schedule where the leader plays a handsign anim
					// that gives us enough time to hear a short sentence or spoken command
					// before he starts pluggin away.
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
							// player
							SENTENCEG_PlayRndSz(ENT(pev), "ST_ALERT", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
						else if ((m_hEnemy != NULL) &&
							(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
							(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
							(m_hEnemy->Classify() != CLASS_MACHINE))
							// monster
							SENTENCEG_PlayRndSz(ENT(pev), "ST_MONST", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);

						JustSpoke();
					}

					if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
					{
						return GetScheduleOfType(SCHED_STROOPER_SUPPRESS);
					}
					else
					{
						return GetScheduleOfType(SCHED_STROOPER_ESTABLISH_LINE_OF_FIRE);
					}
				}
			}
		}
		// no ammo
		else if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo. 
			// He's going to try to find cover to run to and reload, but rarely, if 
			// none is available, he'll drop and reload in the open here. 
			return GetScheduleOfType(SCHED_STROOPER_COVER_AND_RELOAD);
		}

		// damaged just a little
		else if (HasConditions(bits_COND_LIGHT_DAMAGE))
		{
			// if hurt:
			// 90% chance of taking cover
			// 10% chance of flinch.
			int iPercent = RANDOM_LONG(0, 99);

			if (iPercent <= 90 && m_hEnemy != NULL)
			{
				// only try to take cover if we actually have an enemy!

				//!!!KELLY - this grunt was hit and is going to run to cover.
				if (FOkToSpeak()) // && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = STROOPER_SENT_COVER;
					//JustSpoke();
				}
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
			else
			{
				return GetScheduleOfType(SCHED_SMALL_FLINCH);
			}
		}
		// can kick
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}

		// can shoot
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (InSquad())
			{
				// if the enemy has eluded the squad and a squad member has just located the enemy
				// and the enemy does not see the squad member, issue a call to the squad to waste a 
				// little time and give the player a chance to turn.
				if (MySquadLeader()->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;
					return GetScheduleOfType(SCHED_STROOPER_FOUND_ENEMY);
				}
			}

			if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				// try to take an available ENGAGE slot
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				// throw a grenade if can and no engage slots are available
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else
			{
				// hide!
				return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
			}
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "ST_THROW", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				//!!!KELLY - grunt cannot see the enemy and has just decided to 
				// charge the enemy's position. 
				if (FOkToSpeak())// && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = STROOPER_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_STROOPER_ESTABLISH_LINE_OF_FIRE);
			}
			else
			{
				//!!!KELLY - grunt is going to stay put for a couple seconds to see if
				// the enemy wanders back out into the open, or approaches the
				// grunt's covered position. Good place for a taunt, I guess?
				if (FOkToSpeak() && RANDOM_LONG(0, 1))
				{
					SENTENCEG_PlayRndSz(ENT(pev), "ST_TAUNT", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_STANDOFF);
			}
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_STROOPER_ESTABLISH_LINE_OF_FIRE);
		}
	}
	}

	// no special cases here, call the base class
	return CSquadMonster::GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CStrooper::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if (InSquad())
			{
				if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz(ENT(pev), "ST_THROW", STROOPER_SENTENCE_VOLUME, STROOPER_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slGruntTossGrenadeCover;
				}
				else
				{
					return &slGruntTakeCover[0];
				}
			}
			else
			{
				if (RANDOM_LONG(0, 1))
				{
					return &slGruntTakeCover[0];
				}
				else
				{
					return &slGruntGrenadeCover[0];
				}
			}
		}
		break;
	
	default:
		{
			return CHGrunt::GetScheduleOfType(Type);
		}
		break;
	}
}
