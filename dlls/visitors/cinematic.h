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
#ifndef CINEMATIC_H
#define CINEMATIC_H

class CCinematicCamera : public CBaseEntity
{
public:
	BOOL IsCinematic();
	static CCinematicCamera *CreateCinematicCamera();
	void Spawn();
	void SetOrigin( const Vector &p_vecOrigin );
	void SetTarget( CBaseEntity *pEntity );
	void SetPlayer( CBaseEntity *pEntity );
	void SetViewOnTarget();
	void SetViewOnPlayer();

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

// private:
	EHANDLE m_hPlayer;
	EHANDLE m_hTarget;
	Vector m_vecOrigin;
	BOOL m_bIsDeathCamera;
};

#endif // CINEMATIC_H
