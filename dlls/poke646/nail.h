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

#ifndef NAIL_H
#define NAIL_H

#define NAIL_AIR_VELOCITY		2000
#define NAIL_WATER_VELOCITY		1000

//=========================================================
// Nail projectile
//=========================================================
class CNail : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT NailThink(void);
	void EXPORT NailTouch(CBaseEntity *pOther);

public:
	static CNail *NailCreate(void);
};

#endif // NAIL_H