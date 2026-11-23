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
//=========================================================
// Monster Maker - this is an entity that creates monsters
// in the game.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

// Monstermaker spawnflags
#define SF_MONSTERMAKER_START_ON 1 // start active ( if has targetname )
#define SF_MONSTERMAKER_CYCLIC 4 // drop one monster every time fired.
#define SF_MONSTERMAKER_MONSTERCLIP 8 // Children are blocked by monsterclip

//=========================================================
// MonsterMaker - this ent creates monsters during the game.
//=========================================================
class CMonsterMaker : public CBaseMonster
{
public:
	CMonsterMaker();
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void EXPORT ToggleUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT CyclicUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT MakerThink();
	void DeathNotice( entvars_t* child ) override;
	void MakeMonster();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	BOOL alwaysPassKeyValues() override;
	static TYPEDESCRIPTION m_SaveData[];
	string_t m_iszMonsterClassname; // classname of the monster(s) that will be created.
	int m_cNumMonsters; // max number of monsters this ent can create
	int m_cLiveChildren; // how many monsters made by this monster maker that are currently alive
	int m_iMaxLiveChildren; // max number of monsters that this maker may have out at one time.
	float m_flGround; // z coord of the ground under me, used to make sure no monsters are under the maker when it drops a new child
	BOOL m_fActive;
	BOOL m_fFadeChildren;
	bool m_bDontSpawnInView;

private:
	int m_cTargets;
	int m_iKey[16];
	int m_iValue[16];
	int m_passflags;
	int m_monsterModel;
};

LINK_ENTITY_TO_CLASS( monstermaker, CMonsterMaker );

TYPEDESCRIPTION CMonsterMaker::m_SaveData[] =
{
	DEFINE_FIELD( CMonsterMaker, m_iszMonsterClassname, FIELD_STRING ),
	DEFINE_FIELD( CMonsterMaker, m_cNumMonsters, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_cLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_flGround, FIELD_FLOAT ),
	DEFINE_FIELD( CMonsterMaker, m_iMaxLiveChildren, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_fFadeChildren, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_bDontSpawnInView, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMonsterMaker, m_cTargets, FIELD_INTEGER ),
	DEFINE_ARRAY( CMonsterMaker, m_iKey, FIELD_STRING, 16 ),
	DEFINE_ARRAY( CMonsterMaker, m_iValue, FIELD_STRING, 16 ),
	DEFINE_FIELD( CMonsterMaker, m_passflags, FIELD_INTEGER ),
	DEFINE_FIELD( CMonsterMaker, m_monsterModel, FIELD_STRING )
};

IMPLEMENT_SAVERESTORE( CMonsterMaker, CBaseMonster );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CMonsterMaker::CMonsterMaker()
{
	m_bDontSpawnInView = false;
	m_passflags = 0;
	m_monsterModel = 0;
}

BOOL CMonsterMaker::alwaysPassKeyValues()
{
	return TRUE;
}

void CMonsterMaker::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "monstermodel" ) )
		m_monsterModel = ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "monstercount" ) )
		m_cNumMonsters = atoi( data->szValue );
	else if ( FStrEq( data->szKeyName, "m_imaxlivechildren" ) )
		m_iMaxLiveChildren = atoi( data->szValue );
	else if ( FStrEq( data->szKeyName, "monstertype" ) )
		m_iszMonsterClassname = ALLOC_STRING( data->szValue );
	else if ( FStrEq( data->szKeyName, "gagged" ) )
	{
		if ( atoi( data->szValue ) )
			m_passflags |= SF_MONSTER_GAG;
		else
			m_passflags &= ~SF_MONSTER_GAG;
	}
	else if ( FStrEq( data->szKeyName, "deathfade" ) )
	{
		if ( atoi( data->szValue ) )
			m_passflags |= 512;
		else
			m_passflags &= ~512;
	}
	else if ( FStrEq( data->szKeyName, "dontspawninview" ) )
	{
		m_bDontSpawnInView = atoi( data->szValue ) != 0;
		return;
	}
	else if ( FStrEq( data->szKeyName, "origin" ) || FStrEq( data->szKeyName, "target" )
		|| FStrEq( data->szKeyName, "targetname" ) || FStrEq( data->szKeyName, "classname" )
		|| FStrEq( data->szKeyName, "delay" ) )
	{
		CBaseMonster::KeyValue( data );
		return;
	}
	else
	{
		if ( m_cTargets <= 15 )
		{
			char token[128];
			UTIL_StripToken( data->szKeyName, token, sizeof( token ) );
			m_iKey[m_cTargets] = ALLOC_STRING( token );
			UTIL_StripToken( data->szValue, token, sizeof( token ) );
			m_iValue[m_cTargets] = ALLOC_STRING( token );
			++m_cTargets;
			data->fHandled = TRUE;
		}
		return;
	}
	data->fHandled = TRUE;
}

void CMonsterMaker::Precache()
{
	UTIL_PrecacheOther( STRING( m_iszMonsterClassname ) );
	if ( m_monsterModel )
		PRECACHE_MODEL( (char*)STRING( m_monsterModel ) );
}

void CMonsterMaker::Spawn()
{
	pev->solid = SOLID_NOT;
	m_cLiveChildren = 0;
	Precache();
	if ( pev->targetname )
	{
		if ( pev->spawnflags & SF_MONSTERMAKER_CYCLIC )
			SetUse( &CMonsterMaker::CyclicUse ); // drop one monster each time we fire
		else
			SetUse( &CMonsterMaker::ToggleUse ); // so can be turned on/off
		if ( pev->spawnflags & SF_MONSTERMAKER_START_ON )
		{ // start making monsters as soon as monstermaker spawns
			m_fActive = TRUE;
			SetThink( &CMonsterMaker::MakerThink );
		}
		else
		{ // wait to be activated.
			m_fActive = FALSE;
			SetThink( &CBaseEntity::SUB_DoNothing );
		}
	}
	else
	{ // no targetname, just start.
		pev->nextthink = gpGlobals->time + m_flDelay;
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}
	m_flGround = 0;
	m_fFadeChildren = m_cNumMonsters != 1;
}

void CMonsterMaker::ToggleUse( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( !ShouldToggle( useType, m_fActive ) )
		return;
	if ( m_fActive )
	{
		m_fActive = FALSE;
		SetThink( NULL );
	}
	else
	{
		m_fActive = TRUE;
		SetThink( &CMonsterMaker::MakerThink );
	}
	pev->nextthink = gpGlobals->time;
}

void CMonsterMaker::CyclicUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	MakeMonster();
}

void CMonsterMaker::MakerThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;
	MakeMonster();
}

void CMonsterMaker::DeathNotice( entvars_t* child )
{
	--m_cLiveChildren;
	if ( !m_fFadeChildren )
		child->owner = NULL;
}

void CMonsterMaker::MakeMonster()
{
	CBaseEntity* player = UTIL_FindEntityByClassname( NULL, "player" );
	if ( player && m_bDontSpawnInView && ((CBaseMonster*)player)->FInViewCone( this, -1 ) )
		return;
	if ( m_iMaxLiveChildren > 0 && m_cLiveChildren >= m_iMaxLiveChildren )
		return;
	if ( m_flGround == 0 )
	{
		TraceResult trace;
		UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 2048 ), ignore_monsters, edict(), &trace );
		m_flGround = trace.vecEndPos.z;
	}
	Vector mins = pev->origin - Vector( 34, 34, 0 );
	Vector maxs = pev->origin + Vector( 34, 34, 0 );
	mins.z = m_flGround;
	CBaseEntity* list[2];
	if ( UTIL_EntitiesInBox( list, ARRAYSIZE( list ), mins, maxs, FL_CLIENT | FL_MONSTER ) )
		return;
	edict_t* monster = CREATE_NAMED_ENTITY( m_iszMonsterClassname );
	if ( FNullEnt( monster ) )
	{
		ALERT( at_console, "NULL Ent in MonsterMaker!\n" );
		return;
	}
	if ( pev->target )
		FireTargets( STRING( pev->target ), this, this, USE_TOGGLE, 0 );
	monster->v.origin = pev->origin;
	monster->v.angles = pev->angles;
	monster->v.spawnflags |= SF_MONSTER_FALL_TO_GROUND;
	if ( m_passflags )
		monster->v.spawnflags |= m_passflags;
	char key[128];
	char value[128];
	for ( int i = 0; i < m_cTargets; ++i )
	{
		KeyValueData* data = new KeyValueData;
		strncpy( key, STRING( m_iKey[i] ), sizeof( key ) - 1 );
		key[sizeof( key ) - 1] = 0;
		strncpy( value, STRING( m_iValue[i] ), sizeof( value ) - 1 );
		value[sizeof( value ) - 1] = 0;
		data->szKeyName = key;
		data->szValue = value;
		if ( FStrEq( key, "spawnflags" ) )
			continue;
		data->fHandled = FALSE;
		data->szClassName = (char*)STRING( monster->v.classname );
		DispatchKeyValue( monster, data );
		delete data;
	}
	if ( m_monsterModel )
	{
		KeyValueData* data = new KeyValueData;
		strcpy( key, "model" );
		strncpy( value, STRING( m_monsterModel ), sizeof( value ) - 1 );
		value[sizeof( value ) - 1] = 0;
		data->szKeyName = key;
		data->szValue = value;
		data->fHandled = FALSE;
		data->szClassName = (char*)STRING( monster->v.classname );
		DispatchKeyValue( monster, data );
		delete data;
	}

	// Children hit monsterclip brushes
	if ( pev->spawnflags & SF_MONSTERMAKER_MONSTERCLIP )
		monster->v.spawnflags |= SF_MONSTER_HITMONSTERCLIP;
	DispatchSpawn( monster->v.pContainingEntity );
	monster->v.owner = edict();
	if ( pev->netname )
		monster->v.targetname = pev->netname;
	++m_cLiveChildren;
	if ( m_cNumMonsters-- == 1 )
	{
		// Disable this forever.  Don't kill it because it still gets death notices
		SetThink( NULL );
		SetUse( NULL );
	}
}
