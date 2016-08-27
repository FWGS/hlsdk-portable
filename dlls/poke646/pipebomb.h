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

#ifndef PIPEBOMBGRENADE_H
#define PIPEBOMBGRENADE_H

#ifdef _WIN32
#pragma once
#endif

class CPipeBombGrenade : public CGrenade
{
public:

	void Spawn(void);
	void Precache(void);
	void BounceSound(void);

	void EXPORT BombSlide(CBaseEntity *pOther);
	void EXPORT BombThink(void);
	void EXPORT PickupTouch(CBaseEntity* pOther);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];


	EHANDLE		m_hOwner;

public:
	void Deactivate(void);
};

#endif // PIPEBOMBGRENADE_H