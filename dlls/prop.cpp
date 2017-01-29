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

===== generic grenade.cpp ========================================================

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "player.h"
#include "explode.h"
#include "gamerules.h"

#define SF_PROP_RESPAWN		8 // enable autorespawn
#define SF_PROP_BREAKABLE		16 // enable break/explode
#define SF_PROP_FIXED		32 // don't move untill touch
typedef enum { expRandom, expDirected} Explosions;
typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matNone, matLastMaterial } Materials;

// round() has problems, so just implement it here
static inline int myround(float f)
{
	return (int)(f + 0.5);
}

//extern "C" void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
Vector UTIL_AngleVectorsF(const Vector &angles)
{
	float rgflVecOut[3];
	float rgflVecIn[3];
	angles.CopyToArray(rgflVecIn);
	g_engfuncs.pfnAngleVectors(rgflVecIn, rgflVecOut, NULL, NULL);
	return Vector(rgflVecOut);
}
Vector UTIL_AngleVectorsR(const Vector &angles)
{
	float rgflVecOut[3];
	float rgflVecIn[3];
	angles.CopyToArray(rgflVecIn);
	g_engfuncs.pfnAngleVectors(rgflVecIn, NULL, rgflVecOut, NULL);
	return Vector(rgflVecOut);
}
Vector UTIL_AngleVectorsU(const Vector &angles)
{
	float rgflVecOut[3];
	float rgflVecIn[3];
	angles.CopyToArray(rgflVecIn);
	g_engfuncs.pfnAngleVectors(rgflVecIn, NULL, rgflVecOut, NULL);
	return Vector(rgflVecOut);
}
//===================grenade

enum PropShape
{
	SHAPE_CYL_H = 0,
	SHAPE_CYL_V,
	SHAPE_BOX,
	SHAPE_GENERIC,
	SHAPE_SPHERE,
	SHAPE_NOROTATE
};

class CProp : public CBaseEntity
{
public:
	void Spawn(void);
	void Precache();

	void EXPORT BounceTouch(CBaseEntity *pOther);
	//void EXPORT SlideTouch(CBaseEntity *pOther);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void Force(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual int	ObjectCaps(void) { return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE; }
	virtual void BounceSound(void);
	virtual int	BloodColor(void) { return DONT_BLEED; }
	virtual void Killed(entvars_t *pevAttacker, int iGib);
	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		float speed = 2500;
		if( pev->deadflag )
			return 0;
		pev->movetype = MOVETYPE_BOUNCE;
		if(stage)
		{
			pev->nextthink = gpGlobals->time + m_flRespawnTime;
			SetThink( &CProp::RespawnThink );
		}
		if( stage == 2 )
		{
			UTIL_MakeVectors( attacker->pev->v_angle + attacker->pev->punchangle);
			Vector atarget = UTIL_VecToAngles(gpGlobals->v_forward);
			pev->angles.x = UTIL_AngleMod(pev->angles.x);
			pev->angles.y = UTIL_AngleMod(pev->angles.y);
			pev->angles.z = UTIL_AngleMod(pev->angles.z);
			atarget.x = UTIL_AngleMod(atarget.x);
			atarget.y = UTIL_AngleMod(atarget.y);
			atarget.z = UTIL_AngleMod(atarget.z);
			pev->avelocity.x = UTIL_AngleDiff(atarget.x, pev->angles.x) * 10;
			pev->avelocity.y = UTIL_AngleDiff(atarget.y, pev->angles.y) * 10;
			pev->avelocity.z = UTIL_AngleDiff(atarget.z, pev->angles.z) * 10;
		}
		if( stage == 3 )
		{
			pev->avelocity.y = pev->avelocity.y*1.5 + RANDOM_FLOAT(100, -100);
			pev->avelocity.x = pev->avelocity.x*1.5 + RANDOM_FLOAT(100, -100);
		}
		if( !m_attacker || m_attacker == this )
		{
			m_owner2 = attacker;
			m_attacker = attacker;
			return speed;
		}
		if( !m_owner2 && m_attacker && ( pev->velocity.Length() < 400) )
			m_attacker = attacker;
			return speed;
		if( ( stage == 2 ) && ( m_attacker == attacker ) )
		{
			m_owner2 = attacker;

			return speed;
		}
		return 0;
	}
	void CheckRotate();
	void EXPORT RespawnThink();
	void EXPORT AngleThink();
	void EXPORT DeployThink();
	void EXPORT DieThink();
	void DamageSound( void );
	void PropRespawn();
	void KeyValue( KeyValueData* pkvd);

	static const char *pSoundsWood[];
	static const char *pSoundsFlesh[];
	static const char *pSoundsGlass[];
	static const char *pSoundsMetal[];
	static const char *pSoundsConcrete[];
	static const char *pSpawnObjects[];

	inline BOOL		Explodable( void ) { return ExplosionMagnitude() > 0; }
	inline int		ExplosionMagnitude( void ) { return pev->impulse; }
	inline void		ExplosionSetMagnitude( int magnitude ) { pev->impulse = magnitude; }


	static void MaterialSoundPrecache( Materials precacheMaterial );
	static void MaterialSoundRandom( edict_t *pEdict, Materials soundMaterial, float volume );
	static const char **MaterialSoundList( Materials precacheMaterial, int &soundCount );
	void EXPORT		Die( void );

	BOOL m_bBarrel;
	float m_flFloorFriction;
	float m_flCollideFriction;

	// hull sizes
	Vector minsH, maxsH;
	Vector minsV, maxsV;

	// spawn backup;
	Vector spawnOrigin;
	Vector spawnAngles;

	EHANDLE m_owner2;
	EHANDLE m_attacker;
	float m_flNextAttack;
	float m_flRespawnTime;
	PropShape m_shape;
	PropShape m_oldshape;
	EHANDLE m_pHolstered;
	float m_flSpawnHealth;
	int			m_idShard;
	float		m_angle;
	int			m_iszGibModel;
	Materials	m_Material;
	Explosions	m_Explosion;
	int m_iaCustomAnglesX[10];
	int m_iaCustomAnglesZ[10];
	float m_flTouchTimer;
	int m_iTouchCounter;
};
LINK_ENTITY_TO_CLASS(prop, CProp);

static int propTouchSelector;

const char *CProp::pSoundsWood[] =
{
	"debris/wood1.wav",
	"debris/wood2.wav",
	"debris/wood3.wav",
};

const char *CProp::pSoundsFlesh[] =
{
	"debris/flesh1.wav",
	"debris/flesh2.wav",
	"debris/flesh3.wav",
	"debris/flesh5.wav",
	"debris/flesh6.wav",
	"debris/flesh7.wav",
};

const char *CProp::pSoundsMetal[] =
{
	"debris/metal1.wav",
	"debris/metal2.wav",
	"debris/metal3.wav",
};

const char *CProp::pSoundsConcrete[] =
{
	"debris/concrete1.wav",
	"debris/concrete2.wav",
	"debris/concrete3.wav",
};


const char *CProp::pSoundsGlass[] =
{
	"debris/glass1.wav",
	"debris/glass2.wav",
	"debris/glass3.wav",
};

const char **CProp::MaterialSoundList( Materials precacheMaterial, int &soundCount )
{
	const char	**pSoundList = NULL;

	switch ( precacheMaterial )
	{
	case matWood:
		pSoundList = pSoundsWood;
		soundCount = ARRAYSIZE(pSoundsWood);
		break;
	case matFlesh:
		pSoundList = pSoundsFlesh;
		soundCount = ARRAYSIZE(pSoundsFlesh);
		break;
	case matComputer:
	case matUnbreakableGlass:
	case matGlass:
		pSoundList = pSoundsGlass;
		soundCount = ARRAYSIZE(pSoundsGlass);
		break;

	case matMetal:
		pSoundList = pSoundsMetal;
		soundCount = ARRAYSIZE(pSoundsMetal);
		break;

	case matCinderBlock:
	case matRocks:
	case matCeilingTile:
		pSoundList = pSoundsConcrete;
		soundCount = ARRAYSIZE(pSoundsConcrete);
		break;

	case matNone:
	default:
		soundCount = 0;
		break;
	}

	return pSoundList;
}

void CProp::MaterialSoundPrecache( Materials precacheMaterial )
{
	const char	**pSoundList;
	int			i, soundCount = 0;

	pSoundList = MaterialSoundList( precacheMaterial, soundCount );

	for ( i = 0; i < soundCount; i++ )
	{
		PRECACHE_SOUND( (char *)pSoundList[i] );
	}
}

void CProp::MaterialSoundRandom( edict_t *pEdict, Materials soundMaterial, float volume )
{
	const char	**pSoundList;
	int			soundCount = 0;

	pSoundList = MaterialSoundList( soundMaterial, soundCount );

	if ( soundCount )
		EMIT_SOUND( pEdict, CHAN_BODY, pSoundList[ RANDOM_LONG(0,soundCount-1) ], volume, 1.0 );
}

void CProp::Precache( void )
{
	const char *pGibName = NULL;

	if( !pev->model )
		pev->model = MAKE_STRING( "models/xash/barrel_brown.mdl" );

	if( m_Material == matGlass && !(pev->spawnflags & SF_PROP_BREAKABLE) )
		m_Material = matWood;
	switch (m_Material)
	{
	case matWood:
		pGibName = "models/woodgibs.mdl";

		PRECACHE_SOUND("debris/bustcrate1.wav");
		PRECACHE_SOUND("debris/bustcrate2.wav");
		break;
	case matFlesh:
		pGibName = "models/fleshgibs.mdl";

		PRECACHE_SOUND("debris/bustflesh1.wav");
		PRECACHE_SOUND("debris/bustflesh2.wav");
		break;
	case matComputer:
		PRECACHE_SOUND("buttons/spark5.wav");
		PRECACHE_SOUND("buttons/spark6.wav");
		pGibName = "models/computergibs.mdl";

		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;

	case matUnbreakableGlass:
	case matGlass:
		pGibName = "models/glassgibs.mdl";

		PRECACHE_SOUND("debris/bustglass1.wav");
		PRECACHE_SOUND("debris/bustglass2.wav");
		break;
	case matMetal:
		pGibName = "models/metalplategibs.mdl";

		PRECACHE_SOUND("debris/bustmetal1.wav");
		PRECACHE_SOUND("debris/bustmetal2.wav");
		break;
	case matCinderBlock:
		pGibName = "models/cindergibs.mdl";

		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matRocks:
		pGibName = "models/rockgibs.mdl";

		PRECACHE_SOUND("debris/bustconcrete1.wav");
		PRECACHE_SOUND("debris/bustconcrete2.wav");
		break;
	case matCeilingTile:
		pGibName = "models/ceilinggibs.mdl";

		PRECACHE_SOUND ("debris/bustceiling.wav");
		break;
	}
	MaterialSoundPrecache( m_Material );
	if ( m_iszGibModel )
		pGibName = STRING(m_iszGibModel);
	if( pGibName )
		m_idShard = PRECACHE_MODEL( (char *)pGibName );
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}

void CProp::DamageSound( void )
{
	int pitch;
	float fvol;
	char *rgpsz[6];
	int i = 0;
	int material = m_Material;

//	if (RANDOM_LONG(0,1))
//		return;

	if (RANDOM_LONG(0,2))
		pitch = PITCH_NORM;
	else
		pitch = 95 + RANDOM_LONG(0,34);

	fvol = RANDOM_FLOAT(0.75, 1.0);

	if (material == matComputer && RANDOM_LONG(0,1))
		material = matMetal;

	switch (material)
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
		rgpsz[0] = "debris/concrete1.wav";
		rgpsz[1] = "debris/concrete2.wav";
		rgpsz[2] = "debris/concrete3.wav";
		i = 3;
		break;
	}

	if (i)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, rgpsz[RANDOM_LONG(0,i-1)], fvol, ATTN_NORM, 0, pitch);
}

void CProp::Die( void )
{
	Vector vecSpot;// shard origin
	Vector vecVelocity;// shard velocity
	char cFlag = 0;
	int pitch;
	float fvol;

	pitch = 95 + RANDOM_LONG(0,29);

	if (pitch > 97 && pitch < 103)
		pitch = 100;

	// The more negative pev->health, the louder
	// the sound should be.

	fvol = RANDOM_FLOAT(0.85, 1.0) + (fabs(pev->health) / 100.0);

	if (fvol > 1.0)
		fvol = 1.0;


	switch (m_Material)
	{
	case matGlass:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass1.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustglass2.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		}
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate1.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustcrate2.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		}
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
	case matMetal:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		}
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh1.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustflesh2.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		}
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		switch ( RANDOM_LONG(0,1) )
		{
		case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustconcrete1.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustconcrete2.wav", fvol, ATTN_NORM, 0, pitch);
			break;
		}
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustceiling.wav", fvol, ATTN_NORM, 0, pitch);
		break;
	}


	if (m_Explosion == expDirected)
		vecVelocity = g_vecAttackDir * 200;
	else
	{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
	}

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
		WRITE_BYTE( TE_BREAKMODEL);

		// position
		WRITE_COORD( vecSpot.x );
		WRITE_COORD( vecSpot.y );
		WRITE_COORD( vecSpot.z );

		// size
		WRITE_COORD( pev->size.x);
		WRITE_COORD( pev->size.y);
		WRITE_COORD( pev->size.z);

		// velocity
		WRITE_COORD( vecVelocity.x );
		WRITE_COORD( vecVelocity.y );
		WRITE_COORD( vecVelocity.z );

		// randomization
		WRITE_BYTE( 8 );

		// Model
		WRITE_SHORT( m_idShard );	//model id#

		// # of shards
		WRITE_BYTE( 50 );	// let client decide

		// duration
		WRITE_BYTE( 250 );// 2.5 seconds

		// flags
		WRITE_BYTE( cFlag );
	MESSAGE_END();

	float size = pev->size.x;
	if ( size < pev->size.y )
		size = pev->size.y;
	if ( size < pev->size.z )
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable -- should be enough
	CBaseEntity *pList[256];
	int count = UTIL_EntitiesInBox( pList, 256, mins, maxs, FL_ONGROUND );
	if ( count )
	{
		for ( int i = 0; i < count; i++ )
		{
			ClearBits( pList[i]->pev->flags, FL_ONGROUND );
			pList[i]->pev->groundentity = NULL;
		}
	}

	// Don't fire something that could fire myself
	pev->targetname = 0;

	pev->solid = SOLID_NOT;
	// Fire targets on break
	SUB_UseTargets( NULL, USE_TOGGLE, 0 );

	if ( Explodable() && (m_attacker) )
	{
		ExplosionCreate( pev->origin, pev->angles, m_attacker->edict(), ExplosionMagnitude(), FALSE );
		RadiusDamage ( pev->origin, pev, m_attacker->pev, ExplosionMagnitude(), ExplosionMagnitude() * 2.5 , CLASS_NONE, DMG_BLAST );
	}
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
}

void CProp::Killed(entvars_t *pevAttacker, int iGib)
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	pev->nextthink = gpGlobals->time + m_flRespawnTime;
	SetThink( &CProp::RespawnThink );
	SetTouch( NULL );
	SetUse( NULL );
	//m_owner2 = NULL;
	//m_attacker = NULL;
}

void CProp::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if( pev->health <= 0)
		return;
	pev->movetype = MOVETYPE_BOUNCE;
	if (m_owner2 != pActivator)
	{
		if (pev->velocity.Length() < 100 && pActivator->IsPlayer())
		{
			m_owner2 = m_attacker = pActivator;
		}
		else
			return;
	}
	if( pActivator->IsPlayer() )
	{
		m_pHolstered = pActivator;
		CBasePlayer *player = (CBasePlayer*)pActivator;
		if( player )
		{

			if ( player->m_pActiveItem )
			{
				CBasePlayerWeapon *weapon = (CBasePlayerWeapon *) player->m_pActiveItem->GetWeaponPtr();


				//m_Holstered->m_pActiveItem->Holster(); // strange bug here. ValveWHY?

				// HACK: prevent attack
				if( weapon )
				{
					weapon->m_flNextPrimaryAttack += 0.1;
					weapon->m_flNextSecondaryAttack += 0.1;
				}
				player->m_iHideHUD |= HIDEHUD_WEAPONS;
				player->pev->weaponmodel = 0;
				player->pev->viewmodel = 0;
			}
			SetThink( &CProp::DeployThink );
			pev->nextthink = gpGlobals->time + 0.2;
		}
	}
	Vector target = pActivator->pev->origin + UTIL_GetAimVector(m_owner2->edict(), 1000) * 50;
	target.z = target.z + 32;
	pev->velocity = (target - VecBModelOrigin(pev)) * 10;
	Vector atarget = UTIL_VecToAngles(UTIL_GetAimVector(m_owner2->edict(), 1000));
	pev->angles.x = UTIL_AngleMod(pev->angles.x);
	pev->angles.y = UTIL_AngleMod(pev->angles.y);
	pev->angles.z = UTIL_AngleMod(pev->angles.z);
	atarget.x = UTIL_AngleMod(atarget.x);
	atarget.y = UTIL_AngleMod(atarget.y);
	atarget.z = UTIL_AngleMod(atarget.z);
	pev->avelocity.x = UTIL_AngleDiff(atarget.x, pev->angles.x) * 10;
	pev->avelocity.y = UTIL_AngleDiff(atarget.y, pev->angles.y) * 10;
	pev->avelocity.z = UTIL_AngleDiff(atarget.z, pev->angles.z) * 10;
	//pev->angles.z += (0 - pev->angles.z) * 0.06;
	if ((pActivator->pev->button & (IN_ATTACK)))
	{
		pev->velocity = UTIL_GetAimVector(m_owner2->edict(), 1000) * 1000;
		pev->avelocity.y = pev->avelocity.y*1.5 + RANDOM_FLOAT(100, -100);
		pev->avelocity.x = pev->avelocity.x*1.5 + RANDOM_FLOAT(100, -100);
		//pev->avelocity.z = pev->avelocity.z*0.5 + RANDOM_FLOAT ( 100, -100 );
	}
	if ((pActivator->pev->button & (IN_ATTACK2)))
	{
		//m_Horizontal = false;
		//pev->angles.z = 0;
	}
	//	m_Horizontal = (fabs(UTIL_AngleDiff(pev->angles.z, 90)) < 20) || ( sin(pev->angles.x/180*M_PI) > 0.1);
	//	CheckRotate();
	//ALERT( at_console, "Prop use!\n");
}

void CProp::Force(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if( pev->health <= 0 )
		return;
	if (m_owner2 != pActivator)
	{
		if (pev->velocity.Length() < 100 && pActivator->IsPlayer())
			m_attacker = pActivator;
		else
			return;
	}

	if ((pActivator->pev->button & (IN_ATTACK)))
	{
		pev->velocity = UTIL_GetAimVector(m_owner2->edict(), 3000) * 1000;
		pev->avelocity.y = pev->avelocity.y*1.5 + RANDOM_FLOAT(100, -100);
		pev->avelocity.x = pev->avelocity.x*1.5 + RANDOM_FLOAT(100, -100);
		//pev->avelocity.z = pev->avelocity.z*0.5 + RANDOM_FLOAT ( 100, -100 );
	}
	if ((pActivator->pev->button & (IN_ATTACK2)))
	{
		//m_Horizontal = false;
		//pev->angles.z = 0;
	}

	pev->nextthink = gpGlobals->time + m_flRespawnTime;
	SetThink( &CProp::RespawnThink);
}

static int moveupcounter;
// Arrange entities that stucks prop
void UTIL_PropMoveUp( entvars_t *pev, float amount = 11 )
{
	if( moveupcounter > 512 )
		return;
	if( pev->solid != SOLID_BBOX && pev->solid != SOLID_SLIDEBOX )
		return;
	if( pev->movetype != MOVETYPE_BOUNCE && pev->movetype != MOVETYPE_TOSS )
		return;

	if( amount <= 0 )
		return;
	if( pev->size.z > 100 )
		return;

	moveupcounter++;
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;

	mins.z = pev->absmax.z;
	maxs.z += 10;

	if( !CBaseEntity::Instance( pev )->IsPlayer() )
	{
	CBaseEntity *pList[32];
	int count = UTIL_EntitiesInBox( pList, 32, mins, maxs, FL_ONGROUND );
	for( int i = 0; i < count; i++ )
		if( pList[i]->pev != pev && pList[i]->pev->absmin.z > pev->absmin.z )
			UTIL_PropMoveUp( pList[i]->pev,pev->absmax.z - pList[i]->pev->absmin.z + 1 );
	}
	pev->origin.z += amount;
	UTIL_SetOrigin( pev, pev->origin );
}

void CProp::CheckRotate()
{
	if( m_shape != SHAPE_CYL_H && m_shape != SHAPE_CYL_V )
	{
		UTIL_SetSize(pev, minsH, maxsH);
		return;
	}
	if( (fabs(UTIL_AngleDiff(pev->angles.z, 90)) < 20) ||
		(fabs(sin(pev->angles.x / 180 * M_PI)) > 0.3) )
		m_shape = SHAPE_CYL_H;
	else
		m_shape = SHAPE_CYL_V;

	if (m_oldshape != m_shape)
	{

		if (m_shape == SHAPE_CYL_H)
		{
			pev->angles.y += 90;

			ALERT(at_console, "setH: %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z);

			UTIL_SetSize(pev, minsH, maxsH);
		}
		else if (m_shape == SHAPE_CYL_V)
		{

			ALERT(at_console, "setV: %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z);

			moveupcounter = 0;
			UTIL_PropMoveUp(pev);
			UTIL_SetSize(pev, minsV, maxsV);
		}
		//DROP_TO_FLOOR(edict());
		//pev->origin.z += 0.5;
		m_oldshape = m_shape;
	}
}

void CProp::DeployThink( void )
{
	if( m_pHolstered )
	{
		CBasePlayer *player = (CBasePlayer *)(CBaseEntity*)m_pHolstered;
		if( player->m_pActiveItem )
		{
			player->m_pActiveItem->Deploy();
			CBasePlayerWeapon *weapon = (CBasePlayerWeapon *) player->m_pActiveItem->GetWeaponPtr();
			if( weapon )
			{
				weapon->m_flNextPrimaryAttack = 0;
				weapon->m_flNextSecondaryAttack = 0;
			}
		}
		player ->m_iHideHUD &= ~HIDEHUD_WEAPONS;
		m_pHolstered = NULL;
	}
	if( m_pfnThink == &CProp::DeployThink )
	{
		pev->nextthink = gpGlobals->time + m_flRespawnTime;
		SetThink( &CProp::RespawnThink );
	}
}

void CProp::BounceTouch(CBaseEntity *pOther)
{
	if( pev->health <= 0 )
	{
		DieThink();
		return;
	}
	pev->movetype = MOVETYPE_BOUNCE;
	if( gpGlobals->time - m_flTouchTimer < 1 && m_iTouchCounter > 100 )
		{
			if( pOther->pev->solid == SOLID_BBOX || pOther->pev->solid == SOLID_SLIDEBOX )
			{
				moveupcounter = 0;
				UTIL_PropMoveUp( pOther->pev, 1.5 );
				//if( Intersects( pOther ) )
					//pOther->pev->origin.z += pOther->pev->size.z;
				pev->velocity = pev->avelocity = g_vecZero;
				UTIL_SetOrigin( pOther->pev, pOther->pev->origin );

				m_iTouchCounter = 0;
				return;
			}
			m_iTouchCounter = 0;
		}
	m_flTouchTimer = gpGlobals->time;
	if( !pOther->IsPlayer() )
		m_iTouchCounter++;
	//ALERT( at_console, "BounceTouch: %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );
	// only do damage if we're moving fairly fast
	DeployThink();

	if ( m_flNextAttack < gpGlobals->time && pev->velocity.Length() > 300 && pev->absmin.z - pOther->pev->absmin.z > 1 )
	{
		if( !m_attacker )
			m_attacker = this;
		entvars_t *pevOwner = m_attacker->pev;
		if (pevOwner)
		{
			float dmg = 50 + pev->velocity.Length() / 40;
			if (pOther == m_owner2)
			{
				dmg = 5;
				if (pOther->pev->button & (IN_USE))
				{
					dmg = 1;
				}
			}
			TraceResult tr = UTIL_GetGlobalTrace();
			ClearMultiDamage();
			pOther->TraceAttack(pevOwner, dmg, gpGlobals->v_forward, &tr, DMG_CLUB);
			ApplyMultiDamage(pev, pevOwner);
		}
		m_flNextAttack = gpGlobals->time + 1.0; // debounce
	}
	if( (pOther != m_owner2) && (pev->spawnflags & SF_PROP_BREAKABLE) && (pev->velocity.Length() > 1800) )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CProp::DieThink );
	}

	pev->velocity = pev->velocity + pOther->pev->velocity;
	float dp = cos(M_PI / 180 * UTIL_AngleDiff(UTIL_VecToAngles(pev->velocity).y, pev->angles.y));
	if (pev->flags & FL_ONGROUND || fabs(pev->velocity.z) < 40)
	{
		CheckRotate();
		if (m_shape == SHAPE_CYL_H)
		{

			pev->velocity.x *= fabs(dp) * 0.8 + 0.2;
			pev->velocity.y *= fabs(dp) * 0.8 + 0.2;
			pev->velocity.z -= 20;
			pev->avelocity.x = -dp*pev->velocity.Length()* 1.5;
			pev->avelocity.y = 0;
			pev->avelocity.z = 0;
			pev->angles.z += UTIL_AngleDiff(90, pev->angles.z) * 0.7;
			//AngleThink();
		}
		else if (m_shape == SHAPE_CYL_V)
		{
			// pev->angles.z *= 0.3;
			//pev->angles.x *= 0.3;
			//AngleThink();
			//CheckRotate();
			pev->velocity.z *= m_flFloorFriction;
			pev->velocity.x *= m_flFloorFriction;
			pev->velocity.y *= m_flFloorFriction;
			pev->velocity.z -= 10;
			pev->avelocity.y = pev->avelocity.y*0.4 + RANDOM_FLOAT(30, -30);
		}
		else if( m_shape == SHAPE_SPHERE )
		{
			pev->velocity.z -= 20;
			pev->avelocity.x = -cos(M_PI / 180 * UTIL_AngleDiff(UTIL_VecToAngles(pev->velocity).y, pev->angles.y))*pev->velocity.Length()* 1.5;
			pev->avelocity.y =  -sin(M_PI / 180 * UTIL_AngleDiff(UTIL_VecToAngles(pev->velocity).y, pev->angles.y))*pev->velocity.Length()* 1.5;;
			pev->avelocity.z = 0;
		}
		else if( m_shape == SHAPE_BOX || m_shape == SHAPE_GENERIC )
		{
			pev->velocity.z *= m_flFloorFriction;
			pev->velocity.x *= m_flFloorFriction;
			pev->velocity.y *= m_flFloorFriction;
			pev->velocity.z -= 10;
		}

	}
	else
	{
		{
			pev->velocity.z *= 0.3;
			pev->velocity.y *= m_flCollideFriction;
			pev->velocity.x *= m_flCollideFriction;
			if( m_shape != SHAPE_SPHERE )
			{
				pev->avelocity.y = pev->avelocity.y*0.4 + RANDOM_FLOAT(100, -100);
				pev->avelocity.x = pev->avelocity.x*0.5 + RANDOM_FLOAT(100, -100);
			}
		}
		//pev->avelocity.z = pev->avelocity.z*0.5 + RANDOM_FLOAT ( 1, -1 );
		BounceSound();
	}
	pev->framerate = pev->velocity.Length() / 200.0;
	if (pev->framerate > 1.0)
		pev->framerate = 1;
	else if (pev->framerate < 0.2)
	{
		CheckRotate();
		AngleThink();
		if (pev->angles.z == 0 || pev->angles.z == 90)
			pev->framerate = 0;
		else
			pev->framerate = 0.2;
	}
}

void CProp::BounceSound(void)
{
	MaterialSoundRandom( edict(), m_Material, 0.2 );
}

void CProp::Spawn(void)
{

	Precache();

	if( minsH == g_vecZero )
	{
		// default barrel parameters
		minsV = Vector(-10, -10, -17);
		maxsV = Vector(10, 10, 18);
		minsH = Vector(-10, -10, -10);
		maxsH = Vector(10, 10, 13);
	}
	m_flCollideFriction = 0.7;
	m_flFloorFriction = 0.5;
	if( !(pev->spawnflags & SF_PROP_FIXED ) )
	{
		UTIL_SetSize( pev, minsH, maxsH );
		if( m_shape < 3 )
			CheckRotate();

		UTIL_SetOrigin( pev, pev->origin );

		DROP_TO_FLOOR( edict() );
	}
	spawnOrigin = pev->origin;
	spawnAngles = pev->angles;
	m_flSpawnHealth = pev->health;
	if( m_flSpawnHealth <= 0 )
		m_flSpawnHealth = 30;
	if( !m_flRespawnTime )
		m_flRespawnTime = 80;
	pev->dmg = 100;
	PropRespawn();
}

void CProp::PropRespawn()
{
	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0) );
	pev->effects &= ~EF_NODRAW;
	if( pev->spawnflags & SF_PROP_FIXED )
		pev->movetype = MOVETYPE_NONE;
	else
		pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = m_flSpawnHealth;
	pev->velocity = pev->avelocity = g_vecZero;
	pev->angles = spawnAngles;
	pev->deadflag = DEAD_NO;
	SET_MODEL( ENT(pev), STRING(pev->model) );
	m_oldshape = (PropShape)-1;
	CheckRotate();
	SetTouch( &CProp::BounceTouch );
	SetUse( &CProp::Use );

	pev->framerate = 1.0f;
	UTIL_SetOrigin( pev, spawnOrigin );
	m_owner2 = NULL;
	m_attacker = NULL;
}

void CProp::RespawnThink()
{
	if( !(pev->spawnflags & SF_PROP_RESPAWN))
	{
		if( pev->health <= 0 )
		{
			pev->nextthink = gpGlobals->time + 0.1;
			SetThink( &CBaseEntity::SUB_Remove );
		}
		return;
	}
	PropRespawn();
}



void CProp::AngleThink()
{
	pev->nextthink = gpGlobals->time + m_flRespawnTime;
	SetThink( &CProp::RespawnThink);
	if (!(pev->flags & FL_ONGROUND || fabs(pev->velocity.z) < 40))
	{
		m_owner2 = m_attacker = 0;
		return;
	}
	if (m_shape == SHAPE_CYL_H)
	{
		pev->angles.z += UTIL_AngleDiff(90, pev->angles.z) * 0.7;
		if (fabs(UTIL_AngleDiff(90, pev->angles.z)) > 0.1)
		{
			SetThink( &CProp::AngleThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		//ALERT( at_console, "AngleThink: %f %f %f\n", pev->angles.x, pev->angles.y, pev->angles.z );
		pev->avelocity.y = pev->avelocity.z = 0;
	}
	else if (m_shape == SHAPE_CYL_V)
	{
		if (fabs(UTIL_AngleDiff(90, pev->angles.z)) > 0.1)
		{
			SetThink( &CProp::AngleThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		pev->angles.z += UTIL_AngleDiff(0, pev->angles.z) * 0.7;
		//pev->angles.x += UTIL_AngleDiff( 0, pev->angles.x ) * 0.3;
		pev->avelocity.x = pev->avelocity.y = pev->avelocity.z = 0;
	}
	else if (m_shape == SHAPE_BOX)
	{
		Vector iangles;
		iangles.x = myround( pev->angles.x / 90 ) * 90;
		iangles.y = myround( pev->angles.y / 90 ) * 90;
		iangles.z = myround( pev->angles.z / 90 ) * 90;
		if (fabs(UTIL_AngleDiff(iangles.x, pev->angles.x)) > 0.1 ||
			//fabs(UTIL_AngleDiff(iangles.y, pev->angles.y)) > 0.1 ||
			fabs(UTIL_AngleDiff(iangles.z, pev->angles.z)) > 0.1)
		{
			SetThink( &CProp::AngleThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		pev->angles.x += UTIL_AngleDiff(iangles.x, pev->angles.x) * 0.6;
		//pev->angles.y += UTIL_AngleDiff(iangles.y, pev->angles.y) * 0.6;
		pev->angles.z += UTIL_AngleDiff(iangles.z, pev->angles.z) * 0.6;

		pev->avelocity.x = pev->avelocity.y = pev->avelocity.z = 0;
	}
	else if (m_shape == SHAPE_NOROTATE)
	{
		pev->avelocity.x = pev->avelocity.y = pev->avelocity.z = 0;
		Vector iangles = spawnAngles;
		if (fabs(UTIL_AngleDiff(iangles.x, pev->angles.x)) > 0.1 ||
			fabs(UTIL_AngleDiff(iangles.y, pev->angles.y)) > 0.1 ||
			fabs(UTIL_AngleDiff(iangles.z, pev->angles.z)) > 0.1)
		{
			SetThink( &CProp::AngleThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		pev->angles.x += UTIL_AngleDiff(iangles.x, pev->angles.x) * 0.6;
		pev->angles.y += UTIL_AngleDiff(iangles.y, pev->angles.y) * 0.6;
		pev->angles.z += UTIL_AngleDiff(iangles.z, pev->angles.z) * 0.6;
	}
	else if (m_shape == SHAPE_GENERIC)
	{
		float ianglex = 0, ianglez = 0, imaxanglediff=360.0f;
		int i;
		//  if first number is zero, it is angle
		// all other zeroes is array end
		for( i = 0; (i < 10) && ( (i == 0 ) || m_iaCustomAnglesX[i] ); i++)
		{
			float anglediff = fabs(UTIL_AngleDiff(pev->angles.x, m_iaCustomAnglesX[i]));
			if( imaxanglediff > anglediff )
			{
				ianglex = m_iaCustomAnglesX[i];
				imaxanglediff = anglediff;
			}
		}
		imaxanglediff=360.0f;
		for( i = 0; (i < 10) && ( (i == 0 ) || m_iaCustomAnglesZ[i] ); i++)
		{
			float anglediff = fabs(UTIL_AngleDiff(pev->angles.z, m_iaCustomAnglesZ[i]));
			if( imaxanglediff > anglediff )
			{
				ianglez = m_iaCustomAnglesZ[i];
				imaxanglediff = anglediff;
			}
		}
		if (fabs(UTIL_AngleDiff(ianglex, pev->angles.x)) > 0.1 ||
			fabs(UTIL_AngleDiff(ianglez, pev->angles.z)) > 0.1 )
		{
			SetThink( &CProp::AngleThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		pev->angles.x += UTIL_AngleDiff(ianglex, pev->angles.x) * 0.6;
		pev->angles.z += UTIL_AngleDiff(ianglez, pev->angles.z) * 0.6;
		pev->avelocity.x = pev->avelocity.y = pev->avelocity.z = 0;
	}
	pev->angles.x = UTIL_AngleMod(pev->angles.x);
	pev->angles.y = UTIL_AngleMod(pev->angles.y);
	pev->angles.z = UTIL_AngleMod(pev->angles.z);
}

void CProp::DieThink()
{
	Killed( VARS(m_attacker), GIB_NORMAL );
	Die();
}

int CProp::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Vector r = (pevInflictor->origin - pev->origin);
	if ( (!m_attacker
		  || (pev->velocity.Length() < 700)) && ((CBaseEntity*)GET_PRIVATE(ENT(pevAttacker)))
		 && ((CBaseEntity*)GET_PRIVATE(ENT(pevAttacker)))->IsPlayer())
		m_attacker.Set(ENT(pevAttacker));
	DeployThink();

	pev->velocity = r * flDamage / -7;
	pev->avelocity.x = pev->avelocity.x*0.5 + RANDOM_FLOAT(100, -100);
	ALERT(at_console, "Takedmg: %s %s %f %f\n", STRING(pevInflictor->classname), STRING(pevAttacker->classname), flDamage, pev->health );

	// now some func_breakable code

	if ( !(pev->spawnflags & SF_PROP_BREAKABLE ) )
		return 0;
	if ( pev->health <= 0 )
		return 0;
	// Breakables take double damage from the crowbar
	if ( bitsDamageType & DMG_CLUB )
		flDamage *= 2;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if ( bitsDamageType & DMG_POISON )
		flDamage *= 0.1;
	g_vecAttackDir = r.Normalize();

	// do the damage
	pev->health -= flDamage;
	if ( pev->health <= 0 )
	{
		// delayed explode
		SetThink( &CProp::DieThink );
		pev->nextthink = gpGlobals->time + 0.2;
		return 0;
	}

	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if cbreakable actually died.

	DamageSound();
	return 1;
}
void CProp::KeyValue( KeyValueData* pkvd )
{
	ALERT( at_console, "%s %s\n", pkvd->szKeyName, pkvd->szValue);
	// UNDONE_WC: explicitly ignoring these fields, but they shouldn't be in the map file!
	if (FStrEq(pkvd->szKeyName, "explosion"))
	{
		if (!stricmp(pkvd->szValue, "directed"))
			m_Explosion = expDirected;
		else if (!stricmp(pkvd->szValue, "random"))
			m_Explosion = expRandom;
		else
			m_Explosion = expRandom;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "material"))
	{
		int i = atoi( pkvd->szValue);

		// 0:glass, 1:metal, 2:flesh, 3:wood

		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matWood;
		else
			m_Material = (Materials)i;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "shape"))
	{
		int i = atoi( pkvd->szValue);

		if ((i < 0) || (i >= SHAPE_NOROTATE))
			m_shape = SHAPE_NOROTATE;
		else
			m_shape = (PropShape)i;

		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "gibmodel") )
	{
		m_iszGibModel = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "explodemagnitude") )
	{
		ExplosionSetMagnitude( atoi( pkvd->szValue ) );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "respawntime") )
	{
		m_flRespawnTime = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customanglesx"))
	{
		UTIL_StringToIntArray( m_iaCustomAnglesX, ARRAYSIZE( m_iaCustomAnglesX ), pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "customanglesz"))
	{
		UTIL_StringToIntArray( m_iaCustomAnglesZ, ARRAYSIZE( m_iaCustomAnglesZ ), pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "hmin"))
	{
		UTIL_StringToVector( minsH, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "hmax"))
	{
		UTIL_StringToVector( maxsH, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "vmin"))
	{
		UTIL_StringToVector( minsV, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "vmax"))
	{
		UTIL_StringToVector( maxsV, pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}
#include <stdio.h>
void DumpProps()
{
	edict_t		*pEdict = g_engfuncs.pfnPEntityOfEntIndex( 1 );
	FILE *f = fopen( "props.ent", "wb" );

	if( !f )
		return;

	for ( int j = 1; j < gpGlobals->maxEntities; j++, pEdict++ )
	{
		if ( pEdict->free )	// Not in use
			continue;

		if ( strcmp( STRING( pEdict->v.classname ), "prop" ) )
			 continue;
		CProp *prop = (CProp *)CBaseEntity::Instance(pEdict);

		fprintf( f, "{\n");
		fprintf( f, "\"classname\" \"prop\"\n");
		if( pEdict->v.model )
			fprintf( f, "\"model\" \"%s\"\n", STRING( pEdict->v.model ));
		fprintf( f, "\"spawnflags\" \"%d\"\n", pEdict->v.spawnflags );
		fprintf( f, "\"angles\" \"%d %d %d\"\n", (int)pEdict->v.angles.x, (int)pEdict->v.angles.y, (int)pEdict->v.angles.z );
		fprintf( f, "\"origin\" \"%d %d %d\"\n", (int)pEdict->v.origin.x, (int)pEdict->v.origin.y, (int)pEdict->v.origin.z );
		if( prop->m_shape != 1 )
			fprintf( f, "\"shape\" \"%d\"\n", (int)(((int)prop->m_shape) > 1? (int)(prop->m_shape): (int)(!(int)prop->m_shape)) );
		fprintf( f, "\"hmin\" \"%d %d %d\"\n", (int)prop->minsH.x, (int)prop->minsH.y, (int)prop->minsH.z );
		fprintf( f, "\"hmax\" \"%d %d %d\"\n", (int)prop->maxsH.x, (int)prop->maxsH.y, (int)prop->maxsH.z );
		if( prop->m_shape > 1 )
		{
			fprintf( f, "\"vmin\" \"%d %d %d\"\n", (int)prop->minsV.x, (int)prop->minsV.y, (int)prop->minsV.z );
			fprintf( f, "\"vmax\" \"%d %d %d\"\n", (int)prop->maxsV.x, (int)prop->maxsV.y, (int)prop->maxsV.z );
		}
		if( prop->m_Material )
			fprintf( f, "\"material\" \"%d\"\n", prop->m_Material );
		if( pEdict->v.health != 30 )
			fprintf( f, "\"health\" \"%d\"\n", (int)pEdict->v.health );
		if( prop->m_shape == SHAPE_GENERIC)
		{
			int i;
			if( !( !prop->m_iaCustomAnglesX[0] && !prop->m_iaCustomAnglesX[1] ) )
			{
				fprintf( f, "\"customanglesx\" \"%d", prop->m_iaCustomAnglesX[0] );

				for( i = 1; (i < 10) && ( prop->m_iaCustomAnglesX[i] ); i++ )
					fprintf( f, " %d", prop->m_iaCustomAnglesX[i] );

				fprintf( f, "\"\n" );
			}
			if( !( !prop->m_iaCustomAnglesZ[0] && !prop->m_iaCustomAnglesZ[1] ) )
			{
				fprintf( f, "\"customanglesz\" \"%d", prop->m_iaCustomAnglesZ[0] );

				for( i = 1; (i < 10) && ( prop->m_iaCustomAnglesZ[i] ); i++ )
					fprintf( f, " %d", prop->m_iaCustomAnglesZ[i] );

				fprintf( f, "\"\n" );
			}
		}
		if( prop->m_Explosion == expDirected )
			fprintf( f, "\"explosion\" \"directed\"\n" );

		if( prop->m_iszGibModel )
			fprintf( f, "\"gibmodel\" \"%s\"\n", STRING( prop->m_iszGibModel ) );

		if( pEdict->v.impulse )
			fprintf( f, "\"impulse\" \"%d\"\n", pEdict->v.impulse );
		fprintf( f, "}\n" );
	}
}
