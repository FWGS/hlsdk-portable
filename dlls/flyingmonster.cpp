/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "flyingmonster.h"

#define FLYING_AE_FLAP 8
#define FLYING_AE_FLAPSOUND 9

int CFlyingMonster::CheckLocalMove( const Vector& start, const Vector& end, CBaseEntity* target, float* distance )
{
	if ( (pev->flags & FL_SWIM) && UTIL_PointContents( end ) != CONTENTS_WATER )
		return LOCALMOVE_INVALID;
	TraceResult trace;
	UTIL_TraceHull( start + Vector( 0, 0, 32 ), end + Vector( 0, 0, 32 ), dont_ignore_monsters,
		large_hull, edict(), &trace );
	if ( distance )
		*distance = (trace.vecEndPos - Vector( 0, 0, 32 ) - start).Length();
	if ( trace.fStartSolid || trace.flFraction < 1 )
	{
		if ( target && target->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}
	return LOCALMOVE_VALID;
}

BOOL CFlyingMonster::FTriangulate( const Vector& start, const Vector& end, float distance, CBaseEntity* target, Vector* apex )
{
	return CBaseMonster::FTriangulate( start, end, distance, target, apex );
}

Activity CFlyingMonster::GetStoppedActivity()
{
	return pev->movetype == MOVETYPE_FLY ? ACT_HOVER : ACT_IDLE;
}

void CFlyingMonster::Stop()
{
	Activity stopped = GetStoppedActivity();
	if ( m_IdealActivity != stopped )
	{
		m_flightSpeed = 0;
		m_IdealActivity = stopped;
	}
	pev->angles.z = 0;
	pev->angles.x = 0;
	m_vecTravel = g_vecZero;
}

float CFlyingMonster::ChangeYaw( int yawSpeed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float difference = FlYawDiff();
		if ( m_flLastZYawTime == 0 )
			m_flLastZYawTime = gpGlobals->time - gpGlobals->frametime;
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( difference < -20 )
				target = 90;
			else if ( difference > 20 )
				target = -90;
		}
		float interval = gpGlobals->time - m_flLastZYawTime;
		m_flLastZYawTime = gpGlobals->time;
		if ( interval > 0.25f )
			interval = 0.25f;
		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220 * interval );
	}
	return CBaseMonster::ChangeYaw( yawSpeed );
}

void CFlyingMonster::Killed( entvars_t* attacker, int gib )
{
	pev->movetype = MOVETYPE_STEP;
	pev->flags &= ~FL_ONGROUND;
	pev->angles.z = 0;
	pev->angles.x = 0;
	CBaseMonster::Killed( attacker, gib );
}

void CFlyingMonster::HandleAnimEvent( MonsterEvent_t* event )
{
	if ( event->event == FLYING_AE_FLAP )
		m_flightSpeed = 400;
	else if ( event->event == FLYING_AE_FLAPSOUND )
	{
		if ( m_pFlapSound )
			EMIT_SOUND( edict(), CHAN_BODY, m_pFlapSound, 1, ATTN_NORM );
	}
	else
		CBaseMonster::HandleAnimEvent( event );
}

void CFlyingMonster::Move( float interval )
{
	if ( pev->movetype == MOVETYPE_FLY )
		m_flGroundSpeed = m_flightSpeed;
	CBaseMonster::Move( interval );
}

BOOL CFlyingMonster::ShouldAdvanceRoute( float distance )
{
	// Get true 3D distance to the goal so we actually reach the correct height
	if ( m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL )
		distance = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Length();
	return gpGlobals->frametime * m_flGroundSpeed + 64 >= distance;
}

void CFlyingMonster::MoveExecute( CBaseEntity* target, const Vector& direction, float interval )
{
	if ( pev->movetype != MOVETYPE_FLY )
	{
		CBaseMonster::MoveExecute( target, direction, interval );
		return;
	}
	if ( gpGlobals->time - m_stopTime > 1 && m_IdealActivity != m_movementActivity )
	{
		m_IdealActivity = m_movementActivity;
		m_flGroundSpeed = m_flightSpeed = 200;
	}
	Vector move = pev->origin + (direction + m_vecTravel * m_momentum).Normalize() * (m_flGroundSpeed * interval);
	if ( m_IdealActivity != m_movementActivity )
	{
		m_flightSpeed = UTIL_Approach( 100, m_flightSpeed, 75 * gpGlobals->frametime );
		if ( m_flightSpeed < 100 )
			m_stopTime = gpGlobals->time;
	}
	else
		m_flightSpeed = UTIL_Approach( 20, m_flightSpeed, 300 * gpGlobals->frametime );
	if ( CheckLocalMove( pev->origin, move, target, NULL ) )
	{
		m_vecTravel = (move - pev->origin).Normalize();
		UTIL_MoveToOrigin( edict(), move, m_flGroundSpeed * interval, MOVE_STRAFE );
	}
	else
	{
		m_IdealActivity = GetStoppedActivity();
		m_stopTime = gpGlobals->time;
		m_vecTravel = g_vecZero;
	}
}

float CFlyingMonster::CeilingZ( const Vector& position )
{
	Vector up = position + Vector( 0, 0, 4096 );
	TraceResult trace;
	UTIL_TraceLine( position, up, ignore_monsters, NULL, &trace );
	if ( trace.flFraction != 1 )
		up.z = trace.vecEndPos.z;
	if ( pev->flags & FL_SWIM )
		return UTIL_WaterLevel( position, position.z, up.z );
	return up.z;
}

float CFlyingMonster::FloorZ( const Vector& position )
{
	Vector down = position - Vector( 0, 0, 2048 );
	TraceResult trace;
	UTIL_TraceLine( position, down, ignore_monsters, NULL, &trace );
	return trace.flFraction == 1 ? down.z : trace.vecEndPos.z;
}

BOOL CFlyingMonster::ProbeZ( const Vector& position, const Vector& probe, float* fraction )
{
	int contents = UTIL_PointContents( position );
	if ( ((pev->flags & FL_SWIM) != 0) != (contents == CONTENTS_WATER) )
	{
		*fraction = 0;
		return TRUE;
	}
	if ( UTIL_PointContents( probe ) == contents )
	{
		*fraction = 1;
		return FALSE;
	}
	Vector direction = (probe - position).Normalize();
	float length = (probe - position).Length();
	float maximum = length;
	float minimum = 0;
	while ( maximum - minimum > 1 )
	{
		float middle = minimum + (maximum - minimum) * 0.5f;
		if ( UTIL_PointContents( position + direction * middle ) == contents )
			minimum = middle;
		else
			maximum = middle;
	}
	*fraction = minimum / length;
	return TRUE;
}
