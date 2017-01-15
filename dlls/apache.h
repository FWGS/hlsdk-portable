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

#ifndef APACHE_H
#define APACHE_H


class CApache : public CBaseMonster
{
public:
	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	virtual void Spawn(void);
	virtual void Precache(void);
	int  Classify(void) { return CLASS_HUMAN_MILITARY; };
	int  BloodColor(void) { return DONT_BLEED; }
	void Killed(entvars_t *pevAttacker, int iGib);
	void GibMonster(void);

	void SetObjectCollisionBox(void)
	{
		pev->absmin = pev->origin + Vector(-300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	void EXPORT HuntThink(void);
	void EXPORT FlyTouch(CBaseEntity *pOther);
	void EXPORT CrashTouch(CBaseEntity *pOther);
	void EXPORT DyingThink(void);
	void EXPORT StartupUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT NullThink(void);

	void ShowDamage(void);
	void Flight(void);
	void FireRocket(void);
	BOOL FireGun(void);

	int  TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

protected:
	int m_iRockets;
	float m_flForce;
	float m_flNextRocket;

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	Vector m_angGun;
	float m_flLastSeen;
	float m_flPrevSeen;

	int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed;

	int m_iDoSmokePuff;
	CBeam *m_pBeam;
};


#endif // APACHE_H