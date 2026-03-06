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
*	Monster was made by XF-Alien
*
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		DIANA_AE_FEAR	( 2 )


class CDiana : public CTalkMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	void AlertSound(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);

	virtual int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeclineFollowing(void);

	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule(void);
	MONSTERSTATE GetIdealState(void);
	;
	void PainSound(void);
	void DeathSound(void);
	void TalkInit(void);
	void Killed(entvars_t* pevAttacker, int iGib);

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iBaseBody; //LRC - for barneys with different bodies
	BOOL	m_fGunDrawn;
	float	m_painTime;

	float	m_flPlayerDamage;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_diana_hayes, CDiana);

TYPEDESCRIPTION	CDiana::m_SaveData[] =
{
	DEFINE_FIELD(CDiana, m_iBaseBody, FIELD_INTEGER), //LRC
	DEFINE_FIELD(CDiana, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CDiana, m_flPlayerDamage, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CDiana, CTalkMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlDnFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slDnFollow[] =
{
	{
		tlDnFollow,
		ARRAYSIZE(tlDnFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlDianaEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_ARM },
};

Schedule_t slDianaEnemyDraw[] =
{
	{
		tlDianaEnemyDraw,
		ARRAYSIZE(tlDianaEnemyDraw),
		0,
		0,
		"Diana Enemy Draw"
	}
};

Task_t	tlDnFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slDnFaceTarget[] =
{
	{
		tlDnFaceTarget,
		ARRAYSIZE(tlDnFaceTarget),
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


Task_t	tlIdleDnStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleDnStand[] =
{
	{
		tlIdleDnStand,
		ARRAYSIZE(tlIdleDnStand),
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

DEFINE_CUSTOM_SCHEDULES(CDiana)
{
	slDnFollow,
		slDianaEnemyDraw,
		slDnFaceTarget,
		slIdleDnStand,
};


IMPLEMENT_CUSTOM_SCHEDULES(CDiana, CTalkMonster);


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CDiana::ISoundMask(void)
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
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDiana::Classify(void)
{
	return m_iClass ? m_iClass : CLASS_HUMAN_PASSIVE;
}

//=========================================================
// AlertSound
//=========================================================
void CDiana::AlertSound(void)
{
	if (m_hEnemy != NULL && !m_hEnemy->IsPlayer() && FOkToSpeak(SPEAK_DISREGARD_ENEMY))
		PlaySentence("DN_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDiana::SetYawSpeed(void)
{
	int ys;

	ys = 0;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 200;
		break;
	case ACT_WALK:
		ys = 200;
		break;
	case ACT_RUN:
		ys = 200;
		break;
	default:
		ys = 170;
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
void CDiana::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{

	case DIANA_AE_FEAR:
		break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CDiana::Spawn()
{
	Precache();

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/diana.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	if (pev->health == 0) //LRC
		pev->health = gSkillData.dianaHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse(&CDiana::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDiana::Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/diana.mdl");

	PRECACHE_SOUND("weapons/pl_gun3.wav");

	PRECACHE_SOUND("diana/pain1.wav");
	PRECACHE_SOUND("diana/pain2.wav");
	PRECACHE_SOUND("diana/pain3.wav");
	PRECACHE_SOUND("diana/pain4.wav");

	PRECACHE_SOUND("diana/dn_using1.wav");

	PRECACHE_SOUND("diana/death1.wav");
	PRECACHE_SOUND("diana/death2.wav");
	PRECACHE_SOUND("diana/death3.wav");

	PRECACHE_SOUND("diana/fem_step1.wav");
	PRECACHE_SOUND("diana/fem_step2.wav");
	PRECACHE_SOUND("diana/fem_step3.wav");

	PRECACHE_SOUND("diana/diana_sough1.wav");
	PRECACHE_SOUND("diana/diana_sough2.wav");
	PRECACHE_SOUND("diana/diana_sough3.wav");

	TalkInit();
	CTalkMonster::Precache();
}


void CDiana::TalkInit()
{

	CTalkMonster::TalkInit();

	if (!m_iszSpeakAs)
	{
		m_szGrp[TLK_IDLE] = "DN_IDLE";
		m_szGrp[TLK_STARE] = "DN_STARE";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER) //LRC
			m_szGrp[TLK_USE] = "DN_PFOLLOW";
		else
			m_szGrp[TLK_USE] = "DN_OK";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_UNUSE] = "DN_PWAIT";
		else
			m_szGrp[TLK_UNUSE] = "DN_WAIT";
		if (pev->spawnflags & SF_MONSTER_PREDISASTER)
			m_szGrp[TLK_DECLINE] = "DN_POK";
		else
			m_szGrp[TLK_DECLINE] = "DN_NOTOK";
		m_szGrp[TLK_STOP] = "DN_STOP";

		m_szGrp[TLK_NOSHOOT] = "DN_SCARED";

		m_szGrp[TLK_PLHURT1] = "!DN_CUREA";
		m_szGrp[TLK_PLHURT2] = "!DN_CUREB";
		m_szGrp[TLK_PLHURT3] = "!DN_CUREC";
	}

	m_voicePitch = 100;
}


static BOOL IsFacing(entvars_t* pevTest, const Vector& reference)
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


int CDiana::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	// LRC - if my reaction to the player has been overridden, don't do this stuff
	if (m_iPlayerReact) return ret;

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
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf, STRING(m_iszSpeakAs));
					strcat(szBuf, "_MAD");
					PlaySentence(szBuf, 4, VOL_NORM, ATTN_NORM);
				}
				else
				{
					PlaySentence("DN_MAD", 4, VOL_NORM, ATTN_NORM);
				}

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				if (m_iszSpeakAs)
				{
					char szBuf[32];
					strcpy(szBuf, STRING(m_iszSpeakAs));
					strcat(szBuf, "_SHOT");
					PlaySentence(szBuf, 4, VOL_NORM, ATTN_NORM);
				}
				else
				{
					PlaySentence("DN_SHOT", 4, VOL_NORM, ATTN_NORM);
				}
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			if (m_iszSpeakAs)
			{
				char szBuf[32];
				strcpy(szBuf, STRING(m_iszSpeakAs));
				strcat(szBuf, "_SHOT");
				PlaySentence(szBuf, 4, VOL_NORM, ATTN_NORM);
			}
			else
			{
				PlaySentence("DN_SHOT", 4, VOL_NORM, ATTN_NORM);
			}
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CDiana::PainSound(void)
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 3))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CDiana::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/death1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/death2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "diana/death3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

void CDiana::Killed(entvars_t* pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CDiana::GetScheduleOfType(int Type)
{
	Schedule_t* psched;
	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (m_hEnemy != NULL)
		{
			return slDianaEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slDnFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slDnFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleDnStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* CDiana::GetSchedule(void)
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		// Hey, be careful with that
		if (m_iszSpeakAs)
		{
			char szBuf[32];
			strcpy(szBuf, STRING(m_iszSpeakAs));
			strcat(szBuf, "_KILL");
			PlaySentence(szBuf, 4, VOL_NORM, ATTN_NORM);
		}
		else
		{
			PlaySentence("DN_KILL", 4, VOL_NORM, ATTN_NORM);
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster::GetSchedule();
		}
		if (HasConditions(bits_COND_NEW_ENEMY))
			AlertSound();
		// always act surprized with a new enemy
		if (HasConditions(bits_COND_NEW_ENEMY) && HasConditions(bits_COND_LIGHT_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		if (!m_fGunDrawn)
			return GetScheduleOfType(SCHED_ARM_WEAPON);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);
	}
	break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
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

	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CDiana::GetIdealState(void)
{
	return CTalkMonster::GetIdealState();
}


void CDiana::DeclineFollowing(void)
{
	PlaySentence(m_szGrp[TLK_DECLINE], 2, VOL_NORM, ATTN_NORM); //LRC
}