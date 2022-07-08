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
/*

===== explode.cpp ========================================================

  Explosion-related code

*/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "decals.h"
#include "explode.h"

// Spark Shower
class CShower : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	void Touch( CBaseEntity *pOther );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
};

LINK_ENTITY_TO_CLASS( spark_shower, CShower )

void CShower::Spawn( void )
{
	pev->velocity = RANDOM_FLOAT( 200.0f, 300.0f ) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT( -100.0f, 100.0f );
	pev->velocity.y += RANDOM_FLOAT( -100.0f, 100.0f );
	if( pev->velocity.z >= 0 )
		pev->velocity.z += 200.0f;
	else
		pev->velocity.z -= 200.0f;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl" );	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5f, 1.5f );

	pev->angles = g_vecZero;
}

void CShower::Think( void )
{
	UTIL_Sparks( pev->origin );

	pev->speed -= 0.1f;
	if( pev->speed > 0.0f )
		pev->nextthink = gpGlobals->time + 0.1f;
	else
		UTIL_Remove( this );
	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch( CBaseEntity *pOther )
{
	if( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.1f;
	else
		pev->velocity = pev->velocity * 0.6f;

	if( ( pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y ) < 10.0f )
		pev->speed = 0.0f;
}

class CEnvExplosion : public CBaseMonster
{
public:
	void Spawn();
	void EXPORT Smoke( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude;// how large is the fireball? how much damage?
	int m_spriteScale; // what's the exact fireball sprite scale? 


	//modif de Julien
	virtual BOOL IsInGaz ( void );
	void	EXPORT ExplodeDelay ( void );

};

TYPEDESCRIPTION	CEnvExplosion::m_SaveData[] =
{
	DEFINE_FIELD( CEnvExplosion, m_iMagnitude, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_spriteScale, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CEnvExplosion, CBaseMonster )
LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion )

void CEnvExplosion::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		m_iMagnitude = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CEnvExplosion::Spawn( void )
{ 

	// modif de Julien
	if ( FStrEq(STRING(gpGlobals->mapname), "l3m2") || FStrEq(STRING(gpGlobals->mapname), "L3M2") )
	{
		SetThink ( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time +0.1;
		return;
	}

	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;
	/*
	if( m_iMagnitude > 250 )
	{
		m_iMagnitude = 250;
	}
	*/

	float flSpriteScale;
	flSpriteScale = ( m_iMagnitude - 50 ) * 0.6f;

	/*
	if( flSpriteScale > 50.0f )
	{
		flSpriteScale = 50.0f;
	}
	*/
	if( flSpriteScale < 10.0f )
	{
		flSpriteScale = 10.0f;
	}

	m_spriteScale = (int)flSpriteScale;
}

void CEnvExplosion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	TraceResult tr;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	Vector vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector( 0.0f, 0.0f, 8.0f );

	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0.0f, 0.0f, -40.0f ),  ignore_monsters, ENT( pev ), &tr );

	// Pull out of the wall a bit
	if( tr.flFraction != 1.0f )
	{
		pev->origin = tr.vecEndPos + ( tr.vecPlaneNormal * ( m_iMagnitude - 24 ) * 0.6f );
	}
	else
	{
		pev->origin = pev->origin;
	}

	// draw decal
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NODECAL ) )
	{
		if( RANDOM_FLOAT( 0.0f, 1.0f ) < 0.5f )
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		}
		else
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
		}
	}


	// draw fireball
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15 ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
	}

	// do damage
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		// modif de Julien
		if ( pev->owner != NULL )
			RadiusDamage ( pev, VARS(pev->owner), m_iMagnitude, CLASS_NONE, DMG_BLAST );
		else
			RadiusDamage ( pev, pev, m_iMagnitude, CLASS_NONE, DMG_BLAST );
	}

	SetThink( &CEnvExplosion::Smoke );
	pev->nextthink = gpGlobals->time + 0.3f;

	// draw sparks
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );

		for( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}

	//modif de Julien
	if ( IsInGaz () )
	{
		edict_t *pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "player" );
		CBaseEntity *pPlayer = CBaseEntity::Instance ( pFind );
		pPlayer->m_bFireInGaz = TRUE;
	}

}

void CEnvExplosion::Smoke( void )
{
	if( !( pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexSmoke );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 12 ); // framerate
		MESSAGE_END();
	}

	if( !( pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE ) )
	{
		UTIL_Remove( this );
	}
}


//modif de Julien
//=========================================
//	pour les trigger_gaz
//=========================================

BOOL CEnvExplosion::IsInGaz ( void )
{
	Vector vecPlayer = Center ();

	CBaseEntity *pFind = NULL;
	edict_t *pTrigger = NULL;
	pTrigger = FIND_ENTITY_BY_CLASSNAME( NULL, "trigger_gaz" );

	while ( !FNullEnt ( pTrigger ) )
	{
		pFind = CBaseEntity::Instance ( pTrigger );

		if ( vecPlayer.x > pFind->pev->absmin.x && vecPlayer.x < pFind->pev->absmax.x &&
			 vecPlayer.y > pFind->pev->absmin.y && vecPlayer.y < pFind->pev->absmax.y &&
			 vecPlayer.z > pFind->pev->absmin.z && vecPlayer.z < pFind->pev->absmax.z )		// Player est ds le trigger
		{
			return TRUE;
		}
		else
		{
			pTrigger = FIND_ENTITY_BY_CLASSNAME( pTrigger, "trigger_gaz" );
		}

	}
	return FALSE;
}





// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate( const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, BOOL doDamage, float flDelay )
{
	KeyValueData kvd;
	char buf[128];

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, angles, pOwner );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	if( !doDamage )
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();


	// modif de Julien

	CEnvExplosion *pExp = (CEnvExplosion*) pExplosion; //blah-blah-blah ISO forbids...

	pExp->SetThink ( &CEnvExplosion::ExplodeDelay ); //instead of pExp->ExplodeDelay
	pExp->pev->nextthink = gpGlobals->time + flDelay;


	// modif de julien // pExplosion->Use( NULL, NULL, USE_TOGGLE, 0 );
}


// modif de julien
void CEnvExplosion :: ExplodeDelay ( void )
{
	Use( NULL, NULL, USE_TOGGLE, 0 );
}


//---------------------------------------------------------
// modif de julien


class CEnvShower : public CBaseEntity
{
	void Spawn( void );
	void EXPORT UseShower ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( env_sparkshower, CEnvShower );


void CEnvShower::Spawn( void )
{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;

	SetUse ( &CEnvShower::UseShower ); //lacked &CEnvShower:: part
}

void CEnvShower :: UseShower ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBaseEntity *pEntity = UTIL_FindEntityByTargetname ( NULL, STRING(pev->target) );

	if ( pEntity == NULL )
		return;

	int sparkCount = RANDOM_LONG(3,6);

	for ( int i = 0; i < sparkCount; i++ )
	{
		CBaseEntity *pSpark = Create( "spark_shower", pev->origin, (pEntity->pev->origin - pev->origin).Normalize(), NULL );
	}

}
