/*** ****/
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
	void Spawn( void );
	void Think( void );
	void Touch( CBaseEntity *pOther );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
}; 

LINK_ENTITY_TO_CLASS( spark_shower, CShower );

void CShower::Spawn( void )
{
	pev->velocity = RANDOM_FLOAT( 200, 300 ) * pev->angles;
	pev->velocity.x += RANDOM_FLOAT(-100.f,100.f);
	pev->velocity.y += RANDOM_FLOAT(-100.f,100.f);
	if ( pev->velocity.z < 0 )
		pev->velocity.z -= 200;
	else
		pev->velocity.z += 200;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	SET_MODEL( edict(), "models/grenade.mdl");	// Need a model, just use the grenade, we don't draw it anyway
	UTIL_SetSize(pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;
	pev->speed = RANDOM_FLOAT( 0.5, 1.5 );

	pev->angles = g_vecZero;
}


void CShower::Think( void )
{
	UTIL_Sparks( pev->origin );

	pev->speed -= 0.1;
	if ( pev->speed <= 0 )
		UTIL_Remove( this );
	else
		pev->nextthink = gpGlobals->time + 0.1;
	pev->flags &= ~FL_ONGROUND;
}

void CShower::Touch( CBaseEntity *pOther )
{
	if ( pev->flags & FL_ONGROUND )
		pev->velocity = pev->velocity * 0.1;
	else
		pev->velocity = pev->velocity * 0.6;

	if ( (pev->velocity.x*pev->velocity.x+pev->velocity.y*pev->velocity.y) < 10.0 )
		pev->speed = 0;
}

class CEnvExplosion : public CBaseMonster
{
public:
	void Spawn( );
	void Precache();
	void EXPORT Smoke ( void );
	void KeyValue( KeyValueData *pkvd );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int m_iMagnitude = 0;
	int m_spriteScale = 0;
	int m_firespriteName = 0;
	int m_firesprite = 0;
	int m_smokespriteName = 0;
	int m_smokesprite = 0;
	int m_explosionFlags = 0;
	BOOL m_poisonExplosion = FALSE;
};

TYPEDESCRIPTION	CEnvExplosion::m_SaveData[] = 
{
	DEFINE_FIELD( CEnvExplosion, m_iMagnitude, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_spriteScale, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_firespriteName, FIELD_STRING ),
	DEFINE_FIELD( CEnvExplosion, m_smokespriteName, FIELD_STRING ),
	DEFINE_FIELD( CEnvExplosion, m_explosionFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvExplosion, m_poisonExplosion, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEnvExplosion, CBaseMonster );
LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion );

void CEnvExplosion::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		m_iMagnitude = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "spritescale" ) )
	{
		m_spriteScale = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "firesprite" ) )
	{
		m_firespriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "smokesprite" ) )
	{
		m_smokespriteName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "nosound" ) )
	{
		m_explosionFlags = TE_EXPLFLAG_NOSOUND;
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "poisongas" ) )
	{
		m_poisonExplosion = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CEnvExplosion::Precache()
{
	m_firesprite = m_firespriteName ? PRECACHE_MODEL( (char*)STRING( m_firespriteName ) ) : g_sModelIndexFireball;
	m_smokesprite = m_smokespriteName ? PRECACHE_MODEL( (char*)STRING( m_smokespriteName ) ) : g_sModelIndexSmoke;
}

void CEnvExplosion::Spawn( void )
{ 
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;

	pev->movetype = MOVETYPE_NONE;
	if ( m_spriteScale == 0 )
	{
		float flSpriteScale = ( m_iMagnitude - 50 ) * 0.6f;
		m_spriteScale = (int)flSpriteScale;
		if ( m_spriteScale < 0 )
			m_spriteScale = 10;
	}
	Precache();
}

void CEnvExplosion::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	if ( m_poisonExplosion )
	{
		unsigned short usExplosion = PRECACHE_EVENT( 1, "events/createexplo.sc" );
		PLAYBACK_EVENT_FULL( 0, NULL, usExplosion, 0, pev->origin, (float*)&g_vecZero, 0, 0, 0, 2, 1, 0 );
		return;
	}
	TraceResult tr;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);
	
	// Pull out of the wall a bit
	if ( tr.flFraction != 1.0 )
	{
		pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (m_iMagnitude - 24) * 0.6f);
	}
	else
	{
		pev->origin = pev->origin;
	}

	// draw decal
	if (! ( pev->spawnflags & SF_ENVEXPLOSION_NODECAL))
	{
		if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
		}
		else
		{
			UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
		}
	}

	// draw fireball
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOFIREBALL ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_firesprite );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( m_explosionFlags );
		MESSAGE_END();
	}
	else
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_firesprite );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( m_explosionFlags );
		MESSAGE_END();
	}

	// do damage
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NODAMAGE ) )
	{
		RadiusDamage ( pev, pev, m_iMagnitude, CLASS_NONE, DMG_BLAST );
	}

	SetThink( &CEnvExplosion::Smoke );
	pev->nextthink = gpGlobals->time + 0.3;

	// draw sparks
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSPARKS ) )
	{
		int sparkCount = RANDOM_LONG(0,3);

		for ( int i = 0; i < sparkCount; i++ )
		{
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
		}
	}
}

void CEnvExplosion::Smoke( void )
{
	if ( !( pev->spawnflags & SF_ENVEXPLOSION_NOSMOKE ) )
	{
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SMOKE );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_smokesprite );
			WRITE_BYTE( (BYTE)m_spriteScale ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
		MESSAGE_END();
	}
	
	if ( !(pev->spawnflags & SF_ENVEXPLOSION_REPEATABLE) )
	{
		UTIL_Remove( this );
	}
}


// HACKHACK -- create one of these and fake a keyvalue to get the right explosion setup
void ExplosionCreate( const Vector &center, const Vector &angles, edict_t *pOwner, int magnitude, BOOL doDamage,
	float spriteScale )
{
	KeyValueData	kvd;
	char			buf[128];

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, angles, pOwner );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	if ( spriteScale != -1 )
	{
		sprintf( buf, "%.2f", spriteScale );
		KeyValueData kvdScale;
		kvdScale.szKeyName = "spritescale";
		kvdScale.szValue = buf;
		pExplosion->KeyValue( &kvdScale );
	}
	if ( !doDamage )
		pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->Use( NULL, NULL, USE_TOGGLE, 0 );
}
