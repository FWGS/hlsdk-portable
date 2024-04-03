#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"

// ---------------------------------------------------------------------
//
// Sprite Train
//
// ---------------------------------------------------------------------

class CFuncModelTrain : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData* pkvd );

	void EXPORT Next( void );
	void EXPORT Find( void );
	void EXPORT NearestPath( void );
	void EXPORT DeadEnd( void );

	void NextThink( float thinkTime, BOOL alwaysThink );

	void SetTrack( CPathTrack *track ) { m_ppath = track->Nearest(pev->origin); }
	void SetControls( entvars_t *pevControls );
	BOOL OnControls( entvars_t *pev );
	void Animate( float frames );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];
	virtual int	ObjectCaps( void ) { return (CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DIRECTIONAL_USE; }

	virtual void	OverrideReset( void );

	CPathTrack	*m_ppath;
	float		m_length;
	float		m_height;
	float		m_speed;
	float		m_dir;
	float		m_startSpeed;
	Vector		m_controlMins;
	Vector		m_controlMaxs;
	float		m_flBank;
	float		m_oldSpeed;
	int         m_scale;

	float		m_lastTime;
	float		m_maxFrame;
};

TYPEDESCRIPTION	CFuncModelTrain::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncModelTrain, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncModelTrain, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncModelTrain, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncModelTrain, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_oldSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_maxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncModelTrain, m_lastTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CFuncModelTrain, CBaseEntity );
LINK_ENTITY_TO_CLASS( monster_modeltrain, CFuncModelTrain);

void CFuncModelTrain :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "wheels"))//!
	{
		m_length = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "height"))//!
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "speed"))//op4
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bank"))//!
	{
		m_flBank = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "scale"))//op4
	{
		m_scale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}


void CFuncModelTrain:: NextThink( float thinkTime, BOOL alwaysThink )
{
	if ( alwaysThink )
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}

void CFuncModelTrain:: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType != USE_SET )
	{
		if ( !ShouldToggle( useType, (pev->speed != 0) ) )
			return;

		if ( pev->speed == 0 )
		{
			pev->speed = m_speed * m_dir;
			
			Next();
		}
		else
		{
			pev->speed = 0;
			pev->velocity = g_vecZero;
			pev->avelocity = g_vecZero;
			SetThink( NULL );
		}
	}
	else
	{
		float delta = value;

		delta = ((int)(pev->speed * 4) / (int)m_speed)*0.25 + 0.25 * delta;
		if ( delta > 1 )
			delta = 1;
		else if ( delta < -1 )
			delta = -1;
		if ( pev->spawnflags & SF_TRACKTRAIN_FORWARDONLY )
		{
			if ( delta < 0 )
				delta = 0;
		}
		pev->speed = m_speed * delta;
		Next();	
		ALERT( at_aiconsole, "TRAIN(%s), speed to %.2f\n", STRING(pev->targetname), pev->speed );
	}
}


static float Fix( float angle )
{
	while ( angle < 0 )
		angle += 360;
	while ( angle > 360 )
		angle -= 360;

	return angle;
}


static void FixupAngles( Vector &v )
{
	v.x = Fix( v.x );
	v.y = Fix( v.y );
	v.z = Fix( v.z );
}

#define TRAIN_STARTPITCH	60
#define TRAIN_MAXPITCH		200
#define TRAIN_MAXSPEED		1000	// approx max speed for sound pitch calculation

void CFuncModelTrain:: Next( void )
{
	float time = 0.5;

	if ( !pev->speed )
	{
		ALERT( at_aiconsole, "TRAIN(%s): Speed is 0\n", STRING(pev->targetname) );
		return;
	}

	if ( !m_ppath )
	{	
		ALERT( at_aiconsole, "TRAIN(%s): Lost path\n", STRING(pev->targetname) );
		return;
	}

	Vector nextPos = pev->origin;

	nextPos.z -= m_height;
	CPathTrack *pnext = m_ppath->LookAhead( &nextPos, pev->speed * 0.1, 1 );
	nextPos.z += m_height;

	pev->velocity = (nextPos - pev->origin) * 10;
	pev->angles = pnext->pev->angles;	// set modeltrain's angles to angles of path_track
	Vector nextFront = pev->origin;

	nextFront.z -= m_height;
	if ( m_length > 0 )
		m_ppath->LookAhead( &nextFront, m_length, 0 );
	else
		m_ppath->LookAhead( &nextFront, 100, 0 );
	nextFront.z += m_height;

	Vector delta = nextFront - pev->origin;
	Vector angles = UTIL_VecToAngles( delta );
	// The train actually points west
	angles.y += 180;

	// !!!  All of this crap has to be done to make the angles not wrap around, revisit this.
	FixupAngles( angles );
	FixupAngles( pev->angles );

	if ( !pnext || (delta.x == 0 && delta.y == 0) )
		angles = pev->angles;

	float vy, vx;
	if ( !(pev->spawnflags & SF_TRACKTRAIN_NOPITCH) )
		vx = UTIL_AngleDistance( angles.x, pev->angles.x );
	else
		vx = 0;
	vy = UTIL_AngleDistance( angles.y, pev->angles.y );

	pev->avelocity.y = vy * 10;
	pev->avelocity.x = vx * 10;

	if ( m_flBank != 0 )
	{
		if ( pev->avelocity.y < -5 )
			pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( -m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
		else if ( pev->avelocity.y > 5 )
			pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( m_flBank, pev->angles.z, m_flBank*2 ), pev->angles.z);
		else
			pev->avelocity.z = UTIL_AngleDistance( UTIL_ApproachAngle( 0, pev->angles.z, m_flBank*4 ), pev->angles.z) * 4;
	}
		
	if ( pnext )
	{
		if ( pnext != m_ppath )
		{
			CPathTrack *pFire;
			if ( pev->speed >= 0 )
				pFire = pnext;
			else 
				pFire = m_ppath;

			m_ppath = pnext;
			// Fire the pass target if there is one
			if ( pFire->pev->message )
			{
				FireTargets( STRING(pFire->pev->message), this, this, USE_TOGGLE, 0 );
				if ( FBitSet( pFire->pev->spawnflags, SF_PATH_FIREONCE ) )
					pFire->pev->message = 0;
			}

			if ( pFire->pev->spawnflags & SF_PATH_DISABLE_TRAIN )
				pev->spawnflags |= SF_TRACKTRAIN_NOCONTROL;
			
			// Don't override speed if under user control
			if ( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
			{
				if ( pFire->pev->speed != 0 )
				{// don't copy speed from target if it is 0 (uninitialized)
					pev->speed = pFire->pev->speed;
					ALERT( at_aiconsole, "TrackTrain %s speed to %4.2f\n", STRING(pev->targetname), pev->speed );
				}
			}
		}
		SetThink( &CFuncModelTrain::Next );
		NextThink( pev->ltime + time, TRUE );
	}
	else	// end of path, stop
	{
		pev->velocity = (nextPos - pev->origin);
		pev->avelocity = g_vecZero;
		float distance = pev->velocity.Length();
		m_oldSpeed = pev->speed;


		pev->speed = 0;
		
		// Move to the dead end
		
		// Are we there yet?
		if ( distance > 0 )
		{
			// no, how long to get there?
			time = distance / m_oldSpeed;
			pev->velocity = pev->velocity * (m_oldSpeed / distance);
			SetThink( &CFuncModelTrain::DeadEnd );
			NextThink( pev->ltime + time, FALSE );
		}
		else
		{
			DeadEnd();
		}
	}
//Animate( 1 );//was 1 - too fast
Animate( pev->framerate * (gpGlobals->time - m_lastTime) );
m_lastTime = gpGlobals->time;
}


void CFuncModelTrain::DeadEnd( void )
{
	// Fire the dead-end target if there is one
	CPathTrack *pTrack, *pNext;

	pTrack = m_ppath;

	ALERT( at_aiconsole, "TRAIN(%s): Dead end ", STRING(pev->targetname) );
	// Find the dead end path node
	// HACKHACK -- This is bugly, but the train can actually stop moving at a different node depending on it's speed
	// so we have to traverse the list to it's end.
	if ( pTrack )
	{
		if ( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );
				if ( pNext )
					pTrack = pNext;
			} while ( pNext );
		}
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	if ( pTrack )
	{
		ALERT( at_aiconsole, "at %s\n", STRING(pTrack->pev->targetname) );
		if ( pTrack->pev->netname )
			FireTargets( STRING(pTrack->pev->netname), this, this, USE_TOGGLE, 0 );
	}
	else
		ALERT( at_aiconsole, "\n" );
}


void CFuncModelTrain :: SetControls( entvars_t *pevControls )
{
	Vector offset = pevControls->origin - pev->oldorigin;

	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}


BOOL  CFuncModelTrain :: OnControls( entvars_t *pevTest )
{
	Vector offset = pevTest->origin - pev->origin;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return FALSE;

	// Transform offset into local coordinates
	UTIL_MakeVectors( pev->angles );
	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	if ( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z &&
		 local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z )
		 return TRUE;

	return FALSE;
}


void  CFuncModelTrain :: Find( void )
{
	m_ppath = CPathTrack::Instance(FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->target) ));
	if ( !m_ppath )
		return;

	entvars_t *pevTarget = m_ppath->pev;
	if ( !FClassnameIs( pevTarget, "path_track" ) )
	{
		ALERT( at_error, "func_track_train must be on a path of path_track\n" );
		m_ppath = NULL;
		return;
	}

	Vector nextPos = pevTarget->origin;
	nextPos.z += m_height;

	Vector look = nextPos;
	look.z -= m_height;
	m_ppath->LookAhead( &look, m_length, 0 );
	look.z += m_height;

	pev->angles = UTIL_VecToAngles( look - nextPos );
	// The train actually points west
	pev->angles.y += 180;

	if ( pev->spawnflags & SF_TRACKTRAIN_NOPITCH )
		pev->angles.x = 0;
    UTIL_SetOrigin( pev, nextPos );
	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( &CFuncModelTrain::Next );
	pev->speed = m_startSpeed;
}


void CFuncModelTrain :: NearestPath( void )
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist, closest;

	closest = 1024;

	while ((pTrack = UTIL_FindEntityInSphere( pTrack, pev->origin, 1024 )) != NULL)
	{
		// filter out non-tracks
		if ( !(pTrack->pev->flags & (FL_CLIENT|FL_MONSTER)) && FClassnameIs( pTrack->pev, "path_track" ) )
		{
			dist = (pev->origin - pTrack->pev->origin).Length();
			if ( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if ( !pNearest )
	{
		ALERT( at_console, "Can't find a nearby track !!!\n" );
		SetThink(NULL);
		return;
	}

	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING(pev->targetname), STRING(pNearest->pev->targetname) );
	// If I'm closer to the next path_track on this path, then it's my real path
	pTrack = ((CPathTrack *)pNearest)->GetNext();
	if ( pTrack )
	{
		if ( (pev->origin - pTrack->pev->origin).Length() < (pev->origin - pNearest->pev->origin).Length() )
			pNearest = pTrack;
	}

	m_ppath = (CPathTrack *)pNearest;

	if ( pev->speed != 0 )
	{
		NextThink( pev->ltime + 0.1, FALSE );
		SetThink( &CFuncModelTrain::Next );
	}
}

void CFuncModelTrain ::OverrideReset( void )
{
	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( &CFuncModelTrain::NearestPath );
}

void CFuncModelTrain ::Animate( float frames )
{
if ( m_maxFrame > 0 )
		pev->frame = fmod( pev->frame + frames, m_maxFrame );
}

void CFuncModelTrain :: Spawn( void )
{
	Precache();

	if ( pev->speed == 0 )
		m_speed = 100;
	else
		m_speed = pev->speed;
	
	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	pev->impulse = m_speed;
	m_dir = 1;

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;	
	//pev->rendercolor.x = 255;
	//pev->rendercolor.y = 255;
	//pev->rendercolor.z = 255;
	pev->scale = m_scale;
	//pev->rendermode = kRenderTransAdd;
	//pev->renderamt = 255;

	SET_MODEL( ENT(pev), STRING(pev->model) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;
	
	m_lastTime = gpGlobals->time;
	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;

	NextThink( pev->ltime + 0.1, FALSE );
	SetThink( &CFuncModelTrain::Find );
}

void CFuncModelTrain :: Precache( void )
{
        PRECACHE_MODEL( (char *)STRING(pev->model) );
}
