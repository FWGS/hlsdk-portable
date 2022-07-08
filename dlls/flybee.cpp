//---------------------------------------------------------
//---------------------------------------------------------
//-														---
//-					flybee.cpp							---
//-														---
//---------------------------------------------------------
//---------------------------------------------------------
//-	par JujU									-----------
//-		julien.lecorre@free.fr					-----------
//---------------------------------------------------------
//- code du monstre flybee						-----------
//---------------------------------------------------------
//---------------------------------------------------------



//---------------------------------------------------------
// inclusions

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"

extern int gmsgClientDecal;


#define FLYBEE_SPEED		150
#define PROBE_LENGTH		150

#define FEAR_LEVEL		10
#define HATE_LEVEL		-10


//=========================================================
// monster-specific tasks and states
//=========================================================
enum 
{
	TASK_FLYBEE_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_FLYBEE_SWIM,
	TASK_FLYBEE_STOP_MOVING,
	TASK_FLYBEE_RUN_ATTACK,
	TASK_FLYBEE_RANGE_ATTACK2,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

#define FLYBEE_AE_HIT		1
#define FLYBEE_AE_ATTACK1	2
#define FLYBEE_AE_ATTACK2	3


//=========================================================
// D
//=========================================================


class CFlybee : public CFlyingMonster
{
public:

	void	Spawn( void );
	void	Precache( void );
	int		Classify( void );

	int		Save( CSave &save ); 
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;
	void	HandleAnimEvent( MonsterEvent_t *pEvent );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );

	void	StartTask( Task_t *pTask );
	void	RunTask( Task_t *pTask );

	BOOL	CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL	CheckRangeAttack1 ( float flDot, float flDist );
	BOOL	CheckRangeAttack2 ( float flDot, float flDist );

	Activity GetStoppedActivity( void );

	void	MonsterThink( void );

	void	Move( float flInterval );
	void	MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );

	void	Stop( void );
	void	Swim( void );

	void	SetYawSpeed( void );
	float	ChangeYaw( int speed );
	Vector	DoProbe(const Vector &Probe);

	float	VectorToPitch( const Vector &vec);
	float	FlPitchDiff( void );
	float	ChangePitch( int speed );

	Vector	m_SaveVelocity;
	float	m_idealDist;

	float	m_flMaxSpeed;
	float	m_flMinSpeed;
	float	m_flMaxDist;

	Vector	m_vecAttack2;

	int		m_iSpriteTexture;

	CBeam *m_pBeam;

	int		m_iFear;	
	float m_flNextAlert;
	float m_flNextAttack;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pAttackSounds[];
	static const char *pBiteSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];

	void IdleSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void BiteSound( void );
	void DeathSound( void );
	void PainSound( void );
};

LINK_ENTITY_TO_CLASS( monster_flybee, CFlybee );

TYPEDESCRIPTION	CFlybee::m_SaveData[] = 
{
	DEFINE_FIELD( CFlybee, m_SaveVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CFlybee, m_idealDist, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMinSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flMaxDist, FIELD_FLOAT ),
	DEFINE_FIELD( CFlybee, m_flNextAlert, FIELD_TIME ),
	DEFINE_FIELD( CFlybee, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFlybee, m_iFear, FIELD_INTEGER ),
	DEFINE_FIELD( CFlybee, m_vecAttack2, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFlybee, CFlyingMonster );



class CFlyBall : public CBaseMonster
{
public :
	void Spawn( void );
	void Precache( void );
	void EXPORT AnimateThink( void );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );

	int	Classify ( void );
	static CFlyBall *CreateFlyBall( Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner );

	int m_iSprite;
};
LINK_ENTITY_TO_CLASS( flyball, CFlyBall );




const char *CFlybee::pIdleSounds[] = 
{
	"ichy/ichy_idle1.wav",
	"ichy/ichy_idle2.wav",
	"ichy/ichy_idle3.wav",
	"ichy/ichy_idle4.wav",
};

const char *CFlybee::pAlertSounds[] = 
{
	"ichy/ichy_alert2.wav",
	"ichy/ichy_alert3.wav",
};

const char *CFlybee::pAttackSounds[] = 
{
	"ichy/ichy_attack1.wav",
	"ichy/ichy_attack2.wav",
};

const char *CFlybee::pBiteSounds[] = 
{
	"ichy/ichy_bite1.wav",
	"ichy/ichy_bite2.wav",
};

const char *CFlybee::pPainSounds[] = 
{
	"ichy/ichy_pain2.wav",
	"ichy/ichy_pain3.wav",
	"ichy/ichy_pain5.wav",
};

const char *CFlybee::pDieSounds[] = 
{
	"ichy/ichy_die2.wav",
	"ichy/ichy_die4.wav",
};

#define EMIT_ICKY_SOUND( chan, array ) \
	EMIT_SOUND_DYN ( ENT(pev), chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], 1.0, 0.6, 0, RANDOM_LONG(95,105) ); 


void CFlybee :: IdleSound( void )	
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pIdleSounds ); 
}

void CFlybee :: AlertSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pAlertSounds ); 
}

void CFlybee :: AttackSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pAttackSounds );
}

void CFlybee :: BiteSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_WEAPON, pBiteSounds );
}

void CFlybee :: DeathSound( void ) 
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pDieSounds ); 
}

void CFlybee :: PainSound( void )	
{ 
	EMIT_ICKY_SOUND( CHAN_VOICE, pPainSounds ); 
}


//=========================================================
// Spawn
//=========================================================
void CFlybee :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/flybee.mdl");
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24,24,24 ) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.ichthyosaurHealth;
	pev->view_ofs		= Vector ( 0, 0, 16 );
	m_flFieldOfView		= VIEW_FIELD_WIDE;
	m_MonsterState		= MONSTERSTATE_NONE;
	SetFlyingSpeed( FLYBEE_SPEED );
	SetFlyingMomentum( 2.5 );	// Set momentum constant

	m_afCapability		= bits_CAP_MELEE_ATTACK1 | bits_CAP_RANGE_ATTACK1 | bits_CAP_RANGE_ATTACK2 | bits_CAP_SWIM;

	MonsterInit();

	m_idealDist		= 384;
	m_flMinSpeed	= 80;
	m_flMaxSpeed	= 300;
	m_flMaxDist		= 384;

	m_iFear			= 0;
	m_flNextAttack	= 0;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFlybee :: Precache()
{
	PRECACHE_MODEL("models/flybee.mdl");

	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("debris/beamstart4.wav");
	PRECACHE_MODEL("sprites/nhth1.spr");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND_ARRAY( pDieSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	UTIL_PrecacheOther ( "flyball" );

	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");
}



//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFlybee :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}


//=========================================================
// Check Attacks
//=========================================================

BOOL CFlybee :: CheckMeleeAttack1 ( float flDot, float flDist )
{

	if ( flDist <= 64  )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CFlybee :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDot > -0.7 && m_iFear <= HATE_LEVEL )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CFlybee :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && m_iFear >= FEAR_LEVEL && m_flNextAttack < gpGlobals->time )
	{
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFlybee :: SetYawSpeed ( void )
{
	pev->yaw_speed = 100;
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFlybee :: HandleAnimEvent( MonsterEvent_t *pEvent )
{

	switch( pEvent->event )
	{
	case FLYBEE_AE_HIT:
		{
			TraceResult tr;

			UTIL_MakeVectors( pev->angles );
			Vector vecStart = pev->origin;
			vecStart.z += pev->size.z * 0.5;
			Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

			UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
			
			if ( tr.pHit )
			{
				CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
				pEntity->pev->punchangle.z = 25;
				pEntity->TakeDamage( pev, pev,20, DMG_CLUB );
			}
			break;
		}

	case FLYBEE_AE_ATTACK2:
		{
			UTIL_MakeVectors( pev->angles );
			TraceResult tr;

			Vector vecStart	= pev->origin + gpGlobals->v_up * 24 + gpGlobals->v_forward * 32;
			UTIL_TraceLine ( vecStart, m_vecAttack2, dont_ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

			Vector vecEnd = tr.vecEndPos;

			for ( int i = 0; i<4; i ++ )
			{
				CBeam *pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 8 );

				if ( RANDOM_LONG(0,1) )
					pBeam->SetColor( 206,118, 254 );
				else
					pBeam->SetColor( 223,224, 255 );

				pBeam->SetBrightness( 192 );

				pBeam->SetStartPos( vecStart );
				pBeam->SetEndPos ( vecEnd );
				pBeam->RelinkBeam( );

				pBeam->SetNoise ( 30 );
				pBeam->LiveForTime ( 0.35 );
			}

			CSprite *pSprite = CSprite::SpriteCreate ( "sprites/nhth1.spr", vecEnd, TRUE );
			pSprite->AnimateAndDie ( 10 );
			pSprite->SetScale ( 0.8 );
			pSprite->SetTransparency ( kRenderTransAdd, 230, 255, 230, 150, kRenderFxNone );
			pSprite->Expand ( pSprite->pev->scale, 120 );


			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );

				WRITE_BYTE( TE_BEAMCYLINDER );

				WRITE_COORD( vecEnd.x);	WRITE_COORD( vecEnd.y); WRITE_COORD( vecEnd.z);
				WRITE_COORD( vecEnd.x); WRITE_COORD( vecEnd.y); WRITE_COORD( vecEnd.z + 1000 );
				WRITE_SHORT( m_iSpriteTexture );
				WRITE_BYTE( 0 ); // startframe
				WRITE_BYTE( 0 ); // framerate
				WRITE_BYTE( 2 ); // life
				WRITE_BYTE( 16 );  // width
				WRITE_BYTE( 0 );   // noise
				WRITE_BYTE( 206 ); WRITE_BYTE( 118 ); WRITE_BYTE( 255 ); WRITE_BYTE( 80 ); // rgba
				WRITE_BYTE( 0 );		// speed
			MESSAGE_END();

			RadiusDamage ( vecEnd, pev, pev, 50, CLASS_ALIEN_MONSTER, DMG_SHOCK );

			UTIL_EmitAmbientSound( ENT(pev), vecEnd, "debris/beamstart4.wav", 0.9, ATTN_NORM, 0, RANDOM_LONG( 90, 99 ) );

			break;
		}
	case FLYBEE_AE_ATTACK1:
		{
			UTIL_MakeVectors( pev->angles );

			Vector vecSrc	= pev->origin + gpGlobals->v_up * 24 + gpGlobals->v_forward * 32;
			Vector ang		= UTIL_VecToAngles( (m_hEnemy->Center() - vecSrc).Normalize() );

			CFlyBall ::CreateFlyBall ( vecSrc + gpGlobals->v_right * 30, ang, pev );
			CFlyBall ::CreateFlyBall ( vecSrc + gpGlobals->v_right * 10, ang, pev );
			CFlyBall ::CreateFlyBall ( vecSrc - gpGlobals->v_right * 30, ang, pev );
			CFlyBall ::CreateFlyBall ( vecSrc - gpGlobals->v_right * 10, ang, pev );

			break;
		}
	default:
		CFlyingMonster::HandleAnimEvent( pEvent );
		break;

	}
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlFlybeeSwimAround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_FLYBEE_SWIM,				(float)0 },
};

static Schedule_t	slFlybeeSwimAround[] =
{
	{ 
		tlFlybeeSwimAround,
		ARRAYSIZE(tlFlybeeSwimAround), 
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY		|
		bits_COND_NEW_ENEMY		|
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"SwimAround"
	},
};

static Task_t	tlFlybeeSwimAgitated[] =
{
	{ TASK_STOP_MOVING,				(float) 0 },
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_WAIT,					(float)2.0 },
};

static Schedule_t	slFlybeeSwimAgitated[] =
{
	{ 
		tlFlybeeSwimAgitated,
		ARRAYSIZE(tlFlybeeSwimAgitated), 
		0, 
		0, 
		"SwimAgitated"
	},
};


static Task_t	tlFlybeeCircleEnemy[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_FLYBEE_CIRCLE_ENEMY, 0.0 },
};

static Schedule_t	slFlybeeCircleEnemy[] =
{
	{ 
		tlFlybeeCircleEnemy,
		ARRAYSIZE(tlFlybeeCircleEnemy), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"CircleEnemy"
	},
};


Task_t tlFlybeeTwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_DIE,					(float)0 },
};

Schedule_t slFlybeeTwitchDie[] =
{
	{
		tlFlybeeTwitchDie,
		ARRAYSIZE( tlFlybeeTwitchDie ),
		0,
		0,
		"Die"
	},
};

Task_t tlFlybeeRangeAttack1[] =
{
	{ TASK_FLYBEE_STOP_MOVING,	0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t slFlybeeRangeAttack1[] =
{
	{
		tlFlybeeRangeAttack1,
		ARRAYSIZE( tlFlybeeRangeAttack1 ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 1"
	},
};

Task_t tlFlybeeRangeAttack2[] =
{
	{ TASK_FLYBEE_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,					(float)0		},
	{ TASK_FLYBEE_RANGE_ATTACK2,		(float)0		},
};

Schedule_t slFlybeeRangeAttack2[] =
{
	{
		tlFlybeeRangeAttack2,
		ARRAYSIZE( tlFlybeeRangeAttack2 ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 2"
	},
};

Task_t tlFlybeeRunAttack[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},
	{ TASK_FLYBEE_STOP_MOVING,	(float) 0					},
	{ TASK_FLYBEE_RUN_ATTACK,	(float)	0					},
};

Schedule_t slFlybeeRunAttack[] =
{
	{
		tlFlybeeRunAttack,
		ARRAYSIZE( tlFlybeeRunAttack ),

		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Range attack 2"
	},
};



Task_t	tlFlybeeMeleeAttack[] =
{
	{ TASK_FLYBEE_STOP_MOVING,	(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slFlybeeMeleeAttack[] =
{
	{ 
		tlFlybeeMeleeAttack,
		ARRAYSIZE ( tlFlybeeMeleeAttack ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD,
		0,
		"Flybee Melee Attack"
	},
};

DEFINE_CUSTOM_SCHEDULES(CFlybee)
{
    slFlybeeSwimAround,
	slFlybeeSwimAgitated,
	slFlybeeCircleEnemy,
	slFlybeeTwitchDie,
	slFlybeeRangeAttack1,
	slFlybeeRangeAttack2,
	slFlybeeRunAttack,
	slFlybeeMeleeAttack,
};
IMPLEMENT_CUSTOM_SCHEDULES(CFlybee, CFlyingMonster);


//=========================================================
// GetSchedule
//=========================================================

Schedule_t* CFlybee::GetSchedule()
{
	switch(m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 80;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_ALERT:
		m_flightSpeed = 150;
		return GetScheduleOfType( SCHED_IDLE_WALK );

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 400;

		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
//			ALERT ( at_console, "MELEE1\n" );

			m_iFear = Q_max ( HATE_LEVEL, m_iFear - 5 );
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		}

		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
//			ALERT ( at_console, "ATTACK1\n" );

			m_iFear = Q_min ( FEAR_LEVEL, m_iFear + 5 );
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}

		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) )
		{
//			ALERT ( at_console, "ATTACK2\n" );

			m_iFear -= 10;
			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT ( 3,5 );
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}

		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		{
			m_iFear = Q_min ( FEAR_LEVEL, m_iFear + 3 );

			m_flightSpeed = Q_min ( m_flMaxSpeed, m_flightSpeed + 40 );
		}

		return GetScheduleOfType( SCHED_STANDOFF );
		
	}

	return CFlyingMonster :: GetSchedule();
}


//=========================================================
//=========================================================
Schedule_t* CFlybee :: GetScheduleOfType ( int Type ) 
{

	switch	( Type )
	{
	case SCHED_IDLE_WALK:
		return slFlybeeSwimAround;

	case SCHED_STANDOFF:
		return slFlybeeCircleEnemy;

	case SCHED_FAIL:
		return slFlybeeSwimAgitated;

	case SCHED_DIE:
		return slFlybeeTwitchDie;

	case SCHED_CHASE_ENEMY:
		AttackSound( );

	case SCHED_MELEE_ATTACK1:
		return slFlybeeMeleeAttack;

	case SCHED_RANGE_ATTACK1:
		return slFlybeeRangeAttack1;

	case SCHED_RANGE_ATTACK2:

		if ( m_hEnemy->pev->velocity.Length() <= 100 )
		{
			if ( (Center()-m_hEnemy->Center()).Length() <= 300 && fabs(pev->origin.z-m_hEnemy->pev->origin.z) <= 128 )
				return slFlybeeRunAttack;
			else
			{
				if ( RANDOM_LONG ( 0,2 ) == 0 )
					return slFlybeeRangeAttack1;
				else
					return slFlybeeRangeAttack2;
			}
		}
		else
		{
			return slFlybeeRangeAttack1;
		}

	case SCHED_BURNT:
		m_flightSpeed = m_flMaxSpeed;
		return slFlybeeSwimAround;

	}


	return CBaseMonster :: GetScheduleOfType( Type );
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================

void CFlybee::StartTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_FLYBEE_CIRCLE_ENEMY:
		break;

	case TASK_FLYBEE_SWIM:
		break;

	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_FLYBEE_STOP_MOVING:
		m_flightSpeed = 0;

		if ( m_IdealActivity == m_movementActivity )
		{
			m_IdealActivity = GetStoppedActivity();
		}

		RouteClear();
		TaskComplete();
		break;

	case TASK_FLYBEE_RUN_ATTACK:
		m_IdealActivity = ACT_CROUCH;
		m_flightSpeed = 128;
		RouteClear();
		break;

	case TASK_DIE:
		pev->movetype	= MOVETYPE_STEP;
		pev->angles.x = 0;
		UTIL_SetSize( pev, Vector( -8, -8, 0 ), Vector( 8,8,8 ) );

		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_FLYBEE_RANGE_ATTACK2:
		m_vecAttack2 = m_hEnemy->Center();
		m_IdealActivity = ACT_RANGE_ATTACK2;
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CFlybee :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FLYBEE_CIRCLE_ENEMY:
		if (m_hEnemy == NULL)
		{
			TaskComplete( );
		}
		else if (FVisible( m_hEnemy ))
		{

			// nouvelle position

			Vector vecFrom = m_hEnemy->EyePosition( );

			Vector vecDelta = (pev->origin - vecFrom).Normalize( );
			Vector vecSwim = CrossProduct( vecDelta, Vector( 0, 0, 1 ) ).Normalize( );
			
			if (DotProduct( vecSwim, m_SaveVelocity ) < 0)
				vecSwim = vecSwim * -1.0;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

		/*	if (!m_pBeam)
			{
				m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
				m_pBeam->SetColor( 255, 180, 96 );
				m_pBeam->SetBrightness( 192 );
			}
			m_pBeam->SetStartPos( m_hEnemy->Center() );
			m_pBeam->SetEndPos ( vecPos );
			m_pBeam->RelinkBeam( );
		*/							

			// test de la position

			TraceResult tr;
		
			UTIL_TraceHull( vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;



			// console

/*			ALERT ( at_console, "%s %c%c%c%c%c%c%c%c%c%c0%c%c%c%c%c%c%c%c%c%c - %i\n",

			HasConditions( bits_COND_ENEMY_FACING_ME ) && m_hEnemy->FVisible( this ) ? "FACING " : "facing ",
			m_iFear <= -10 ? '|' : '-',	m_iFear <= -9 ? '|' : '-',	m_iFear <= -8 ? '|' : '-',
			m_iFear <= -7 ? '|' : '-',	m_iFear <= -6 ? '|' : '-',	m_iFear <= -5 ? '|' : '-',
			m_iFear <= -4 ? '|' : '-',	m_iFear <= -3 ? '|' : '-',	m_iFear <= -2 ? '|' : '-',
			m_iFear <= -1 ? '|' : '-',	m_iFear >= 1 ? '|' : '-',	m_iFear >= 2 ? '|' : '-',
			m_iFear >= 3 ? '|' : '-',	m_iFear >= 4 ? '|' : '-',	m_iFear >= 5 ? '|' : '-',
			m_iFear >= 6 ? '|' : '-',	m_iFear >= 7 ? '|' : '-',	m_iFear >= 8 ? '|' : '-',
			m_iFear >= 9 ? '|' : '-',	m_iFear >= 10 ? '|' : '-',
			m_iFear);
*/


			// cons


			if ( m_hEnemy->MyMonsterPointer()->FInViewCone ( this ) && m_hEnemy->FVisible( this ))
			{
				m_flNextAlert -= 0.1;

				if ( m_iFear < FEAR_LEVEL )
					m_iFear ++;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flightSpeed < m_flMaxSpeed)
				{
					m_flightSpeed += 40;
				}
				else if (m_flightSpeed < m_flMinSpeed)
				{
					m_flightSpeed += 80;
				}
			}
			else 
			{
				m_flNextAlert += 0.1;

				if ( m_iFear > HATE_LEVEL )
					m_iFear --;


				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flightSpeed > m_flMinSpeed)
				{
					m_flightSpeed -= 3;
				}
			}

			if (m_flightSpeed < m_flMinSpeed)
			{
				m_flightSpeed = 128;
			}

			if (m_flightSpeed > m_flMaxSpeed)
			{
				m_flightSpeed = m_flMaxSpeed;
			}

			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound( );
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
		}

		break;


	case TASK_FLYBEE_RUN_ATTACK:
		{
			if (m_fSequenceFinished && m_IdealActivity == ACT_CROUCH )
			{
				m_IdealActivity = ACT_RUN;
			}
			else if (m_fSequenceFinished )
				TaskComplete( );

			TraceResult tr;
			UTIL_TraceHull( pev->origin, m_hEnemy->Center(), ignore_monsters, large_hull, m_hEnemy->edict(), &tr );

			if (tr.flFraction < 0.9)
			{
//				ALERT ( at_console, "RUNATTACK fails\n" );
				TaskFail ();
				break;
			}

			m_SaveVelocity = (tr.vecEndPos - pev->origin).Normalize() * m_flightSpeed;

			m_flightSpeed = Q_min ( m_flMaxSpeed, m_flightSpeed *= 1.2 );


			break;
		}

	case TASK_FLYBEE_RANGE_ATTACK2:
		{
			if ( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}



	case TASK_FLYBEE_SWIM:
		if (m_fSequenceFinished )
		{
			TaskComplete( );
		}
		break;

	case TASK_DIE:

		if ( m_fSequenceFinished && m_IdealActivity == ACT_DIESIMPLE && FBitSet( pev->flags, FL_ONGROUND) )
		{
			pev->deadflag = DEAD_DYING;
			m_IdealActivity = ACT_LAND;

			ResetSequenceInfo ();
		}
		else if ( m_fSequenceFinished && m_IdealActivity == ACT_DIESIMPLE )
		{
			pev->deadflag = DEAD_DYING;
		}
		else if ( m_fSequenceFinished && m_IdealActivity == ACT_LAND )
		{
			CFlyingMonster :: RunTask ( pTask );
			pev->deadflag = DEAD_DEAD;
		}
		break;

	default: 
		CFlyingMonster :: RunTask ( pTask );
		break;
	}
}



float CFlybee::VectorToPitch( const Vector &vec )
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int) (atan2(vec.z, sqrt(vec.x*vec.x+vec.y*vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CFlybee::Move(float flInterval)
{
	CFlyingMonster::Move( flInterval );
}

float CFlybee::FlPitchDiff( void )
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod( pev->angles.z );

	if ( flCurrentPitch == pev->idealpitch )
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if ( pev->idealpitch > flCurrentPitch )
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else 
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CFlybee :: ChangePitch( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlPitchDiff();
		float target = 0;
		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}
		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * 0.1 );
	}
	return 0;
}

float CFlybee::ChangeYaw( int speed )
{
	if ( pev->movetype == MOVETYPE_FLY )
	{
		float diff = FlYawDiff();
		float target = 0;

		if ( m_IdealActivity != GetStoppedActivity() )
		{
			if ( diff < -20 )
				target = 20;
			else if ( diff > 20 )
				target = -20;
		}
		pev->angles.z = UTIL_Approach( target, pev->angles.z, 220.0 * 0.1 );
	}
	return CFlyingMonster::ChangeYaw( speed );
}


Activity CFlybee:: GetStoppedActivity( void )
{ 
	if ( pev->movetype != MOVETYPE_FLY )		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CFlybee::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}


void CFlybee::MonsterThink ( void )
{
	CFlyingMonster::MonsterThink( );

	if ( pev->deadflag == DEAD_NO && m_MonsterState != MONSTERSTATE_SCRIPT )
	{
		Swim( );
	}
}

void CFlybee :: Stop( void ) 
{
		m_flightSpeed = 80.0;
}

void CFlybee::Swim( )
{
	int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80;

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );


	// ALERT( at_console, "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );

/*
	m_pBeam->SetStartPos( pev->origin + pev->velocity );
	m_pBeam->RelinkBeam( );
*/

	// ALERT( at_console, "speed %f\n", m_flightSpeed );
	
	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -80) pev->angles.x = -80;
	if (pev->angles.x >  80) pev->angles.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20) pev->angles.z = -20;
	if (pev->angles.z >  20) pev->angles.z =  20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);

	// UTIL_MoveToOrigin ( ENT(pev), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}


Vector CFlybee::DoProbe(const Vector &Probe)
{
	Vector WallNormal = Vector(0,0,-1); // WATER normal is Straight Down for fish.
	float frac;
	BOOL bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0) tr.flFraction = 0.0;
		if (tr.flFraction > 1.0) tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();
		
		return SteeringVector;
	}
	return Vector(0, 0, 0);
}




//--------------------------------------------------------------------------


CFlyBall *CFlyBall :: CreateFlyBall ( Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner )
{
	CFlyBall *pBall = GetClassPtr( (CFlyBall *)NULL );

	UTIL_MakeAimVectors ( vecAngles );

	float x, y, z;
	do
	{
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	}
	while (z > 1);

	Vector vecDir = gpGlobals->v_forward +
					x * VECTOR_CONE_6DEGREES.x * gpGlobals->v_right +
					y * VECTOR_CONE_6DEGREES.y * gpGlobals->v_up;

	pBall->pev->angles = UTIL_VecToAngles ( vecDir.Normalize() );

	pBall->pev->angles.x = -pBall->pev->angles.x;

	UTIL_SetOrigin( pBall->pev, vecOrigin );

	pBall->pev->owner = ENT ( pevOwner );

	pBall->Spawn();
	pBall->SetTouch( &CFlyBall::ExplodeTouch );
	
	return pBall;
}

void CFlyBall :: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("flyball");

	SET_MODEL(ENT(pev), "sprites/muz7.spr");
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 190;
	pev->scale = 0.2;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CFlyBall::AnimateThink );
	SetTouch( &CFlyBall::ExplodeTouch );

	pev->dmgtime = gpGlobals->time; // keep track of when ball spawned
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeVectors ( pev->angles );
	pev->velocity = gpGlobals->v_forward * 1000;

	m_flFieldOfView = -1;
	m_hEnemy = NULL;
}

int CFlyBall::Classify ( void )
{
	return CLASS_ALIEN_BIOWEAPON;
}

void CFlyBall :: Precache( void )
{
	PRECACHE_MODEL("sprites/xspark3.spr");
	PRECACHE_SOUND("weapons/electro4.wav");

	m_iSprite = PRECACHE_MODEL("sprites/muz7.spr");
}


void CFlyBall :: AnimateThink( void  )
{
	pev->nextthink = gpGlobals->time + 0.05;

	// sprite
	
	float delta = gpGlobals->time - pev->dmgtime;

	if ( delta > 5 || pev->velocity.Length() < 10)
	{
		SetTouch( NULL );
		UTIL_Remove( this );
	}

	// train

	if ( delta > 0 && delta < 0.9 )
	{
		CSprite *pTrail = CSprite::SpriteCreate ( "sprites/xspark3.spr", pev->origin, TRUE );
		pTrail->AnimateAndDie ( 22 );
		pTrail->SetScale ( 0.2 );
		pTrail->SetTransparency ( kRenderTransAdd, 230, 255, 230, 150, kRenderFxNone );
		pTrail->Expand ( pTrail->pev->scale, 120 );

	}
}


void CFlyBall::ExplodeTouch( CBaseEntity *pOther )
{
	// truc affreux pour creer une particule a travers ce message pour les decals

	TraceResult trace = UTIL_GetGlobalTrace( );
	Vector vecNorm = trace.vecPlaneNormal.Normalize();

	for ( int i = 0; i<2; i++ )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgClientDecal );

			WRITE_COORD( trace.vecEndPos.x );			// xyz source
			WRITE_COORD( trace.vecEndPos.y );
			WRITE_COORD( trace.vecEndPos.z );
			WRITE_COORD( vecNorm.x );						// xyz norme
			WRITE_COORD( vecNorm.y );
			WRITE_COORD( vecNorm.z );
			WRITE_CHAR ( 'C' );						// type de texture
			WRITE_BYTE ( 1 );						//  4 == electro-rocket

		MESSAGE_END();
	}

	if (pOther->pev->takedamage)
	{
		if ( pOther->pev != VARS ( pev->owner ) )
		{
			ClearMultiDamage( );
			pOther->TraceAttack(pev, 20, pev->velocity.Normalize(), &trace, DMG_ENERGYBEAM ); 
			ApplyMultiDamage( pev, pev );
		}
	}

	UTIL_EmitAmbientSound( ENT(pev), trace.vecEndPos, "weapons/electro4.wav", 0.3, ATTN_NORM, 0, RANDOM_LONG( 90, 99 ) );

	UTIL_Remove( this );
}


