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

#ifndef GMAN_H
#define GMAN_H

//=========================================================
// GMan - misunderstood servant of the people
//=========================================================
class CGMan : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	int ISoundMask(void);

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	int  TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void PlayScriptedSentence(const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener);

	EHANDLE m_hPlayer;
	EHANDLE m_hTalkTarget;
	float m_flTalkTime;
};

#endif // GMAN_H