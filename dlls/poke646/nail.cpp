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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"nail.h"

LINK_ENTITY_TO_CLASS( nailgun_nail, CNailGunNail )

CNailGunNail *CNailGunNail::NailCreate( BOOL bIsBradnailer )
{
	// Create a new entity with CCrossbowBolt private data
	CNailGunNail *pNail = GetClassPtr( (CNailGunNail *)NULL );
	pNail->pev->classname = MAKE_STRING( "nailgun_nail" );
	pNail->m_bIsBradnailer = bIsBradnailer;
	pNail->Spawn();

	return pNail;
}

void CNailGunNail::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5f;

	SET_MODEL( ENT( pev ), "models/nail.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );

	SetTouch( &CNailGunNail::NailTouch );
	SetThink( &CNailGunNail::BubbleThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}


void CNailGunNail::Precache()
{
	PRECACHE_MODEL( "models/nail.mdl" );
	PRECACHE_SOUND( "weapons/brad_hit1.wav" );
	PRECACHE_SOUND( "weapons/brad_hit2.wav" );
}

int CNailGunNail::Classify()
{
	return	CLASS_NONE;
}

void CNailGunNail::NailTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t *pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		pOther->TraceAttack( pevOwner, m_bIsBradnailer ? gSkillData.plrDmg9MM : gSkillData.plrDmgMP5, pev->velocity.Normalize(), &tr, DMG_NEVERGIB );

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = g_vecZero;

		// play body "thwack" sound
		switch( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/brad_hit1.wav", 1, ATTN_NORM );
			break;
		case 1:
			if( m_bIsBradnailer )
				EMIT_SOUND( ENT( pev ), CHAN_BODY, "weapons/brad_hit2.wav", 1, ATTN_NORM );
			break;
		}

		Killed( pev, GIB_NEVER );
	}
	else
	{
		// EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/brad_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time; // this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * RANDOM_LONG( 6, 10 ) );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = g_vecZero;
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 2.0f;
		}

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER && RANDOM_LONG( 0, 4 ) == 4 )
		{
			UTIL_Sparks( pev->origin );
		}
	}
}

void CNailGunNail::BubbleThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;

	if( pev->waterlevel == 0 )
		return;
                        
	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1f, pev->origin, 1 );
}
