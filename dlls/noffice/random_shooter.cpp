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
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"
#include "decals.h"
#include "func_break.h"
#include "shake.h"

#define RANDOM_SHOOTER_INTERVAL_MIN		10
#define RANDOM_SHOOTER_INTERVAL_MAX		15

#define RANDOM_SHOOTER_CAPACITY_MIN		1
#define RANDOM_SHOOTER_CAPACITY_MAX		4

#define RANDOM_SHOOTER_VARIANCE_MIN		0.15
#define RANDOM_SHOOTER_VARIANCE_MAX		0.5

#define RANDOM_SHOOTER_VELOCITY_MIN		150
#define RANDOM_SHOOTER_VELOCITY_MAX		200

#define	SF_RANDOMSHOOTER_REPEATABLE	1 // allows a env_random_shooter to be refired

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Entity: env_random_shooter. An updated version
//  of env_shooter only this fires repeatedly in bursts,
//  so one entity can be used instead of many env_shooters.

class CRandomShooter : public CEnvShooter
{
	void Spawn(void);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void EXPORT RandomThink(void);
};

LINK_ENTITY_TO_CLASS(env_random_shooter, CRandomShooter);

void CRandomShooter::Spawn(void)
{
	CEnvShooter::Spawn();

	SetThink( NULL );
}

void CRandomShooter::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CRandomShooter::RandomThink);

	pev->nextthink = gpGlobals->time;
}

void CRandomShooter::RandomThink(void)
{
	Vector vecShootDir;

	vecShootDir = pev->movedir;

	vecShootDir = vecShootDir + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * m_flVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_forward * RANDOM_FLOAT(-1, 1) * m_flVariance;;
	vecShootDir = vecShootDir + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * m_flVariance;;

	vecShootDir = vecShootDir.Normalize();
	CGib *pGib = CreateGib();

	if (pGib)
	{
		if (FStrEq(STRING(pGib->pev->model), "models/rocks.mdl"))
		{
			pGib->pev->body = RANDOM_LONG(0, 3);
		}

		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = vecShootDir * m_flGibVelocity;

		pGib->pev->avelocity.x = RANDOM_FLOAT(100, 200);
		pGib->pev->avelocity.y = RANDOM_FLOAT(100, 300);

		float thinkTime = pGib->pev->nextthink - gpGlobals->time;

		pGib->m_lifeTime = (m_flGibLife * RANDOM_FLOAT(0.95, 1.05));	// +/- 5%
		if (pGib->m_lifeTime < thinkTime)
		{
			pGib->pev->nextthink = gpGlobals->time + pGib->m_lifeTime;
			pGib->m_lifeTime = 0;
		}
	}

	if (--m_iGibs <= 0)
	{
		m_iGibs			= RANDOM_LONG (RANDOM_SHOOTER_CAPACITY_MIN, RANDOM_SHOOTER_CAPACITY_MAX);
		m_flGibVelocity = RANDOM_FLOAT(RANDOM_SHOOTER_VELOCITY_MIN, RANDOM_SHOOTER_VELOCITY_MAX);
		m_flVariance	= RANDOM_FLOAT(RANDOM_SHOOTER_VARIANCE_MIN, RANDOM_SHOOTER_VARIANCE_MAX);

		pev->nextthink = gpGlobals->time + RANDOM_FLOAT(RANDOM_SHOOTER_INTERVAL_MIN, RANDOM_SHOOTER_INTERVAL_MAX);
	}
	else
	{
		pev->nextthink = gpGlobals->time;
	}
}