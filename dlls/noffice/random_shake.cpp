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

#define RANDOM_SHAKE_REFIRE_MIN	8
#define RANDOM_SHAKE_REFIRE_MAX	15

enum
{
	SHAKE_TYPE_SMALL = 0,
	SHAKE_TYPE_MEDIUM,
	SHAKE_TYPE_HUGE,

	SHAKE_TYPE_COUNT,
};

// ==========================================
// Code changes for- Night at the Office:
// ==========================================
//
// -Entity: env_random_shake. An entity that randomly shakes
//  the screen globally across the map, includes sounds effects.
//  Purpose is to allow multiple shakes without the need of many
//  entitys and triggers. The intensity of the shake is random,
//  the duration is random and the delay between fires is also
//  random.

class CRandomShake : public CShake
{
public:
	void	Spawn(void);
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	void	EXPORT RandomThink(void);

	virtual int		ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS(env_random_shake, CRandomShake);

void CRandomShake::Spawn(void)
{
	CShake::Spawn();

	SetThink(NULL);
}

void CRandomShake::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(&CRandomShake::RandomThink);

	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(RANDOM_SHAKE_REFIRE_MIN, RANDOM_SHAKE_REFIRE_MAX);
}

void CRandomShake::RandomThink(void)
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(RANDOM_SHAKE_REFIRE_MIN, RANDOM_SHAKE_REFIRE_MAX);

	int amplitude, frequency, duration;

	switch (RANDOM_LONG(0, SHAKE_TYPE_COUNT - 1))
	{
	default:
	case SHAKE_TYPE_SMALL:
		amplitude = RANDOM_LONG(2, 3);
		frequency = RANDOM_LONG(20, 40);
		duration = RANDOM_FLOAT(2, 4);
		break;

	case SHAKE_TYPE_MEDIUM:
		amplitude = RANDOM_LONG(3, 5);
		frequency = RANDOM_LONG(40, 80);
		duration = RANDOM_FLOAT(4, 6);
		break;

	case SHAKE_TYPE_HUGE:
		amplitude = RANDOM_LONG(5, 10);
		frequency = RANDOM_LONG(80, 100);
		duration  = RANDOM_FLOAT(6, 8);
		break;
	}

	SetAmplitude(amplitude);
	SetFrequency(frequency);
	SetDuration(duration);

	UTIL_ScreenShake(pev->origin, Amplitude(), Frequency(), Duration(), 0);
}