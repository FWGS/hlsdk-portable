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
#pragma once
#ifndef GRAPPLE_H
#define GRAPPLE_H
class CGrapple : public CBaseEntity
{
public:
	//Yes, I have no imagination so I use standard touch, spawn and think function names.
	//Sue me! =P.
	void Spawn();
	int Classify() { return CLASS_PROJECTILE; };
	void EXPORT OnAirThink();
	void EXPORT GrappleTouch( CBaseEntity *pOther );
	void Reset_Grapple();
	void EXPORT Grapple_Track();

	float m_flNextIdleTime;
};
#endif // GRAPPLE_H
