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
// human scientist (passive lab worker)
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "scripted.h"
#include "animation.h"
#include "soundent.h"

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED
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
	TASK_WANDER
};

#define SCIENTIST_AE_HEAL 1
#define SCIENTIST_AE_NEEDLEON 2
#define SCIENTIST_AE_NEEDLEOFF 3

class CNPC : public CTalkMonster
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CNPC() : m_fleeOnDanger(FALSE), m_canCower(TRUE), m_canProvoke(TRUE), m_canWander(FALSE) {}
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	void SetYawSpeed();
	int Classify() { return CLASS_HUMAN_PASSIVE; }
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	int ObjectCaps() { return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int FriendNumber(int arrayNumber);
	void SetActivity(Activity newActivity);
	Activity GetStoppedActivity();
	int ISoundMask() { return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_PLAYER; }
	void DeclineFollowing();
	float CoverRadius() { return 1200; }
	BOOL DisregardEnemy(CBaseEntity *pEnemy) { return !pEnemy->IsAlive() || gpGlobals->time - m_fearTime > 15; }
	BOOL CanHeal();
	void Heal();
	void Scream();

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule();
	MONSTERSTATE GetIdealState();
	void DeathSound();
	void PainSound();
	void TalkInit();
	void Killed(entvars_t *pevAttacker, int iGib);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	CUSTOM_SCHEDULES;
private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
	BOOL m_fleeOnDanger;
	BOOL m_canCower;
	BOOL m_canProvoke;
	float m_wanderTime;
	Vector m_wanderOrigin;
	BOOL m_canWander;
};

LINK_ENTITY_TO_CLASS(monster_npc, CNPC);

TYPEDESCRIPTION CNPC::m_SaveData[] =
{
	DEFINE_FIELD(CNPC, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CNPC, m_healTime, FIELD_TIME),
	DEFINE_FIELD(CNPC, m_fearTime, FIELD_TIME),
	DEFINE_FIELD(CNPC, m_fleeOnDanger, FIELD_BOOLEAN),
	DEFINE_FIELD(CNPC, m_canCower, FIELD_BOOLEAN),
	DEFINE_FIELD(CNPC, m_canProvoke, FIELD_BOOLEAN),
	DEFINE_FIELD(CNPC, m_wanderTime, FIELD_TIME),
	DEFINE_FIELD(CNPC, m_wanderOrigin, FIELD_VECTOR),
	DEFINE_FIELD(CNPC, m_canWander, FIELD_BOOLEAN)
};

IMPLEMENT_SAVERESTORE(CNPC, CTalkMonster);

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t tlFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_CANT_FOLLOW },
	{ TASK_MOVE_TO_TARGET_RANGE, 128 }
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};
Schedule_t slFollow[] =
{
	{ tlFollow, ARRAYSIZE(tlFollow), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "Follow" }
};

Task_t tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_CHASE },
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, 128 }
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};
Schedule_t slFollowScared[] =
{
	{ tlFollowScared, ARRAYSIZE(tlFollowScared), bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER, "FollowScared" }
};

Task_t tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, SCHED_TARGET_CHASE_SCARED }
};
Schedule_t slFaceTargetScared[] =
{
	{ tlFaceTargetScared, ARRAYSIZE(tlFaceTargetScared), bits_COND_HEAR_SOUND | bits_COND_NEW_ENEMY, bits_SOUND_DANGER, "FaceTargetScared" }
};

Task_t tlStopFollowing[] = { { TASK_CANT_FOLLOW, 0 } };
Schedule_t slStopFollowing[] = { { tlStopFollowing, ARRAYSIZE(tlStopFollowing), 0, 0, "StopFollowing" } };

Task_t tlHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 50 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_TARGET_CHASE },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_SAY_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, ACT_ARM },
	{ TASK_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, ACT_DISARM }
};
Schedule_t slHeal[] = { { tlHeal, ARRAYSIZE(tlHeal), 0, 0, "Heal" } };

Task_t tlFaceTarget[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_SET_SCHEDULE, SCHED_TARGET_CHASE }
};
Schedule_t slFaceTarget[] =
{
	{ tlFaceTarget, ARRAYSIZE(tlFaceTarget), bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "FaceTarget" }
};

Task_t tlSciPanic[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_SCREAM, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_EXCITED },
	{ TASK_SET_ACTIVITY, ACT_IDLE }
};
Schedule_t slSciPanic[] = { { tlSciPanic, ARRAYSIZE(tlSciPanic), 0, 0, "SciPanic" } };

Task_t tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 }
};
Schedule_t slIdleSciStand[] =
{
	{ tlIdleSciStand, ARRAYSIZE(tlIdleSciStand), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_CLIENT_PUSH | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE, "IdleSciStand" }
};

Task_t tlNPCCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH_SCARED, 0 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_SET_SCHEDULE, SCHED_HIDE }
};
Schedule_t slNPCCover[] = { { tlNPCCover, ARRAYSIZE(tlNPCCover), bits_COND_NEW_ENEMY, 0, "ScientistCover" } };

Task_t tlNPCHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_CROUCH },
	{ TASK_SET_ACTIVITY, ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 10 }
};
Schedule_t slNPCHide[] =
{
	{ tlNPCHide, ARRAYSIZE(tlNPCHide), bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE | bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE, bits_SOUND_DANGER, "ScientistHide" }
};

Task_t tlNPCStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_PANIC },
	{ TASK_RANDOM_SCREAM, 0.3f },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, 0.1f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 1 }
};
Schedule_t slNPCStartle[] =
{
	{ tlNPCStartle, ARRAYSIZE(tlNPCStartle), bits_COND_NEW_ENEMY | bits_COND_SEE_ENEMY | bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE, 0, "ScientistStartle" }
};

Task_t tlFear[] = { { TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_SAY_FEAR, 0 } };
Schedule_t slFear[] = { { tlFear, ARRAYSIZE(tlFear), bits_COND_NEW_ENEMY, 0, "Fear" } };

Task_t tlWander[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_IDLE_STAND },
	{ TASK_WANDER, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};
Schedule_t slWander[] =
{
	{ tlWander, ARRAYSIZE(tlWander), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_CLIENT_PUSH | bits_COND_PROVOKED, bits_SOUND_DANGER, "Wander" }
};

DEFINE_CUSTOM_SCHEDULES(CNPC)
{
	slFollow, slFaceTarget, slIdleSciStand, slFear, slNPCCover, slNPCHide, slNPCStartle,
	slHeal, slStopFollowing, slSciPanic, slFollowScared, slFaceTargetScared, slWander
};
IMPLEMENT_CUSTOM_SCHEDULES(CNPC, CTalkMonster);

void CNPC::Spawn()
{
	if (pev->model && (strstr(STRING(pev->model), "us_gal") || strstr(STRING(pev->model), "schoolgirl")))
		m_femaleFlag = true;
	Precache();
	SET_MODEL(ENT(pev), pev->model ? STRING(pev->model) : "models/npc-scientist.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50); // position of the eyes relative to monster's origin.

//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	MonsterInit();
	if (m_invulnerable)
		pev->takedamage = DAMAGE_NO;
	if (m_head > 0)
		SetBodygroup(1, m_head);
	if (!m_language)
		m_language = ALLOC_STRING("AM");
	SetUse(&CTalkMonster::FollowerUse);
}

void CNPC::Precache()
{
	PRECACHE_MODEL((char *)(pev->model ? STRING(pev->model) : "models/npc-scientist.mdl"));

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CNPC::TalkInit()
{
	CTalkMonster::TalkInit();
	m_szFriends[0] = "monster_npc";
	m_szFriends[1] = "monster_sitting_npc";
	m_szFriends[2] = "monster_barney";
	m_szGrp[TLK_ANSWER] = "NPC_ANSWER";
	m_szGrp[TLK_QUESTION] = "NPC_QUESTION";
	m_szGrp[TLK_IDLE] = "NPC_IDLE";
	m_szGrp[TLK_STARE] = "NPC_STARE";
	m_szGrp[TLK_USE] = "NPC_OK";
	m_szGrp[TLK_UNUSE] = "NPC_WAIT";
	m_szGrp[TLK_STOP] = "NPC_STOP";
	m_szGrp[TLK_NOSHOOT] = "NPC_SCARED";
	m_szGrp[TLK_HELLO] = "NPC_HELLO";
	m_szGrp[TLK_PLHURT1] = "!NPC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!NPC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!NPC_CUREC";
	m_szGrp[TLK_PHELLO] = "NPC_PHELLO";
	m_szGrp[TLK_PIDLE] = "NPC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "NPC_PQUEST";
	m_szGrp[TLK_SMELL] = "NPC_SMELL";
	m_szGrp[TLK_WOUND] = "NPC_WOUND";
	m_szGrp[TLK_MORTAL] = "NPC_MORTAL";
	m_voicePitch = 100;
}

void CNPC::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "fleeondanger"))
	{
		m_fleeOnDanger = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "cower"))
	{
		m_canCower = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "provoke"))
	{
		m_canProvoke = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "wander"))
	{
		m_canWander = atoi(pkvd->szValue) != 0;
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

void CNPC::SetYawSpeed()
{
	switch (m_Activity)
	{
	case ACT_IDLE: pev->yaw_speed = 120; break;
	case ACT_WALK: pev->yaw_speed = 180; break;
	case ACT_RUN: pev->yaw_speed = 150; break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT: pev->yaw_speed = 120; break;
	default: pev->yaw_speed = 90; break;
	}
}

int CNPC::FriendNumber(int arrayNumber)
{
	static int array[3] = { 1, 2, 0 };
	return arrayNumber < 3 ? array[arrayNumber] : arrayNumber;
}

void CNPC::SetActivity(Activity newActivity)
{
	int sequence;
	if (((newActivity >= ACT_DIESIMPLE && newActivity <= ACT_DIEVIOLENT) ||
		(newActivity >= ACT_DIE_HEADSHOT && newActivity <= ACT_DIE_BACKSHOT)) && m_deathAnim)
		sequence = LookupSequence(STRING(m_deathAnim));
	else if (newActivity == ACT_IDLE && m_idleAnim)
		sequence = LookupSequence(STRING(m_idleAnim));
	else
		sequence = LookupActivity(newActivity);
	if (sequence == -1)
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity(newActivity);
}

Activity CNPC::GetStoppedActivity()
{
	return m_hEnemy != NULL ? ACT_EXCITED : ACT_IDLE;
}

int CNPC::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if (m_canProvoke && pevInflictor && (pevInflictor->flags & FL_CLIENT))
	{
		Remember(bits_MEMORY_PROVOKED);
		StopFollowing(TRUE);
	}

	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CNPC::Killed(entvars_t *pevAttacker, int iGib)
{
	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

void CNPC::PainSound()
{
	if (m_painTime <= gpGlobals->time)
	{
		m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);
		SayNamedSentence("NPC_PAIN", 1, ATTN_NORM, 0, GetVoicePitch());
	}
}

void CNPC::DeathSound()
{
	if (!FOkToSpeak())
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/null.wav", 0.5, ATTN_NORM, 0, m_voicePitch);
	PainSound();
}

void CNPC::DeclineFollowing()
{
	Talk(10);
	m_hTalkTarget = m_hEnemy;
	SayNamedSentence("NPC_POK", 1, ATTN_NORM, 0, GetVoicePitch());
}

void CNPC::Scream()
{
	if (FOkToSpeak())
	{
		Talk(10);
		m_hTalkTarget = m_hEnemy;
		SayNamedSentence("NPC_SCREAM", 1, ATTN_NORM, 0, GetVoicePitch());
	}
}

BOOL CNPC::CanHeal()
{
	if (m_healTime > gpGlobals->time || m_hTargetEnt == NULL)
		return FALSE;
	return !(m_hTargetEnt->pev->health > 0.5 * m_hTargetEnt->pev->max_health);
}

void CNPC::Heal()
{
	if (m_healTime <= gpGlobals->time && m_hTargetEnt != NULL &&
		m_hTargetEnt->pev->health <= 0.5 * m_hTargetEnt->pev->max_health &&
		(m_hTargetEnt->pev->origin - pev->origin).Length() <= 100)
	{
		m_hTargetEnt->TakeHealth(gSkillData.scientistHeal, DMG_GENERIC);
		m_healTime = gpGlobals->time + 60;
	}
}

void CNPC::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case SCIENTIST_AE_HEAL: Heal(); break;
	case SCIENTIST_AE_NEEDLEON: pev->body = pev->body % 4 + 4; break;
	case SCIENTIST_AE_NEEDLEOFF: pev->body %= 4; break;
	default: CTalkMonster::HandleAnimEvent(pEvent); break;
	}
}

void CNPC::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SAY_HEAL:
		Talk(2);
		m_hTalkTarget = m_hTargetEnt;
		SayNamedSentence("NPC_HEAL", 1, ATTN_NORM, 0, GetVoicePitch());
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_SAY_FEAR:
		if (FOkToSpeak())
		{
			Talk(2);
			m_hTalkTarget = m_hEnemy;
			SayNamedSentence(m_hEnemy->IsPlayer() ? "NPC_PLFEAR" : "NPC_FEAR", 1, ATTN_NORM, 0, GetVoicePitch());
		}
		TaskComplete();
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if (RANDOM_FLOAT(0, 1) < pTask->flData)
			Scream();
		TaskComplete();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		if ((m_hTargetEnt->pev->origin - pev->origin).Length() < 1)
			TaskComplete();
		else
		{
			m_vecMoveGoal = m_hTargetEnt->pev->origin;
			Activity activity = LookupActivity(ACT_WALK_SCARED) == -1 ? ACT_WALK : ACT_WALK_SCARED;
			if (!MoveToTarget(activity, 0.5))
				TaskFail();
		}
		break;
	case TASK_WANDER:
		if (!MoveToLocation(ACT_WALK, 0, m_wanderOrigin))
			TaskFail();
		TaskComplete();
		break;
	default:
		CTalkMonster::StartTask(pTask);
		break;
	}
}

void CNPC::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RUN_PATH_SCARED:
		if (MovementIsComplete())
			TaskComplete();
		if (RANDOM_LONG(0, 31) < 8)
			Scream();
		break;
	case TASK_HEAL:
		if (m_fSequenceFinished)
			TaskComplete();
		else
		{
			if (TargetDistance() > 90)
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw(m_hTargetEnt->pev->origin - pev->origin);
			ChangeYaw((int)pev->yaw_speed);
		}
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if (RANDOM_LONG(0, 63) < 8)
				Scream();
			if (m_hEnemy == NULL)
			{
				TaskFail();
				return;
			}
			float distance = (m_vecMoveGoal - pev->origin).Length2D();
			if (distance < pTask->flData || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5)
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				distance = (m_vecMoveGoal - pev->origin).Length2D();
				FRefreshRoute();
			}
			if (distance < pTask->flData)
			{
				TaskComplete();
				RouteClear();
			}
			else if (distance < 190 && m_movementActivity != ACT_WALK_SCARED)
				m_movementActivity = ACT_WALK_SCARED;
			else if (distance >= 270 && m_movementActivity != ACT_RUN_SCARED)
				m_movementActivity = ACT_RUN_SCARED;
		}
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}

Schedule_t *CNPC::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_IDLE_STAND:
		{
			Schedule_t *pSchedule = CTalkMonster::GetScheduleOfType(Type);
			return pSchedule == slIdleStand ? slIdleSciStand : pSchedule;
		}
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		{
			Schedule_t *pSchedule = CTalkMonster::GetScheduleOfType(Type);
			return pSchedule == slIdleStand ? slFaceTarget : pSchedule;
		}
	case SCHED_TARGET_CHASE: return slFollow;
	case SCHED_WANDER: return slWander;
	case SCHED_CANT_FOLLOW: return slStopFollowing;
	case SCHED_HIDE: return slNPCHide;
	case SCHED_FEAR: return slFear;
	case SCHED_PANIC: return slSciPanic;
	case SCHED_STARTLE: return slNPCStartle;
	case SCHED_TARGET_CHASE_SCARED: return slFollowScared;
	case SCHED_TARGET_FACE_SCARED: return slFaceTargetScared;
	default: return CTalkMonster::GetScheduleOfType(Type);
	}
}

Schedule_t *CNPC::GetSchedule()
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound *pSound = PBestSound();
		if (pSound && (pSound->m_iType & (m_fleeOnDanger ? bits_SOUND_COMBAT | bits_SOUND_DANGER : bits_SOUND_DANGER)))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if (pEnemy)
		{
			if (HasConditions(bits_COND_SEE_ENEMY))
				m_fearTime = gpGlobals->time;
			else if (DisregardEnemy(pEnemy))		// After 15 seconds of being hidden, return to alert
			{
				pEnemy = NULL;
				m_hEnemy = NULL;
			}
		}
		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		if (HasConditions(bits_COND_HEAR_SOUND) && m_canCower)
		{
			CSound *pSound = PBestSound();
			if (pSound && (pSound->m_iType & (bits_SOUND_COMBAT | bits_SOUND_DANGER)) && gpGlobals->time - m_fearTime > 3)
			{
				m_fearTime = gpGlobals->time;		// Update last fear
				return GetScheduleOfType(SCHED_STARTLE);	// This will just duck for a second
			}
		}

		// Behavior for following the player
		if (IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
				StopFollowing(FALSE);
			else
			{
				if (pEnemy)
				{
					int relationship = IRelationship(pEnemy);
					if (relationship == R_DL || relationship == R_HT)
						return GetScheduleOfType(HasConditions(bits_COND_NEW_ENEMY) ? SCHED_FEAR : SCHED_TARGET_FACE_SCARED);
				}
				// If I'm already close enough to my target
				if (TargetDistance() <= 128)
				{
					if (m_healTime <= gpGlobals->time && m_hTargetEnt != NULL && m_hTargetEnt->pev->health <= 0.5 * m_hTargetEnt->pev->max_health)
						return slHeal;
					if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
						return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);	// Just face and follow.
			}
		}
		else
		{
			if (HasConditions(bits_COND_CLIENT_PUSH))
				return GetScheduleOfType(SCHED_MOVE_AWAY);
			if (m_hEnemy == NULL && gpGlobals->time > m_wanderTime && m_canWander && !m_cantMove)
			{
				float distance = RANDOM_FLOAT(64, 256);
				m_vecMoveGoal.x += RANDOM_LONG((int)-distance, (int)distance);
				m_vecMoveGoal.y += RANDOM_LONG((int)-distance, (int)distance);
				m_vecMoveGoal.z = pev->origin.z;
				m_wanderOrigin = m_vecMoveGoal;
				m_wanderTime = gpGlobals->time + RANDOM_FLOAT(8, 15);
				return GetScheduleOfType(SCHED_WANDER);
			}
		}
		break;
	case MONSTERSTATE_COMBAT:
		if (HasConditions(bits_COND_NEW_ENEMY))
			return slFear;					// Point and scream!
		if (!HasConditions(bits_COND_SEE_ENEMY) && HasConditions(bits_COND_HEAR_SOUND))
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!
		return slNPCCover;
	default:
		break;
	}
	return CBaseMonster::GetSchedule();
}

MONSTERSTATE CNPC::GetIdealState()
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if (HasConditions(bits_COND_NEW_ENEMY) && IsFollowing())
		{
			if (IRelationship(m_hEnemy) == R_FR && HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
				StopFollowing(TRUE);
			else
				return m_IdealMonsterState = MONSTERSTATE_ALERT;
		}
		else if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) && IsFollowing())
			StopFollowing(TRUE);
		break;
	case MONSTERSTATE_COMBAT:
		if (m_hEnemy == NULL)
			break;
		if (m_hEnemy != NULL && DisregardEnemy(m_hEnemy))
		{
			m_hEnemy = NULL;
			return m_IdealMonsterState = MONSTERSTATE_ALERT;
		}
		// Follow if only scared a little
		if (m_hTargetEnt != NULL)
			return m_IdealMonsterState = MONSTERSTATE_ALERT;
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			m_fearTime = gpGlobals->time;
			return m_IdealMonsterState = MONSTERSTATE_COMBAT;
		}
		break;
	default:
		break;
	}
	return CBaseMonster::GetIdealState();
}

class CDeadNPC : public CBaseMonster
{
public:
	void Spawn();
	void KeyValue(KeyValueData *pkvd);
	int Classify() { return CLASS_HUMAN_PASSIVE; }
	int m_iPose;
	int m_deadAnim;
	static TYPEDESCRIPTION m_SaveData[];
	static const char *m_szPoses[7];
	int Save(CSave &save);
	int Restore(CRestore &restore);
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(monster_npc_dead, CDeadNPC);

const char *CDeadNPC::m_szPoses[7] =
{
	"lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3"
};

TYPEDESCRIPTION CDeadNPC::m_SaveData[] =
{
	DEFINE_FIELD(CDeadNPC, m_iPose, FIELD_INTEGER),
	DEFINE_FIELD(CDeadNPC, m_deadAnim, FIELD_STRING)
};
IMPLEMENT_SAVERESTORE(CDeadNPC, CBaseMonster);

void CDeadNPC::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "deadanim"))
	{
		m_deadAnim = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

void CDeadNPC::Spawn()
{
	PRECACHE_MODEL((char *)(pev->model ? STRING(pev->model) : "models/npc-scientist.mdl"));
	SET_MODEL(ENT(pev), pev->model ? STRING(pev->model) : "models/npc-scientist.mdl");
	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8; //gSkillData.scientistHealth;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence(m_deadAnim ? STRING(m_deadAnim) : m_szPoses[m_iPose]);
	if (pev->sequence == -1)
		ALERT(at_console, "Dead NPC with bad pose\n");

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

class CSittingNPC : public CNPC
{
public:
	void Spawn();
	void Precache();
	void EXPORT SittingThink();
	int Classify() { return CLASS_HUMAN_PASSIVE; }
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
	void SetAnswerQuestion(CTalkMonster *pSpeaker);
	int FriendNumber(int arrayNumber);
	int FIdleSpeak();
	int m_baseSequence;
	int m_headTurn;
	float m_flResponseDelay;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(monster_sitting_npc, CSittingNPC);

TYPEDESCRIPTION CSittingNPC::m_SaveData[] =
{
	DEFINE_FIELD(CSittingNPC, m_headTurn, FIELD_INTEGER),
	DEFINE_FIELD(CSittingNPC, m_flResponseDelay, FIELD_FLOAT)
};
IMPLEMENT_SAVERESTORE(CSittingNPC, CNPC);

enum SITTING_ANIM
{
	SITTING_ANIM_sitlookleft,
	SITTING_ANIM_sitlookright,
	SITTING_ANIM_sitscared,
	SITTING_ANIM_sitting2,
	SITTING_ANIM_sitting3
};

void CSittingNPC::Spawn()
{
	PRECACHE_MODEL("models/npc-scientist.mdl");
	SET_MODEL(ENT(pev), "models/npc-scientist.mdl");
	Precache();
	InitBoneControllers();
	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 75;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	pev->spawnflags |= SF_MONSTER_PREDISASTER;
	if (pev->body == -1)
		pev->body = RANDOM_LONG(0, 3);
	if (pev->body == 2)
		pev->skin = 1;
	m_baseSequence = LookupSequence("sitlookleft");
	pev->sequence = m_baseSequence + RANDOM_LONG(0, 4);
	ResetSequenceInfo();
	SetThink(&CSittingNPC::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;
	DROP_TO_FLOOR(ENT(pev));
}

void CSittingNPC::Precache()
{
	m_baseSequence = LookupSequence("sitlookleft");
	CNPC::TalkInit();
}

int CSittingNPC::FriendNumber(int arrayNumber)
{
	static int array[3] = { 2, 1, 0 };
	return arrayNumber < 3 ? array[arrayNumber] : arrayNumber;
}

void CSittingNPC::SetAnswerQuestion(CTalkMonster *pSpeaker)
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = pSpeaker;
}

int CSittingNPC::FIdleSpeak()
{
	if (!FOkToSpeak())
		return FALSE;
	g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
	GetVoicePitch();
	CBaseEntity *pFriend = FindNearestFriend(FALSE);
	if (pFriend && RANDOM_LONG(0, 1))
	{
		CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pFriend->pev);
		pTalkMonster->SetAnswerQuestion(this);
		IdleHeadTurn(pFriend->pev->origin);
		g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}
	if (RANDOM_LONG(0, 1))
	{
		g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}
	g_talkWaitTime = 0;
	return FALSE;
}

void CSittingNPC::SittingThink()
{
	StudioFrameAdvance();

	// try to greet player
	if (FIdleHello())
	{
		CBaseEntity *pFriend = FindNearestFriend(TRUE);
		if (pFriend)
		{
			float yaw = VecToYaw(pFriend->pev->origin - pev->origin) - pev->angles.y;
			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
			pev->sequence = m_baseSequence + (yaw > 0 ? SITTING_ANIM_sitlookleft : SITTING_ANIM_sitlookright);
			ResetSequenceInfo();
			pev->frame = 0;
			SetBoneController(0, 0);
		}
	}
	else if (m_fSequenceFinished)
	{
		int choice = RANDOM_LONG(0, 99);
		m_headTurn = 0;
		if (m_flResponseDelay && gpGlobals->time > m_flResponseDelay)
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if (choice < 30)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			CBaseEntity *pFriend = FindNearestFriend((m_bitsSaid & bit_saidHelloPlayer) == 0);
			if (FIdleSpeak() && pFriend)
			{
				// only turn head if we spoke
				float yaw = VecToYaw(pFriend->pev->origin - pev->origin) - pev->angles.y;
				if (yaw > 180) yaw -= 360;
				if (yaw < -180) yaw += 360;
				pev->sequence = m_baseSequence + (yaw > 0 ? SITTING_ANIM_sitlookleft : SITTING_ANIM_sitlookright);
			}
			else
			{
				m_headTurn = RANDOM_LONG(0, 8) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
		}
		else if (choice < 60)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			m_headTurn = RANDOM_LONG(0, 8) * 10 - 40;
			if (RANDOM_LONG(0, 99) < 5)
				FIdleSpeak();
		}
		else if (choice < 80)
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		else if (choice < 100)
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController(0, (float)m_headTurn);
	}
	pev->nextthink = gpGlobals->time + 0.1;
}
