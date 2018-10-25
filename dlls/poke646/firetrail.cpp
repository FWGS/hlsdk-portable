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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"
#include "effects.h"

// Fire Trail
class CFireTrail : public CBaseEntity
{
public:
	void Spawn(void);
	void Think(void);
	void Touch(CBaseEntity *pOther);
	int ObjectCaps(void) { return FCAP_DONT_SAVE; }

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];
private:
	int m_spriteScale; // what's the exact fireball sprite scale?
};

LINK_ENTITY_TO_CLASS( fire_trail, CFireTrail );

TYPEDESCRIPTION CFireTrail::m_SaveData[] =
{
        DEFINE_FIELD( CFireTrail, m_spriteScale, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CFireTrail, CBaseEntity )

void CFireTrail::Spawn( void )
{
	pev->velocity = RANDOM_FLOAT(100.0f, 150.0f) * pev->angles;
	if( RANDOM_LONG( 0, 1 ) )
		pev->velocity.x += RANDOM_FLOAT(-300.f, -100.f);
	else
		pev->velocity.x += RANDOM_FLOAT(100.f, 300.f);

	if( RANDOM_LONG( 0, 1 ) )
		pev->velocity.y += RANDOM_FLOAT(-300.f, -100.f);
	else
		pev->velocity.y += RANDOM_FLOAT(100.f, 300.f);

	if( pev->velocity.z >= 0 )
		pev->velocity.z += 200;
	else
		pev->velocity.z -= 200;

	m_spriteScale = RANDOM_LONG( 7, 13 );
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL(edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT(0.5, 1.5);
	pev->maxspeed = pev->speed;

	pev->angles = g_vecZero;
}

void CFireTrail::Think(void)
{
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION);
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( (BYTE)( m_spriteScale * pev->speed ) ); // scale * 10
		WRITE_BYTE( 15 ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
	MESSAGE_END();

	pev->speed -= 0.1;
	if (pev->speed > 0)
		pev->nextthink = gpGlobals->time + 0.1;
	else
		UTIL_Remove(this);

	pev->flags &= ~FL_ONGROUND;
}

void CFireTrail::Touch(CBaseEntity *pOther)
{
	if (pev->flags & FL_ONGROUND)
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ((pev->velocity.x*pev->velocity.x + pev->velocity.y*pev->velocity.y) < 10.0)
		pev->speed = 0;
}
