/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef XENSPIT_H
#define XENSPIT_H

#ifdef _WIN32
#pragma once
#endif

#define XENSPIT_MAX_PROJECTILES	4

class CXenLargeSpit : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);

	static CXenLargeSpit* Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT CycleThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CBaseEntity* m_pChildren[XENSPIT_MAX_PROJECTILES];
	int m_iChildCount;
};


class CXenSmallSpit : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache(void);

	static CXenSmallSpit* ShootStraight(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity);
	static CXenSmallSpit* ShootCycle(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity, CBaseEntity* pParent, float flCycle = 0.0f);
	void Touch(CBaseEntity *pOther);
	void EXPORT CycleThink(void);
	void EXPORT StraightThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iTrail;
	float	m_flCycle;
	Vector  m_vecOldVelocity;

	CBaseEntity* m_pParent;

	CBeam*	m_pBeam;
};



#endif // XENSPIT_H