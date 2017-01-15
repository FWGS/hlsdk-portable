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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#ifndef HEADCRAB_H
#define HEADCRAB_H

class CHeadCrab : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void RunTask(Task_t *pTask);
	void StartTask(Task_t *pTask);
	void SetYawSpeed(void);
	virtual void EXPORT LeapTouch(CBaseEntity *pOther);
	Vector Center(void);
	Vector BodyTarget(const Vector &posSrc);
	virtual void PainSound(void);
	virtual void DeathSound(void);
	virtual void IdleSound(void);
	virtual void AlertSound(void);
	virtual void PrescheduleThink(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);

	virtual float GetDamageAmount(void) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch(void) { return 100; }
	virtual float GetSoundVolue(void) { return 1.0; }
	Schedule_t* GetScheduleOfType(int Type);

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];
};

#endif