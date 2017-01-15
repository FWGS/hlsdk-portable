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

#ifndef OSPREY_H
#define OSPREY_H

#define OSPREY_MAX_CARRY	24

class COsprey : public CBaseMonster
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
	int		ObjectCaps(void) { return CBaseMonster::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual void Spawn(void);
	virtual void Precache(void);
	int  Classify(void) { return CLASS_MACHINE; };
	int  BloodColor(void) { return DONT_BLEED; }
	void Killed(entvars_t *pevAttacker, int iGib);

	void UpdateGoal(void);
	BOOL HasDead(void);
	void EXPORT FlyThink(void);
	void EXPORT DeployThink(void);
	void Flight(void);
	void EXPORT HitTouch(CBaseEntity *pOther);
	virtual void EXPORT FindAllThink(void);
	void EXPORT HoverThink(void);
	virtual CBaseMonster *MakeGrunt(Vector vecSrc);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT CommandUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	// int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void ShowDamage(void);

	CBaseEntity *m_pGoalEnt;
	Vector m_vel1;
	Vector m_vel2;
	Vector m_pos1;
	Vector m_pos2;
	Vector m_ang1;
	Vector m_ang2;
	float m_startTime;
	float m_dTime;

	Vector m_velocity;

	float m_flIdealtilt;
	float m_flRotortilt;

	float m_flRightHealth;
	float m_flLeftHealth;

	int	m_iUnits;
	EHANDLE m_hGrunt[OSPREY_MAX_CARRY];
	Vector m_vecOrigin[OSPREY_MAX_CARRY];
	EHANDLE m_hRepel[4];

	int m_iSoundState;
	int m_iSpriteTexture;

	int m_iPitch;

	int m_iExplode;
	int	m_iTailGibs;
	int	m_iBodyGibs;
	int	m_iEngineGibs;

	int m_iDoLeftSmokePuff;
	int m_iDoRightSmokePuff;
};

#endif // OSPREY_H