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

#ifndef FLAME_H
#define FLAME_H

//=========================================================
// Flamethrower flame entity
//=========================================================
class CFlame : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

#endif // FLAME_H