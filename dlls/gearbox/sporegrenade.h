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

#ifndef SPORE_GRENADE_H
#define SPORE_GRENADE_H

// Contact/Timed spore grenade
class CSpore : public CGrenade
{
public:
	enum SporeType
	{
		ROCKET = 1,
		GRENADE = 2
	};

public:
#ifndef CLIENT_DLL
	int Save(CSave& save);
	int Restore(CRestore& restore);

	static TYPEDESCRIPTION m_SaveData[];
#endif

	void Precache();
	void Spawn();
	void UpdateOnRemove();

	void BounceSound();

	void EXPORT IgniteThink();
	void EXPORT FlyThink();
	void EXPORT GibThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);
	void EXPORT MyBounceTouch(CBaseEntity* pOther);

	static CSpore* CreateSpore(
		const Vector& vecOrigin, const Vector& vecAngles, CBaseEntity* pOwner,
		SporeType sporeType, bool bIsAI, bool bPuked);

private:
	int m_iBlow;
	int m_iBlowSmall;

	int m_iSpitSprite;
	int m_iTrail;

	SporeType m_SporeType;

	float m_flIgniteTime;
	float m_flSoundDelay;

	BOOL m_bIsAI;
	EHANDLE m_hSprite;
	BOOL m_bPuked;
};

#endif // SPORE_GRENADE_H
