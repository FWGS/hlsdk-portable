#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"

#define SF_GENERICITEM_DROPTOFLOOR	1
#define SF_GENERICITEM_SOLID			2

class CGenericItem : public CBaseMonster
{
public:
	CGenericItem();
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void Restart() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void EXPORT StartItem();
	void EXPORT AnimateThink();

	static TYPEDESCRIPTION m_SaveData[];
	float	m_lastTime;
	int		m_iSequence;
	BOOL	m_bCanSpawn;
	int		m_iLightValue;
	BOOL	m_bSetPVSDefault;
	BOOL	m_removeOnUse;
	BOOL	m_nopvs;
};

LINK_ENTITY_TO_CLASS( item_generic, CGenericItem );

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CGenericItem::CGenericItem()
{
	m_bCanSpawn = TRUE;
	m_iLightValue = 0;
	m_bSetPVSDefault = FALSE;
	m_removeOnUse = FALSE;
	m_nopvs = FALSE;
}

TYPEDESCRIPTION CGenericItem::m_SaveData[] =
{
	DEFINE_FIELD( CGenericItem, m_iSequence, FIELD_STRING ),
	DEFINE_FIELD( CGenericItem, m_lastTime, FIELD_TIME ),
	DEFINE_FIELD( CGenericItem, m_iLightValue, FIELD_INTEGER ),
	DEFINE_FIELD( CGenericItem, m_bSetPVSDefault, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGenericItem, m_removeOnUse, FIELD_BOOLEAN ),
	DEFINE_FIELD( CGenericItem, m_nopvs, FIELD_BOOLEAN )
};

int CGenericItem::Save( CSave& save )
{
	if ( !CBaseMonster::Save( save ) )
		return 0;
	return save.WriteFields( "CGenericItem", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CGenericItem::Restore( CRestore& restore )
{
	if ( !CBaseMonster::Restore( restore ) )
		return 0;
	int result = restore.ReadFields( "CGenericItem", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	pev->iuser2 = m_nopvs;
	if ( m_iLightValue )
		pev->iuser4 = m_iLightValue;
	return result;
}

void CGenericItem::KeyValue( KeyValueData* data )
{
	if ( !m_bSetPVSDefault )
	{
		pev->iuser2 = 0;
		m_bSetPVSDefault = TRUE;
	}
	if ( FStrEq( data->szKeyName, "sequencename" ) )
	{
		m_iSequence = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "skin" ) )
	{
		pev->skin = (int)atof( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "body" ) )
	{
		pev->body = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "lightmultiplier" ) )
	{
		m_iLightValue = (int)( atof( data->szValue ) * 10 );
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "nopvs" ) )
	{
		pev->iuser2 = atoi( data->szValue );
		m_nopvs = pev->iuser2 != 0;
		data->fHandled = TRUE;
	}
	else if ( FStrEq( data->szKeyName, "removeonuse" ) )
	{
		m_removeOnUse = atoi( data->szValue ) != 0;
		data->fHandled = TRUE;
	}
	else
		CBaseDelay::KeyValue( data );
}

void CGenericItem::Spawn()
{
	m_bCanSpawn = g_pGameRules != NULL;
	if ( !g_pGameRules )
	{
		SetThink( &CGenericItem::SUB_Remove );
		pev->nextthink = gpGlobals->time + 1;
		return;
	}
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	pev->iuser1 = 0;
	pev->iuser3 = pev->spawnflags;
	pev->flags |= FL_IMMUNE_LAVA;
	Precache();
	SET_MODEL( edict(), STRING( pev->model ) );
	if ( m_iSequence )
	{
		SetThink( &CGenericItem::StartItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	if ( ( pev->spawnflags & SF_GENERICITEM_DROPTOFLOOR ) && !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
	if ( pev->spawnflags & SF_GENERICITEM_SOLID )
	{
		pev->solid = SOLID_SLIDEBOX;
		int sequence = LookupSequence( STRING( m_iSequence ) );
		if ( sequence == -1 )
		{
			ALERT( at_console, "ERROR! FIX ME: item generic: %s, model: %s, does not have animation: %s\n",
				STRING( pev->targetname ), STRING( pev->model ), STRING( m_iSequence ) );
			sequence = 0;
		}
		Vector mins, maxs;
		ExtractBbox( sequence, &mins.x, &maxs.x );
		UTIL_SetSize( pev, mins, maxs );
		UTIL_SetOrigin( pev, pev->origin );
	}
	if ( m_iLightValue )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = m_iLightValue;
	}
	if ( pev->rendermode == kRenderNormal && pev->renderamt == 0 )
	{
		pev->renderamt = 255;
		pev->rendermode = kRenderTransAlpha;
	}
}

void CGenericItem::Restart()
{
	pev->effects &= ~EF_NODRAW;
	if ( m_iSequence )
	{
		SetThink( &CGenericItem::StartItem );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	if ( ( pev->spawnflags & SF_GENERICITEM_DROPTOFLOOR ) && !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
	if ( pev->spawnflags & SF_GENERICITEM_SOLID )
	{
		pev->solid = SOLID_SLIDEBOX;
		int sequence = LookupSequence( STRING( m_iSequence ) );
		if ( sequence == -1 )
		{
			ALERT( at_console, "ERROR! FIX ME: item generic: %s, model: %s, does not have animation: %s\n",
				STRING( pev->targetname ), STRING( pev->model ), STRING( m_iSequence ) );
			sequence = 0;
		}
		Vector mins, maxs;
		ExtractBbox( sequence, &mins.x, &maxs.x );
		UTIL_SetSize( pev, mins, maxs );
		UTIL_SetOrigin( pev, pev->origin );
	}
}

void CGenericItem::Precache()
{
	PRECACHE_MODEL( (char*)STRING( pev->model ) );
}

void CGenericItem::StartItem()
{
	pev->effects = 0;
	pev->sequence = LookupSequence( STRING( m_iSequence ) );
	if ( pev->sequence == -1 )
	{
		pev->sequence = 0;
		ALERT( at_console, "ERROR! FIX ME: item generic: %s, model: %s, does not have animation: %s\n",
			STRING( pev->targetname ), STRING( pev->model ), STRING( m_iSequence ) );
	}
	pev->frame = 0;
	ResetSequenceInfo();
	SetThink( &CGenericItem::AnimateThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->frame = 0;
}

void CGenericItem::AnimateThink()
{
	DispatchAnimEvents();
	StudioFrameAdvance();
	if ( m_fSequenceFinished && !m_fSequenceLoops )
	{
		pev->frame = 0;
		ResetSequenceInfo();
	}
	pev->nextthink = gpGlobals->time + 0.1;
	m_lastTime = gpGlobals->time;
}

void CGenericItem::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( NULL );
	if ( m_removeOnUse )
		UTIL_Remove( this );
	else
	{
		pev->solid = SOLID_NOT;
		pev->effects |= EF_NODRAW;
	}
}
