
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"climbingmonster.h"


float CClimbingMonster :: ChangeYaw( int speed )
{
	if (mbIsClimbing)
	{
		if ( pev->movetype == MOVETYPE_FLY )
		{
			float diff = FlYawDiff();
			float target = 0;

			if ( m_IdealActivity != GetStoppedActivity() )
			{
				if ( diff < -20 )
					target = 90;
				else if ( diff > 20 )
					target = -90;
			}
			pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * gpGlobals->frametime );
		}
	}
	return CBaseMonster::ChangeYaw( speed );
}

BOOL CClimbingMonster::FGetNodeRoute ( Vector vecDest )
{
	// if we are using the nodes, then we may be climbing
	mbIsClimbing = TRUE;

	// while calculating the node route, assume the monster can fly.
	// this will allow it to use the info_node_air nodes.
	SetBits( pev->flags, FL_FLY );
	mOldType = pev->movetype;
	pev->movetype = MOVETYPE_FLY;
	m_flGroundSpeed = 100;
	m_afCapability |= bits_CAP_FLY;

	BOOL b = CBaseMonster::FGetNodeRoute( vecDest );

	return b;
}

//
// We may need to override Move, in order to reset the FL_FLY flag again.
//

BOOL CClimbingMonster::BuildRoute ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget )
{
	// by default, we are not climbing
	mbIsClimbing = FALSE;
	pev->movetype = mOldType;
	ClearBits( pev->flags, FL_FLY );
	m_velocity = Vector(0,0,0);
	m_afCapability &= ~bits_CAP_FLY;

	return CBaseMonster::BuildRoute ( vecGoal, iMoveFlag, pTarget );
}

BOOL CClimbingMonster::FRefreshRoute( void )
{
	// by default, we are not climbing
	mbIsClimbing = FALSE;
	pev->movetype = mOldType;
	ClearBits( pev->flags, FL_FLY );
	m_afCapability &= ~bits_CAP_FLY;

	return CBaseMonster::FRefreshRoute();
}

int CClimbingMonster :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	// if we are climbing, then we are effectively flying (like the controller)
	if (mbIsClimbing)
	{
		TraceResult tr;

		UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, human_hull, edict(), &tr );

		// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
		// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

		if (pflDist)
		{
			*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 32 )) - vecStart ).Length();// get the distance.
		}

		// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
		// (tr.fStartSolid || tr.flFraction < 1.0)
		if (tr.flFraction < 1.0)
		{
			if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
				return LOCALMOVE_VALID;
			return LOCALMOVE_INVALID;
		}

		return LOCALMOVE_VALID;
	}
	else // otherwise we walk, just like anyone else
	{
		return CBaseMonster::CheckLocalMove( vecStart, vecEnd, pTarget, pflDist );
	}
}

BOOL CClimbingMonster:: ShouldAdvanceRoute( float flWaypointDist )
{
	if (mbIsClimbing)
	{
		// Get true 3D distance to the goal so we actually reach the correct height
		if ( m_Route[ m_iRouteIndex ].iType & bits_MF_IS_GOAL )
			flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();

		if ( flWaypointDist <= 64 + (m_flGroundSpeed * gpGlobals->frametime) )
			return TRUE;

		return FALSE;
	}
	else
	{
		return CBaseMonster::ShouldAdvanceRoute(flWaypointDist);
	}
}

void CClimbingMonster :: Move( float flInterval )
{
	if (mbIsClimbing)
	{
		if ( pev->movetype == MOVETYPE_FLY )
			m_flGroundSpeed = m_climbSpeed;
	}
	CBaseMonster::Move( flInterval );
}

void CClimbingMonster::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if (mbIsClimbing)
	{
		if ( pev->movetype == MOVETYPE_FLY )
		{
			if ( gpGlobals->time - m_stopTime > 1.0 )
			{
				if ( m_IdealActivity != m_movementActivity )
				{
					m_IdealActivity = m_movementActivity;
					m_flGroundSpeed = m_climbSpeed = 200;
				}
			}
			// hardcoded momentum
			Vector vecMove = pev->origin + (( vecDir + (m_vecTravel * 2.5) ).Normalize() * (m_flGroundSpeed * flInterval));

			if ( m_IdealActivity != m_movementActivity )
			{
				m_climbSpeed = UTIL_Approach( 100, m_climbSpeed, 75 * gpGlobals->frametime );
				if ( m_climbSpeed < 100 )
					m_stopTime = gpGlobals->time;
			}
			else
				m_climbSpeed = UTIL_Approach( 20, m_climbSpeed, 300 * gpGlobals->frametime );
			
			if ( CheckLocalMove ( pev->origin, vecMove, pTargetEnt, NULL ) )
			{
				m_vecTravel = (vecMove - pev->origin);
				m_vecTravel = m_vecTravel.Normalize();
				UTIL_MoveToOrigin(ENT(pev), vecMove, (m_flGroundSpeed * flInterval), MOVE_STRAFE);
			}
			else
			{
				m_IdealActivity = GetStoppedActivity();
				m_stopTime = gpGlobals->time;
				m_vecTravel = g_vecZero;
			}
		}
	}
	else
	{
		CBaseMonster::MoveExecute( pTargetEnt, vecDir, flInterval );
	}
}

void CClimbingMonster :: Stop( void ) 
{ 
	if (mbIsClimbing)
	{
		Activity stopped = GetStoppedActivity();
		if ( m_IdealActivity != stopped )
		{
			m_climbSpeed = 0;
			m_IdealActivity = stopped;
		}
		pev->angles.z = 0;
		pev->angles.x = 0;
		m_vecTravel = g_vecZero;
	}
	else
	{
		CBaseMonster::Stop();
	}
}

void CClimbingMonster :: Killed( entvars_t *pevAttacker, int iGib )
{
	if (mbIsClimbing)
	{
		pev->movetype = MOVETYPE_STEP;
		ClearBits( pev->flags, FL_ONGROUND );
		pev->angles.z = 0;
		pev->angles.x = 0;
	}
	CBaseMonster::Killed( pevAttacker, iGib );
}
