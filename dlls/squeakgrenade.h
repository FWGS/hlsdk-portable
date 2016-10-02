/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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

#ifndef SQUEAKGRENADE_H
#define SQUEAKGRENADE_H

class CSqueakGrenade : public CGrenade
{
public:
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT SuperBounceTouch(CBaseEntity *pOther);
	void EXPORT HuntThink(void);
	int  BloodColor(void) { return BLOOD_COLOR_YELLOW; }
	void Killed(entvars_t *pevAttacker, int iGib);
	void GibMonster(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int  m_iMyClass;
};
#endif // SQUEAKGRENADE_H
