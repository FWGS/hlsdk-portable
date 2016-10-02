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
// Crispen
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"scientist.h"

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CCrispen : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);

	void StartTask(Task_t *pTask);
	void DeclineFollowing(void);

	BOOL	CanHeal(void);
	void	Scream(void);

	void PainSound(void);
	void TalkInit(void);

	static const char* pPainSounds[];
};

LINK_ENTITY_TO_CLASS(monster_crispen, CCrispen);

const char* CCrispen::pPainSounds[] =
{
	"crispen/pain1.wav",
	"crispen/pain2.wav",
	"crispen/pain3.wav",
	"crispen/pain4.wav",
	"crispen/pain5.wav",
};

void CCrispen::DeclineFollowing(void)
{
	Talk(10);
	m_hTalkTarget = m_hEnemy;
	PlaySentence("CS_POK", 2, VOL_NORM, ATTN_NORM);
}


void CCrispen::Scream(void)
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		PlaySentence("CS_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}


void CCrispen::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{

	case TASK_SAY_FEAR:
		if (FOkToSpeak())
		{
			Talk(2);
			m_hTalkTarget = m_hEnemy;
			if (m_hEnemy->IsPlayer())
				PlaySentence("CS_PLFEAR", 5, VOL_NORM, ATTN_NORM);
			else
				PlaySentence("CS_FEAR", 5, VOL_NORM, ATTN_NORM);
		}
		TaskComplete();
		break;

	default:
		CScientist::StartTask(pTask);
		break;
	}
}



//=========================================================
// Spawn
//=========================================================
void CCrispen::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/crispen.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	MonsterInit();
	SetUse(&CCrispen::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCrispen::Precache(void)
{
	PRECACHE_MODEL("models/crispen.mdl");

	PRECACHE_SOUND_ARRAY(pPainSounds);

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

// Init talk data
void CCrispen::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:

	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]		= "CS_ANSWER";
	m_szGrp[TLK_QUESTION]	= "CS_QUESTION";
	m_szGrp[TLK_IDLE]		= "CS_IDLE";
	m_szGrp[TLK_STARE]		= "CS_STARE";
	m_szGrp[TLK_USE]		= "CS_OK";
	m_szGrp[TLK_UNUSE]		= "CS_WAIT";
	m_szGrp[TLK_STOP]		= "CS_STOP";
	m_szGrp[TLK_NOSHOOT]	= "CS_SCARED";
	m_szGrp[TLK_HELLO]		= "CS_HELLO";

	m_szGrp[TLK_PLHURT1]	= "!CS_CUREA";
	m_szGrp[TLK_PLHURT2]	= "!CS_CUREB";
	m_szGrp[TLK_PLHURT3]	= "!CS_CUREC";

	m_szGrp[TLK_PHELLO]		= "CS_PHELLO";
	m_szGrp[TLK_PIDLE]		= "CS_PIDLE";
	m_szGrp[TLK_PQUESTION]	= "CS_PQUEST";
	m_szGrp[TLK_SMELL]		= "CS_SMELL";

	m_szGrp[TLK_WOUND]		= "CS_WOUND";
	m_szGrp[TLK_MORTAL]		= "CS_MORTAL";
}

//=========================================================
// PainSound
//=========================================================
void CCrispen::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM, 0, GetVoicePitch());
}

BOOL CCrispen::CanHeal(void)
{
	return FALSE;
}

//=========================================================
// Dead Crispen PROP
//=========================================================

class CDeadCrispen : public CDeadScientist
{
	void Spawn(void);
	static char *m_szPoses[7];
};


char *CDeadCrispen::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

LINK_ENTITY_TO_CLASS(monster_crispen_dead, CDeadCrispen);

//
// ********** DeadCrispen SPAWN **********
//
void CDeadCrispen::Spawn()
{
	PRECACHE_MODEL("models/crispen.mdl");
	SET_MODEL(ENT(pev), "models/crispen.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead crispen with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Crispen PROP
//=========================================================

class CSittingCrispen : public CSittingScientist
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(monster_sitting_crispen, CSittingCrispen);

// animation sequence aliases 
typedef enum
{
	SITTING_ANIM_sitlookleft,
	SITTING_ANIM_sitlookright,
	SITTING_ANIM_sitscared,
	SITTING_ANIM_sitting2,
	SITTING_ANIM_sitting3
} SITTING_ANIM;


//
// ********** Crispen SPAWN **********
//
void CSittingCrispen::Spawn()
{
	PRECACHE_MODEL("models/crispen.mdl");
	SET_MODEL(ENT(pev), "models/crispen.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 50;

	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	m_baseSequence = LookupSequence("sitlookleft");
	pev->sequence = m_baseSequence + RANDOM_LONG(0, 4);
	ResetSequenceInfo();

	SetThink(&CSittingCrispen::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR(ENT(pev));
}