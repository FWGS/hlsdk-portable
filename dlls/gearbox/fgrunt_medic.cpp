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
// friendly grunt - medic
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"fgrunt.h"


extern int g_fGruntQuestion;

extern Schedule_t	slIdleFgStand[];
extern Schedule_t	slFgFaceTarget[];

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	MGRUNT_CLIP_SIZE_EAGLE				7		// how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define	MGRUNT_CLIP_SIZE_9MMHANDGUN			17		// Same as above
#define MGRUNT_VOL							0.35		// volume of grunt sounds
#define MGRUNT_ATTN							ATTN_NORM	// attenutation of grunt sentences
#define MGRUNT_LIMP_HEALTH					20
#define MGRUNT_DMG_HEADSHOT					( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define MGRUNT_NUM_HEADS					2 // how many grunt heads are there? 
#define MGRUNT_MINIMUM_HEADSHOT_DAMAGE		15 // must do at least this much damage in one shot to head to score a headshot kill
#define	MGRUNT_SENTENCE_VOLUME				(float)0.35 // volume of grunt sentences
#define MGRUNT_MAX_HEALTH_RESTORE			10	// Maximum health restore. // 200

#define MGRUNT_EAGLE			(1 << 0)
#define MGRUNT_9MMHANDGUN		(1 << 1)
#define MGRUNT_NEEDLE			(1 << 2)

#define HEAD_GROUP				2
#define GUN_GROUP				3

#define HEAD_WHITE				0
#define HEAD_BLACK				1

#define GUN_EAGLE				0
#define GUN_9MMHANDGUN			1
#define GUN_NEEDLE				2
#define GUN_NONE				3


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MGRUNT_AE_BURST1		( 4 )
#define		MGRUNT_AE_BURST2		( 5 ) 
#define		MGRUNT_AE_BURST3		( 6 ) 

#define		MGRUNT_AE_GUN_HOLSTER	( 15 )
#define		MGRUNT_AE_NEEDLE_DRAW	( 16 )
#define		MGRUNT_AE_NEEDLE_STORE	( 17 )
#define		MGRUNT_AE_GUN_DRAW		( 18 )

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

#define MGRUNT_NUM_DEATH_SOUNDS		ARRAYSIZE( g_pszDeathSounds )
#define MGRUNT_NUM_PAIN_SOUNDS		ARRAYSIZE( g_pszPainSounds )


/*

	MGrunt animations.

*/

/*
pull_needle		(23) 	ACT_SIGNAL3
store_needle	(24) 	ACT_TWITCH
give_shot		(25) 	ACT_COWER
heal_crouch		(46) 	ACT_INSPECT_WALL
*/

class CMGrunt : public CFGrunt
{
public:
	void TalkInit(void);

	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);

	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL IsMedic(void) const { return TRUE; }

	CBaseEntity* DropGun(const Vector& vecSrc, const Vector& vecAngles, char* szClassname = NULL);

	void FireEagle( void );
	void FirePistol( void );

	CUSTOM_SCHEDULES;

	static const char *pMGruntSentences[];

	int m_nHealthRestore;
};

LINK_ENTITY_TO_CLASS(monster_human_medic_ally, CMGrunt);

TYPEDESCRIPTION	CMGrunt::m_SaveData[] =
{
	DEFINE_FIELD(CMGrunt, m_nHealthRestore, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CMGrunt, CTalkMonster);

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_MGRUNT_FIRST_SCHEDULE = LAST_COMMON_SCHEDULE + 1,
	SCHED_MGRUNT_HEAL_ALLY,
	SCHED_MGRUNT_HEAL_PLAYER,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_MGRUNT_NEEDLE_PULL = LAST_COMMON_TASK + 1,
	TASK_MGRUNT_NEEDLE_STORE,
	TASK_MGRUNT_GIVE_SHOT,
};




//=========================================================
// Medic draws the needle
//=========================================================
Task_t	tlMgNeedlePull[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_SIGNAL3 },
};

Schedule_t slMgNeedlePull[] =
{
	{
		tlMgNeedlePull,				// task array pointer.
		ARRAYSIZE(tlMgNeedlePull),	// task count
		0,							// COND interrupts (.i.e Light damage, heavy damage)
		0,							// soundmask (.i.e hear danger...)
		"MGrunt Needle Pull"		// name
	}
};


//=========================================================
// Medic stores the needle
//=========================================================
Task_t	tlMgNeedleStore[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_TWITCH },
};

Schedule_t slMgNeedleStore[] =
{
	{
		tlMgNeedleStore,
		ARRAYSIZE(tlMgNeedleStore),
		0,
		0,
		"MGrunt Needle Store"
	}
};


//=========================================================
// Medic gives a needle shot
//=========================================================
Task_t	tlMgGiveShot[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_COWER },
};

Schedule_t slMgGiveShot[] =
{
	{
		tlMgGiveShot,
		ARRAYSIZE(tlMgGiveShot),
		0,
		0,
		"MGrunt Give Shot"
	}
};

//=========================================================
// Medic heals a target (crouched)
//=========================================================
Task_t	tlMgHealCrouch[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_INSPECT_WALL },
};

Schedule_t slMgHealCrouch[] =
{
	{
		tlMgHealCrouch,
		ARRAYSIZE(tlMgHealCrouch),
		0,
		0,
		"MGrunt Heal Crouch"
	}
};



DEFINE_CUSTOM_SCHEDULES(CMGrunt)
{
	slMgNeedlePull,
	slMgNeedleStore,
	slMgGiveShot,
};

IMPLEMENT_CUSTOM_SCHEDULES(CMGrunt, CFGrunt);

const char *CMGrunt::pMGruntSentences[] =
{
	"MG_HEAL",
	"MG_NOTHEAL",
};

enum
{
	MGRUNT_SENT_NONE = -1,
	MGRUNT_SENT_HEAL = 0,
	MGRUNT_SENT_NOTHEAL,
} MGRUNT_SENTENCE_TYPES;


//=========================================================
// Spawn
//=========================================================
void CMGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hgrunt_medic.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.medicAllyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	// Medic has no torso support.
	torso = 0;

	// Select a random head.
	if (head == -1)
	{
		SetBodygroup(HEAD_GROUP, RANDOM_LONG(0, MGRUNT_NUM_HEADS - 1));
	}
	else
	{
		SetBodygroup(HEAD_GROUP, head);
	}

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MGRUNT_9MMHANDGUN;
	}

	// Setup bodygroups.
	if (FBitSet(pev->weapons, MGRUNT_EAGLE))
	{
		SetBodygroup(GUN_GROUP, GUN_EAGLE);
		m_cClipSize = MGRUNT_CLIP_SIZE_EAGLE;
	}
	else if (FBitSet(pev->weapons, MGRUNT_9MMHANDGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_9MMHANDGUN);
		m_cClipSize = MGRUNT_CLIP_SIZE_9MMHANDGUN;
	}
	else
	{
		ALERT(at_console, "ERROR: entity %s uses unsupported weapon flags %d\n", pev->classname, pev->weapons);
		m_cClipSize = -1;
	}

	m_cAmmoLoaded = m_cClipSize;

	m_nHealthRestore = MGRUNT_MAX_HEALTH_RESTORE;

#if 0

	const char* szSeqName = "pull_needle";
	int seq = LookupSequence(szSeqName);
	int i;

	for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
	{
		if (i == seq)
		{
			ALERT(at_console, "%s has sequence: %s with activity value (%d).\n", pev->classname, szSeqName, i);
			break;
		}
	}

	szSeqName = "store_needle";
	seq = LookupSequence(szSeqName);

	for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
	{
		if (i == seq)
		{
			ALERT(at_console, "%s has sequence: %s with activity value (%d).\n", pev->classname, szSeqName, i);
			break;
		}
	}

	szSeqName = "give_shot";
	seq = LookupSequence(szSeqName);

	for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
	{
		if (i == seq)
		{
			ALERT(at_console, "%s has sequence: %s with activity value (%d).\n", pev->classname, szSeqName, i);
			break;
		}
	}

	szSeqName = "heal_crouch";
	seq = LookupSequence(szSeqName);
	
	for (i = 0; i < ACT_FLINCH_RIGHTLEG; i++)
	{
		if (i == seq)
		{
			ALERT(at_console, "%s has sequence: %s with activity value (%d).\n", pev->classname, szSeqName, i);
			break;
		}
	}

#endif

	MonsterInit();
	SetUse(&CMGrunt::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMGrunt::Precache()
{
	PRECACHE_MODEL("models/hgrunt_medic.mdl");

	PRECACHE_SOUND("barney/ba_attack1.wav");
	PRECACHE_SOUND("barney/ba_attack2.wav");

	PRECACHE_SOUND("barney/desert_eagle_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("barney/medic_give_shot.wav");

	PRECACHE_SOUND_ARRAY( g_pszDeathSounds );

	PRECACHE_SOUND_ARRAY( g_pszPainSounds );

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}


//=========================================================
// Purpose:
//=========================================================
void CMGrunt::TalkInit( void )
{
	CFGrunt::TalkInit();
}

//=========================================================
// Purpose:
//=========================================================
void CMGrunt::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{

	case MGRUNT_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MGRUNT_EAGLE))
		{
			FireEagle();
		}
		else
		{
			FirePistol();
		}
	}
	break;

	case MGRUNT_AE_BURST2:
		break;

	case MGRUNT_AE_BURST3:
		break;


	case MGRUNT_AE_GUN_DRAW:
	{
		if (FBitSet(pev->weapons, MGRUNT_EAGLE))
		{
			SetBodygroup(GUN_GROUP, GUN_EAGLE);
		}
		else if (FBitSet(pev->weapons, MGRUNT_9MMHANDGUN))
		{
			SetBodygroup(GUN_GROUP, MGRUNT_9MMHANDGUN);
		}
	}
	break;

	case MGRUNT_AE_GUN_HOLSTER:
	case MGRUNT_AE_NEEDLE_STORE:
	{
		SetBodygroup( GUN_GROUP, GUN_NONE );
	}
	break;

	case MGRUNT_AE_NEEDLE_DRAW:
	{
		SetBodygroup( GUN_GROUP, GUN_NEEDLE );
	}
	break;

	default:
		CFGrunt::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// DropGun
//=========================================================
CBaseEntity* CMGrunt::DropGun(const Vector& vecGunPos, const Vector& vecGunAngles, char* szClassname)
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

	if (FBitSet(pev->weapons, MGRUNT_EAGLE))
	{
		pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
	}
	else
	{
		pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
	}

	return pGun;
}

//=========================================================
// Shoot
//=========================================================
void CMGrunt::FireEagle(void)
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 1024, BULLET_PLAYER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// Shoot
//=========================================================
void CMGrunt::FirePistol(void)
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}


void CMGrunt::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case 0:
	default:
		break;
	}
}

void CMGrunt::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case 0:
	default:
		break;
	}
}