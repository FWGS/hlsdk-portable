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

#ifndef SCIENTIST_H
#define SCIENTIST_H

#define		NUM_SCIENTIST_HEADS		4 // four heads available for scientist model

enum { HEAD_GLASSES = 0, HEAD_EINSTEIN = 1, HEAD_LUTHER = 2, HEAD_SLICK = 3 };

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

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);

	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void RunTask(Task_t *pTask);
	virtual void StartTask(Task_t *pTask);
	int	ObjectCaps(void) { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int FriendNumber(int arrayNumber);
	void SetActivity(Activity newActivity);
	Activity GetStoppedActivity(void);
	int ISoundMask(void);
	virtual void DeclineFollowing(void);

	float	CoverRadius(void) { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy(CBaseEntity *pEnemy) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal(void);
	void	Heal(void);
	virtual void	Scream(void);

	// Override these to set behavior
	Schedule_t *GetScheduleOfType(int Type);
	Schedule_t *GetSchedule(void);
	MONSTERSTATE GetIdealState(void);

	void DeathSound(void);
	virtual void PainSound(void);

	virtual void TalkInit(void);

	void			Killed(entvars_t *pevAttacker, int iGib);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

protected:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	virtual void Spawn(void);
	int	Classify(void) { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue(KeyValueData *pkvd);
	int	m_iPose;// which sequence to display
	static char *m_szPoses[7];
};



//=========================================================
// Sitting Scientist PROP
//=========================================================

class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	virtual void Spawn(void);
	virtual void  Precache(void);

	void EXPORT SittingThink(void);
	int	Classify(void);
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void SetAnswerQuestion(CTalkMonster *pSpeaker);
	int FriendNumber(int arrayNumber);

	int FIdleSpeak(void);
	int		m_baseSequence;
	int		m_headTurn;
	float	m_flResponseDelay;
};

#endif // SCIENTIST_H