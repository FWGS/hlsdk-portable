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
#include "effects.h"

//=========================================================
// CPitwormGibShooter
//=========================================================
class CPitwormGib : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void EXPORT GibFloat();
};

LINK_ENTITY_TO_CLASS(pitworm_gib, CPitwormGib)

void CPitwormGib::Precache()
{
	PRECACHE_MODEL("models/pit_worm_gibs.mdl");
}

void CPitwormGib::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55; // deading the bounce a bit

	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_NOT;
	pev->classname = MAKE_STRING( "pitworm_gib" );

	SET_MODEL( ENT( pev ), "models/pit_worm_gibs.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, -4 ), Vector( 8, 8, 16 ) );

	pev->nextthink = gpGlobals->time + 0.1;
	SetThink(&CPitwormGib::GibFloat);
}

void CPitwormGib::GibFloat()
{
	if (pev->waterlevel == 3)
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8;
		pev->avelocity = pev->avelocity * 0.9;
		pev->velocity.z += 8;
	}
	else if (pev->waterlevel)
	{
		pev->velocity.z -= 8;
	}
	else
	{
		pev->movetype = MOVETYPE_BOUNCE;
		pev->velocity.z -= 8;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

class CPitwormGibShooter : public CBaseDelay
{
public:
	void Spawn();
	void Precache(void);
	void KeyValue( KeyValueData *pkvd );
	void EXPORT ShootThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	CPitwormGib *CreateGib(void);

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iGibModelIndex;
	float m_flGibVelocity;
};

TYPEDESCRIPTION CPitwormGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( CPitwormGibShooter, m_flGibVelocity, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CPitwormGibShooter, CBaseDelay )

LINK_ENTITY_TO_CLASS(pitworm_gibshooter, CPitwormGibShooter)

void CPitwormGibShooter::Precache(void)
{
	m_iGibModelIndex = PRECACHE_MODEL("models/pit_worm_gibs.mdl");
	UTIL_PrecacheOther("pitworm_gib");
}

void CPitwormGibShooter::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	if( m_flDelay == 0 )
	{
		m_flDelay = 0.1;
	}

	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

void CPitwormGibShooter::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "m_flVelocity" ) )
	{
		m_flGibVelocity = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseDelay::KeyValue( pkvd );
	}
}

void CPitwormGibShooter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CPitwormGibShooter::ShootThink );
	pev->nextthink = gpGlobals->time;
}

CPitwormGib *CPitwormGibShooter::CreateGib(void)
{
	if (CVAR_GET_FLOAT("violence_hgibs") == 0)
		return NULL;

	CPitwormGib *pGib = GetClassPtr((CPitwormGib *)NULL);
	pGib->Spawn();

	if (pev->body <= 1)
	{
		ALERT(at_aiconsole, "PitwormGibShooter Body is <= 1!\n");
	}

	pGib->pev->body = RANDOM_LONG(0, pev->body - 1);

	return pGib;
}

void CPitwormGibShooter::ShootThink()
{
	UTIL_MakeVectors(pev->angles);
	CPitwormGib *pGib = CreateGib();
	if (pGib)
	{
		pGib->pev->origin = pev->origin;
		pGib->pev->velocity = gpGlobals->v_forward * m_flGibVelocity;
	}
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

#include "displacerball.h"
#include "shock.h"
#include "sporegrenade.h"

enum
{
	BLOWERCANNON_SPOREROCKET = 1,
	BLOWERCANNON_SPOREGRENADE,
	BLOWERCANNON_SHOCKBEAM,
	BLOWERCANNON_DISPLACERBALL,
};

enum
{
	BLOWERCANNON_TOGGLE = 1,
	BLOWERCANNON_FIRE,
};

class CBlowerCannon : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue(KeyValueData* pkvd);
	void EXPORT BlowerCannonThink( void );
	void EXPORT BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);

	static TYPEDESCRIPTION m_SaveData[];

	int m_iWeapType;
	float m_flDelay;
	int m_iFireType;
	int m_iZOffSet;
};

LINK_ENTITY_TO_CLASS(env_blowercannon, CBlowerCannon)

TYPEDESCRIPTION	CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD(CBlowerCannon, m_iFireType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iWeapType, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_iZOffSet, FIELD_INTEGER),
	DEFINE_FIELD(CBlowerCannon, m_flDelay, FIELD_FLOAT),
};
IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseEntity )


void CBlowerCannon::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "firetype"))
	{
		m_iFireType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "delay"))
	{
		m_flDelay = (float)atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "weaptype"))
	{
		m_iWeapType = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "zoffset"))
	{
		m_iZOffSet = (int)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CBlowerCannon::Spawn(void)
{
	Precache();
	UTIL_SetSize( pev, Vector(-16, -16, -16), Vector( 16, 16, 16 ) );
	pev->solid = SOLID_TRIGGER;
	if (m_flDelay <= 0.0f)
		m_flDelay = 1.0f;
	SetUse( &CBlowerCannon::BlowerCannonStart );
}

void CBlowerCannon::Precache( void )
{
	UTIL_PrecacheOther( "shock_beam" );
	UTIL_PrecacheOther( "displacer_ball" );
	UTIL_PrecacheOther( "spore" );
}

void CBlowerCannon::BlowerCannonStart( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStop );
	SetThink( &CBlowerCannon::BlowerCannonThink );
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CBlowerCannon::BlowerCannonStop( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStart );
	SetThink( NULL );
}

void CBlowerCannon::BlowerCannonThink( void )
{
	CBaseEntity *pTarget = GetNextTarget();

	if( pTarget )
	{
		Vector direction = pTarget->pev->origin - pev->origin;
		direction.z = m_iZOffSet + pTarget->pev->origin.z - pev->origin.z;

		Vector angles = UTIL_VecToAngles( direction );
		UTIL_MakeVectors( angles );

		switch (m_iWeapType)
		{
		case BLOWERCANNON_SPOREROCKET:
			CSporeGrenade::ShootContact(pev, pev->origin, gpGlobals->v_forward * 1500);
			break;
		case BLOWERCANNON_SPOREGRENADE:
			CSporeGrenade::ShootTimed(pev, pev->origin, gpGlobals->v_forward * 700, false);
			break;
		case BLOWERCANNON_SHOCKBEAM:
			CShock::Shoot(pev, pev->angles, pev->origin, gpGlobals->v_forward * 2000);
			break;
		case BLOWERCANNON_DISPLACERBALL:
			CDisplacerBall::Shoot(pev, pev->origin, gpGlobals->v_forward * 500, angles);
			break;
		default:
			ALERT(at_console, "Unknown projectile type in blowercannon: %d\n", m_iWeapType);
			break;
		}
	}
	if( m_iFireType == BLOWERCANNON_FIRE )
	{
		SetUse( &CBlowerCannon::BlowerCannonStart );
		SetThink( NULL );
	}

	pev->nextthink = gpGlobals->time + m_flDelay;
}
