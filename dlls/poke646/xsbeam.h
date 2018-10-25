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
#pragma once
#ifndef XSBEAM_H
#define XSBEAM_H

#define XENSQUASHER_MAX_BEAMS	4

class CXSBeam : public CBaseEntity
{
public:
	void Spawn();
	void Precache();

	static CXSBeam* CXSBeamCreate( float flDamage );
	void Init();
	void EXPORT BeamTouch(CBaseEntity *pOther);
	void EXPORT FlyThink();
	void EXPORT RemoveThink();

	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iTrail;
	int m_iBeamCount;
	float m_flDmg;
	float m_flDeflectionDist;
	float m_flDeflectionDot[XENSQUASHER_MAX_BEAMS];
	Vector m_vecOldOrigin;
	CSprite *m_pBeam[XENSQUASHER_MAX_BEAMS];
};
#endif // XSBEAM_H
