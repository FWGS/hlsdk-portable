#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "trains.h"
#include "saverestore.h"

#define VEHICLE_SPEED0_ACCELERATION	0.005000000000000000
#define VEHICLE_SPEED1_ACCELERATION	0.002142857142857143
#define VEHICLE_SPEED2_ACCELERATION	0.003333333333333334
#define VEHICLE_SPEED3_ACCELERATION	0.004166666666666667
#define VEHICLE_SPEED4_ACCELERATION	0.004000000000000000
#define VEHICLE_SPEED5_ACCELERATION	0.003800000000000000
#define VEHICLE_SPEED6_ACCELERATION	0.004500000000000000
#define VEHICLE_SPEED7_ACCELERATION	0.004250000000000000
#define VEHICLE_SPEED8_ACCELERATION	0.002666666666666667
#define VEHICLE_SPEED9_ACCELERATION	0.002285714285714286
#define VEHICLE_SPEED10_ACCELERATION	0.001875000000000000
#define VEHICLE_SPEED11_ACCELERATION	0.001444444444444444
#define VEHICLE_SPEED12_ACCELERATION	0.001200000000000000
#define VEHICLE_SPEED13_ACCELERATION	0.000916666666666666

#define VEHICLE_STARTPITCH		60
#define VEHICLE_MAXPITCH		200
#define VEHICLE_MAXSPEED		1500

TYPEDESCRIPTION CFuncVehicle::m_SaveData[] =
{
	DEFINE_FIELD( CFuncVehicle, m_ppath, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncVehicle, m_length, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_height, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_speed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_dir, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_startSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_controlMins, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncVehicle, m_controlMaxs, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncVehicle, m_sounds, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncVehicle, m_flVolume, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_flBank, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncVehicle, m_oldSpeed, FIELD_FLOAT ),
};

static float Fix2( float angle )
{
	while( angle < 0 )
		angle += 360;

	while( angle > 360 )
		angle -= 360;

	return angle;
}

static void FixupAngles2( Vector &v )
{
	v.x = Fix2( v.x );
	v.y = Fix2( v.y );
	v.z = Fix2( v.z );
}

IMPLEMENT_SAVERESTORE( CFuncVehicle, CBaseEntity )

LINK_ENTITY_TO_CLASS( func_vehicle, CFuncVehicle )

void CFuncVehicle::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "length" ))
	{
		m_length = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "width" ))
	{
		m_width = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "height" ))
	{
		m_height = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "startspeed" ))
	{
		m_startSpeed = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sounds" ))
	{
		m_sounds = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "volume" ))
	{
		m_flVolume = (float)atoi( pkvd->szValue );
		m_flVolume *= 0.1f;

		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bank" ))
	{
		m_flBank = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "acceleration" ))
	{
		m_acceleration = atoi( pkvd->szValue );

		if( m_acceleration < 1 )
			m_acceleration = 1;

		else if( m_acceleration > 10 )
			m_acceleration = 10;

		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CFuncVehicle::NextThink( float thinkTime, BOOL alwaysThink )
{
	if( alwaysThink )
		pev->flags |= FL_ALWAYSTHINK;
	else
		pev->flags &= ~FL_ALWAYSTHINK;

	pev->nextthink = thinkTime;
}

void CFuncVehicle::Blocked( CBaseEntity *pOther )
{
	entvars_t *pevOther = pOther->pev;

	if( ( pevOther->flags & FL_ONGROUND ) && VARS( pevOther->groundentity ) == pev )
	{
		pevOther->velocity = pev->velocity;
		return;
	}

	pevOther->velocity = ( pevOther->origin - pev->origin ).Normalize() * pev->dmg;
	pevOther->velocity.z += 300;
	pev->velocity = pev->velocity * 0.85f;

	ALERT( at_aiconsole, "TRAIN(%s): Blocked by %s (dmg:%.2f)\n", STRING( pev->targetname ), STRING( pOther->pev->classname ), pev->dmg );
	UTIL_MakeVectors( pev->angles );

	Vector forward, right, vOrigin;
	Vector vFrontLeft = ( gpGlobals->v_forward * -1 ) * ( m_length * 0.5f );
	Vector vFrontRight = ( gpGlobals->v_right * -1 ) * ( m_width * 0.5f );

	Vector vBackLeft = pev->origin + vFrontLeft - vFrontRight;
	Vector vBackRight = pev->origin - vFrontLeft + vFrontRight;

	float minx = Q_min( vBackLeft.x, vBackRight.x );
	float miny = Q_min( vBackLeft.y, vBackRight.y );
	float maxx = Q_max( vBackLeft.x, vBackRight.x );
	float maxy = Q_max( vBackLeft.y, vBackRight.y );

	float minz = pev->origin.z;
	float maxz = pev->origin.z + ( 2 * abs( (int)( pev->mins.z - pev->maxs.z )));

	if ( pOther->pev->origin.x < minx
		|| pOther->pev->origin.x > maxx
		|| pOther->pev->origin.y < miny
		|| pOther->pev->origin.y > maxy
		|| pOther->pev->origin.z < pev->origin.z
		|| pOther->pev->origin.z > maxz )
	{
		pOther->TakeDamage( pev, pev, 150, DMG_CRUSH );
	}
}

void CFuncVehicle::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	float delta = value;

	if( useType != USE_SET )
	{
		if( ShouldToggle( useType, pev->speed != 0 ))
		{
			if( pev->speed == 0 )
			{
				pev->speed = m_dir * m_speed;
				Next();
			}
			else
			{
				pev->speed = 0;
				pev->velocity = g_vecZero;
				pev->avelocity = g_vecZero;

				StopSound();
				SetThink( NULL );
			}
		}

		return;
	}

	if( delta < 10 )
	{
		if( delta < 0 )
		{
			if( pev->speed > 145 )
			{
				StopSound();
			}
		}

		float flSpeedRatio = delta;

		if( delta > 0 )
		{
			flSpeedRatio = (float)( pev->speed / m_speed );

			if( pev->speed < 0 )		flSpeedRatio = m_acceleration * 0.0005 + flSpeedRatio + VEHICLE_SPEED0_ACCELERATION;
			else if( pev->speed < 10 )	flSpeedRatio = m_acceleration * 0.0006 + flSpeedRatio + VEHICLE_SPEED1_ACCELERATION;
			else if( pev->speed < 20 )	flSpeedRatio = m_acceleration * 0.0007 + flSpeedRatio + VEHICLE_SPEED2_ACCELERATION;
			else if( pev->speed < 30 )	flSpeedRatio = m_acceleration * 0.0007 + flSpeedRatio + VEHICLE_SPEED3_ACCELERATION;
			else if( pev->speed < 45 )	flSpeedRatio = m_acceleration * 0.0007 + flSpeedRatio + VEHICLE_SPEED4_ACCELERATION;
			else if( pev->speed < 60 )	flSpeedRatio = m_acceleration * 0.0008 + flSpeedRatio + VEHICLE_SPEED5_ACCELERATION;
			else if( pev->speed < 80 )	flSpeedRatio = m_acceleration * 0.0008 + flSpeedRatio + VEHICLE_SPEED6_ACCELERATION;
			else if( pev->speed < 100 )	flSpeedRatio = m_acceleration * 0.0009 + flSpeedRatio + VEHICLE_SPEED7_ACCELERATION;
			else if( pev->speed < 150 )	flSpeedRatio = m_acceleration * 0.0008 + flSpeedRatio + VEHICLE_SPEED8_ACCELERATION;
			else if( pev->speed < 225 )	flSpeedRatio = m_acceleration * 0.0007 + flSpeedRatio + VEHICLE_SPEED9_ACCELERATION;
			else if( pev->speed < 300 )	flSpeedRatio = m_acceleration * 0.0006 + flSpeedRatio + VEHICLE_SPEED10_ACCELERATION;
			else if( pev->speed < 400 )	flSpeedRatio = m_acceleration * 0.0005 + flSpeedRatio + VEHICLE_SPEED11_ACCELERATION;
			else if( pev->speed < 550 )	flSpeedRatio = m_acceleration * 0.0005 + flSpeedRatio + VEHICLE_SPEED12_ACCELERATION;
			else if( pev->speed < 800 )	flSpeedRatio = m_acceleration * 0.0005 + flSpeedRatio + VEHICLE_SPEED13_ACCELERATION;
		}
		else if( delta < 0 )
		{
			flSpeedRatio = pev->speed / m_speed;

										// TODO: fix float for test demo
			if( flSpeedRatio > 0 )						flSpeedRatio = (float)flSpeedRatio - 0.0125f;
			else if( flSpeedRatio <= 0 && flSpeedRatio > -0.05f )		flSpeedRatio = (float)flSpeedRatio - 0.0075f;
			else if( flSpeedRatio <= 0.05f && flSpeedRatio > -0.1f )	flSpeedRatio = (float)flSpeedRatio - 0.01f;
			else if( flSpeedRatio <= 0.15f && flSpeedRatio > -0.15f )	flSpeedRatio = (float)flSpeedRatio - 0.0125f;
			else if( flSpeedRatio <= 0.15f && flSpeedRatio > -0.22f )	flSpeedRatio = (float)flSpeedRatio - 0.01375f;
			else if( flSpeedRatio <= 0.22f && flSpeedRatio > -0.3f )	flSpeedRatio = (float)flSpeedRatio - 0.0175f;
			else if( flSpeedRatio <= 0.3f )					flSpeedRatio = (float)flSpeedRatio - 0.0125f;
		}

		if( flSpeedRatio > 1 )
		{
			flSpeedRatio = 1;
		}
		else if( flSpeedRatio < -0.35f )
		{
			flSpeedRatio = -0.35f;
		}

		pev->speed = flSpeedRatio * m_speed;
		Next();
		m_flAcceleratorDecay = gpGlobals->time + 0.25f;
	}
	else if( m_flCanTurnNow < gpGlobals->time )
	{
		if( delta == 20 )
		{
			m_iTurnAngle++;
			m_flSteeringWheelDecay = gpGlobals->time + 0.075f;

			if (m_iTurnAngle > 8)
			{
				m_iTurnAngle = 8;
			}
		}
		else if( delta == 30 )
		{
			m_iTurnAngle--;
			m_flSteeringWheelDecay = gpGlobals->time + 0.075f;

			if( m_iTurnAngle < -8 )
			{
				m_iTurnAngle = -8;
			}
		}

		m_flCanTurnNow = gpGlobals->time + 0.05f;
	}
}

void CFuncVehicle::StopSound()
{
	if( m_soundPlaying && pev->noise )
	{
		unsigned short us_sound = ( (unsigned short)m_sounds & 0x0007 ) << 12;
		unsigned short us_encode = us_sound;

		PLAYBACK_EVENT_FULL( FEV_RELIABLE | FEV_UPDATE, edict(), m_usAdjustPitch, 0, g_vecZero, g_vecZero, 0, 0, us_encode, 0, 1, 0 );
	}

	m_soundPlaying = 0;
}

void CFuncVehicle::UpdateSound()
{
	if( !pev->noise )
		return;

	float flpitch = VEHICLE_STARTPITCH + ( abs( (int)pev->speed ) * ( VEHICLE_MAXPITCH - VEHICLE_STARTPITCH ) / VEHICLE_MAXSPEED );

	if( flpitch > 200 )
		flpitch = 200;

	if( !m_soundPlaying )
	{
		if( m_sounds < 5 )
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "plats/vehicle_brake1.wav", m_flVolume, ATTN_NORM, 0, PITCH_NORM );
		}

		EMIT_SOUND_DYN( ENT( pev ), CHAN_STATIC, STRING( pev->noise ), m_flVolume, ATTN_NORM, 0, (int)flpitch );
		m_soundPlaying = 1;
	}
	else
	{
		unsigned short us_sound = ( (unsigned short)( m_sounds ) & 0x0007 ) << 12;
		unsigned short us_pitch = ( (unsigned short)( flpitch / 10.0 ) & 0x003F ) << 6;
		unsigned short us_volume = ( (unsigned short)( m_flVolume * 40 ) & 0x003F );
		unsigned short us_encode = us_sound | us_pitch | us_volume;

		PLAYBACK_EVENT_FULL( FEV_UPDATE, edict(), m_usAdjustPitch, 0.0, g_vecZero, g_vecZero, 0.0, 0.0, us_encode, 0, 0, 0 );
	}
}

void CFuncVehicle::CheckTurning()
{
	float maxspeed;
	TraceResult tr;
	bool bTurnIntoWall = false;

	if( m_iTurnAngle < 0 )
	{
		if( pev->speed > 0 )
		{
			UTIL_TraceLine( m_vFrontRight, m_vFrontRight - ( gpGlobals->v_right * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );
		}
		else if( pev->speed < 0 )
		{
			UTIL_TraceLine( m_vBackLeft, m_vBackLeft + ( gpGlobals->v_right * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );
		}

		if( tr.flFraction != 1.0f )
		{
			m_iTurnAngle = 1;
		}
	}
	else if( m_iTurnAngle > 0 )
	{
		if( pev->speed > 0 )
		{
			UTIL_TraceLine( m_vFrontLeft, m_vFrontLeft + ( gpGlobals->v_right * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );
		}
		else if( pev->speed < 0 )
		{
			UTIL_TraceLine( m_vBackRight, m_vBackRight - ( gpGlobals->v_right * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );
		}

		if( tr.flFraction != 1.0f )
		{
			m_iTurnAngle = -1;
		}
	}

	if( pev->speed > 0 )
	{
		int iCountTurn = abs( m_iTurnAngle );

		if( iCountTurn > 4 )
		{
			if ( m_flTurnStartTime != -1 )
			{
				float flTurnTime = gpGlobals->time - m_flTurnStartTime;

				if( flTurnTime >= 0 )		maxspeed = m_speed * 0.98f;
				else if( flTurnTime > 0.3f )	maxspeed = m_speed * 0.95f;
				else if( flTurnTime > 0.6f )	maxspeed = m_speed * 0.9f;
				else if( flTurnTime > 0.8f )	maxspeed = m_speed * 0.8f;
				else if( flTurnTime > 1 )	maxspeed = m_speed * 0.7f;
				else if( flTurnTime > 1.2f )	maxspeed = m_speed * 0.5f;
				else				maxspeed = flTurnTime;
			}
			else
			{
				m_flTurnStartTime = gpGlobals->time;
				maxspeed = m_speed;
			}
		}
		else
		{
			m_flTurnStartTime = -1;

			if( iCountTurn > 2 )
				maxspeed = m_speed * 0.9f;
			else
				maxspeed = m_speed;
		}

		if( maxspeed < pev->speed )
		{
			pev->speed -= m_speed * 0.1f;
		}
	}
}

void CFuncVehicle::CollisionDetection()
{
	TraceResult tr;
	bool bHitSomething = false;

	if( pev->speed < 0 )
	{
		UTIL_TraceLine( m_vBackLeft, m_vBackLeft + ( gpGlobals->v_forward * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f )
		{
			UTIL_TraceLine( m_vBackRight, m_vBackRight + ( gpGlobals->v_forward * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

			if( tr.flFraction == 1.0f )
			{
				UTIL_TraceLine( m_vBack, m_vBack + ( gpGlobals->v_forward * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

				if( tr.flFraction == 1.0f )
				{
					return;
				}
			}

			if( DotProduct( gpGlobals->v_forward, tr.vecPlaneNormal * -1.0f ) < 0.7f && tr.vecPlaneNormal.z < 0.1f )
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
				m_vSurfaceNormal.z = 0;

				pev->speed *= 0.99f;
			}
			else if( tr.vecPlaneNormal.z < 0.65f || tr.fStartSolid )
			{
				pev->speed *= -1.0f;
			}
			else
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
			}
		}
		else
		{
			if( DotProduct( gpGlobals->v_forward, tr.vecPlaneNormal * -1.0f ) < 0.7f && tr.vecPlaneNormal.z < 0.1f )
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
				m_vSurfaceNormal.z = 0;

				pev->speed *= 0.99f;
			}
			else if( tr.vecPlaneNormal[2] < 0.65f || tr.fStartSolid )
			{
				pev->speed *= -1.0f;
			}
			else
			{
				m_vSurfaceNormal = tr.vecPlaneNormal;
			}

			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );

			if( pHit && pHit->Classify() == CLASS_VEHICLE )
			{
				bHitSomething = true;
				ALERT( at_console, "I hit another vehicle\n" );
			}
		}
	}
	else if( pev->speed > 0 )
	{
		UTIL_TraceLine( m_vFrontLeft, m_vFrontLeft - ( gpGlobals->v_forward * 16.0f ), dont_ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

		if( tr.flFraction == 1.0f )
		{
			UTIL_TraceLine( m_vFrontRight, m_vFrontRight - ( gpGlobals->v_forward * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

			if( tr.flFraction == 1.0f )
			{
				UTIL_TraceLine( m_vFront, m_vFront - ( gpGlobals->v_forward * 16.0f ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

				if( tr.flFraction == 1.0f )
				{
					return;
				}
			}
		}

		if( DotProduct( gpGlobals->v_forward, tr.vecPlaneNormal * -1.0f ) > -0.7f && tr.vecPlaneNormal.z < 0.1f )
		{
			m_vSurfaceNormal = tr.vecPlaneNormal;
			m_vSurfaceNormal.z = 0;

			pev->speed *= 0.99f;
		}
		else if( tr.vecPlaneNormal.z < 0.65f || tr.fStartSolid )
		{
			pev->speed *= -1.0f;
		}
		else
		{
			m_vSurfaceNormal = tr.vecPlaneNormal;
		}
	}
}

void CFuncVehicle::TerrainFollowing()
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, ( m_height + 48 ) * -1 ), ignore_monsters, dont_ignore_glass, ENT( pev ), &tr );

	if( tr.flFraction != 1.0f )
	{
		m_vSurfaceNormal = tr.vecPlaneNormal;
	}
	else if( tr.fInWater )
	{
		m_vSurfaceNormal = Vector( 0, 0, 1 );
	}
}

void CFuncVehicle::Next()
{
	Vector vGravityVector, forward, right, up;
	float time = 0.1f;

	vGravityVector = g_vecZero;
	UTIL_MakeVectors( pev->angles );

	forward = ( gpGlobals->v_forward * -1 ) * ( m_length * 0.5f );
	right = ( gpGlobals->v_right * -1 ) * ( m_width * 0.5f );
	up = gpGlobals->v_up * 16;

	m_vFrontLeft = pev->origin + forward - right + up;
	m_vFrontRight = pev->origin + forward + right + up;
	m_vFront = pev->origin + forward + up;
	m_vBackLeft = pev->origin - forward - right + up;
	m_vBackRight = pev->origin - forward + right + up;
	m_vBack = pev->origin - forward + up;
	m_vSurfaceNormal = g_vecZero;

	CheckTurning();

	if( m_flSteeringWheelDecay < gpGlobals->time )
	{
		m_flSteeringWheelDecay = gpGlobals->time + 0.1f;

		if( m_iTurnAngle < 0 )
			m_iTurnAngle++;

		else if( m_iTurnAngle > 0 )
			m_iTurnAngle--;
	}

	if( m_flAcceleratorDecay < gpGlobals->time )
	{
		m_flAcceleratorDecay = gpGlobals->time + 0.1f;

		if( pev->speed < 0 )
		{
			pev->speed += 20;

			if( pev->speed > 0 )
				pev->speed = 0;
		}
		else if( pev->speed > 0 )
		{
			pev->speed -= 20;

			if( pev->speed < 0 )
				pev->speed = 0;
		}
	}

	if( pev->speed == 0 )
	{
		m_iTurnAngle = 0;
		pev->avelocity = g_vecZero;
		pev->velocity = g_vecZero;

		SetThink( &CFuncVehicle::Next );
		NextThink( pev->ltime + time, TRUE );
		return;
	}

	TerrainFollowing();
	CollisionDetection();

	Vector temp;
	if( m_vSurfaceNormal != g_vecZero )
	{
		Vector vTargetAngle, vAngle;

		float vx;
		float vy;

		m_vVehicleDirection = CrossProduct( m_vSurfaceNormal, gpGlobals->v_forward );
		m_vVehicleDirection = CrossProduct( m_vSurfaceNormal, m_vVehicleDirection );

		vTargetAngle = UTIL_VecToAngles( m_vVehicleDirection );
		vAngle = pev->angles;

		vTargetAngle.y += 180;

		if( m_iTurnAngle != 0 )
		{
			vTargetAngle.y += m_iTurnAngle;
		}

		FixupAngles2( vTargetAngle );
		FixupAngles2( vAngle );

		vx = UTIL_AngleDistance( vTargetAngle.x, vAngle.x );
		vy = UTIL_AngleDistance( vTargetAngle.y, vAngle.y );

		if( vx > 10 )
			vx = 10;
		else if( vx < -10 )
			vx = -10;

		if( vy > 10 )
			vy = 10;
		else if( vy < -10 )
			vy = -10;

		pev->avelocity.y = (int)( vy * 10 );
		pev->avelocity.x = (int)( vx * 10 );

		m_flLaunchTime = -1;
		m_flLastNormalZ = m_vSurfaceNormal.z;
	}
	else
	{
		if( m_flLaunchTime != -1 )
		{
			vGravityVector.x = 0;
			vGravityVector.y = 0;
			vGravityVector.z = ( gpGlobals->time - m_flLaunchTime ) * -35;

			if( vGravityVector.z < -400 )
			{
				vGravityVector.z = -400;
			}
		}
		else
		{
			m_flLaunchTime = gpGlobals->time;
			vGravityVector = Vector( 0, 0, 0 );
			pev->velocity = pev->velocity * 1.5f;
		}

		m_vVehicleDirection = gpGlobals->v_forward * -1;
	}

	UTIL_VecToAngles( m_vVehicleDirection );

	if( m_flUpdateSound < gpGlobals->time )
	{
		UpdateSound();
		m_flUpdateSound = gpGlobals->time + 1.0f;
	}

	if( m_vSurfaceNormal != g_vecZero )
	{
		pev->velocity = m_vVehicleDirection.Normalize() * pev->speed;
	}
	else
	{
		pev->velocity = pev->velocity + vGravityVector;
	}

	SetThink( &CFuncVehicle::Next );
	NextThink( pev->ltime + time, TRUE );
}

void CFuncVehicle::DeadEnd()
{
	CPathTrack *pTrack = m_ppath;
	ALERT( at_aiconsole, "TRAIN(%s): Dead end ", STRING( pev->targetname ));

	if( pTrack != NULL )
	{
		CPathTrack *pNext;

		if( m_oldSpeed < 0 )
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetPrevious(), TRUE );

				if( pNext != NULL )
				{
					pTrack = pNext;
				}
			}
			while( pNext != NULL );
		}
		else
		{
			do
			{
				pNext = pTrack->ValidPath( pTrack->GetNext(), TRUE );

				if( pNext != NULL )
				{
					pTrack = pNext;
				}
			}
			while( pNext != NULL );
		}
	}

	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;

	if( pTrack != NULL )
	{
		ALERT( at_aiconsole, "at %s\n", STRING( pTrack->pev->targetname ));

		if( !FStringNull( pTrack->pev->netname ))
		{
			FireTargets( STRING( pTrack->pev->netname ), this, this, USE_TOGGLE, 0 );
		}
	}
	else
		ALERT( at_aiconsole, "\n" );
}

void CFuncVehicle::SetControls(entvars_t *pevControls)
{
	Vector offset = pevControls->origin - pev->oldorigin;
	m_controlMins = pevControls->mins + offset;
	m_controlMaxs = pevControls->maxs + offset;
}

BOOL CFuncVehicle::OnControls(entvars_t *pevTest)
{
	if( pev->spawnflags & SF_TRACKTRAIN_NOCONTROL )
		return FALSE;

	Vector offset = pevTest->origin - pev->origin;

	UTIL_MakeVectors( pev->angles );

	Vector local;
	local.x = DotProduct( offset, gpGlobals->v_forward );
	local.y = -DotProduct( offset, gpGlobals->v_right );
	local.z = DotProduct( offset, gpGlobals->v_up );

	return ( local.x >= m_controlMins.x && local.y >= m_controlMins.y && local.z >= m_controlMins.z
		&& local.x <= m_controlMaxs.x && local.y <= m_controlMaxs.y && local.z <= m_controlMaxs.z );
}

void CFuncVehicle::Find()
{
	m_ppath = CPathTrack::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target )));

	if( !m_ppath )
		return;

	entvars_t *pevTarget = m_ppath->pev;

	if( !FClassnameIs( pevTarget, "path_track" ))
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
	pev->angles.y += 180;

	if( pev->spawnflags & SF_TRACKTRAIN_NOPITCH )
	{
		pev->angles.x = 0;
	}

	UTIL_SetOrigin( pev, nextPos );
	NextThink( pev->ltime + 0.1f, FALSE );
	SetThink( &CFuncVehicle::Next );
	pev->speed = m_startSpeed;
	UpdateSound();
}

void CFuncVehicle::NearestPath()
{
	CBaseEntity *pTrack = NULL;
	CBaseEntity *pNearest = NULL;
	float dist;
	float closest = 1024;

	while( ( pTrack = UTIL_FindEntityInSphere( pTrack, pev->origin, 1024 )) != NULL )
	{
		if( !( pTrack->pev->flags & ( FL_CLIENT | FL_MONSTER )) && FClassnameIs( pTrack->pev, "path_track" ))
		{
			dist = ( pev->origin - pTrack->pev->origin ).Length();

			if( dist < closest )
			{
				closest = dist;
				pNearest = pTrack;
			}
		}
	}

	if( !pNearest )
	{
		ALERT( at_console, "Can't find a nearby track !!!\n" );
		SetThink( NULL );
		return;
	}

	ALERT( at_aiconsole, "TRAIN: %s, Nearest track is %s\n", STRING( pev->targetname ), STRING( pNearest->pev->targetname ));
	pTrack = ( (CPathTrack *)pNearest )->GetNext();

	if( pTrack != NULL )
	{
		if( ( pev->origin - pTrack->pev->origin ).Length() < ( pev->origin - pNearest->pev->origin ).Length())
		{
			pNearest = pTrack;
		}
	}

	m_ppath = (CPathTrack *)pNearest;
	if( pev->speed != 0 )
	{
		NextThink( pev->ltime + 0.1f, FALSE );
		SetThink( &CFuncVehicle::Next );
	}
}

void CFuncVehicle::OverrideReset()
{
	NextThink( pev->ltime + 0.1f, FALSE );
	SetThink( &CFuncVehicle::NearestPath );
}

CFuncVehicle *CFuncVehicle::Instance(edict_t *pent)
{
	if( FClassnameIs( pent, "func_vehicle" ))
	{
		return (CFuncVehicle *)GET_PRIVATE( pent );
	}

	return NULL;
}

int CFuncVehicle::Classify()
{
	return CLASS_VEHICLE;
}

void CFuncVehicle::Spawn()
{
	if( pev->speed == 0 )
		m_speed = 165;
	else
		m_speed = pev->speed;

	if( !m_sounds )
		m_sounds = 3;

	ALERT( at_console, "M_speed = %f\n", m_speed );

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;

	pev->impulse = (int)m_speed;
	m_acceleration = 5;

	m_dir = 1;
	m_flTurnStartTime = -1;

	if( FStringNull( pev->target ))
	{
		ALERT( at_console, "Vehicle with no target\n" );
	}

	if( pev->spawnflags & SF_TRACKTRAIN_PASSABLE )
		pev->solid = SOLID_NOT;
	else
		pev->solid = SOLID_BSP;

	pev->movetype = MOVETYPE_PUSH;

	SET_MODEL( ENT( pev ), STRING(pev->model ));
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->oldorigin = pev->origin;

	m_controlMins = pev->mins;
	m_controlMaxs = pev->maxs;
	m_controlMaxs.z += 72;

	NextThink( pev->ltime + 0.1f, FALSE );
	SetThink( &CFuncVehicle::Find );
	Precache();
}

void CFuncVehicle::Restart()
{
	ALERT( at_console, "M_speed = %f\n", m_speed );

	pev->speed = 0;
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;

	pev->impulse = (int)m_speed;
	m_flTurnStartTime = -1;
	m_flUpdateSound = -1;
	m_dir = 1;
	m_pDriver = NULL;

	if( FStringNull( pev->target ))
	{
		ALERT( at_console, "Vehicle with no target\n" );
	}

	UTIL_SetOrigin( pev, pev->oldorigin );
	STOP_SOUND( ENT( pev ), CHAN_STATIC, STRING( pev->noise ));

	NextThink( pev->ltime + 0.1f, FALSE );
	SetThink( &CFuncVehicle::Find );
}

void CFuncVehicle::Precache()
{
	if( m_flVolume == 0.0f )
		m_flVolume = 1.0f;

	switch( m_sounds )
	{
	case 1: PRECACHE_SOUND( "plats/vehicle1.wav" );pev->noise = MAKE_STRING( "plats/vehicle1.wav" ); break;
	case 2: PRECACHE_SOUND( "plats/vehicle2.wav" );pev->noise = MAKE_STRING( "plats/vehicle2.wav" ); break;
	case 3: PRECACHE_SOUND( "plats/vehicle3.wav" );pev->noise = MAKE_STRING( "plats/vehicle3.wav" ); break;
	case 4: PRECACHE_SOUND( "plats/vehicle4.wav" );pev->noise = MAKE_STRING( "plats/vehicle4.wav" ); break;
	case 5: PRECACHE_SOUND( "plats/vehicle6.wav" );pev->noise = MAKE_STRING( "plats/vehicle6.wav" ); break;
	case 6: PRECACHE_SOUND( "plats/vehicle7.wav" );pev->noise = MAKE_STRING( "plats/vehicle7.wav" ); break;
	}

	PRECACHE_SOUND( "plats/vehicle_brake1.wav" );
	PRECACHE_SOUND( "plats/vehicle_start1.wav" );

	m_usAdjustPitch = PRECACHE_EVENT( 1, "events/vehicle.sc" );
}

LINK_ENTITY_TO_CLASS( func_vehiclecontrols, CFuncVehicleControls );

void CFuncVehicleControls::Find()
{
	edict_t *pTarget = NULL;

	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING( pev->target ));
	}
	while( !FNullEnt( pTarget ) && !FClassnameIs( pTarget, "func_vehicle" ));

	if( FNullEnt( pTarget ))
	{
		ALERT( at_console, "No vehicle %s\n", STRING( pev->target ));
		return;
	}

	CFuncVehicle *pvehicle = CFuncVehicle::Instance( pTarget );

	pvehicle->SetControls( pev );
	UTIL_Remove( this );
}

void CFuncVehicleControls::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ));

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CFuncVehicleControls::Find );
	pev->nextthink = gpGlobals->time;
}
