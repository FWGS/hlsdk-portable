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
// shockroach
//=========================================================

#ifndef SHOCKROACH_H
#define SHOCKROACH_H

class CShockRoach : public CHeadCrab
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT LeapTouch(CBaseEntity *pOther);
	void PainSound(void);
	void DeathSound(void);
	void IdleSound(void);
	void AlertSound(void);
	void PrescheduleThink(void);
	void StartTask(Task_t* pTask);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pDeathSounds[];
	static const char *pBiteSounds[];

	float m_flDie;
};


#endif // SHOCKROACH_H