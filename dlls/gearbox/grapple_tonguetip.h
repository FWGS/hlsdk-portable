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

#ifndef GRAPPLE_TONGUETIP_H
#define GRAPPLE_TONGUETIP_H

class CBarnacleGrappleTip : public CBaseEntity
{
public:

/*	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
*/
	int targetClass;
	void Precache();
	void Spawn();

	void FlyThink();
	void OffsetThink();

	void TongueTouch( CBaseEntity* pOther );

	int CheckTarget( CBaseEntity* pTarget );

	void SetPosition( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner );

	int GetGrappleType() const { return m_GrappleType; }

	bool IsStuck() const { return m_bIsStuck; }

	bool HasMissed() const { return m_bMissed; }
#if !CLIENT_DLL
	EHANDLE& GetGrappleTarget() { return m_hGrappleTarget; }
	void SetGrappleTarget( CBaseEntity* pTarget )
	{
		m_hGrappleTarget = pTarget;
	}
#endif
private:
	int m_GrappleType;
	bool m_bIsStuck;
	bool m_bMissed;
#if !CLIENT_DLL
	EHANDLE m_hGrappleTarget;
#endif
	Vector m_vecOriginOffset;
};

#endif // GRAPPLE_TONGUETIP_H
