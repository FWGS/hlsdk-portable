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
class CSporeGrenade : public CGrenade
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	static	TYPEDESCRIPTION m_SaveData[];

	void Precache(void);
	void Spawn(void);

	static CGrenade *ShootTimed(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	static CGrenade *ShootContact(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);

	void Explode(TraceResult *pTrace, int bitsDamageType);
	void EXPORT Smoke(void);

	void EXPORT BounceTouch(CBaseEntity *pOther); 
	void EXPORT SlideTouch(CBaseEntity *pOther);
	void EXPORT ExplodeTouch(CBaseEntity *pOther);
	void EXPORT DangerSoundThink(void);
	void EXPORT PreDetonate(void);
	void EXPORT Detonate(void);
	void EXPORT DetonateUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void EXPORT TumbleThink(void);
	void EXPORT FlyThink(void);

	virtual void BounceSound(void);
	virtual int	BloodColor(void) { return DONT_BLEED; }
	virtual void Killed(entvars_t *pevAttacker, int iGib);
	static void SpawnTrailParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);
	static void SpawnExplosionParticles(const Vector& origin, const Vector& direction, int modelindex, int count, float speed, float noise);

	CSprite* m_pSporeGlow;
	float m_flNextSpriteTrailSpawn;
};

#endif // SPORE_GRENADE_H