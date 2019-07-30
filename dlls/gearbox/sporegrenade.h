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
class CSporeGrenade : public CBaseMonster
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Precache(void);
	void Spawn(void);

	static CBaseEntity *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, bool ai);
	static CBaseEntity *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);

	void Explode(TraceResult *pTrace);

	void EXPORT BounceTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT Detonate(void);
	void EXPORT TumbleThink(void);

	void BounceSound(void);
	void DangerSound();
	static void SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);
	static void SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);

	void UpdateOnRemove();

	CSprite* m_pSporeGlow;
};

#endif // SPORE_GRENADE_H
