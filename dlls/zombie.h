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

//=========================================================
// Zombie
//=========================================================
class CZombie : public CBaseMonster
{
public:
	virtual void Spawn(void);
	virtual void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	virtual void HandleAnimEvent(MonsterEvent_t *pEvent);
	int IgnoreConditions(void);

	float m_flNextFlinch;

	void PainSound(void);
	void AlertSound(void);
	void IdleSound(void);
	void AttackSound(void);

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1(float flDot, float flDist) { return FALSE; }
	BOOL CheckRangeAttack2(float flDot, float flDist) { return FALSE; }
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
};


#endif // ZOMBIE_H