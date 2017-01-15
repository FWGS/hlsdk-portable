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

#ifndef FUNC_TANK_H
#define FUNC_TANK_H

//=========================================================
// CFuncTank
//=========================================================

#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_PLAYER			0x0002
#define SF_TANK_HUMANS			0x0004
#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_SOUNDON			0x8000


enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3,
};


//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{
public:
	void	Spawn(void);
	void	Precache(void);
	void	KeyValue(KeyValueData *pkvd);
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void	Think(void);
	void	TrackTarget(void);

	virtual void Fire(const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker);
	virtual Vector UpdateTargetPosition(CBaseEntity *pTarget)
	{
		return pTarget->BodyTarget(pev->origin);
	}

	void	StartRotSound(void);
	void	StopRotSound(void);

	// Bmodels don't go across transitions
	virtual int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline BOOL IsActive(void) { return (pev->spawnflags & SF_TANK_ACTIVE) ? TRUE : FALSE; }
	inline void TankActivate(void) { pev->spawnflags |= SF_TANK_ACTIVE; pev->nextthink = pev->ltime + 0.1; m_fireLast = 0; }
	inline void TankDeactivate(void) { pev->spawnflags &= ~SF_TANK_ACTIVE; m_fireLast = 0; StopRotSound(); }
	inline BOOL CanFire(void) { return (gpGlobals->time - m_lastSightTime) < m_persist; }
	BOOL		InRange(float range);

	// Acquire a target.  pPlayer is a player in the PVS
	edict_t		*FindTarget(edict_t *pPlayer);

	void		TankTrace(const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr);

	Vector		BarrelPosition(void)
	{
		Vector forward, right, up;
		UTIL_MakeVectorsPrivate(pev->angles, forward, right, up);
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	void		AdjustAnglesForBarrel(Vector &angles, float distance);

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL OnControls(entvars_t *pevTest);
	BOOL StartControl(CBasePlayer* pController);
	void StopControl(void);
	void ControllerPostFrame(void);


protected:
	CBasePlayer* m_pController;
	float		m_flNextAttack;
	Vector		m_vecControllerUsePos;

	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
	// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	int			m_iszSpriteSmoke;
	int			m_iszSpriteFlash;
	TANKBULLET	m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage

	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	int			m_iszMaster;	// Master entity (game_team_master or multisource)
};

//=========================================================
// CFuncTankGun
//=========================================================
class CFuncTankGun : public CFuncTank
{
public:
	void Fire(const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker);
};

//=========================================================
// CFuncTankLaser
//=========================================================

class CFuncTankLaser : public CFuncTank
{
public:
	void	Activate(void);
	void	KeyValue(KeyValueData *pkvd);
	void	Fire(const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker);
	void	Think(void);
	CLaser *GetLaser(void);

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	CLaser	*m_pLaser;
	float	m_laserTime;
};

//=========================================================
// CFuncTankRocket
//=========================================================

class CFuncTankRocket : public CFuncTank
{
public:
	void Precache(void);
	void Fire(const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker);
};

//=========================================================
// CFuncTankMortar
//=========================================================

class CFuncTankMortar : public CFuncTank
{
public:
	void KeyValue(KeyValueData *pkvd);
	void Fire(const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker);
};

//============================================================================
// FUNC TANK CONTROLS
//============================================================================
class CFuncTankControls : public CBaseEntity
{
public:
	virtual int	ObjectCaps(void);
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Think(void);

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	CFuncTank *m_pTank;
};

#endif // FUNC_TANK_H