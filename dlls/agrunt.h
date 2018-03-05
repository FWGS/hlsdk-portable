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
****/
//=========================================================
// Agrunt - Dominant, warlike alien grunt monster
//=========================================================
#ifndef AGRUNT_H
#define AGRUNT_H

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"

class CAGrunt : public CSquadMonster
{
public:
	virtual void Spawn();
	virtual void Precache();
	void SetYawSpeed();
	int Classify();
	int ISoundMask();
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetObjectCollisionBox()
	{
		pev->absmin = pev->origin + Vector( -32, -32, 0 );
		pev->absmax = pev->origin + Vector( 32, 32, 85 );
	}

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType( int Type );
	BOOL FCanCheckAttacks();
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	void StartTask( Task_t *pTask );
	virtual void AlertSound();
	virtual void DeathSound();
	virtual void PainSound();
	virtual void AttackSound();
	void PrescheduleThink();
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int IRelationship( CBaseEntity *pTarget );
	void StopTalking();
	BOOL ShouldSpeak();
	CUSTOM_SCHEDULES

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pAttackSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];

	string_t szCustomAttackSounds[3];
        string_t szCustomIdleSounds[4];
        string_t szCustomAlertSounds[4];
        string_t szCustomPainSounds[5];
	string_t szCustomDieSounds[3];

	BOOL m_fCanHornetAttack;
	float m_flNextHornetAttackCheck;

	float m_flNextPainTime;

	// three hacky fields for speech stuff. These don't really need to be saved.
	float m_flNextSpeakTime;
	float m_flNextWordTime;
	int m_iLastWord;
};
#endif // AGRUNT_H
