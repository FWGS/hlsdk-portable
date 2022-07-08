//-------------------------------------------------
//-												---
//-			luciole.cpp							---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code de la luciole				   -------------
//-------------------------------------------------


//----------------------------------------
// inclusions


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"effects.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"squadmonster.h"
#include	"customentity.h"


// alertes consoles et beams
#define DEBUG_ALERTS

extern Vector Intersect( Vector vecSrc, Vector vecDst, Vector vecMove, float flSpeed );

#define MAX_LUCIOLE_SQUAD_MEMBERS		20
#define ATTACK_DELAY					0.7
#define WAYPOINT_DELAY					0.5

#define ATTACK_DIST						60
#define CHANGEMOVE_DIST					60


#define	MOUCHARD_OFF					0
#define MOUCHARD_ON						1
#define MOUCHARD_LANCEMENT				2




enum 
{
	TASK_FLYBEE_WAIT_FOR_MOVEMENT = LAST_COMMON_TASK + 1,
	TASK_LUCIOLE_GET_PATH,
};


//----------------------------------------------
// d


class CLuciole : public CBaseMonster
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );	// balls
	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	CUSTOM_SCHEDULES;

	void Stop( void );
	void Move ( float flInterval );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void SetActivity ( Activity NewActivity );
	BOOL ShouldAdvanceRoute( float flWaypointDist );

	float m_flNextFlinch;

	float m_flShootTime;
	float m_flShootEnd;

	void AlertSound( void );
	void IdleSound( void );

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];

	void Killed( entvars_t *pevAttacker, int iGib );

	Vector m_velocity;
	int m_fInCombat;

	void		ReorganiseSquad ( void );

	float		m_flLastAttack;

	Vector		m_vecRoute;

	Vector		m_vecMouchard1;
	Vector		m_vecMouchard2;
	int			m_iMouchard;
};


LINK_ENTITY_TO_CLASS( monster_luciole, CLuciole );

TYPEDESCRIPTION	CLuciole::m_SaveData[] = 
{
	DEFINE_FIELD( CLuciole, m_flLastAttack, FIELD_TIME ),
	DEFINE_FIELD( CLuciole, m_vecRoute, FIELD_VECTOR ),
	DEFINE_FIELD( CLuciole, m_vecMouchard1, FIELD_VECTOR ),
	DEFINE_FIELD( CLuciole, m_vecMouchard2, FIELD_VECTOR ),
	DEFINE_FIELD( CLuciole, m_iMouchard, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CLuciole, CBaseMonster );



const char *CLuciole::pIdleSounds[] = 
{
	"controller/con_idle1.wav",
	"controller/con_idle2.wav",
	"controller/con_idle3.wav",
	"controller/con_idle4.wav",
	"controller/con_idle5.wav",
};

const char *CLuciole::pAlertSounds[] = 
{
	"controller/con_alert1.wav",
	"controller/con_alert2.wav",
	"controller/con_alert3.wav",
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CLuciole :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CLuciole :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}


//=========================================================
// Killed - en avertit le groupe



void CLuciole::Killed( entvars_t *pevAttacker, int iGib )
{

	CBaseMonster::Killed( pevAttacker, GIB_ALWAYS );	// explose toujours
}




void CLuciole :: AlertSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds ); 
}

void CLuciole :: IdleSound( void )
{
	EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pIdleSounds ); 
}



//=========================================================
// Spawn
//=========================================================
void CLuciole :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/gorg.mdl");
	UTIL_SetSize( pev, Vector( -4, -4, -0 ), Vector( 4, 4, 8 ));

	pev->solid			= SOLID_SLIDEBOX;	//SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.LucioleHealth;
	pev->view_ofs		= Vector( 0, 0, 3 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flLastAttack		= 0;

	m_vecRoute			= pev->origin;

	m_iMouchard			= MOUCHARD_OFF;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CLuciole :: Precache()
{
	PRECACHE_MODEL("models/gorg.mdl");

	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );

	PRECACHE_SOUND("debris/beamstart14.wav");

}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


// Chase enemy schedule
Task_t tlLucioleChaseEnemy[] = 
{
	{ TASK_LUCIOLE_GET_PATH,			(float)0		},
};

Schedule_t slLucioleChaseEnemy[] =
{
	{ 
		tlLucioleChaseEnemy,
		ARRAYSIZE ( tlLucioleChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"LucioleChaseEnemy"
	},
};

Task_t	tlLucioleTakeCover[] =
{
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
};

Schedule_t	slLucioleTakeCover[] =
{
	{ 
		tlLucioleTakeCover,
		ARRAYSIZE ( tlLucioleTakeCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"LucioleTakeCover"
	},
};


Task_t	tlLucioleStrafe[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_GET_PATH_TO_ENEMY,		(float)128					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slLucioleStrafe[] =
{
	{ 
		tlLucioleStrafe,
		ARRAYSIZE ( tlLucioleStrafe ), 
		bits_COND_NEW_ENEMY,
		0,
		"LucioleStrafe"
	},
};




Task_t	tlLucioleFail[] =
{
	{ TASK_STOP_MOVING,			0				},
//	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
//	{ TASK_WAIT,				(float)1		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slLucioleFail[] =
{
	{
		tlLucioleFail,
		ARRAYSIZE ( tlLucioleFail ),
		0,
		0,
		"LucioleFail"
	},
};

// modif de julien

Task_t	tlLucioleBurn[] =
{
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)0		},
	{ TASK_FLYBEE_WAIT_FOR_MOVEMENT,		(float)0		},
};

Schedule_t	slLucioleBurn[] =
{
	{
		tlLucioleBurn,
		ARRAYSIZE ( tlLucioleBurn ),
		0,
		0,
		"LucioleBurn"
	},
};



DEFINE_CUSTOM_SCHEDULES( CLuciole )
{
	slLucioleChaseEnemy,
	slLucioleStrafe,
	slLucioleTakeCover,
	slLucioleFail,
	slLucioleBurn,	// modif de julien
};

IMPLEMENT_CUSTOM_SCHEDULES( CLuciole, CBaseMonster );



//=========================================================
// StartTask
//=========================================================
void CLuciole :: StartTask ( Task_t *pTask )
{


	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		CBaseMonster :: StartTask ( pTask );
		break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, (pEnemy->pev->origin - pev->origin).Length() + 1024 ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}

	// modif de julien

	case TASK_FLYBEE_WAIT_FOR_MOVEMENT:
		{
			if (FRouteClear())
			{
				TaskComplete();
			}
			break;
		}


	// trouve son chemin vers le joueur

	case TASK_LUCIOLE_GET_PATH:
		break;


	case TASK_FIND_COVER_FROM_ENEMY:
	{
		entvars_t *pevCover;

		if ( m_hEnemy == NULL )
		{
			// Find cover from self if no enemy available
			pevCover = pev;
		}
		else
			pevCover = m_hEnemy->pev;

		if ( FindLateralCover( pevCover->origin, pevCover->view_ofs ) )
		{
			// try lateral first
			m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
			TaskComplete();
		}
		else if ( FindCover( pevCover->origin, pevCover->view_ofs, 0, CoverRadius() ) )
		{
			// then try for plain ole cover
			m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
			TaskComplete();
		}

		// passe de l'autre cot

		else if (BuildNearestRoute( pev->origin + (pevCover->origin-pev->origin).Normalize() * 128, pev->view_ofs, 0, 4096) )
		{
			TaskComplete();
		}
			
		else
		{
			// no coverwhatsoever.
			TaskFail();
		}
		break;
	}


	default:
		CBaseMonster :: StartTask ( pTask );
		break;
	}
}




//=========================================================
// RunTask 
//=========================================================
void CLuciole :: RunTask ( Task_t *pTask )
{
	// petite loupiote

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, Center() );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(Center().x);	// X
		WRITE_COORD(Center().y);	// Y
		WRITE_COORD(Center().z);	// Z
		WRITE_BYTE( 7 );		// radius * 0.1
		WRITE_BYTE( 150 );		// r
		WRITE_BYTE( 71 );		// g
		WRITE_BYTE( 245 );		// b
		WRITE_BYTE( 3 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );

	
	// position de tir

	if ( m_hEnemy != NULL &&  (Center()-m_hEnemy->Center()).Length() < ATTACK_DIST && gpGlobals->time - m_flLastAttack > ATTACK_DELAY )
	{
		m_flLastAttack = gpGlobals->time;

		EMIT_SOUND ( edict(), CHAN_ITEM, "debris/beamstart14.wav", 1.0, ATTN_NORM );


		for ( int i=0; i<3; i++ )
		{
			CBeam *pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 2 );

			if ( RANDOM_LONG(0,1) )
				pBeam->SetColor( 206,118, 254 );
			else
				pBeam->SetColor( 223,224, 255 );

			pBeam->SetBrightness( 192 );

			pBeam->PointEntInit( m_hEnemy->Center(), entindex( ) );
			pBeam->SetEndAttachment( 0 );

			pBeam->RelinkBeam( );

			pBeam->SetNoise ( 30 );
			pBeam->LiveForTime ( 0.4 );

			m_hEnemy->TakeDamage ( pev, pev, gSkillData.LucioleDamage, DMG_SHOCK );
		}



		return;
	}



	MakeIdealYaw( m_vecEnemyLKP );
	ChangeYaw( pev->yaw_speed );


	// run task classique

	switch ( pTask->iTask )
	{
	case TASK_FLYBEE_WAIT_FOR_MOVEMENT:
		{
			if (MovementIsComplete())
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			break;
		}

	case TASK_LUCIOLE_GET_PATH:
		{
			// consid
			BOOL bFinDeRoute = FALSE;

			if ( (pev->origin-m_vecRoute).Length() < 10 )
			{
				bFinDeRoute = TRUE;
			}


			// actualise la position ennemie

			if ( m_hEnemy == NULL )
			{
				TaskComplete ();
				break;
			}

			Vector vecEnemy = m_hEnemy->Center();

			// v

			TraceResult tr;
			UTIL_TraceLine ( pev->origin, vecEnemy, dont_ignore_monsters, dont_ignore_glass, edict(), &tr ); 

			if ( tr.flFraction == 1.0 || FClassnameIs(tr.pHit, "player") )
			{
				// champ libre jusqu'au joueur

				m_vecRoute = vecEnemy;

				m_iMouchard = MOUCHARD_OFF;
			}


			// joueur invisible

			else
			{
				// trajectoire non finie - on continue

				if ( bFinDeRoute == FALSE )
				{
					// active le mouchard

					if ( m_iMouchard == MOUCHARD_OFF )
					{
						m_iMouchard = MOUCHARD_LANCEMENT;
						m_vecMouchard1 = m_vecRoute;
					}
				}

				// pas de trajectoire d

				else
				{
					// tente d'utiliser le mouchard

					TraceResult trMouchard;
					UTIL_TraceLine ( pev->origin, m_vecMouchard2, dont_ignore_monsters, dont_ignore_glass, edict(), &trMouchard ); 

					if ( m_iMouchard == MOUCHARD_ON && ( trMouchard.flFraction == 1.0 || FClassnameIs(trMouchard.pHit, "player") ) )
					{
						// c parti

						m_vecRoute = m_vecMouchard2;

						// mouchard obsol

						m_iMouchard = MOUCHARD_LANCEMENT;
					}

					else
					{
						TraceResult trTete;
						UTIL_TraceLine ( pev->origin, m_hEnemy->pev->view_ofs, dont_ignore_monsters, dont_ignore_glass, edict(), &trTete ); 

						TraceResult trPieds;
						UTIL_TraceLine ( pev->origin, m_hEnemy->pev->origin, dont_ignore_monsters, dont_ignore_glass, edict(), &trPieds ); 

						if ( trTete.flFraction == 1.0 || FClassnameIs(trTete.pHit, "player") )
						{
							// champ libre jusqu'au joueur
							m_vecRoute = m_hEnemy->pev->view_ofs;
						}
						else if ( trPieds.flFraction == 1.0 || FClassnameIs(trPieds.pHit, "player") )
						{
							// champ libre jusqu'au joueur

							m_vecRoute = m_hEnemy->pev->origin;
						}

						// ennemi totalement invisible

						else
						{

							// bloqu

							TraceResult trEvite [2];

							UTIL_MakeVectors ( pev->angles );

							int ordre [4];

							switch ( RANDOM_LONG(0,3) )
							{
								case 0:	ordre[0] = 0; ordre[1] = 1; ordre[2] = 2; ordre[3] = 3; break;
								case 1:	ordre[0] = 0; ordre[1] = 3; ordre[2] = 2; ordre[3] = 1; break;
								case 2:	ordre[0] = 2; ordre[1] = 3; ordre[2] = 0; ordre[3] = 1; break;
								case 3:	ordre[0] = 3; ordre[1] = 1; ordre[2] = 0; ordre[3] = 2; break;
							}

							UTIL_TraceLine ( pev->origin, pev->origin + Vector (0,0,RANDOM_FLOAT(40,60)), dont_ignore_monsters, dont_ignore_glass, edict(), &trEvite[ordre[0]] ); 
							UTIL_TraceLine ( pev->origin, pev->origin - Vector (0,0,RANDOM_FLOAT(40,60)), dont_ignore_monsters, dont_ignore_glass, edict(), &trEvite[ordre[1]] );
							UTIL_TraceLine ( pev->origin, pev->origin + gpGlobals->v_right * RANDOM_FLOAT(40,60), dont_ignore_monsters, dont_ignore_glass, edict(), &trEvite[ordre[2]] );
							UTIL_TraceLine ( pev->origin, pev->origin - gpGlobals->v_right * RANDOM_FLOAT(40,60), dont_ignore_monsters, dont_ignore_glass, edict(), &trEvite[ordre[3]] );
							

							for ( int i=0; i<4; i++ )
							{
								if ( trEvite[i].flFraction >= 0.5 )

								m_vecRoute = trEvite[i].vecEndPos - (trEvite[i].vecEndPos-pev->origin).Normalize() * 5;

								break;
							}
						}
					}
				}
			}



			// actualise le mouchard

			if ( m_iMouchard != MOUCHARD_OFF )
			{

				if ( m_iMouchard == MOUCHARD_LANCEMENT )
				{
					m_vecMouchard2 = m_vecMouchard1;
				}


				// v

				TraceResult trMouchard;
				UTIL_TraceLine ( m_vecMouchard1, vecEnemy, dont_ignore_monsters, dont_ignore_glass, edict(), &trMouchard ); 

				if ( trMouchard.flFraction == 1.0 || FClassnameIs(trMouchard.pHit, "player") )
				{
					m_vecMouchard2 = vecEnemy;
					m_iMouchard = MOUCHARD_ON;
				}
			}


			// trajectoire 

			float flDot = DotProduct ( pev->velocity.Normalize(), (m_vecRoute-pev->origin).Normalize() );

			float flRatio = 0.6 + (flDot+1)*0.6;

			float speed = pev->velocity.Length() * flRatio;

			speed = Q_max ( 100, speed );
			speed = Q_min ( speed, 250 );

			pev->velocity = (m_vecRoute - pev->origin).Normalize() * speed;

			break;
		}



	default: 
		CBaseMonster :: RunTask ( pTask );
		break;
	}
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CLuciole :: GetSchedule ( void )
{

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		break;

	case MONSTERSTATE_ALERT:
		break;

	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions(bits_COND_NEW_ENEMY) )	// une seule r
			{
			//	if ( VARS(m_hEnemy) != VARS(Leader()->m_hEnemy) )
				{
					ReorganiseSquad ();
				}
			}

/*			if ( gpGlobals->time - m_flLastAttack < ATTACK_DELAY && IsLeader() )
				return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );

			else
*/				return GetScheduleOfType ( SCHED_CHASE_ENEMY );


		}
		break;
	}

	return CBaseMonster :: GetSchedule();
}



//=========================================================
//=========================================================
Schedule_t* CLuciole :: GetScheduleOfType ( int Type ) 
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slLucioleChaseEnemy;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slLucioleTakeCover;
	case SCHED_FAIL:
		return slLucioleFail;

	// modif de julien
	case SCHED_BURNT:
		return slLucioleBurn;
	}

	return CBaseMonster :: GetScheduleOfType( Type );
}





//=========================================================
// CheckRangeAttack1  - shoot a bigass energy ball out of their head
//
//=========================================================
BOOL CLuciole :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > 0.5 && flDist > 256 && flDist <= 2048 )
	{
		return TRUE;
	}
	return FALSE;
}



void CLuciole :: SetActivity ( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch ( m_Activity)
	{
	case ACT_WALK:
		m_flGroundSpeed = 100;
		break;
	default:
		m_flGroundSpeed = 100;
		break;
	}
}




extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );

void CLuciole::Stop( void ) 
{ 
	m_IdealActivity = GetStoppedActivity(); 
}


#define DIST_TO_CHECK	200
void CLuciole :: Move ( float flInterval ) 
{
	float		flWaypointDist;
	float		flCheckDist;
	float		flDist;// how far the lookahead check got before hitting an object.
	float		flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}
	
	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;


	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = 100;
	}

	flMoveDist = m_flGroundSpeed * flInterval;

	do 
	{
		// local move to waypoint.
		vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();
		
		// MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
		// ChangeYaw ( pev->yaw_speed );

		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if ( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}
		
		if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
		{
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if (pBlocker)
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}
			if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
			{
				// Can we still move toward our target?
				if ( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
	//				ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else 
			{
				// try to triangulate around whatever is in the way.
				if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 			    ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if ( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						TaskFail();
						ALERT( at_aiconsole, "Failed to move!\n" );
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if (flCheckDist < flMoveDist)
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if ( ShouldAdvanceRoute( flWaypointDist - flMoveDist ) )
			{
				AdvanceRoute( flWaypointDist );
			}
			flMoveDist = 0;
		}

		if ( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while (flMoveDist > 0 && flCheckDist > 0);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();

		if (m_flGroundSpeed > 100)
			m_flGroundSpeed -= 40;
	}
	else
	{
		if (m_flGroundSpeed < 400)
			m_flGroundSpeed += 10;
	}
}



BOOL CLuciole:: ShouldAdvanceRoute( float flWaypointDist )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CLuciole :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 32 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}


void CLuciole::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;


	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.5;

	m_velocity = m_velocity.Normalize() * Q_max ( m_velocity.Length(), 100 );
	m_velocity = m_velocity.Normalize() * Q_min ( 300, m_velocity.Length() );

	UTIL_MoveToOrigin ( ENT(pev), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );
	
}



//------------------------------------------
// squad
//------------------------------------------



void CLuciole :: ReorganiseSquad ( void )
{
/*
#if defined DEBUG_ALERTS
	ALERT ( at_console, "reorganisesquad\n" );
	UTIL_ParticleEffect ( pev->origin, Vector(90,0,0), 0x0000FF, 128 );
#endif

	if ( m_hEnemy == NULL )
	{
#if defined DEBUG_ALERTS
		ALERT ( at_console, "pas d'ennemi pour la reorganisation\n" );
#endif

		return;
	}


	// cherche nouveau leader

	CLuciole *pLeader = NULL;
	float mindist = 4096;

	CLuciole *pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( NULL, "monster_luciole" );

	while ( pLuciole != NULL )
	{
		if ( pLuciole->IsAlive() && (pLuciole->pev->origin - m_hEnemy->pev->origin).Length() < mindist )
		{
			mindist = (pLuciole->pev->origin - m_hEnemy->pev->origin).Length();
			pLeader = pLuciole;
		}				

		pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( pLuciole, "monster_luciole" );
	}

	if ( pLeader == NULL )
	{
#if defined DEBUG_ALERTS
		ALERT ( at_error, "pLeader a NULL\n" );
#endif
		return;
	}


	// nouveau leader

	pLuciole = NULL;
	pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( NULL, "monster_luciole" );

	while ( pLuciole != NULL )
	{
		if ( pLuciole->IsAlive() )
		{
			pLuciole->SetLeader ( pLeader );
			pLuciole->m_pFollow = NULL;

			pLuciole->m_hEnemy = m_hEnemy;
		}

		pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( pLuciole, "monster_luciole" );
	}

	// r

	pLuciole = Leader();

	while ( pLuciole != NULL )
	{
		CLuciole *pPres = NULL;
		mindist = 4096;

		// cherche le plus pr

		CLuciole *pFind = (CLuciole*)UTIL_FindEntityByClassname ( NULL, "monster_luciole" );

		while ( pFind != NULL )
		{
			// luciole encore solitaires

			if ( pLuciole->IsAlive() && pFind != pLuciole && pFind->m_pFollow == NULL && pFind->IsLeader() == FALSE )
			{
				if ( (pLuciole->pev->origin - pFind->pev->origin).Length() < mindist )
				{
					mindist = (pLuciole->pev->origin - pFind->pev->origin).Length();
					pPres = pFind;
				}				
			}

			pFind = (CLuciole*)UTIL_FindEntityByClassname ( pFind, "monster_luciole" );
		}

		// le plus pres le suit

		if ( pPres != NULL )
			pPres->m_pFollow = pLuciole;

		pLuciole->SquadChanged ();	// notification de changement de route etc

		pLuciole = pPres;
	}
	*/


	// nouvel ennemi

	CLuciole *pLuciole = NULL;
	pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( NULL, "monster_luciole" );

	while ( pLuciole != NULL )
	{
		if ( pLuciole->IsAlive() )
		{
			pLuciole->m_hEnemy = m_hEnemy;
		}

		pLuciole = (CLuciole*)UTIL_FindEntityByClassname ( pLuciole, "monster_luciole" );
	}


}

