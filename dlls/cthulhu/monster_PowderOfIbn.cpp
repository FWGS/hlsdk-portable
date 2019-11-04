/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
/*

===== powder of ibn.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"

#include "monster_PowderOfIbn.h"

//===================powder of ibn


LINK_ENTITY_TO_CLASS( powderofibn, CMonsterPowderOfIbn );

//
// Powder Explode
//
void CMonsterPowderOfIbn::Explode( Vector vecSrc, Vector vecAim )
{
	TraceResult tr;
	UTIL_TraceLine ( pev->origin, pev->origin + Vector ( 0, 0, -32 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_POWDER_IBN );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CMonsterPowderOfIbn::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		//WRITE_SHORT( g_sModelIndexFireball );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 5  ); // scale * 10
		//WRITE_BYTE( (pev->dmg - 50) * .60  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( TE_EXPLFLAG_NOSOUND );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 64, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev, pevOwner, pev->dmg, CLASS_NONE, bitsDamageType );

	flRndSound = RANDOM_FLOAT( 0 , 1 );

	switch ( RANDOM_LONG( 0, 2 ) )
	{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);	break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( Smoke );
	pev->velocity = g_vecZero;
	SetNextThink( 0.3 );
}

void CMonsterPowderOfIbn::ExplodeTouch( CBaseEntity *pOther )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	pev->enemy = pOther->edict();

	vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );

	Explode( &tr, DMG_POWDER_IBN );
}

void CMonsterPowderOfIbn::Smoke( void )
{
	if (UTIL_PointContents ( pev->origin ) == CONTENTS_WATER)
	{
		UTIL_Bubbles( pev->origin - Vector( 64, 64, 64 ), pev->origin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( 8 ); // scale * 10
			//WRITE_BYTE( (pev->dmg - 50) * 0.80 ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	UTIL_Remove( this );
}

void CMonsterPowderOfIbn::Killed( entvars_t *pevAttacker, int iGib )
{
	Detonate( );
}


void CMonsterPowderOfIbn::PreDetonate( void )
{
	CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, 400, 0.3 );

	SetThink( Detonate );
	SetNextThink( 1 );
}


void CMonsterPowderOfIbn::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_POWDER_IBN );
}

void CMonsterPowderOfIbn::BounceTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this powder
	if ( pOther->edict() == pev->owner )
		return;

	Vector vecTestVelocity;
	// pev->avelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the powder velocity because powder dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = pev->velocity; 
	vecTestVelocity.z *= 0.45;

	if ( !m_fRegisteredSound && vecTestVelocity.Length() <= 60 )
	{
		//ALERT( at_console, "Powder of Ibn Registered!: %f\n", vecTestVelocity.Length() );

		// powder is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// go ahead and emit the player sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
		CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin, pev->dmg / 0.4, 0.3 );
		m_fRegisteredSound = TRUE;
	}

	if (pev->flags & FL_ONGROUND)
	{
		// add a bit of static friction
		pev->velocity = pev->velocity * 0.8;

		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.5)
		pev->framerate = 0;

}

void CMonsterPowderOfIbn :: BounceSound( void )
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/powder_hit1.wav", 0.25, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/powder_hit2.wav", 0.25, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/powder_hit3.wav", 0.25, ATTN_NORM);	break;
	}
}

void CMonsterPowderOfIbn :: TumbleThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance( );
	SetNextThink( 0.1 ); 

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		SetThink( Detonate );
	}
	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
		pev->framerate = 0.2;
	}
}


void CMonsterPowderOfIbn:: Spawn( void )
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING( "powder of ibn" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_powderofibn.mdl");
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));

	pev->dmg = 10;
	m_fRegisteredSound = FALSE;
}

void CMonsterPowderOfIbn::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	CSoundEnt::InsertSound ( bits_SOUND_PLAYER, pev->origin + pev->velocity * 0.5, pev->velocity.Length( ), 0.2 );
	SetNextThink( 0.2 );

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.5;
	}
}

CMonsterPowderOfIbn * CMonsterPowderOfIbn::ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CMonsterPowderOfIbn *pPowder = GetClassPtr( (CMonsterPowderOfIbn *)NULL );
	pPowder->Spawn();
	// powders arc lower
	pPowder->pev->gravity = 0.5;// lower gravity since powder is aerodynamic and engine doesn't know it.
	UTIL_SetOrigin( pPowder, vecStart );
	pPowder->pev->velocity = vecVelocity;
	pPowder->pev->angles = UTIL_VecToAngles (pPowder->pev->velocity);
	pPowder->pev->owner = ENT(pevOwner);
	
	// make monsters afaid of it while in the air
	pPowder->SetThink( DangerSoundThink );
	pPowder->SetNextThink(0);
	
	// Tumble in air
	pPowder->pev->avelocity.x = RANDOM_FLOAT ( -100, -500 );
	
	// Explode on contact
	pPowder->SetTouch( ExplodeTouch );

	// powder does a little damage, but makes Dunwich Horror visible
	pPowder->pev->dmg = 10;

	return pPowder;
}


//======================end powder of ibn

