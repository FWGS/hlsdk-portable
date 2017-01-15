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
// friendly grunt
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squad.h"
#include	"squadmonster.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"fgrunt.h"

#ifdef DEBUG
#include	"gearbox_utils.h"
#endif

extern int g_fGruntQuestion;		// true if an idle grunt asked a question. Cleared when someone answers.

extern DLL_GLOBAL int		g_iSkillLevel;

extern Schedule_t	slIdleStand[];
extern Schedule_t	slGruntFail[];
extern Schedule_t	slGruntCombatFail[];
extern Schedule_t	slGruntVictoryDance[];
extern Schedule_t	slGruntEstablishLineOfFire[];
extern Schedule_t	slGruntFoundEnemy[];
extern Schedule_t	slGruntCombatFace[];
extern Schedule_t	slGruntSignalSuppress[];
extern Schedule_t	slGruntSuppress[];
extern Schedule_t	slGruntWaitInCover[];
extern Schedule_t	slGruntTakeCover[];
extern Schedule_t	slGruntGrenadeCover[];
extern Schedule_t	slGruntTossGrenadeCover[];
extern Schedule_t	slGruntTakeCoverFromBestSound[];
extern Schedule_t	slGruntHideReload[];
extern Schedule_t	slGruntSweep[];
extern Schedule_t	slGruntRangeAttack1A[];
extern Schedule_t	slGruntRangeAttack1B[];
extern Schedule_t	slGruntRangeAttack2[];
extern Schedule_t	slGruntRepel[];
extern Schedule_t	slGruntRepelAttack[];
extern Schedule_t	slGruntRepelLand[];

extern Schedule_t	slBaFollow[];
extern Schedule_t	slBarneyEnemyDraw[];
extern Schedule_t	slBaFaceTarget[];
extern Schedule_t	slIdleBaStand[];


//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	FGRUNT_CLIP_SIZE_MP5				36		// how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define	FGRUNT_CLIP_SIZE_SAW				50		// Same as above
#define FGRUNT_VOL						0.35		// volume of grunt sounds
#define FGRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define FGRUNT_LIMP_HEALTH				20
#define FGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define FGRUNT_NUM_HEADS				8 // how many grunt heads are there? 
#define FGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	FGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define FGRUNT_9MMAR				( 1 << 0 )
#define FGRUNT_HANDGRENADE			( 1 << 1 )
#define FGRUNT_GRENADELAUNCHER		( 1 << 2 )
#define FGRUNT_SHOTGUN				( 1 << 3 )
#define FGRUNT_SAW					( 1 << 4 )

#define HEAD_GROUP					1
#define TORSO_GROUP					2
#define GUN_GROUP					3

#define HEAD_GASMASK				0
#define HEAD_BERET					1
#define HEAD_OPS_MASK				2
#define HEAD_BANDANA_WHITE			3
#define HEAD_BANDANA_BLACK			4
#define HEAD_MP						5
#define HEAD_MAJOR					6
#define HEAD_BERET_BLACK			7

#define TORSO_BACKPACK				0
#define TORSO_SAW					1
#define TORSO_NOBACKPACK			2
#define TORSO_SHELLS				3

#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_SAW						2
#define GUN_NONE					3

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		FGRUNT_AE_RELOAD		( 2 )
#define		FGRUNT_AE_KICK			( 3 )
#define		FGRUNT_AE_BURST1		( 4 )
#define		FGRUNT_AE_BURST2		( 5 ) 
#define		FGRUNT_AE_BURST3		( 6 ) 
#define		FGRUNT_AE_GREN_TOSS		( 7 )
#define		FGRUNT_AE_GREN_LAUNCH	( 8 )
#define		FGRUNT_AE_GREN_DROP		( 9 )
#define		FGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		FGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_FGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_FGRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_FGRUNT_COVER_AND_RELOAD,
	SCHED_FGRUNT_SWEEP,
	SCHED_FGRUNT_FOUND_ENEMY,
	SCHED_FGRUNT_REPEL,
	SCHED_FGRUNT_REPEL_ATTACK,
	SCHED_FGRUNT_REPEL_LAND,
	SCHED_FGRUNT_WAIT_FACE_ENEMY,
	SCHED_FGRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_FGRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_FGRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_FGRUNT_SPEAK_SENTENCE,
	TASK_FGRUNT_CHECK_FIRE,
};

static const char* g_pszDeathSounds[] =
{
	"fgrunt/death1.wav",
	"fgrunt/death2.wav",
	"fgrunt/death3.wav",
	"fgrunt/death4.wav",
	"fgrunt/death5.wav",
	"fgrunt/death6.wav",
};

static const char* g_pszPainSounds[] =
{
	"fgrunt/pain1.wav",
	"fgrunt/pain2.wav",
	"fgrunt/pain3.wav",
	"fgrunt/pain4.wav",
	"fgrunt/pain5.wav",
	"fgrunt/pain6.wav",
};

#define FGRUNT_NUM_DEATH_SOUNDS		ARRAYSIZE( g_pszDeathSounds )
#define FGRUNT_NUM_PAIN_SOUNDS		ARRAYSIZE( g_pszPainSounds )

//=========================================================
// CFGrunt
//=========================================================


LINK_ENTITY_TO_CLASS(monster_human_grunt_ally, CFGrunt);

TYPEDESCRIPTION	CFGrunt::m_SaveData[] =
{
	DEFINE_FIELD(CFGrunt, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CFGrunt, m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(CFGrunt, m_lastAttackCheck, FIELD_BOOLEAN),
	DEFINE_FIELD(CFGrunt, m_flPlayerDamage, FIELD_FLOAT),

	DEFINE_FIELD(CFGrunt, head, FIELD_INTEGER),
	DEFINE_FIELD(CFGrunt, torso, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CFGrunt, CTalkMonster);


#if 0

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFgFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t	slFgFollow[] =
{
	{
		tlFgFollow,
		ARRAYSIZE(tlFgFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlFgEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_ARM },
};

Schedule_t slFgEnemyDraw[] =
{
	{
		tlFgEnemyDraw,
		ARRAYSIZE(tlFgEnemyDraw),
		0,
		0,
		"FGrunt Enemy Draw"
	}
};

Task_t	tlFgFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t	slFgFaceTarget[] =
{
	{
		tlFgFaceTarget,
		ARRAYSIZE(tlFgFaceTarget),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleFgStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t	slIdleFgStand[] =
{
	{
		tlIdleFgStand,
		ARRAYSIZE(tlIdleFgStand),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES(CFGrunt)
{
	slFgFollow,
		slFgEnemyDraw,
		slFgFaceTarget,
		slIdleFgStand,
};
#else
DEFINE_CUSTOM_SCHEDULES(CFGrunt)
{
	slGruntFail,
	slGruntCombatFail,
	slGruntVictoryDance,
	slGruntEstablishLineOfFire,
	slGruntFoundEnemy,
	slGruntCombatFace,
	slGruntSignalSuppress,
	slGruntSuppress,
	slGruntWaitInCover,
	slGruntTakeCover,
	slGruntGrenadeCover,
	slGruntTossGrenadeCover,
	slGruntTakeCoverFromBestSound,
	slGruntHideReload,
	slGruntSweep,
	slGruntRangeAttack1A,
	slGruntRangeAttack1B,
	slGruntRangeAttack2,
	slGruntRepel,
	slGruntRepelAttack,
	slGruntRepelLand,

	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};
#endif


IMPLEMENT_CUSTOM_SCHEDULES(CFGrunt, CTalkMonster);


const char *CFGrunt::pGruntSentences[] =
{
	"FG_GREN", // grenade scared grunt
	"FG_ALERT", // sees player
	"FG_MONSTER", // sees monster
	"FG_COVER", // running to cover
	"FG_THROW", // about to throw grenade
	"FG_CHARGE",  // running out to get the enemy
	"FG_TAUNT", // say rude things
};

enum
{
	FGRUNT_SENT_NONE = -1,
	FGRUNT_SENT_GREN = 0,
	FGRUNT_SENT_ALERT,
	FGRUNT_SENT_MONSTER,
	FGRUNT_SENT_COVER,
	FGRUNT_SENT_THROW,
	FGRUNT_SENT_CHARGE,
	FGRUNT_SENT_TAUNT,

} FGRUNT_SENTENCE_TYPES;

//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after 
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has 
// started moving.
//=========================================================
void CFGrunt::SpeakSentence(void)
{
	if (m_iSentence == FGRUNT_SENT_NONE)
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz(ENT(pev), pGruntSentences[m_iSentence], FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are 
// Human Grunt's nemesis.
//=========================================================
int CFGrunt::IRelationship(CBaseEntity *pTarget)
{
	if (FClassnameIs(pTarget->pev, "monster_alien_grunt") || (FClassnameIs(pTarget->pev, "monster_gargantua")))
	{
		return R_NM;
	}

	return CTalkMonster::IRelationship(pTarget);
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CFGrunt::GibMonster(void)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	if (GetBodygroup(2) != 2)
	{// throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		CBaseEntity *pGun;
		if (FBitSet(pev->weapons, FGRUNT_SHOTGUN))
		{
			pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
		}
		else if (FBitSet(pev->weapons, FGRUNT_SAW))
		{
			pGun = DropItem("weapon_m249", vecGunPos, vecGunAngles);
		}
		else
		{
			pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
		}
		if (pGun)
		{
			pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
			pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
		}

		if (FBitSet(pev->weapons, FGRUNT_GRENADELAUNCHER))
		{
			pGun = DropItem("ammo_ARgrenades", vecGunPos, vecGunAngles);
			if (pGun)
			{
				pGun->pev->velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
				pGun->pev->avelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);
			}
		}
	}

	CTalkMonster::GibMonster();
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CFGrunt::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}


//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CFGrunt::FOkToSpeak(void)
{
	// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if (pev->spawnflags & SF_MONSTER_GAG)
	{
		if (m_MonsterState != MONSTERSTATE_COMBAT)
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
void CFGrunt::JustSpoke(void)
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = FGRUNT_SENT_NONE;
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFGrunt::Classify(void)
{
	return	CLASS_PLAYER_ALLY;
}


//=========================================================
//=========================================================
CBaseEntity *CFGrunt::Kick(void)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

Vector CFGrunt::GetGunPosition()
{
	if (m_fStanding)
	{
		return pev->origin + Vector(0, 0, 60);
	}
	else
	{
		return pev->origin + Vector(0, 0, 48);
	}
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CFGrunt::AlertSound(void)
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("FG_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFGrunt::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
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


//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds. 
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CFGrunt::FCanCheckAttacks(void)
{
	if (!HasConditions(bits_COND_ENEMY_TOOFAR))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CFGrunt::CheckMeleeAttack1(float flDot, float flDist)
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

	if (flDist <= 64 && flDot >= 0.7	&&
		pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON)
	{
		return TRUE;
	}
	return FALSE;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CFGrunt::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy))
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}

	CSquadMonster* pSquadMonster = MySquadMonsterPointer();

	if (pSquadMonster && pSquadMonster->InSquad())
	{
		if (!HasConditions(bits_COND_ENEMY_OCCLUDED) && flDist <= 2048 && flDot >= 0.5 && pSquadMonster->NoFriendlyFire())
		{
			TraceResult	tr;

			if (!m_hEnemy->IsPlayer() && flDist <= 64)
			{
				// kick nonclients, but don't shoot at them.
				return FALSE;
			}

			Vector vecSrc = GetGunPosition();

			// verify that a bullet fired from the gun will hit the enemy before the world.
			UTIL_TraceLine(vecSrc, m_hEnemy->BodyTarget(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

			if (tr.flFraction == 1.0)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}


//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack. 
//=========================================================
BOOL CFGrunt::CheckRangeAttack2(float flDot, float flDist)
{
	if (!FBitSet(pev->weapons, (FGRUNT_HANDGRENADE | FGRUNT_GRENADELAUNCHER)))
	{
		return FALSE;
	}

	// if the grunt isn't moving, it's ok to check.
	if (m_flGroundSpeed != 0)
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck)
	{
		return m_fThrowGrenade;
	}

	if (!FBitSet(m_hEnemy->pev->flags, FL_ONGROUND) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z)
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet(pev->weapons, FGRUNT_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0, 1))
		{
			// magically know where they are
			vecTarget = Vector(m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z);
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget(pev->origin) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions(bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	CSquadMonster* pSquadMonster = MySquadMonsterPointer();
	if (pSquadMonster && pSquadMonster->InSquad())
	{
		if (pSquadMonster->SquadMemberInRange(vecTarget, 256))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}

	if ((vecTarget - pev->origin).Length2D() <= 256)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}


	if (FBitSet(pev->weapons, FGRUNT_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss(pev, GetGunPosition(), vecTarget, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow(pev, GetGunPosition(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5);

		if (vecToss != g_vecZero)
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}

	return m_fThrowGrenade;
}

//=========================================================
// Fire
//=========================================================
void CFGrunt::Fire(const Vector& vecShootOrigin, const Vector& vecShootDir, const Vector& vecSpread, int model, int effects, int bulletType, int soundType)
{
	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, model, soundType);
	FireBullets(1, vecShootOrigin, vecShootDir, vecSpread, 2048, bulletType); // shoot +-5 degrees

	pev->effects |= effects;

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// Shoot
//=========================================================
void CFGrunt::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Fire(GetGunPosition(), ShootAtEnemy(GetGunPosition()), VECTOR_CONE_10DEGREES, m_iBrassShell, EF_MUZZLEFLASH, BULLET_MONSTER_MP5, TE_BOUNCE_SHELL);

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// Shotgun
//=========================================================
void CFGrunt::Shotgun(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Fire(GetGunPosition(), ShootAtEnemy(GetGunPosition()), VECTOR_CONE_15DEGREES, m_iShotgunShell, EF_MUZZLEFLASH, BULLET_PLAYER_BUCKSHOT, TE_BOUNCE_SHELL);

	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// Saw
//=========================================================
void CFGrunt::Saw(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Fire(GetGunPosition(), ShootAtEnemy(GetGunPosition()), VECTOR_CONE_10DEGREES, m_iSawShell, EF_MUZZLEFLASH, BULLET_PLAYER_556, TE_BOUNCE_SHELL);

	m_cAmmoLoaded--;// take away a bullet!
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CFGrunt::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case FGRUNT_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		GetAttachment(0, vecGunPos, vecGunAngles);

		// switch to body group with no gun.
		SetBodygroup(GUN_GROUP, GUN_NONE);

		// now spawn a gun.
		DropGun(vecGunPos, vecGunAngles);

		// Drop grenades if supported.
		if (FBitSet(pev->weapons, FGRUNT_GRENADELAUNCHER))
		{
			DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
		}

	}
	break;

	case FGRUNT_AE_RELOAD:
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM);
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case FGRUNT_AE_GREN_TOSS:
	{
		UTIL_MakeVectors(pev->angles);
		// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
		CGrenade::ShootTimed(pev, GetGunPosition(), m_vecTossVelocity, 3.5);

		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
		// !!!LATER - when in a group, only try to throw grenade if ordered.
	}
	break;

	case FGRUNT_AE_GREN_LAUNCH:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
		CGrenade::ShootContact(pev, GetGunPosition(), m_vecTossVelocity);
		m_fThrowGrenade = FALSE;
		if (g_iSkillLevel == SKILL_HARD)
			m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT(2, 5);// wait a random amount of time before shooting again
		else
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
	}
	break;

	case FGRUNT_AE_GREN_DROP:
	{
		UTIL_MakeVectors(pev->angles);
		CGrenade::ShootTimed(pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3);
	}
	break;

	case FGRUNT_AE_BURST1:
	{
		if (FBitSet(pev->weapons, FGRUNT_9MMAR))
		{
			Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else if (FBitSet(pev->weapons, FGRUNT_SAW))
		{
			Saw();

			switch (RANDOM_LONG(0, 2))
			{
			case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/saw_fire1.wav", 1, ATTN_NORM); break;
			case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/saw_fire2.wav", 1, ATTN_NORM); break;
			case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/saw_fire3.wav", 1, ATTN_NORM); break;
			default:
				break;
			}
		}
		else
		{
			Shotgun();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case FGRUNT_AE_BURST2:
	case FGRUNT_AE_BURST3:
		if (FBitSet(pev->weapons, FGRUNT_9MMAR))
		{
			Shoot();
		}
		else if(FBitSet(pev->weapons, FGRUNT_SAW))
		{
			Saw();
		}
		break;

	case FGRUNT_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.hgruntAllyDmgKick, DMG_CLUB);
		}
	}
	break;

	case FGRUNT_AE_CAUGHT_ENEMY:
	{
		if (FOkToSpeak())
		{
			SENTENCEG_PlayRndSz(ENT(pev), "FG_ALERT", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
			JustSpoke();
		}

	}

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CFGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hgrunt_opfor.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.hgruntAllyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;


	// Select a random head.
	if (head == -1)
	{
		SetBodygroup(HEAD_GROUP, RANDOM_LONG(0, FGRUNT_NUM_HEADS - 1 ));
	}
	else
	{
		SetBodygroup(HEAD_GROUP, head);
	}

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = FGRUNT_9MMAR | FGRUNT_HANDGRENADE;
	}

	// Setup bodygroups.
	if (FBitSet(pev->weapons, FGRUNT_SHOTGUN))
	{
		torso = TORSO_SHELLS;

		SetBodygroup(TORSO_GROUP, torso);
		SetBodygroup(GUN_GROUP, GUN_SHOTGUN);

		m_cClipSize = 8;
	}
	else if (FBitSet(pev->weapons, FGRUNT_SAW))
	{
		torso = TORSO_SAW;

		SetBodygroup(TORSO_GROUP, torso);
		SetBodygroup(GUN_GROUP, GUN_SAW);

		m_cClipSize = FGRUNT_CLIP_SIZE_SAW;
	}
	else
	{
		torso = TORSO_BACKPACK;

		SetBodygroup(TORSO_GROUP, torso);
		SetBodygroup(GUN_GROUP, GUN_MP5);

		m_cClipSize = FGRUNT_CLIP_SIZE_MP5;
	}


	m_cAmmoLoaded = m_cClipSize;

	MonsterInit();
	SetUse(&CFGrunt::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFGrunt::Precache()
{
	PRECACHE_MODEL("models/hgrunt_opfor.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/saw_fire1.wav");
	PRECACHE_SOUND("hgrunt/saw_fire2.wav");
	PRECACHE_SOUND("hgrunt/saw_fire3.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND_ARRAY( g_pszDeathSounds );

	PRECACHE_SOUND_ARRAY( g_pszPainSounds );

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
	m_iSawShell = PRECACHE_MODEL("models/saw_shell.mdl");


	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CFGrunt::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "FG_ANSWER";
	m_szGrp[TLK_QUESTION] = "FG_QUESTION";
	m_szGrp[TLK_IDLE] = "FG_IDLE";
	m_szGrp[TLK_STARE] = "FG_STARE";
	m_szGrp[TLK_USE] = "FG_OK";
	m_szGrp[TLK_UNUSE] = "FG_WAIT";
	m_szGrp[TLK_STOP] = "FG_STOP";

	m_szGrp[TLK_NOSHOOT] = "FG_SCARED";
	m_szGrp[TLK_HELLO] = "FG_HELLO";

	m_szGrp[TLK_PLHURT1] = "!FG_CUREA";
	m_szGrp[TLK_PLHURT2] = "!FG_CUREB";
	m_szGrp[TLK_PLHURT3] = "!FG_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;	
	m_szGrp[TLK_PQUESTION] = "FG_PQUEST";

	m_szGrp[TLK_SMELL] = "FG_SMELL";

	m_szGrp[TLK_WOUND] = "FG_WOUND";
	m_szGrp[TLK_MORTAL] = "FG_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}


static BOOL IsFacing(entvars_t *pevTest, const Vector &reference)
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate(angle, forward, NULL, NULL);
	// He's facing me, he meant it
	if (DotProduct(forward, vecDir) > 0.96)	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CFGrunt::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("FG_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("FG_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("FG_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CFGrunt::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, g_pszPainSounds[ RANDOM_LONG(0, FGRUNT_NUM_PAIN_SOUNDS - 1) ], 1, ATTN_NORM, 0, GetVoicePitch());
}

//=========================================================
// DeathSound 
//=========================================================
void CFGrunt::DeathSound(void)
{
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, g_pszDeathSounds[RANDOM_LONG(0, FGRUNT_NUM_DEATH_SOUNDS - 1)], 1, ATTN_NORM, 0, GetVoicePitch());
}


//=========================================================
// TractAttack
//=========================================================
void CFGrunt::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				flDamage = 0.01;
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CFGrunt::Killed(entvars_t *pevAttacker, int iGib)
{
	if (GetBodygroup(2) != 2)
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		// throw a gun if the grunt has one
		GetAttachment(0, vecGunPos, vecGunAngles);

		DropGun(vecGunPos, vecGunAngles);
	}

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}


//=========================================================
// DropGun
//=========================================================
CBaseEntity* CFGrunt::DropGun(const Vector& vecGunPos, const Vector& vecGunAngles, char* szClassname)
{
	CBaseEntity* pGun = NULL;

	if (szClassname && *szClassname)
	{
		pGun = DropItem(szClassname, vecGunPos, vecGunAngles);

		if (pGun)
		{
			return pGun;
		}
		else
		{
			ALERT(at_console, "ERROR: Could not find classname %s. No such class.\n", szClassname);
		}
	}

	if (FBitSet(pev->weapons, FGRUNT_SHOTGUN))
	{
		pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
	}
	else if (FBitSet(pev->weapons, FGRUNT_SAW))
	{
		pGun = DropItem("weapon_m249", vecGunPos, vecGunAngles);
	}
	else
	{
		pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
	}

	return pGun;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CFGrunt::PrescheduleThink(void)
{
	CSquadMonster* pSquadMonster = MySquadMonsterPointer();

	if (pSquadMonster && pSquadMonster->InSquad() && m_hEnemy != NULL)
	{
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			// update the squad's last enemy sighting time.
			

			pSquadMonster->MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if (gpGlobals->time - pSquadMonster->MySquadLeader()->m_flLastEnemySightTime > 5)
			{
				// been a while since we've seen the enemy
				pSquadMonster->MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CFGrunt::GetScheduleOfType(int Type)
{
#if 1
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		CSquadMonster* pSquadMonster = MySquadMonsterPointer();
		if (pSquadMonster && pSquadMonster->InSquad())
		{
			if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_THROW", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
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
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
	{
		return &slGruntTakeCoverFromBestSound[0];
	}
	case SCHED_FGRUNT_TAKECOVER_FAILED:
	{
		CSquadMonster* pSquadMonster = MySquadMonsterPointer();
		if (pSquadMonster && pSquadMonster->InSquad())
		{
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK1) && pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
		}

		return GetScheduleOfType(SCHED_FAIL);
	}
	break;
	case SCHED_FGRUNT_ELOF_FAIL:
	{
		// human grunt is unable to move to a position that allows him to attack the enemy.
		return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case SCHED_FGRUNT_ESTABLISH_LINE_OF_FIRE:
	{
		return &slGruntEstablishLineOfFire[0];
	}
	break;
	case SCHED_RANGE_ATTACK1:
	{
		// randomly stand or crouch
		if (RANDOM_LONG(0, 9) == 0)
			m_fStanding = RANDOM_LONG(0, 1);

		if (m_fStanding)
			return &slGruntRangeAttack1B[0];
		else
			return &slGruntRangeAttack1A[0];
	}
	case SCHED_RANGE_ATTACK2:
	{
		return &slGruntRangeAttack2[0];
	}
	case SCHED_COMBAT_FACE:
	{
		return &slGruntCombatFace[0];
	}
	case SCHED_FGRUNT_WAIT_FACE_ENEMY:
	{
		return &slGruntWaitInCover[0];
	}
	case SCHED_FGRUNT_SWEEP:
	{
		return &slGruntSweep[0];
	}
	case SCHED_FGRUNT_COVER_AND_RELOAD:
	{
		return &slGruntHideReload[0];
	}
	case SCHED_FGRUNT_FOUND_ENEMY:
	{
		return &slGruntFoundEnemy[0];
	}
	case SCHED_VICTORY_DANCE:
	{
		CSquadMonster* pSquadMonster = MySquadMonsterPointer();
		if (pSquadMonster && pSquadMonster->InSquad())
		{
			if (!pSquadMonster->IsLeader())
			{
				return &slGruntFail[0];
			}
		}

		return &slGruntVictoryDance[0];
	}
	case SCHED_FGRUNT_SUPPRESS:
	{
		if (m_hEnemy->IsPlayer() && m_fFirstEncounter)
		{
			m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
			return &slGruntSignalSuppress[0];
		}
		else
		{
			return &slGruntSuppress[0];
		}
	}
	case SCHED_FAIL:
	{
		if (m_hEnemy != NULL)
		{
			// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
			return &slGruntCombatFail[0];
		}

		return &slGruntFail[0];
	}
	case SCHED_FGRUNT_REPEL:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slGruntRepel[0];
	}
	case SCHED_FGRUNT_REPEL_ATTACK:
	{
		if (pev->velocity.z > -128)
			pev->velocity.z -= 32;
		return &slGruntRepelAttack[0];
	}
	case SCHED_FGRUNT_REPEL_LAND:
	{
		return &slGruntRepelLand[0];
	}

	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this gfor different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;	
	}

#else
	Schedule_t *psched;

	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (m_hEnemy != NULL)
		{
			// face enemy, then draw.
			return slFgEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slFgFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slFgFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleFgStand;
		}
		else
			return psched;
	}
#endif

	return CTalkMonster::GetScheduleOfType(Type);
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//
// Only supported when this monster has squad support!
//
//=========================================================
Schedule_t *CFGrunt::GetSquadSchedule(void)
{
	CSquadMonster* pSquadMonster = MySquadMonsterPointer();

	// This method is reserved for NPCs with squad support!.
	ASSERT(pSquadMonster != NULL);

	CSquadMonster* pSquadLeader = pSquadMonster->MySquadLeader();

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// new enemy
		if (HasConditions(bits_COND_NEW_ENEMY))
		{
			pSquadLeader->m_fEnemyEluded = FALSE;

			if (!pSquadMonster->IsLeader())
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
						SENTENCEG_PlayRndSz(ENT(pev), "FG_ALERT", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
					else if ((m_hEnemy != NULL) &&
						(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
						(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
						(m_hEnemy->Classify() != CLASS_MACHINE))
						// monster
						SENTENCEG_PlayRndSz(ENT(pev), "FG_MONST", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);

					JustSpoke();
				}

				if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
				{
					return GetScheduleOfType(SCHED_FGRUNT_SUPPRESS);
				}
				else
				{
					return GetScheduleOfType(SCHED_FGRUNT_ESTABLISH_LINE_OF_FIRE);
				}
			}
		}

		// can grenade launch
		else if (FBitSet(pev->weapons, FGRUNT_GRENADELAUNCHER) && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
		{
			// shoot a grenade if you can
			return GetScheduleOfType(SCHED_RANGE_ATTACK2);
		}
		// can shoot
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			// if the enemy has eluded the squad and a squad member has just located the enemy
			// and the enemy does not see the squad member, issue a call to the squad to waste a 
			// little time and give the player a chance to turn.
			if (pSquadLeader->m_fEnemyEluded && !HasConditions(bits_COND_ENEMY_FACING_ME))
			{
				pSquadLeader->m_fEnemyEluded = FALSE;
				return GetScheduleOfType(SCHED_FGRUNT_FOUND_ENEMY);
			}

			if (pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				// try to take an available ENGAGE slot
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
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
			if (HasConditions(bits_COND_CAN_RANGE_ATTACK2) && pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
			{
				//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz(ENT(pev), "FG_THROW", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			else if (pSquadMonster->OccupySlot(bits_SLOTS_HGRUNT_ENGAGE))
			{
				//!!!KELLY - grunt cannot see the enemy and has just decided to 
				// charge the enemy's position. 
				if (FOkToSpeak())// && RANDOM_LONG(0,1))
				{
					//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					m_iSentence = FGRUNT_SENT_CHARGE;
					//JustSpoke();
				}

				return GetScheduleOfType(SCHED_FGRUNT_ESTABLISH_LINE_OF_FIRE);
			}
		}

	}
	break;
	default:
		break;
	}

	return NULL;
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CFGrunt::GetSchedule(void)
{
	// clear old sentence
	m_iSentence = FGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if (pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE)
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType(SCHED_FGRUNT_REPEL_LAND);
		}
		else
		{
			// repel down a rope, 
			if (m_MonsterState == MONSTERSTATE_COMBAT)
				return GetScheduleOfType(SCHED_FGRUNT_REPEL_ATTACK);
			else
				return GetScheduleOfType(SCHED_FGRUNT_REPEL);
		}
	}

	// grunts place HIGH priority on running away from danger sounds.
	if (HasConditions(bits_COND_HEAR_SOUND))
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
					SENTENCEG_PlayRndSz( ENT(pev), "FG_GREN", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
			MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/
		}
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("FG_KILL", 4, VOL_NORM, ATTN_NORM);
	}

#if 1
	// Check if we have squad support.
	CSquadMonster* pSquadMonster = MySquadMonsterPointer();

	if (pSquadMonster)
	{
		// squad schedule.
		Schedule_t* squadSched = GetSquadSchedule();

		if (squadSched)
		{
			return squadSched;
		}
	}
#endif


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

		}

		// no ammo
		else if (HasConditions(bits_COND_NO_AMMO_LOADED))
		{
			//!!!KELLY - this individual just realized he's out of bullet ammo. 
			// He's going to try to find cover to run to and reload, but rarely, if 
			// none is available, he'll drop and reload in the open here. 
			return GetScheduleOfType(SCHED_FGRUNT_COVER_AND_RELOAD);
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
					m_iSentence = FGRUNT_SENT_COVER;
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
		else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (FBitSet(pev->weapons, FGRUNT_GRENADELAUNCHER) && HasConditions(bits_COND_CAN_RANGE_ATTACK2))
			{
				// shoot a grenade if you can
				return GetScheduleOfType(SCHED_RANGE_ATTACK2);
			}
			// can shoot
			else if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			{
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
			}
		}
		// can't see enemy
		else if (HasConditions(bits_COND_ENEMY_OCCLUDED))
		{
			//!!!KELLY - grunt is going to stay put for a couple seconds to see if
			// the enemy wanders back out into the open, or approaches the
			// grunt's covered position. Good place for a taunt, I guess?
			if (FOkToSpeak() && RANDOM_LONG(0, 1))
			{
				SENTENCEG_PlayRndSz(ENT(pev), "FG_TAUNT", FGRUNT_SENTENCE_VOLUME, FGRUNT_ATTN, 0, m_voicePitch);
				JustSpoke();
			}
			return GetScheduleOfType(SCHED_STANDOFF);
		}

		if (HasConditions(bits_COND_SEE_ENEMY) && !HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_FGRUNT_ESTABLISH_LINE_OF_FIRE);
		}
	}
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
	{
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	}

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CFGrunt::GetIdealState(void)
{
	return CTalkMonster::GetIdealState();
}

void CFGrunt::DeclineFollowing(void)
{
	PlaySentence("FG_POK", 2, VOL_NORM, ATTN_NORM);
}

void CFGrunt::StartTask(Task_t *pTask)
{
#if 1
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_FGRUNT_CHECK_FIRE:
	{
		CSquadMonster* pSquadMonster = MySquadMonsterPointer();

		if (pSquadMonster && !pSquadMonster->NoFriendlyFire())
		{
			SetConditions(bits_COND_SPECIAL1); // bits_COND_GRUNT_NOFIRE
		}
		TaskComplete();
	}
	break;

	case TASK_FGRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget(bits_MEMORY_INCOVER);
		CTalkMonster::StartTask(pTask);
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_FGRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CTalkMonster::StartTask(pTask);
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default:
		CTalkMonster::StartTask(pTask);
		break;
	}
#endif
	CTalkMonster::StartTask(pTask);
}

void CFGrunt::RunTask(Task_t *pTask)
{
#if 1
	switch ( pTask->iTask )
	{
	case TASK_FGRUNT_FACE_TOSS_DIR:
	{
		// project a point along the toss vector and turn to face that point.
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( pev->yaw_speed );

		if ( FacingIdeal() )
		{
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
	}
	break;

	case TASK_RANGE_ATTACK1:
	{
		//if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		//{
		//	pev->framerate = 1.5;
		//}
		CTalkMonster::RunTask(pTask);
	}
	break;

	default:
	{
		CTalkMonster::RunTask(pTask);
		break;
	}
}

#else
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask(pTask);
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
#endif
}


void CFGrunt::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "head"))
	{
		head = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CTalkMonster::KeyValue(pkvd);
}

//=========================================================
// SetActivity 
//=========================================================
void CFGrunt::SetActivity(Activity NewActivity)
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR(ENT(pev));

#ifdef DEBUG
	ALERT(at_console, "ALERT: incoming activity: %s\n", UTIL_GetActivityNameBySequence(LookupActivity(NewActivity)));
#endif

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched
		if (FBitSet(pev->weapons, FGRUNT_9MMAR))
		{
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
		}
		else if (FBitSet(pev->weapons, FGRUNT_SAW))
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_saw");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_saw");
			}
		}
		else
		{
			if (m_fStanding)
			{
				// get aimable sequence
				iSequence = LookupSequence("standing_shotgun");
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence("crouching_shotgun");
			}
		}
		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown 
		// grenade or fired grenade, we must determine which and pick proper sequence
		if (pev->weapons & FGRUNT_HANDGRENADE)
		{
			// get toss anim
			iSequence = LookupSequence("throwgrenade");
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence("launchgrenade");
		}
		break;
	case ACT_RUN:
		if (pev->health <= FGRUNT_LIMP_HEALTH)
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
		if (pev->health <= FGRUNT_LIMP_HEALTH)
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
#if 1
	
	case ACT_FLINCH_LEFTLEG:
	case ACT_FLINCH_RIGHTLEG:
	case ACT_FLINCH_LEFTARM:
	case ACT_FLINCH_RIGHTARM:
	{
		NewActivity = ACT_SMALL_FLINCH;
		iSequence = LookupActivity(NewActivity);
	}
	break;
#endif

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


#ifdef DEBUG
	UTIL_PrintActivity(this);
#endif
}


//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CFGruntRepel : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void EXPORT RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int m_iSpriteTexture;	// Don't save, precache
};



//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

LINK_ENTITY_TO_CLASS(monster_grunt_ally_repel, CFGruntRepel);

void CFGruntRepel::Spawn(void)
{
	Precache();
	pev->solid = SOLID_NOT;

	SetUse(&CFGruntRepel::RepelUse);
}

void CFGruntRepel::Precache(void)
{
	UTIL_PrecacheOther("monster_human_grunt_ally");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/rope.spr");
}

void CFGruntRepel::RepelUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
	return NULL;
	*/

	CBaseEntity *pEntity = Create("monster_human_grunt_ally", pev->origin, pev->angles);
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer();
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector(0, 0, RANDOM_FLOAT(-196, -128));
	pGrunt->SetActivity(ACT_GLIDE);
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate("sprites/rope.spr", 10);
	pBeam->PointEntInit(pev->origin + Vector(0, 0, 112), pGrunt->entindex());
	pBeam->SetFlags(BEAM_FSOLID);
	pBeam->SetColor(255, 255, 255);
	pBeam->SetThink(&CBeam::SUB_Remove);
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove(this);
}






//=========================================================
// DEAD FGRUNT PROP
//=========================================================
class CDeadFGrunt : public CBaseMonster
{
public:
	void Spawn(void);
	int	Classify(void) { return	CLASS_HUMAN_MILITARY; }

	void KeyValue(KeyValueData *pkvd);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	int m_iHead;
	static char *m_szPoses[6];
};

LINK_ENTITY_TO_CLASS(monster_human_grunt_ally_dead, CDeadFGrunt);

TYPEDESCRIPTION CDeadFGrunt::m_SaveData[] =
{
	DEFINE_FIELD(CDeadFGrunt, m_iHead, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CDeadFGrunt, CBaseMonster);

char *CDeadFGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting", "dead_on_back", "dead_on_stomach", "dead_headcrabed" };

void CDeadFGrunt::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "head"))
	{
		m_iHead = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}


//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadFGrunt::Spawn(void)
{
	PRECACHE_MODEL("models/hgrunt_opfor.mdl");
	SET_MODEL(ENT(pev), "models/hgrunt_opfor.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead fgrunt with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;


	// Select a random head.
	if (m_iHead == -1)
	{
		SetBodygroup(HEAD_GROUP, RANDOM_LONG(0, FGRUNT_NUM_HEADS - 1));
	}

	if (pev->weapons == 0)
	{
		SetBodygroup(HEAD_GROUP, HEAD_GASMASK);
		SetBodygroup(TORSO_GROUP, TORSO_BACKPACK);
		SetBodygroup(GUN_GROUP, GUN_MP5);
	}
	else if (FBitSet(pev->weapons, FGRUNT_9MMAR))
	{
		SetBodygroup(TORSO_GROUP, TORSO_BACKPACK);
		SetBodygroup(GUN_GROUP, GUN_MP5);
	}

#if 0
	// map old bodies onto new bodies
	switch (pev->body)
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(HEAD_GROUP, HEAD_GRUNT);
		SetBodygroup(GUN_GROUP, GUN_MP5);
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(HEAD_GROUP, HEAD_COMMANDER);
		SetBodygroup(GUN_GROUP, GUN_MP5);
		break;
	case 2: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(HEAD_GROUP, HEAD_GRUNT);
		SetBodygroup(GUN_GROUP, GUN_NONE);
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(HEAD_GROUP, HEAD_COMMANDER);
		SetBodygroup(GUN_GROUP, GUN_NONE);
		break;
	}
#else
	// Until we get head support, set these to default.
	pev->body = 0;
	pev->skin = 0;
#endif

	MonsterInitDead();
}