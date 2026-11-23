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
//
// ========================== PATH_CORNER ===========================
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"

class CPathCorner : public CPointEntity
{
public:
	CPathCorner() : m_flWait( 0 ) {}
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	float GetDelay() override { return m_flWait; }
	float GetWaitTime() override { return m_flWait; }
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_flWait;
};

LINK_ENTITY_TO_CLASS( path_corner, CPathCorner );

// Global Savedata for Delay
TYPEDESCRIPTION CPathCorner::m_SaveData[] =
{
	DEFINE_FIELD( CPathCorner, m_flWait, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CPathCorner, CPointEntity );

//
// Cache user-entity-field values until spawn is called.
//
void CPathCorner::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "wait" ) )
	{
		m_flWait = atof( data->szValue );
		data->fHandled = TRUE;
	}
	else
	{
		data->fHandled = FALSE;
	}
}

void CPathCorner::Spawn()
{
}

TYPEDESCRIPTION CPathTrack::m_SaveData[] =
{
	DEFINE_FIELD( CPathTrack, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CPathTrack, m_pnext, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPathTrack, m_paltpath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPathTrack, m_pprevious, FIELD_CLASSPTR ),
	DEFINE_FIELD( CPathTrack, m_altName, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CPathTrack, CBaseEntity );
LINK_ENTITY_TO_CLASS( path_track, CPathTrack );

//
// Cache user-entity-field values until spawn is called.
//
void CPathTrack::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "altpath" ) )
	{
		m_altName = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else
	{
		data->fHandled = FALSE;
	}
}

void CPathTrack::Use( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	int flag = m_paltpath ? SF_PATH_ALTERNATE : SF_PATH_DISABLED;
	BOOL on = !FBitSet( pev->spawnflags, flag );
	if ( ShouldToggle( useType, on ) )
	{
		if ( on )
		{
			SetBits( pev->spawnflags, flag );
		}
		else
		{
			ClearBits( pev->spawnflags, flag );
		}
	}
}

void CPathTrack::Link()
{
	if ( !FStringNull( pev->target ) )
	{
		edict_t* target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
		if ( !FNullEnt( target ) )
		{
			m_pnext = Instance( target );
			if ( m_pnext )		// If no next pointer, this is the end of a path
			{
				m_pnext->SetPrevious( this );
			}
		}
		else
		{
			ALERT( at_console, "Dead end link %s\n", STRING( pev->target ) );
		}
	}

	// Find "alternate" path
	if ( m_altName )
	{
		edict_t* target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_altName ) );
		if ( !FNullEnt( target ) )
		{
			m_paltpath = Instance( target );
			if ( m_paltpath )		// If no next pointer, this is the end of a path
			{
				m_paltpath->SetPrevious( this );
			}
		}
	}
}

void CPathTrack::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	m_pnext = NULL;
	m_pprevious = NULL;
}

void CPathTrack::Activate()
{
	if ( !FStringNull( pev->targetname ) )		// Link to next, and back-link
	{
		Link();
	}
}

CPathTrack* CPathTrack::ValidPath( CPathTrack* path, int testFlag )
{
	if ( !path || ( testFlag && FBitSet( path->pev->spawnflags, SF_PATH_DISABLED ) ) )
	{
		return NULL;
	}
	return path;
}

void CPathTrack::Project( CPathTrack* start, CPathTrack* end, Vector* origin, float distance )
{
	if ( start && end )
	{
		Vector direction = ( end->pev->origin - start->pev->origin ).Normalize();
		*origin = end->pev->origin + direction * distance;
	}
}

CPathTrack* CPathTrack::GetNext()
{
	if ( m_paltpath && FBitSet( pev->spawnflags, SF_PATH_ALTERNATE ) && !FBitSet( pev->spawnflags, SF_PATH_ALTREVERSE ) )
	{
		return m_paltpath;
	}
	return m_pnext;
}

CPathTrack* CPathTrack::GetPrevious()
{
	if ( m_paltpath && FBitSet( pev->spawnflags, SF_PATH_ALTERNATE ) && FBitSet( pev->spawnflags, SF_PATH_ALTREVERSE ) )
	{
		return m_paltpath;
	}
	return m_pprevious;
}

void CPathTrack::SetPrevious( CPathTrack* previous )
{
	if ( previous && !FStrEq( STRING( previous->pev->targetname ), STRING( m_altName ) ) )
	{
		m_pprevious = previous;
	}
}


// Assumes this is ALWAYS enabled
CPathTrack* CPathTrack::LookAhead( Vector* origin, float distance, int move )
{
	CPathTrack* current = this;
	Vector currentPosition = *origin;
	float originalDistance = distance;
	if ( distance < 0 )
	{
		distance = -distance;
		while ( distance > 0 )
		{
			Vector direction = current->pev->origin - currentPosition;
			float length = direction.Length();
			if ( !length )
			{
				if ( !ValidPath( current->GetPrevious(), move ) ) 	// If there is no previous node, or it's disabled, return now.
				{
					if ( !move )
					{
						Project( current->GetNext(), current, origin, distance );
					}
					return NULL;
				}
				current = current->GetPrevious();
			}
			else if ( length > distance )	// enough left in this path to move
			{
				*origin = currentPosition + direction * ( distance / length );
				return current;
			}
			else
			{
				distance -= length;
				currentPosition = current->pev->origin;
				*origin = currentPosition;
				if ( !ValidPath( current->GetPrevious(), move ) )
				{
					return NULL;
				}
				current = current->GetPrevious();
			}
		}
		*origin = currentPosition;
		return current;
	}

	while ( distance > 0 )
	{
		if ( !ValidPath( current->GetNext(), move ) )	// If there is no next node, or it's disabled, return now.
		{
			if ( !move )
			{
				Project( current->GetPrevious(), current, origin, distance );
			}
			return NULL;
		}
		Vector direction = current->GetNext()->pev->origin - currentPosition;
		float length = direction.Length();
		if ( !length && !ValidPath( current->GetNext()->GetNext(), move ) )
		{
			return distance == originalDistance ? NULL : current;
		}
		if ( length > distance )	// enough left in this path to move
		{
			*origin = currentPosition + direction * ( distance / length );
			return current;
		}
		distance -= length;
		currentPosition = current->GetNext()->pev->origin;
		current = current->GetNext();
		*origin = currentPosition;
	}
	*origin = currentPosition;
	return current;
}


// Assumes this is ALWAYS enabled
CPathTrack* CPathTrack::Nearest( Vector origin )
{
	Vector delta = origin - pev->origin;
	delta.z = 0;
	float minimumDistance = delta.Length();
	CPathTrack* nearest = this;
	CPathTrack* path = GetNext();
	int count = 0;
	while ( path && path != this )
	{
		if ( ++count > 9999 )
		{
			ALERT( at_error, "Bad sequence of path_tracks from %s", STRING( pev->targetname ) );
			return NULL;
		}
		delta = origin - path->pev->origin;
		delta.z = 0;
		float distance = delta.Length();
		if ( distance < minimumDistance )
		{
			minimumDistance = distance;
			nearest = path;
		}
		path = path->GetNext();
	}
	return nearest;
}

CPathTrack* CPathTrack::Instance( edict_t* entity )
{
	if ( entity && FClassnameIs( entity, "path_track" ) )
	{
		return (CPathTrack*)GET_PRIVATE( entity );
	}
	return NULL;
}
