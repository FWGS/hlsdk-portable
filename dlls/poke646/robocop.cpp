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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"explode.h"

int gRobocopGibModel, gWaveSprite;

void SpawnExplosion( Vector center, float randomRange, float time, int magnitude );

#define ROBOCOP_DAMAGE				(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)
#define ROBOCOP_EYE_SPRITE_NAME			"sprites/gargeye1.spr"
#define ROBOCOP_EYE_BEAM_NAME			"sprites/smoke.spr"

#define ROBOCOP_MELEE_ATTACK_DIST		60.0f

#define ROBOCOP_DEATH_DURATION			2.1f

#define LF_ROBOCOP_LASER		1
#define LF_ROBOCOP_BEAMSPOT		2
#define LF_ROBOCOP_BEAM			4
#define LF_ROBOCOP_LOWBRIGHTNESS	8
#define LF_ROBOCOP_HIGHBRIGHTNESS	16
#define LF_ROBOCOP_FULLBRIGHTNESS	32

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define ROBOCOP_AE_RIGHT_FOOT		0x03
#define ROBOCOP_AE_LEFT_FOOT		0x04
#define ROBOCOP_AE_FIST				0x05

class CRoboCop : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void UpdateOnRemove();
	void SetYawSpeed( void );
	int  Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetActivity( Activity NewActivity );

	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack2( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -80, -80, 0 );
		pev->absmax = pev->origin + Vector( 80, 80, 214 );
	}

	void PrescheduleThink( void );
	BOOL ShouldGibMonster( int iGib ) { return FALSE; }
	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void FistAttack( void );
	void CreateLaser( void );
	void ChangeLaserState( void );
	void HeadControls( float angleX, float angleY, bool zeropoint );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	CSprite	*m_pLaserPointer;
	CBeam	*m_pBeam;
	CSprite	*m_pBeamSpot;
	int	m_iLaserFlags;
	// int	m_iLaserAlpha;
	float	m_flHeadX;
	float	m_flHeadY;
	float	m_flLaserTime;
	float	m_flSparkTime;
	Vector	m_vecAimPos;
};

LINK_ENTITY_TO_CLASS( monster_robocop, CRoboCop )

TYPEDESCRIPTION	CRoboCop::m_SaveData[] =
{
	DEFINE_FIELD( CRoboCop, m_pLaserPointer, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_pBeamSpot, FIELD_CLASSPTR ),
	DEFINE_FIELD( CRoboCop, m_iLaserFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CRoboCop, m_flHeadX, FIELD_FLOAT ),
	DEFINE_FIELD( CRoboCop, m_flHeadY, FIELD_FLOAT ),
	DEFINE_FIELD( CRoboCop, m_vecAimPos, FIELD_POSITION_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CRoboCop, CBaseMonster )

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	SCHED_ROBOCOP_LASERFAIL = LAST_COMMON_SCHEDULE + 1,
};

enum
{
	TASK_ROBOCOP_LASER_SOUND = LAST_COMMON_TASK + 1,
	TASK_ROBOCOP_LASER_CHARGE,
	TASK_ROBOCOP_LASER_ON,
	TASK_ROBOCOP_MORTAR_SPAWN,
	TASK_ROBOCOP_LASER_OFF
};

Task_t	tlRoboCopLaser[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_ROBOCOP_LASERFAIL },
	{ TASK_STOP_MOVING, 0.0f },
	{ TASK_FACE_ENEMY, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_RANGE_ATTACK1 },
	{ TASK_ROBOCOP_LASER_CHARGE, 2.0f },
	{ TASK_ROBOCOP_LASER_SOUND, 0.0f },
	{ TASK_ROBOCOP_LASER_ON, 1.0f },
	{ TASK_ROBOCOP_MORTAR_SPAWN, 0.1f },
	{ TASK_ROBOCOP_LASER_OFF, 1.0f }
};

Schedule_t	slRoboCopLaser[] =
{
	{
		tlRoboCopLaser,
		ARRAYSIZE( tlRoboCopLaser ),
		bits_COND_TASK_FAILED |
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		0,
		"RoboCopLaser"
	}
};

Task_t	tlRoboCopLaserFail[] =
{
	{ TASK_ROBOCOP_LASER_OFF, 0.0f },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2.0f },
	{ TASK_WAIT_PVS, 0.0f }
};

Schedule_t	slRoboCopLaserFail[] =
{
	{
		tlRoboCopLaserFail,
		ARRAYSIZE( tlRoboCopLaserFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"RoboCopLaserFail"
	}
};

DEFINE_CUSTOM_SCHEDULES( CRoboCop )
{
	slRoboCopLaser,
	slRoboCopLaserFail
};

IMPLEMENT_CUSTOM_SCHEDULES( CRoboCop, CBaseMonster )

void CRoboCop::FistAttack( void )
{
	int i;
	unsigned char r, g, b;
	TraceResult trace;
	Vector vecDist;
	float flDist, flAdjustedDamage;

	UTIL_MakeVectors( pev->angles );
	Vector vecSrc = pev->origin + 12 * gpGlobals->v_right + 95 * gpGlobals->v_forward;
          
	for( i = 0; i < 3; i++ )
	{
		switch( i )
		{
		case 0:
			r = 101; g = 133; b = 221;
			break;
		case 1:
			r = 67; g = 85; b = 255;
			break;
		case 2:
			r = 62; g = 33; b = 211;
			break;
		}

		// blast circles
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z + 16 );
			WRITE_COORD( vecSrc.x );
			WRITE_COORD( vecSrc.y );
			WRITE_COORD( vecSrc.z + gSkillData.robocopSWRadius / ( ( i + 1 ) * 0.2f ) ); // reach damage radius over .3 seconds
			WRITE_SHORT( gWaveSprite );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 10 ); // framerate
			WRITE_BYTE( i + 2 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( r );  // r
			WRITE_BYTE( g );  // g
			WRITE_BYTE( b ); // b
			WRITE_BYTE( 255 ); //brightness
			WRITE_BYTE( 0 );          // speed
		MESSAGE_END();
	}

	CBaseEntity *pEntity = NULL;

	// iterate on all entities in the vicinity.
	while( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, gSkillData.robocopSWRadius ) ) != NULL )
	{
		if( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// Robocop does not take damage from it's own attacks.
			if( pEntity != this )
			{
				// houndeyes do FULL damage if the ent in question is visible. Half damage otherwise.
				// This means that you must get out of the houndeye's attack range entirely to avoid damage.
				// Calculate full damage first

				vecDist = pEntity->Center() - vecSrc;
				flDist = Q_max( 0, gSkillData.robocopSWRadius - vecDist.Length() );

				flDist = flDist / gSkillData.robocopSWRadius;

				if( !FVisible( pEntity ) )
				{
					if( pEntity->IsPlayer() )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flDist *= 0.5f;
					}
					else if( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) )
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flDist = 0;
					}
				}

				flAdjustedDamage = gSkillData.robocopDmgFist * flDist;
				//ALERT ( at_aiconsole, "Damage: %f\n", flAdjustedDamage );

				if( flAdjustedDamage > 0 )
				{
					pEntity->TakeDamage( pev, pev, flAdjustedDamage, DMG_SONIC );
				}

				if( pEntity->IsPlayer() )
				{
					vecDist = vecDist.Normalize();
					vecDist.x = vecDist.x * flDist * 600.0f;
					vecDist.y = vecDist.y * flDist * 600.0f;
					vecDist.z = flDist * 450.0f;
					pEntity->pev->velocity = vecDist + pEntity->pev->velocity;
					pEntity->pev->punchangle.x = 5;
				}
			}
		}
	}

	UTIL_ScreenShake( pev->origin, 12.0f, 100.0f, 2.0f, 1000 );
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "robocop/rc_fist.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG( -10, 10 ) );
}

void CRoboCop::CreateLaser( void )
{
	m_pLaserPointer = CSprite::SpriteCreate( ROBOCOP_EYE_SPRITE_NAME, pev->origin, FALSE );
	m_pLaserPointer->SetTransparency( kRenderTransAdd, 255, 255, 255, 0, kRenderFxNone );
	m_pLaserPointer->SetAttachment( edict(), 1 );
	m_pLaserPointer->SetScale( 0.5f );

	m_pBeamSpot = CSprite::SpriteCreate( ROBOCOP_EYE_SPRITE_NAME, pev->origin, FALSE );
	m_pBeamSpot->pev->origin = pev->origin;
	m_pBeamSpot->SetTransparency( kRenderTransAdd, 255, 255, 255, 0, kRenderFxNone );
	m_pBeamSpot->SetScale( 0.3f );

	m_pBeam = CBeam::BeamCreate( ROBOCOP_EYE_BEAM_NAME, 30 );
	m_pBeam->PointEntInit( pev->origin, entindex() );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->SetBrightness( 0 );
	m_pBeam->SetColor( 255, 0, 0 );
	m_pBeam->SetScrollRate( 15 );

	ChangeLaserState();
}

void CRoboCop::ChangeLaserState( void )
{
	float time;
	int brightness;

	if( m_pBeam )
		m_pBeam->SetEndAttachment( 1 );

	if( !m_iLaserFlags )
	{
		if( m_pBeam )
			m_pBeam->SetBrightness( 0 );

		if( m_pLaserPointer )
			m_pLaserPointer->SetBrightness( 0 );

		if( m_pBeamSpot )
			m_pBeamSpot->SetBrightness( 0 );

		return;
	}

	if( m_iLaserFlags & LF_ROBOCOP_LOWBRIGHTNESS )
	{
		time = gpGlobals->time;
	}
	else
	{
		if( !( m_iLaserFlags & LF_ROBOCOP_HIGHBRIGHTNESS ) )
		{
			if( m_iLaserFlags & LF_ROBOCOP_FULLBRIGHTNESS )
			{
				brightness = 224;
				goto end;
			}
		}

		time = gpGlobals->time * 5.0f;
	}

	brightness = fabs( sin( time ) * 255.0f );

end:
	if( m_iLaserFlags & LF_ROBOCOP_LASER )
	{
		if( m_pLaserPointer )
			m_pLaserPointer->SetBrightness( brightness );
	}

	if( m_iLaserFlags & LF_ROBOCOP_BEAM )
	{
		if( m_pBeam )
			m_pBeam->SetBrightness( brightness );		
	}

	if( m_iLaserFlags & LF_ROBOCOP_BEAMSPOT )
	{
		if( m_pBeamSpot )
			m_pBeamSpot->SetBrightness( brightness );
	}
}

void CRoboCop::HeadControls( float angleX, float angleY, bool zeropoint )
{
        if( angleY < -180 )
                angleY += 360;
        else if( angleY > 180 )
                angleY -= 360;
                                
        if( angleY < -45 )
                angleY = -45;
        else if( angleY > 45 )
                angleY = 45;
                 
	if( zeropoint )
	{
		m_flHeadX = angleX;
		m_flHeadY = angleY;
	}
	else
	{
		m_flHeadX = UTIL_ApproachAngle( angleX, m_flHeadX, 4 );
		m_flHeadY = UTIL_ApproachAngle( angleY, m_flHeadY, 8 );
	}

        SetBoneController( 0, m_flHeadY );
        SetBoneController( 1, m_flHeadX );
}

void CRoboCop::PrescheduleThink( void )
{
	if( m_flLaserTime < gpGlobals->time && m_iLaserFlags != ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS ) )
	{
		m_iLaserFlags = 0;
		ChangeLaserState();
		m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS );
	}

	ChangeLaserState();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CRoboCop::Classify( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CRoboCop::SetYawSpeed( void )
{
	int ys;

	switch( m_Activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Spawn
//=========================================================
void CRoboCop::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/robocop.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 128 ) );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health		= gSkillData.robocopHealth;
	m_flFieldOfView		= -0.7;// width of forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	CreateLaser();

	m_flSparkTime = gpGlobals->time + 2.0f;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CRoboCop::Precache()
{
	PRECACHE_MODEL( "models/robocop.mdl" );

	PRECACHE_MODEL( ROBOCOP_EYE_SPRITE_NAME );
	PRECACHE_MODEL( ROBOCOP_EYE_BEAM_NAME );

	gWaveSprite = PRECACHE_MODEL( "sprites/xbeam3.spr" );
	gRobocopGibModel = PRECACHE_MODEL( "models/metalplategibs.mdl" );
	PRECACHE_SOUND( "weapons/mortar.wav" );
	PRECACHE_SOUND( "ambience/sparks.wav" );
	PRECACHE_SOUND( "robocop/rc_charge.wav" );
	PRECACHE_SOUND( "robocop/rc_fist.wav" );
	PRECACHE_SOUND( "robocop/rc_laser.wav" );
	PRECACHE_SOUND( "robocop/rc_step1.wav" );
	PRECACHE_SOUND( "robocop/rc_step2.wav" );

	UTIL_PrecacheOther( "monster_mortar" );
}

void CRoboCop::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();
	
	if( m_pLaserPointer )
	{
		UTIL_Remove( m_pLaserPointer );
		m_pLaserPointer = 0;
	}

	if( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = 0;
	}

	if( m_pBeamSpot )
	{
		UTIL_Remove( m_pBeamSpot );
		m_pBeamSpot = 0;
	}
}

void CRoboCop::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	bitsDamageType &= ROBOCOP_DAMAGE;

	if( IsAlive() && !FBitSet( bitsDamageType, ROBOCOP_DAMAGE ) )
	{
		if( pev->dmgtime != gpGlobals->time || (RANDOM_LONG( 0, 100 ) < 20 ) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 0.5f, 1.5f ) );
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.0f;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CRoboCop::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{               
	ALERT( at_aiconsole, "RoboCop: TakeDamage\n" );

	if( IsAlive() )
	{
		if( !( bitsDamageType & ROBOCOP_DAMAGE ) )
			flDamage *= 0.01f;

		if( bitsDamageType & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CRoboCop::Killed( entvars_t *pevAttacker, int iGib )
{
        UTIL_Remove( m_pLaserPointer );
	UTIL_Remove( m_pBeam );
	UTIL_Remove( m_pBeamSpot );

	m_pLaserPointer = NULL;
	m_pBeam = NULL;
	m_pBeamSpot = NULL;

	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

BOOL CRoboCop::CheckMeleeAttack1( float flDot, float flDist )
{
	//ALERT( at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist );

	if( gpGlobals->time > m_flLaserTime )
	{
		if( flDot >= 0.8f && flDist < gSkillData.robocopSWRadius )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
BOOL CRoboCop::CheckRangeAttack1( float flDot, float flDist )
{
	if( gpGlobals->time > m_flLaserTime && gpGlobals->time > m_flSparkTime )
	{
		if( flDot >= 0.8f && flDist > gSkillData.robocopSWRadius )
		{
			if( flDist < 4096.0f )
				return TRUE;
		}
	}

	return FALSE;
}

void CRoboCop::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case ROBOCOP_AE_RIGHT_FOOT:
	case ROBOCOP_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0f, 3.0f, 1.0f, 250.0f );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, RANDOM_LONG( 0, 1 ) ? "robocop/rc_step2.wav" : "robocop/rc_step1.wav", 1, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG( -10, 10 ) );
		break;
	case ROBOCOP_AE_FIST:
		FistAttack();
		m_flLaserTime = gpGlobals->time + 2.0f;
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

Schedule_t *CRoboCop::GetScheduleOfType( int Type )
{
	switch( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return slRoboCopLaser;

	case SCHED_ROBOCOP_LASERFAIL:
		return slRoboCopLaserFail;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

void CRoboCop::StartTask( Task_t *pTask )
{
	TraceResult tr;

	switch( pTask->iTask )
	{
	case TASK_ROBOCOP_LASER_CHARGE: // 92
		{
			ALERT( at_aiconsole, "RoboCop: Charge Laser\n" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "robocop/rc_charge.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
			m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_HIGHBRIGHTNESS );
			m_flWaitFinished = gpGlobals->time + pTask->flData;
			m_flSparkTime = m_flLaserTime = gpGlobals->time + 10.0f;
		}
		break;

	case TASK_ROBOCOP_LASER_ON: // 93:
		{
			ALERT( at_aiconsole, "RoboCop: Laser on\n" );
			m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_BEAM | LF_ROBOCOP_BEAMSPOT | LF_ROBOCOP_FULLBRIGHTNESS );
			m_flWaitFinished = gpGlobals->time + pTask->flData;

			if( m_pBeam )
			{
				m_pBeam->pev->origin = m_vecAimPos;
				m_pBeam->SetEndAttachment( 1 );
			}

			if( m_pBeamSpot )
				m_pBeamSpot->pev->origin = m_vecAimPos;

			m_failSchedule = 0;
		}
		break;

	case TASK_ROBOCOP_MORTAR_SPAWN: //94:
		{
			ALERT( at_aiconsole, "RoboCop: Spawn mortar\n" );
			UTIL_TraceLine( m_vecAimPos, m_vecAimPos - Vector( 0, 0, 1024 ), ignore_monsters, ENT( pev ), &tr );
			CBaseEntity *pMortar = Create( "monster_mortar", tr.vecEndPos, g_vecZero, 0 );
			pMortar->pev->nextthink = gpGlobals->time + 0.1f;
			pMortar->pev->dmg = gSkillData.robocopDmgMortar;
			m_flWaitFinished = gpGlobals->time + pTask->flData; 
		}
		break;
	case TASK_ROBOCOP_LASER_SOUND: // 91
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "robocop/rc_laser.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );		
		TaskComplete();
		break;
	case TASK_ROBOCOP_LASER_OFF: // 95
		HeadControls( 0.0f, 0.0f, true );
		m_iLaserFlags = 0;
		ChangeLaserState();
		ALERT( at_aiconsole, "RoboCop: Laser off\n" );
		m_flSparkTime = gpGlobals->time + 2.0f;
		m_flLaserTime = gpGlobals->time + 1.0f;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "common/null.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		TaskComplete();
		break;
	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + ROBOCOP_DEATH_DURATION;
		ALERT( at_aiconsole, "RoboCop: Die\n" );
		m_iLaserFlags = ( LF_ROBOCOP_LASER | LF_ROBOCOP_LOWBRIGHTNESS );
		pev->renderamt = 15;
		pev->renderfx = kRenderFxGlowShell;
		pev->rendercolor = Vector( 67, 85, 255 );
		pev->health = 0;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "ambience/sparks.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		m_flSparkTime = gpGlobals->time + 0.3f;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CRoboCop::RunTask( Task_t *pTask )
{
	TraceResult tr;

	switch( pTask->iTask )
	{
	case TASK_ROBOCOP_LASER_ON:
	case TASK_ROBOCOP_MORTAR_SPAWN:
		if( gpGlobals->time > m_flWaitFinished )
			TaskComplete();
		break;
	case TASK_ROBOCOP_LASER_CHARGE:
	{
		if( gpGlobals->time > m_flWaitFinished )
			TaskComplete();

		if( m_hEnemy == 0 )
		{
			TaskFail();
			return;
		}

		Vector vecSrc = m_hEnemy->pev->origin;
		Vector vecDir, vecAngle;
		UTIL_TraceLine( vecSrc + Vector( 0, 0, 1024 ), vecSrc - Vector( 0, 0, 1024 ), ignore_monsters, ENT( pev ), &tr );
		GetAttachment( 0, vecSrc, vecAngle );
		vecDir = ( tr.vecEndPos - vecSrc ).Normalize();

		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 4096, dont_ignore_monsters, ENT( pev ), &tr );

		m_vecAimPos = tr.vecEndPos;

		vecAngle = UTIL_VecToAngles( vecDir );

		vecAngle.y = UTIL_AngleDiff( vecAngle.y, pev->angles.y );
		vecAngle.x = -vecAngle.x;

		if( fabs( vecAngle.y ) > ROBOCOP_MELEE_ATTACK_DIST )
		{
			ALERT( at_aiconsole, "RoboCop: Lost - FOV\n" );
			TaskFail();
			return;
		}

		if( ( m_vecAimPos - vecSrc ).Length() < gSkillData.robocopSWRadius )
		{
			ALERT( at_aiconsole, "RoboCop: Lost - Proximity\n" );
			TaskFail();
			return;
		}

		HeadControls( vecAngle.x, vecAngle.y, false );
	}
		break;

	case TASK_DIE:
		if( m_flWaitFinished > gpGlobals->time )
		{
			if( m_fSequenceFinished && pev->frame >= 255.0f )
			{
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "common/null.wav", 1.0, ATTN_NORM, 0, 100 );

				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
					WRITE_BYTE( TE_BREAKMODEL );

					// position
					WRITE_COORD( pev->origin.x );
					WRITE_COORD( pev->origin.y );
					WRITE_COORD( pev->origin.z );

					// size
					WRITE_COORD( 200 );
					WRITE_COORD( 200 );
					WRITE_COORD( 128 );

					// velocity
					WRITE_COORD( 0 );
					WRITE_COORD( 0 );
					WRITE_COORD( 0 );

					// randomization
					WRITE_BYTE( 200 );

					// Model
					WRITE_SHORT( gRobocopGibModel );   //model id#

					// # of shards
					WRITE_BYTE( 20 );

					// duration
					WRITE_BYTE( 20 );// 3.0 seconds

					// flags
					WRITE_BYTE( BREAK_FLESH );
				MESSAGE_END();

				SpawnExplosion( pev->origin, 70, 0, 150 );

				int trailCount = RANDOM_LONG( 2, 4 );

				for( int i = 0; i < trailCount; i++ )
					Create( "fire_trail", pev->origin, Vector( 0, 0, 1 ), NULL );

				SetBodygroup( 0, 1 );

				CBaseMonster::RunTask( pTask );
				return;
			}
		}

		if( gpGlobals->time <= m_flSparkTime )
		{
			Create( "spark_shower", pev->origin, Vector( 0, 0, 1 ), NULL );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "ambience/sparks.wav", 1.0, ATTN_NORM, 0, 100 );
			m_flSparkTime = gpGlobals->time + 0.3f;
		}
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

void CRoboCop::SetActivity( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch( m_Activity )
	{
	case ACT_WALK:
		m_flGroundSpeed = 220.0f;
		break;
	default:
		m_flGroundSpeed = 220.0f;
		break;
	}
}

