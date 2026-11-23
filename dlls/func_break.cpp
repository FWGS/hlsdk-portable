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
/*

===== bmodels.cpp ========================================================

  spawn, think, and use functions for entities that use brush models

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "func_break.h"
#include "player.h"
#include "decals.h"
#include "explode.h"

extern cvar_t sv_pushable_fixed_tick_fudge;
extern DLL_GLOBAL Vector g_vecAttackDir;

int g_numBreakableGlass;

const char* CBreakable::pSoundsWood[] =
{
	"debris/wood1.wav",
	"debris/wood2.wav",
	"debris/wood3.wav"
};

const char* CBreakable::pSoundsVine[] =
{
	"debris/vine1.wav",
	"debris/vine2.wav",
	"debris/vine3.wav"
};

const char* CBreakable::pSoundsFlesh[] =
{
	"debris/flesh1.wav",
	"debris/flesh2.wav",
	"debris/flesh3.wav",
	"debris/flesh5.wav",
	"debris/flesh6.wav",
	"debris/flesh7.wav"
};

const char* CBreakable::pSoundsGlass[] =
{
	"debris/glass1.wav",
	"debris/glass2.wav",
	"debris/glass3.wav"
};

const char* CBreakable::pSoundsMetal[] =
{
	"debris/metal1.wav",
	"debris/metal2.wav",
	"debris/metal3.wav"
};

const char* CBreakable::pSoundsConcrete[] =
{
	"debris/concrete1.wav",
	"debris/concrete2.wav",
	"debris/concrete3.wav"
};

TYPEDESCRIPTION CBreakable::m_SaveData[] =
{
	DEFINE_FIELD( CBreakable, m_Material, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_Explosion, FIELD_INTEGER ),

// Don't need to save/restore these because we precache after restore
//	DEFINE_FIELD( CBreakable, m_idShard, FIELD_INTEGER ),

	DEFINE_FIELD( CBreakable, m_angle, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakable, m_iszGibModel, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iszSpawnObject, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iOnlyDamagedBy, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_iNotDamagedBy, FIELD_STRING ),
	DEFINE_FIELD( CBreakable, m_flHealth, FIELD_FLOAT ),
	DEFINE_FIELD( CBreakable, m_vStoredNormal, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vHitPos, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vHitDir, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vNormal, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vMins, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_vDecay, FIELD_VECTOR ),
	DEFINE_FIELD( CBreakable, m_iCreatedClientGlass, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBreakable, m_iIndex, FIELD_INTEGER ),
	DEFINE_FIELD( CBreakable, m_grenadeTouch, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBreakable, m_spriteScale, FIELD_FLOAT )
};


//
// func_breakable - bmodel that breaks into pieces after taking damage
//
LINK_ENTITY_TO_CLASS( func_breakable, CBreakable );
IMPLEMENT_SAVERESTORE( CBreakable, CBaseEntity );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CBreakable::CBreakable()
{
	m_iGibTowardsEntity = 0;
	m_iOnlyDamagedBy = 0;
	m_bCanSpawn = true;
	m_iNotDamagedBy = 0;
	m_grenadeTouch = TRUE;
	m_spriteScale = -1;
	m_idShard = 0;
}

int CBreakable::ObjectCaps()
{
	return 0;
}

void CBreakable::KeyValue( KeyValueData* data )
{
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if ( FStrEq( data->szKeyName, "explosion" ) )
	{
		if ( !stricmp( data->szValue, "directed" ) )
			m_Explosion = expDirected;
		else if ( !stricmp( data->szValue, "random" ) )
			m_Explosion = expRandom;
		else
			m_Explosion = expRandom;
	}
	else if ( FStrEq( data->szKeyName, "material" ) )
	{
		int material = atoi( data->szValue );
		m_Material = (unsigned int)material >= matLastMaterial ? matWood : (Materials)material;
	}
	else if ( FStrEq( data->szKeyName, "deadmodel" ) || FStrEq( data->szKeyName, "shards" ) )
		data->fHandled = TRUE;
	else if ( FStrEq( data->szKeyName, "gibmodel" ) )
		m_iszGibModel = ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "onlydamagedby" ) )
		m_iOnlyDamagedBy = FStrEq( data->szValue, "default" ) ? 0 : ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "grenadetouch" ) )
		m_grenadeTouch = atoi( data->szValue ) != 0;
	else if ( FStrEq( data->szKeyName, "notdamagedby" ) )
		m_iNotDamagedBy = ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "spawnobject" ) )
		m_iszSpawnObject = ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "explodemagnitude" ) )
		ExplosionSetMagnitude( atoi( data->szValue ) );
	else if ( FStrEq( data->szKeyName, "explodespritescale" ) )
		m_spriteScale = (float)atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "lip" ) )
		data->fHandled = TRUE;
	else
	{
		CBaseDelay::KeyValue( data );
		return;
	}
	data->fHandled = TRUE;
}

void CBreakable::Spawn()
{
	if ( g_numBreakableGlass > 39 && (pev->spawnflags & 8) )
	{
		m_bCanSpawn = false;
		ALERT( at_console, "Too Many fancy glass!\n" );
	}
	if ( !m_bCanSpawn )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 1;
		return;
	}
	Precache();
	pev->takedamage = (pev->spawnflags & SF_BREAK_TRIGGER_ONLY) ? DAMAGE_NO : DAMAGE_YES;
	m_flHealth = pev->health;
	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	m_angle = pev->angles.y;
	pev->angles.y = 0;
	if ( m_Material == matGlass )
		pev->playerclass = 1;
	if ( pev->spawnflags & 8 )
		m_iIndex = g_numBreakableGlass++;
	m_iCreatedClientGlass = FALSE;
	SET_MODEL( edict(), STRING( pev->model ) );
	SetTouch( &CBreakable::BreakTouch );
	if ( pev->spawnflags & SF_BREAK_TRIGGER_ONLY )
		SetTouch( NULL );
	if ( m_Material == matUnbreakableGlass && pev->rendermode != kRenderNormal && !(pev->spawnflags & 16) )
		pev->flags |= FL_WORLDBRUSH;
}

void CBreakable::Restart()
{
	pev->solid = SOLID_BSP;
	pev->deadflag = DEAD_NO;
	pev->takedamage = (pev->spawnflags & SF_BREAK_TRIGGER_ONLY) ? DAMAGE_NO : DAMAGE_YES;
	pev->deadflag = DEAD_NO;
	pev->health = m_flHealth;
	pev->effects &= ~EF_NODRAW;
	m_angle = pev->angles.y;
	pev->angles.y = 0;
	if ( pev->spawnflags & 8 )
	{
		m_iCreatedClientGlass = FALSE;
		m_vDecay = g_vecZero;
	}
	SET_MODEL( edict(), STRING( pev->model ) );
	SetTouch( &CBreakable::BreakTouch );
	if ( pev->spawnflags & SF_BREAK_TRIGGER_ONLY )
		SetTouch( NULL );
	if ( m_Material == matUnbreakableGlass && pev->rendermode != kRenderNormal && !(pev->spawnflags & 16) )
		pev->flags |= FL_WORLDBRUSH;
}

const char** CBreakable::MaterialSoundList( Materials material, int& count )
{
	count = 3;
	switch ( material )
	{
	case matGlass:
	case matComputer:
	case matUnbreakableGlass:
		return pSoundsGlass;
	case matWood:
		return pSoundsWood;
	case matMetal:
		return pSoundsMetal;
	case matFlesh:
		count = 6;
		return pSoundsFlesh;
	case matCinderBlock:
	case matRocks:
		return pSoundsConcrete;
	case matVine:
		count = 0;
		return pSoundsVine;
	default:
		count = 0;
		return NULL;
	}
}

void CBreakable::MaterialSoundPrecache( Materials material )
{
	int count;
	const char** sounds = MaterialSoundList( material, count );
	for ( int i = 0; i < count; ++i )
		PRECACHE_SOUND( (char*)sounds[i] );
}

void CBreakable::MaterialSoundRandom( edict_t* entity, Materials material, float volume )
{
	int count;
	const char** sounds = MaterialSoundList( material, count );
	if ( count )
		EMIT_SOUND( entity, CHAN_BODY, sounds[RANDOM_LONG( 0, count - 1 )], volume, 1 );
}

void CBreakable::Precache()
{
	const char* gibModel = NULL;
	switch ( m_Material )
	{
	case matGlass:
	case matUnbreakableGlass:
		PRECACHE_SOUND( "debris/bustglass1.wav" );
		PRECACHE_SOUND( "debris/bustglass2.wav" );
		gibModel = "models/glassgibs.mdl";
		break;
	case matWood:
		PRECACHE_SOUND( "debris/bustcrate1.wav" );
		PRECACHE_SOUND( "debris/bustcrate2.wav" );
		gibModel = "models/woodgibs.mdl";
		break;
	case matMetal:
		PRECACHE_SOUND( "debris/bustmetal1.wav" );
		PRECACHE_SOUND( "debris/bustmetal2.wav" );
		gibModel = "models/metalplategibs.mdl";
		break;
	case matFlesh:
		PRECACHE_SOUND( "debris/bustflesh1.wav" );
		PRECACHE_SOUND( "debris/bustflesh2.wav" );
		gibModel = "models/fleshgibs.mdl";
		break;
	case matCinderBlock:
		PRECACHE_SOUND( "debris/bustconcrete1.wav" );
		PRECACHE_SOUND( "debris/bustconcrete2.wav" );
		gibModel = "models/cindergibs.mdl";
		break;
	case matCeilingTile:
		PRECACHE_SOUND( "debris/bustceiling.wav" );
		gibModel = "models/ceilinggibs.mdl";
		break;
	case matComputer:
		PRECACHE_SOUND( "buttons/spark5.wav" );
		PRECACHE_SOUND( "buttons/spark6.wav" );
		PRECACHE_SOUND( "debris/bustmetal1.wav" );
		PRECACHE_SOUND( "debris/bustmetal2.wav" );
		gibModel = "models/computergibs.mdl";
		break;
	case matRocks:
		PRECACHE_SOUND( "debris/bustconcrete1.wav" );
		PRECACHE_SOUND( "debris/bustconcrete2.wav" );
		gibModel = "models/rockgibs.mdl";
		break;
	case matVine:
		PRECACHE_SOUND( "debris/vine1.wav" );
		PRECACHE_SOUND( "debris/vine2.wav" );
		PRECACHE_SOUND( "debris/vine3.wav" );
		break;
	default:
		break;
	}
	MaterialSoundPrecache( m_Material );
	if ( m_iszGibModel )
		gibModel = STRING( m_iszGibModel );
	if ( gibModel )
		m_idShard = PRECACHE_MODEL( (char*)gibModel );
	m_usGlass = PRECACHE_EVENT( 1, "events/create_glass.sc" );
	if ( m_iszSpawnObject )
		UTIL_PrecacheOther( STRING( m_iszSpawnObject ) );
}

void CBreakable::TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType )
{
	if ( FStrEq( STRING( m_iOnlyDamagedBy ), "weapon_blowtorch" ) )
		UTIL_Sparks( trace->vecEndPos );
	// random spark if this is a 'computer' object
	if ( RANDOM_LONG( 0, 1 ) )
	{
		if ( m_Material == matComputer )
		{
			UTIL_Sparks( trace->vecEndPos );
			float volume = RANDOM_FLOAT( 0.7f, 1 );
			const char* sample = RANDOM_LONG( 0, 1 ) ? "buttons/spark6.wav" : "buttons/spark5.wav";
			EMIT_SOUND( edict(), CHAN_VOICE, sample, volume, ATTN_NORM );
		}
		else if ( m_Material == matUnbreakableGlass )
			UTIL_Ricochet( trace->vecEndPos, RANDOM_FLOAT( 0.5f, 1.5f ) );
	}
	m_vStoredNormal = trace->vecPlaneNormal;
	m_vHitPos = trace->vecEndPos;
	m_vHitDir = direction;
	CBaseEntity::TraceAttack( attacker, damage, direction, trace, damageType );
}

bool CBreakable::CheckForProperItem( entvars_t* inflictor, entvars_t* activator )
{
	if ( !m_iOnlyDamagedBy && !m_iNotDamagedBy )
		return true;
	CBaseEntity::Instance( inflictor );
	if ( CBaseEntity::Instance( activator )->IsPlayer() )
	{
		CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
		if ( !player )
			return false;
		if ( m_iOnlyDamagedBy && FStrEq( STRING( player->GetActiveWeaponClassname() ), STRING( m_iOnlyDamagedBy ) ) )
			return true;
		if ( m_iNotDamagedBy && FStrEq( STRING( player->GetActiveWeaponClassname() ), STRING( m_iNotDamagedBy ) ) )
			return false;
	}
	if ( m_iOnlyDamagedBy )
	{
		const char* required = STRING( m_iOnlyDamagedBy );
		if ( FStrEq( STRING( inflictor->classname ), required )
			|| (FStrEq( required, "weapon_radiocontrolledbomb" ) && FClassnameIs( inflictor, "bomb" )) )
			return true;
	}
	return m_iNotDamagedBy && !FStrEq( STRING( inflictor->classname ), STRING( m_iNotDamagedBy ) );
}

int CBreakable::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	if ( !CheckForProperItem( inflictor, attacker ) )
		return 0;
	Vector direction = inflictor->origin - (pev->absmin + pev->size * 0.5f);
	if ( inflictor == attacker && (inflictor->flags & FL_CLIENT) && (pev->spawnflags & SF_BREAK_CROWBAR)
		&& (damageType & DMG_CLUB) )
		damage = pev->health;
	if ( !IsBreakable() )
		return 0;
	if ( damageType & DMG_CLUB )
		damage *= 2;
	if ( damageType & DMG_POISON )
		damage *= 0.1;
	g_vecAttackDir = direction.Normalize();
	pev->health -= damage;
	if ( pev->health <= 0 )
	{
		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->effects = EF_NODRAW;
		Die();
		if ( m_flDelay == 0 )
			m_flDelay = 0.1;
		pev->nextthink = pev->ltime + m_flDelay;
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	DamageSound();
	return 1;
}

void CBreakable::BreakTouch( CBaseEntity* other )
{
	entvars_t* toucher = other->pev;
	if ( !((other->IsPlayer() && IsBreakable()) || !m_grenadeTouch) )
	{
		if ( pev->rendermode == kRenderNormal || !FClassnameIs( toucher, "grenade" ) )
			return;
		pev->angles.y = m_angle;
		UTIL_MakeVectors( pev->angles );
		g_vecAttackDir = gpGlobals->v_forward;
		pev->takedamage = DAMAGE_NO;
		pev->deadflag = DEAD_DEAD;
		pev->effects = EF_NODRAW;
		Die();
	}
	if ( pev->spawnflags & SF_BREAK_TOUCH )
	{
		float damage = toucher->velocity.Length() * 0.01;
		if ( damage >= pev->health )
		{
			SetTouch( NULL );
			TakeDamage( toucher, toucher, damage, DMG_CRUSH );
			other->TakeDamage( pev, pev, damage * 0.25f, DMG_SLASH );
		}
	}
	if ( (pev->spawnflags & SF_BREAK_PRESSURE) && toucher->absmin.z >= pev->maxs.z - 2 )
	{ // can be broken when stood upon

		// play creaking sound here.
		DamageSound();
		SetThink( &CBreakable::Die );
		SetTouch( NULL );
		if ( m_flDelay == 0 )
			m_flDelay = 0.1f;
		pev->nextthink = pev->ltime + m_flDelay;
	}
}


//
// Smash the our breakable object
//

// Break when triggered
void CBreakable::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	if ( !IsBreakable() && !(pev->spawnflags & SF_BREAK_TRIGGER_ONLY) )
		return;
	pev->angles.y = m_angle;
	UTIL_MakeVectors( pev->angles );
	g_vecAttackDir = gpGlobals->v_forward;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->effects = EF_NODRAW;
	Die();
}

void CBreakable::DamageSound()
{
	int pitch;
	float fvol;
	const char* rgpsz[6];
	int i;
	int material = m_Material;

//	if (RANDOM_LONG(0,1))
//		return;

	if ( RANDOM_LONG( 0, 2 ) )
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG( 0, 34 );

	fvol = RANDOM_FLOAT( 0.75f, 1 );

	if ( material == matComputer && RANDOM_LONG( 0, 1 ) )
		material = matMetal;

	switch ( material )
	{
	case matComputer:
	case matGlass:
	case matUnbreakableGlass:
		rgpsz[0] = "debris/glass1.wav";
		rgpsz[1] = "debris/glass2.wav";
		rgpsz[2] = "debris/glass3.wav";
		i = 3;
		break;

	case matWood:
		rgpsz[0] = "debris/wood1.wav";
		rgpsz[1] = "debris/wood2.wav";
		rgpsz[2] = "debris/wood3.wav";
		i = 3;
		break;

	case matMetal:
		rgpsz[0] = "debris/metal1.wav";
		rgpsz[1] = "debris/metal3.wav";
		rgpsz[2] = "debris/metal2.wav";
		i = 2;
		break;

	case matFlesh:
		rgpsz[0] = "debris/flesh1.wav";
		rgpsz[1] = "debris/flesh2.wav";
		rgpsz[2] = "debris/flesh3.wav";
		rgpsz[3] = "debris/flesh5.wav";
		rgpsz[4] = "debris/flesh6.wav";
		rgpsz[5] = "debris/flesh7.wav";
		i = 6;
		break;

	case matRocks:
	case matCinderBlock:
		rgpsz[0] = "debris/concrete1.wav";
		rgpsz[1] = "debris/concrete2.wav";
		rgpsz[2] = "debris/concrete3.wav";
		i = 3;
		break;

	case matCeilingTile:
		// UNDONE: no ceiling tile shard sound yet
		i = 0;
		break;

	case matVine:
		rgpsz[0] = "debris/vine1.wav";
		rgpsz[1] = "debris/vine2.wav";
		rgpsz[2] = "debris/vine3.wav";
		i = 3;
		break;

	default:
		i = 0;
		break;
	}

	if ( i )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, rgpsz[RANDOM_LONG( 0, i - 1 )], fvol, ATTN_NORM, 0, pitch );
}

BOOL CBreakable::IsBreakable()
{
	return m_Material != matUnbreakableGlass;
}

int CBreakable::DamageDecal( int damageType )
{
	if ( m_Material == matGlass )
		return DECAL_GLASSBREAK1 + RANDOM_LONG( 0, 2 );
	if ( m_Material == matUnbreakableGlass )
		return DECAL_BPROOF1;
	return CBaseEntity::DamageDecal( damageType );
}

void CBreakable::CreateClientGlass( Vector surfaceNormal )
{
	Vector positiveNormal = surfaceNormal;
	Vector negativeNormal = -surfaceNormal;
	Vector forward, right, up;
	Vector angles = UTIL_VecToAngles( positiveNormal );
	UTIL_MakeVectorsPrivate( angles, forward, right, up );
	float positiveDistance = (pev->maxs - (pev->mins + right)).Length();
	angles = UTIL_VecToAngles( negativeNormal );
	UTIL_MakeVectorsPrivate( angles, forward, right, up );
	float negativeDistance = (pev->maxs - (pev->mins + right)).Length();
	Vector normal = fabsf( positiveDistance ) > fabsf( negativeDistance ) ? negativeNormal : positiveNormal;
	angles = UTIL_VecToAngles( normal );
	UTIL_MakeVectorsPrivate( angles, forward, right, up );
	float projection = DotProduct( pev->maxs - pev->mins, normal );
	float distance = fabsf( projection ) / normal.Length();
	Vector mins = pev->mins;
	Vector maxs = pev->maxs + forward * distance;
	Vector edge = maxs - mins;
	Vector rightEdge = mins + right - mins;
	float width = DotProduct( rightEdge, edge ) / (rightEdge.Length() * edge.Length()) * edge.Length();
	float height = (float)sqrt( edge.Length() * edge.Length() - width * width );
	m_vMaxs = maxs;
	m_vNormal = normal;
	m_vMins = mins;
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, NULL, m_usGlass, 0, mins, normal, width, height, m_iIndex, 0, 0, 0 );
}

void CBreakable::DecayGlass()
{
	Vector angles = UTIL_VecToAngles( m_vNormal );
	Vector forward, right, up;
	UTIL_MakeVectorsPrivate( angles, forward, right, up );
	Vector rightEdge = right + m_vMins - m_vMins;
	float projection = DotProduct( m_vHitPos - m_vMins, m_vNormal );
	float distance = fabsf( projection ) / m_vNormal.Length();
	Vector offset = forward * distance + m_vHitPos - m_vMins;
	float width = DotProduct( offset, rightEdge ) / (offset.Length() * rightEdge.Length()) * offset.Length();
	float height = (float)sqrt( offset.Length() * offset.Length() - width * width );
	float decayTime = RANDOM_FLOAT( 10, 15 );
	if ( m_vDecay == g_vecZero )
	{
		m_vDecay.x = width;
		m_vDecay.y = height;
	}
	PLAYBACK_EVENT_FULL( FEV_GLOBAL, NULL, m_usGlass, 0, m_vDecay, g_vecAttackDir,
		width, height, m_iIndex, (int)decayTime, 1, 0 );
}

void CBreakable::Die()
{
	int pitch = 95 + RANDOM_LONG( 0, 29 );
	if ( pitch > 97 && pitch < 103 )
		pitch = PITCH_NORM;
	float volume = RANDOM_FLOAT( 0.85f, 1 ) + fabsf( pev->health ) / 100.0;
	if ( volume > 1 )
		volume = 1;
	int flags = 0;
	switch ( m_Material )
	{
	case matGlass:
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustglass1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustglass2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_GLASS;
		break;

	case matWood:
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustcrate1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustcrate2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustmetal1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustmetal2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_METAL;
		break;

	case matFlesh:
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustflesh1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustflesh2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustconcrete1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustconcrete2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/bustceiling.wav", volume, ATTN_NORM, 0, pitch );
		break;

	case matVine:
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/vine1.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 1:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/vine2.wav", volume, ATTN_NORM, 0, pitch );
			break;
		case 2:	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "debris/vine3.wav", volume, ATTN_NORM, 0, pitch );
			break;
		}
		flags = BREAK_WOOD;
		break;
	}
	Vector velocity = m_Explosion == expDirected ? g_vecAttackDir * 200 : g_vecZero;
	if ( pev->spawnflags & 8 )
	{
		if ( m_Material == matGlass )
		{
			Vector normal = m_vStoredNormal;
			if ( !(normal.Length() >= 0.0000001) )
			{
				CreateClientGlass( Vector( 0, 1, 0 ) );
				normal = Vector( 1, 0, 0 );
			}
			CreateClientGlass( normal );
			DecayGlass();
		}
	}
	else if ( m_idShard )
	{
		Vector spot = pev->origin + (pev->mins + pev->maxs) * 0.5f;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, spot );
			WRITE_BYTE( TE_BREAKMODEL );
			WRITE_COORD( spot.x );
			WRITE_COORD( spot.y );
			WRITE_COORD( spot.z );

			// size
			WRITE_COORD( pev->size.x );
			WRITE_COORD( pev->size.y );
			WRITE_COORD( pev->size.z );
			WRITE_COORD( velocity.x );
			WRITE_COORD( velocity.y );
			WRITE_COORD( velocity.z );

			// randomization
			WRITE_BYTE( 10 );

			// Model
			WRITE_SHORT( m_idShard );	//model id#

			// # of shards
			WRITE_BYTE( 0 );	// let client decide

			// duration
			WRITE_BYTE( 25 ); // 2.5 seconds

			// flags
			WRITE_BYTE( flags );
		MESSAGE_END();
	}

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;
	CBaseEntity* list[256];
	int count = UTIL_EntitiesInBox( list, ARRAYSIZE( list ), mins, maxs, FL_ONGROUND );
	for ( int i = 0; i < count; ++i )
	{
		list[i]->pev->flags &= ~FL_ONGROUND;
		list[i]->pev->groundentity = NULL;
	}
	pev->solid = SOLID_NOT;
	// Fire targets on break
	SUB_UseTargets( NULL, USE_TOGGLE, 0 );
	if ( m_iszSpawnObject )
		CBaseEntity::Create( (char*)STRING( m_iszSpawnObject ), VecBModelOrigin( pev ), pev->angles, edict() );
	if ( Explodable() )
		ExplosionCreate( Center(), pev->angles, edict(), ExplosionMagnitude(), TRUE, m_spriteScale );
	UTIL_Remove( this );
}

class CPushable : public CBreakable
{
public:
	void Spawn() override;
	void Precache() override;
	void Touch( CBaseEntity* other ) override;
	void Move( CBaseEntity* other, int push );
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int ObjectCaps() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	float MaxSpeed() { return m_maxSpeed; }
	void EXPORT PushThink();
	Vector m_pushDir;	// not saved: a push never lasts across a load
	float m_pushUntil;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;

	static TYPEDESCRIPTION m_SaveData[];
	static const char* m_soundNames[3];
	int		m_lastSound;	// no need to save/restore, just keeps the same sound from playing twice in a row
	float	m_maxSpeed;
	float	m_soundTime;
	BOOL	m_silent;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

TYPEDESCRIPTION CPushable::m_SaveData[] =
{
	DEFINE_FIELD( CPushable, m_maxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CPushable, m_soundTime, FIELD_TIME ),
	DEFINE_FIELD( CPushable, m_silent, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CPushable, CBreakable );
LINK_ENTITY_TO_CLASS( func_pushable, CPushable );

const char* CPushable::m_soundNames[] =
{
	"debris/pushbox1.wav",
	"debris/pushbox2.wav",
	"debris/pushbox3.wav"
};

int CPushable::ObjectCaps()
{
	return FCAP_CONTINUOUS_USE;
}

void CPushable::Spawn()
{
	if ( pev->spawnflags & SF_PUSH_BREAKABLE )
		CBreakable::Spawn();
	else
		Precache();
	pev->movetype = MOVETYPE_PUSHSTEP;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( pev->friction > 399 )
		pev->friction = 399;
	m_maxSpeed = 400 - pev->friction;
	pev->flags |= FL_FLOAT;
	pev->friction = 0;
	pev->origin.z += 1;	// Pick up off of the floor
	UTIL_SetOrigin( pev, pev->origin );
	pev->skin = (int)((pev->maxs.x - pev->mins.x) * pev->skin * (pev->maxs.y - pev->mins.y) * 0.0005);
	m_soundTime = 0;
}

void CPushable::Precache()
{
	for ( int i = 0; i < ARRAYSIZE( m_soundNames ); ++i )
		PRECACHE_SOUND( m_soundNames[i] );
	if ( pev->spawnflags & SF_PUSH_BREAKABLE )
		CBreakable::Precache();
}

void CPushable::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "size" ) )
	{
		switch ( atoi( data->szValue ) )
		{
		case 0:	// Point
			UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
			break;
		case 2: // Big Hull!?!?	!!!BUGBUG Figure out what this hull really is
			UTIL_SetSize( pev, Vector( -32, -32, -36 ), Vector( 32, 32, 36 ) );
			break;
		case 3:
			UTIL_SetSize( pev, Vector( -16, -16, -18 ), Vector( 16, 16, 18 ) );
			break;
		default:
			UTIL_SetSize( pev, Vector( -16, -16, -36 ), Vector( 16, 16, 36 ) );
			break;
		}
	}
	else if ( FStrEq( data->szKeyName, "buoyancy" ) )
		pev->skin = (int)atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "silent" ) )
		m_silent = atoi( data->szValue ) != 0;
	else
	{
		CBreakable::KeyValue( data );
		return;
	}
	data->fHandled = TRUE;
}


// Pull the func_pushable
void CPushable::Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	if ( !activator || !activator->IsPlayer() )
	{
		if ( pev->spawnflags & SF_PUSH_BREAKABLE )
			CBreakable::Use( activator, caller, useType, value );
		return;
	}
	if ( activator->pev->velocity != g_vecZero )
		Move( activator, 0 );
}

void CPushable::PushThink()
{
	if ( gpGlobals->time > m_pushUntil )
	{
		SetThink( NULL );
		return;
	}
	Vector step = m_pushDir * ( MaxSpeed() * gpGlobals->frametime );
	TraceResult tr;
	TRACE_MONSTER_HULL( edict(), pev->origin, pev->origin + step, ignore_monsters, edict(), &tr );
	if ( !tr.fStartSolid && !tr.fAllSolid && tr.flFraction > 0 )
		UTIL_SetOrigin( pev, tr.vecEndPos );
	pev->nextthink = gpGlobals->time;
}

void CPushable::Touch( CBaseEntity* other )
{
	if ( !FClassnameIs( other->pev, "worldspawn" ) )
		Move( other, 1 );
}

void CPushable::Move( CBaseEntity* other, int push )
{
	entvars_t* toucher = other->pev;
	if ( (toucher->flags & FL_ONGROUND) && toucher->groundentity && VARS( toucher->groundentity ) == pev )
	{
		// Only push if floating
		if ( pev->waterlevel > 0 )
			pev->velocity.z += toucher->velocity.z * 0.1;
		return;
	}
	BOOL playerTouch = other->IsPlayer();
	if ( playerTouch && push && (toucher->button & IN_USE) )
		return;
	if ( playerTouch && push )
	{
		// czeror: pushing by a player. Xash sees the box as stuck inside the player (a trace of the
		// box hull starts in him), so its own physics never moves it, and the player's velocity at
		// the moment of the touch is already cut by the box. So: when the player walks into the box,
		// move the box along the axis away from him at its top speed, by hand, with a trace that
		// ignores monsters (the player) and hits the world and the other solid things
		if ( !( toucher->flags & FL_ONGROUND ) )
			return;
		UTIL_MakeVectors( Vector( 0, toucher->v_angle.y, 0 ) );
		Vector wish = g_vecZero;
		if ( toucher->button & IN_FORWARD ) wish = wish + gpGlobals->v_forward;
		if ( toucher->button & IN_BACK ) wish = wish - gpGlobals->v_forward;
		if ( toucher->button & IN_MOVERIGHT ) wish = wish + gpGlobals->v_right;
		if ( toucher->button & IN_MOVELEFT ) wish = wish - gpGlobals->v_right;
		wish.z = 0;
		if ( wish.Length() < 0.1f )
			return;
		wish = wish.Normalize();
		Vector center = ( pev->absmin + pev->absmax ) * 0.5f;
		Vector away = center - toucher->origin;
		away.z = 0;
		if ( away.Length() < 1 )
			return;
		away = away.Normalize();
		if ( DotProduct( wish, away ) < 0.5f )
			return;
		Vector dir = g_vecZero;
		if ( fabsf( away.x ) > fabsf( away.y ) )
			dir.x = away.x > 0 ? 1 : -1;
		else
			dir.y = away.y > 0 ? 1 : -1;
		// the box is moved on its own think, every frame while the pushing lasts. Moving it only on
		// the frames when the touch happens made it jerk and slow
		m_pushDir = dir;
		m_pushUntil = gpGlobals->time + 0.1f;
		if ( m_pfnThink != (void ( CBaseEntity::* )())&CPushable::PushThink )
		{
			SetThink( &CPushable::PushThink );
			pev->nextthink = gpGlobals->time;
		}
		toucher->velocity.x = dir.x * MaxSpeed();
		toucher->velocity.y = dir.y * MaxSpeed();
		if ( gpGlobals->time - m_soundTime > 0.7 && !m_silent )
		{
			m_soundTime = gpGlobals->time;
			m_lastSound = RANDOM_LONG( 0, 2 );
			EMIT_SOUND( edict(), CHAN_WEAPON, m_soundNames[m_lastSound], 0.5f, ATTN_NORM );
		}
		return;
	}
	float factor = 0.25f;
	if ( playerTouch )
	{
		if ( !(toucher->flags & FL_ONGROUND) )	// Don't push away from jumping/falling players unless in water
		{
			if ( pev->waterlevel <= 0 )
				return;
			factor = 0.1;
		}
		else
			factor = 1;
	}
	if ( !push && sv_pushable_fixed_tick_fudge.value >= 0 )
		factor *= sv_pushable_fixed_tick_fudge.value * gpGlobals->frametime;
	if ( !push && playerTouch )
	{
		// czeror: pulling with USE. The movement code slows a player who holds USE to a third of
		// his speed on every frame, so he walks at 10 to 20 units per second at 150+ frames per
		// second. The box got a small share of that each frame and the engine friction
		// (4 x 100 units per second squared) took it all back: it never moved. Follow the
		// player's own velocity instead
		pev->velocity.x = toucher->velocity.x;
		pev->velocity.y = toucher->velocity.y;
	}
	else
	{
		if ( push || fabsf( pev->velocity.x ) < fabsf( toucher->velocity.x - toucher->velocity.x * factor ) )
			pev->velocity.x += toucher->velocity.x * factor;
		if ( push || fabsf( pev->velocity.y ) < fabsf( toucher->velocity.y - toucher->velocity.y * factor ) )
			pev->velocity.y += toucher->velocity.y * factor;
	}
	float speed = pev->velocity.Length2D();
	if ( speed > MaxSpeed() )
	{
		pev->velocity.x = pev->velocity.x * MaxSpeed() / speed;
		pev->velocity.y = pev->velocity.y * MaxSpeed() / speed;
	}
	speed = pev->velocity.Length2D();
	float playerSpeed = toucher->velocity.Length2D();
	if ( push )
	{
		if ( !playerTouch )
			return;
		toucher->velocity.x = pev->velocity.x;
		toucher->velocity.y = pev->velocity.y;

	}
	else if ( speed >= playerSpeed )
		pev->velocity = pev->velocity.Normalize() * (playerSpeed * 1.1);
	if ( playerTouch && gpGlobals->time - m_soundTime > 0.7 && !m_silent )
	{
		m_soundTime = gpGlobals->time;
		if ( speed > 0 && (pev->flags & FL_ONGROUND) )
		{
			m_lastSound = RANDOM_LONG( 0, 2 );
			EMIT_SOUND( edict(), CHAN_WEAPON, m_soundNames[m_lastSound], 0.5f, ATTN_NORM );
		}
		else
			STOP_SOUND( edict(), CHAN_WEAPON, m_soundNames[m_lastSound] );
	}
}

int CPushable::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	if ( pev->spawnflags & SF_PUSH_BREAKABLE )
		return CBreakable::TakeDamage( inflictor, attacker, damage, damageType );
	return 1;
}
