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

#ifndef ZOMBIE_H
#define ZOMBIE_H

//
// Zombie Flags
//
#define ZF_FEMALE			1
#define ZF_NURSE			2
#define ZF_COP				4
#define ZF_NEWSOUNDS		8

class CZombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	void	RunAI(void);

	BOOL	IsFemale() const;
	BOOL	IsNurse() const;
	BOOL	IsCop() const;
	BOOL	UseNewSounds() const;

	static const char *pCopAttackSounds[];
	static const char *pCopIdleSounds[];
	static const char *pCopAlertSounds[];
	static const char *pCopPainSounds[];

	static const char *pFemaleAttackSounds[];
	static const char *pFemaleIdleSounds[];
	static const char *pFemaleAlertSounds[];
	static const char *pFemalePainSounds[];

	static const char *pNurseAttackSounds[];
	static const char *pNurseIdleSounds[];
	static const char *pNurseAlertSounds[];
	static const char *pNursePainSounds[];

	static const char *pNewAttackSounds[];
	static const char *pNewIdleSounds[];
	static const char *pNewAlertSounds[];
	static const char *pNewPainSounds[];

	int		m_iZombieFlags;
};
#endif // ZOMBIE_H
