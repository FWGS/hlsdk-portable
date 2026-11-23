#include "extdll.h"
#include "util.h"
#include "cbase.h"

extern int gmsgParticle;
extern int gmsgPartState;

#define SF_PARTICLE_STARTON 0x0020

enum particle_type
{
	PARTICLE_SPRITE,
	PARTICLE_PLANE,
	PARTICLE_SPHERE,
	PARTICLE_CYLINDER,
	PARTICLE_DISK,
	PARTICLE_CONE,
	PARTICLE_WATERDROP
};

class CParticleEmitter : public CBaseEntity
{
public:
	CParticleEmitter();
	void KeyValue( KeyValueData* pkvd ) override;
	void Spawn() override;
	void Precache() override;
	void EXPORT UseToggle( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	bool UpdateOnClientConnect( edict_t* pEntity ) override;
	void Restart() override;

	int m_iszTargetOrigin;
	int m_iszTargetDirection;
	float m_iSpeed;
	int m_iNoise;
	int m_iCount;
	int m_iCountRand;
	float m_iFrequency;
	int m_iTexture;
	int m_iszTextureName;
	float m_flGravity;
	particle_type m_eType;
	float m_iScale;
	float m_iScaleSpeed;
	float m_fBrightness;
	float m_iFadeSpeed;
	float m_iLife;
	int m_iszVisPoint;
	int m_iOn;
	int m_iAttachmentPoint;
};

LINK_ENTITY_TO_CLASS( env_particle, CParticleEmitter );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CParticleEmitter::CParticleEmitter()
{
	m_iszVisPoint = 0;
	m_iAttachmentPoint = 0;
	m_fBrightness = 255;
}

void CParticleEmitter::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/null.mdl" );
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_iOn = 0;
	SetTouch( NULL );
	SetUse( &CParticleEmitter::UseToggle );
	if ( pev->spawnflags & SF_PARTICLE_STARTON )
		m_iOn = 1;
}

void CParticleEmitter::Precache()
{
	PRECACHE_MODEL( "models/null.mdl" );
	if ( m_iszTextureName )
		m_iTexture = PRECACHE_MODEL( (char*)STRING( m_iszTextureName ) );
}

void CParticleEmitter::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "target_origin" ) )
	{
		m_iszTargetOrigin = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "target_direction" ) )
	{
		m_iszTargetDirection = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_speed" ) )
	{
		m_iSpeed = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_noise" ) )
	{
		m_iNoise = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_count" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_brightness" ) )
	{
		m_fBrightness = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "rnd_count" ) )
	{
		m_iCountRand = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		m_iFrequency = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}

	if ( FStrEq( pkvd->szKeyName, "particle_texture" ) )
	{
		m_iszTextureName = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_gravity" ) )
	{
		m_flGravity = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_type" ) )
	{
		m_eType = (particle_type)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_scale" ) )
	{
		m_iScale = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "scale_speed" ) )
	{
		m_iScaleSpeed = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "fade_speed" ) )
	{
		m_iFadeSpeed = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_life" ) )
	{
		m_iLife = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "particle_avelocity" ) )
	{
		Vector velocity;
		UTIL_StringToVector( velocity, pkvd->szValue );
		if ( velocity != g_vecZero )
			pev->avelocity = velocity;
	}
	else if ( FStrEq( pkvd->szKeyName, "vis_point" ) )
	{
		m_iszVisPoint = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "attach_point" ) )
	{
		m_iAttachmentPoint = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CParticleEmitter::UseToggle( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	m_iOn = !m_iOn;
	MESSAGE_BEGIN( MSG_ALL, gmsgPartState );
		WRITE_SHORT( entindex() );
		WRITE_BYTE( m_iOn );
	MESSAGE_END();
}

void CParticleEmitter::Restart()
{
	m_iOn = ( pev->spawnflags & SF_PARTICLE_STARTON ) != 0;
	MESSAGE_BEGIN( MSG_ALL, gmsgPartState );
		WRITE_SHORT( entindex() );
		WRITE_BYTE( m_iOn );
	MESSAGE_END();
}

bool CParticleEmitter::UpdateOnClientConnect( edict_t* pEntity )
{
	int visIndex;
	const char* visibility = STRING( m_iszVisPoint );
	if ( m_iszVisPoint && !FStrEq( visibility, "none" ) && *visibility )
	{
		visIndex = 0;
		if ( !FStrEq( visibility, "player" ) )
		{
			CBaseEntity* pVisible = UTIL_FindEntityByTargetname( NULL, visibility );
			visIndex = pVisible ? pVisible->entindex() : entindex();
		}
	}
	else
		visIndex = entindex();

	int srcIndex = 1;
	const char* source = STRING( m_iszTargetOrigin );
	if ( !FStrEq( source, "player" ) )
	{
		CBaseEntity* pSource = NULL;
		if ( !FStrEq( source, "none" ) )
			pSource = UTIL_FindEntityByTargetname( NULL, source );
		srcIndex = pSource ? pSource->entindex() : entindex();
	}
	CBaseEntity* pDirection = NULL;
	const char* direction = STRING( m_iszTargetDirection );
	if ( FStrEq( direction, "player" ) )
		pDirection = CBaseEntity::Instance( INDEXENT( 1 ) );
	else if ( !FStrEq( direction, "none" ) )
		pDirection = UTIL_FindEntityByTargetname( NULL, direction );
	int directionIndex = pDirection ? pDirection->entindex() : 0;
	Vector color = pev->rendercolor;
	int brightness = (int)m_fBrightness;
	if ( !brightness )
		brightness = (int)pev->renderamt;
	MESSAGE_BEGIN( MSG_ONE, gmsgParticle, NULL, pEntity );
		WRITE_SHORT( m_iTexture );
		WRITE_BYTE( m_iCount );
		WRITE_SHORT( (int)m_iSpeed );
		WRITE_BYTE( m_iNoise );
		WRITE_BYTE( pev->rendermode );
		WRITE_SHORT( (int)m_flGravity );
		WRITE_SHORT( (int)m_iScale );
		WRITE_BYTE( (int)m_iScaleSpeed );
		WRITE_BYTE( brightness );
		WRITE_BYTE( (int)m_iFadeSpeed );
		WRITE_SHORT( (int)m_iLife );
		WRITE_BYTE( m_eType );
		WRITE_BYTE( pev->spawnflags );
		WRITE_COORD( pev->angles.x );
		WRITE_COORD( pev->angles.y );
		WRITE_COORD( pev->angles.z );
		WRITE_COORD( pev->avelocity.x );
		WRITE_COORD( pev->avelocity.y );
		WRITE_COORD( pev->avelocity.z );
		WRITE_BYTE( (int)color.x );
		WRITE_BYTE( (int)color.y );
		WRITE_BYTE( (int)color.z );
		WRITE_BYTE( m_iOn );
		WRITE_SHORT( (int)m_iFrequency );
		WRITE_BYTE( m_iCountRand );
		WRITE_SHORT( directionIndex );
		WRITE_SHORT( srcIndex );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( visIndex );
		WRITE_BYTE( m_iAttachmentPoint );
	MESSAGE_END();
	return true;
}

class CWaterDrop : public CParticleEmitter
{
public:
	void Precache() override;
	void KeyValue( KeyValueData* pkvd ) override;
	bool UpdateOnClientConnect( edict_t* pEntity ) override;
	int m_fParticles;
};

void CWaterDrop::Precache()
{
	PRECACHE_MODEL( "models/null.mdl" );
	PRECACHE_MODEL( "sprites/waterripple.spr" );
	m_iTexture = PRECACHE_MODEL( "sprites/snow_01.spr" );
}

void CWaterDrop::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "drop_size" ) )
	{
		m_iScale = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "splash_particles" ) )
	{
		m_fParticles = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "frequency" ) )
	{
		m_iFrequency = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CParticleEmitter::KeyValue( pkvd );
}

bool CWaterDrop::UpdateOnClientConnect( edict_t* pEntity )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgParticle, NULL, pEntity );
		WRITE_SHORT( m_iTexture );
		WRITE_BYTE( 1 );
		WRITE_SHORT( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( kRenderTransAlpha );
		WRITE_SHORT( 10 );
		WRITE_SHORT( (int)m_iScale );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 0 );
		WRITE_SHORT( 600 );
		WRITE_BYTE( PARTICLE_WATERDROP );
		WRITE_BYTE( pev->spawnflags | 1 );
		WRITE_COORD( pev->angles.x );
		WRITE_COORD( pev->angles.y );
		WRITE_COORD( pev->angles.z );
		WRITE_COORD( pev->avelocity.x );
		WRITE_COORD( pev->avelocity.y );
		WRITE_COORD( pev->avelocity.z );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( m_iOn );
		WRITE_SHORT( (int)m_iFrequency );
		WRITE_BYTE( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( 0 );
	MESSAGE_END();
	return true;
}
