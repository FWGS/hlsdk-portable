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
class CEinarFlameRocket : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void EXPORT FlameTouch( CBaseEntity *pOther );
	static CEinarFlameRocket *FlameCreate( const Vector &pOrigin, const Vector &pAngles, edict_t *pevOwner );
};

#endif // FLAME_H
