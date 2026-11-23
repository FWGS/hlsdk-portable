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

#pragma once

#include "cbase.h"

typedef enum
{
	expRandom,
	expDirected
} Explosions;

typedef enum
{
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matNone,
	matVine,
	matLastMaterial
} Materials;

#define NUM_SHARDS 6 // this many shards spawned when breakable objects break;

class CBreakable : public CBaseDelay
{
public:
	CBreakable();
	void Spawn() override;
	void Precache() override;
	void Restart() override;
	void KeyValue( KeyValueData* data ) override;
	void EXPORT BreakTouch( CBaseEntity* other );
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void DamageSound();
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType ) override;
	BOOL IsBreakable();
	BOOL SparkWhenHit();
	int DamageDecal( int damageType ) override;
	void EXPORT Die();
	int ObjectCaps() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	BOOL Explodable() { return ExplosionMagnitude() > 0; }
	int ExplosionMagnitude() { return pev->impulse; }
	void ExplosionSetMagnitude( int magnitude ) { pev->impulse = magnitude; }
	void CreateClientGlass( Vector surfaceNormal );
	void DecayGlass();
	bool CheckForProperItem( entvars_t* inflictor, entvars_t* activator );

	static void MaterialSoundPrecache( Materials material );
	static void MaterialSoundRandom( edict_t* entity, Materials material, float volume );
	static const char** MaterialSoundList( Materials material, int& count );
	static const char* pSoundsWood[];
	static const char* pSoundsVine[];
	static const char* pSoundsFlesh[];
	static const char* pSoundsGlass[];
	static const char* pSoundsMetal[];
	static const char* pSoundsConcrete[];
	static const char* pSpawnObjects[];
	static TYPEDESCRIPTION m_SaveData[];

	Materials		m_Material;
	Explosions		m_Explosion;
	int				m_idShard;
	float			m_angle;
	int				m_iszGibModel;
	int				m_iszSpawnObject;
	float			m_flHealth;
	int				m_iGibVelocity;
	int				m_iGibTowardsEntity;
	int				m_iOnlyDamagedBy;
	int				m_iNotDamagedBy;
	Vector			m_vStoredNormal;
	Vector			m_vHitPos;
	Vector			m_vHitDir;
	Vector			m_vNormal;
	Vector			m_vMins;
	Vector			m_vMaxs;
	Vector			m_vDecay;
	BOOL			m_iCreatedClientGlass;
	int				m_iIndex;
	unsigned short	m_usGlass;
	BOOL			m_grenadeTouch;
	float			m_spriteScale;

protected:
	bool			m_bCanSpawn;
};
