/********************************************************
*														*
*			= == rpggrunt.cpp == =						*
*														*
*			par Julien									*
*														*
********************************************************/

//================================
//
// rpggrunt : code du soldat rpg
//
//================================


//================================
//	includes
//	
//================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include    "weapons.h"
#include	"soundent.h"


#ifndef RPG_H
#include "rpg.h"	//modif de Julien
#endif



//=====================================================
//	Bodygroups
//
//=====================================================

#define RPG_GROUP_BODY			0
#define RPG_GROUP_WEAPON		1
#define RPG_GROUP_RLEFT			2
#define RPG_GROUP_RRIGHT		3
#define RPG_GROUP_HEAD			4

#define RPG_SUB_RBACK			0
#define RPG_SUB_RHAND			1
#define RPG_SUB_RHAND_OPEN		2
#define RPG_SUB_RGUN			3
#define RPG_SUB_RNO				4

#define HEAD1					0
#define HEAD2					1
#define HEAD_NO					2

//=====================================================
// Monster's anim events
// Constantes associ
//=====================================================

#define RPG_AE_FIRE			( 1 )
#define RPG_AE_RELOAD		( 2 )
#define RPG_AE_RHAND		( 3 )
#define RPG_AE_DROPGUN		( 4 )
#define RPG_AE_STEP			( 5 )
#define RPG_AE_BODYDROP		( 6 )
#define RPG_AE_OPENRPG		( 7 )





//=====================================================
//  Schedule types :
//	Attitudes sp
//=====================================================
enum
{
	SCHED_RPG_RANGE_ATTACK1 = LAST_COMMON_SCHEDULE + 1,
	SCHED_RPG_RELOAD,
	SCHED_RPG_TAKE_COVER_RELOAD,
	SCHED_RPG_CHASE_ENEMY_FAILED,
};

//=========================================================
//	Tasks :
//	t
//=========================================================
enum 
{
	TASK_RPG_CROUCH = LAST_COMMON_TASK + 1,
//	TASK_RPG_STAND,
	TASK_RPG_FIRE,
};




#define RPG_VOICE_VOLUME	0.8	//volume de la voix ( 0 - 1 )





//=====================================================
//	D
//	CRpggrunt
//=====================================================


class CRpggrunt : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	int  Classify ( void ) { return	CLASS_HUMAN_MILITARY; }
	int	 IRelationship ( CBaseEntity *pTarget );

	void SetYawSpeed( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void CheckAmmo ( void );
	void Shoot ( void );

	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );

	BOOL FOkToSpeak( void );
	void JustSpoke( void );
	void IdleSound( void );
	void DeathSound ( void );
	int ISoundMask ( void );

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void MakeGib ( int body, entvars_t *pevAttacker );

	void GibMonster( void );

	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

	BOOL CheckRangeAttack1 ( float flDot , float flDist );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	int	Save( CSave &save ); 
	int Restore( CRestore &restore );


	BOOL	m_bStanding;
	BOOL	m_bAmmoLoaded;
	float	m_talkWaitTime;

};
LINK_ENTITY_TO_CLASS( monster_rpg_grunt, CRpggrunt );

TYPEDESCRIPTION	CRpggrunt::m_SaveData[] = 
{
	DEFINE_FIELD( CRpggrunt, m_bStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRpggrunt, m_bAmmoLoaded, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CRpggrunt, CBaseMonster );





//====================================================
// Vitesse de rotation
// en degres par seconde
//====================================================

void CRpggrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:	
	case ACT_RUN:	
		ys = 150;	
		break;
	case ACT_WALK:	
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



//====================================================
// Spawn()
//
//====================================================

void CRpggrunt :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/rpggrunt.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->health			= gSkillData.RpggruntHealth;

//	pev->view_ofs		= Vector ( 0, 0, 6 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.2;
	m_MonsterState		= MONSTERSTATE_NONE;
	pev->effects		= 0;

	m_bStanding			= 1;
	m_bAmmoLoaded		= 1;


	m_afCapability		= bits_CAP_HEAR |
						  bits_CAP_RANGE_ATTACK1 |
						  bits_CAP_AUTO_DOORS |
						  bits_CAP_OPEN_DOORS;

	SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RGUN);

	if ( RANDOM_LONG(0,1) )
		SetBodygroup ( RPG_GROUP_HEAD, HEAD1 );
	else
		SetBodygroup ( RPG_GROUP_HEAD, HEAD2 );


	MonsterInit();

	m_flDistLook = 4096;
}



//=================================
// Precache ()
//
//=================================

void CRpggrunt :: Precache()
{
	PRECACHE_MODEL("models/rpggrunt.mdl");
	PRECACHE_MODEL("models/hg_gibs.mdl");
		
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );


}

//====================================
// ISoundMask
//
//====================================

int CRpggrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}


//=================================
//	CheckAmmo ()
//	Controle des munitions
//=================================

void CRpggrunt :: CheckAmmo ( void )
{
//	ALERT ( at_console, "CHECK AMMO : %i\n" , m_bAmmoLoaded );
	if ( m_bAmmoLoaded == FALSE )
	{
//		ALERT ( at_console, "NO AMMO LOADED\n" );
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}


//===============================
//	FOkToSpeak
//
//===============================


BOOL CRpggrunt :: FOkToSpeak( void )
{
	if (gpGlobals->time <= m_talkWaitTime)
		return FALSE;

	return TRUE;
}


//===================================
//	IdleSound
//
//===================================

void CRpggrunt :: IdleSound( void )
{
	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), "RPG_IDLE", RPG_VOICE_VOLUME, ATTN_NORM, 0, 100);
		JustSpoke();
	}
}


//=========================================================
//	DeathSound
//	supprime toute sequence deja en cours
//=========================================================
void CRpggrunt :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );	
		break;
	case 2:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );	
		break;
	}
}




//================================
// JustSpoke
//
//================================

void CRpggrunt :: JustSpoke( void )
{
	m_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
}


//================================
// IRelationship
//
//================================

int CRpggrunt::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) )
	{
		return R_NM;
	}

	if ( FClassnameIs( pTarget->pev, "vehicle_tank" ) )
	{
		if ( pTarget->Classify() == CLASS_NONE )
			return R_NO;
		return R_HT;
	}

	return CBaseMonster::IRelationship( pTarget );
}


//===================================
//	CheckRangeAttack1
//	tire toujours si l ennemi est visible
//===================================

BOOL CRpggrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	TraceResult	tr;
	UTIL_TraceLine( BodyTarget(pev->origin), m_hEnemy->BodyTarget(pev->origin), ignore_monsters, ignore_glass, ENT(pev), &tr);

	if ( tr.flFraction == 1.0 )
	{
		return TRUE;
	}
	return FALSE;

/*	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) )
	{
		return TRUE;
	}
	return FALSE;
*/
}





//================================================
// Handle Anim Event :
// 
//================================================

void CRpggrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{

	switch (pEvent->event)
	{
		case RPG_AE_FIRE:
			{
				m_bAmmoLoaded = 0;
				SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RBACK);
				Shoot();
				break;
			}
		case RPG_AE_RELOAD :
			{
	//			ALERT ( at_console, "RECHARGE\n");
				m_bAmmoLoaded = 1;
				ClearConditions(bits_COND_NO_AMMO_LOADED);
				SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RGUN);
				break;
			}
		case RPG_AE_RHAND :
			{
				SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RHAND);
				break;
			}
		case RPG_AE_DROPGUN:
			{
				SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RNO);
				SetBodygroup( RPG_GROUP_RRIGHT, 1);
				SetBodygroup( RPG_GROUP_WEAPON, 1);

				Vector	vecGunPos;
				Vector	vecGunAngles;

				GetAttachment( 0, vecGunPos, vecGunAngles );
				DropItem( "weapon_rpg", vecGunPos, vecGunAngles );
				break;
			}
		case RPG_AE_STEP:
			{
				switch ( RANDOM_LONG(0,3) )
				{
					case 0:	EMIT_SOUND ( ENT(pev), CHAN_BODY, "player/pl_step1.wav", 1, ATTN_NORM);	break;
					case 1:	EMIT_SOUND ( ENT(pev), CHAN_BODY, "player/pl_step2.wav", 1, ATTN_NORM);	break;
					case 2:	EMIT_SOUND ( ENT(pev), CHAN_BODY, "player/pl_step3.wav", 1, ATTN_NORM);	break;
					case 3:	EMIT_SOUND ( ENT(pev), CHAN_BODY, "player/pl_step4.wav", 1, ATTN_NORM);	break;
				}
				break;
			}
		case RPG_AE_BODYDROP:
			{
				if ( pev->flags & FL_ONGROUND )
				{
					if ( RANDOM_LONG( 0, 1 ) == 0 )
					{
						EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "common/bodydrop3.wav", 1, ATTN_NORM, 0, 90 );
					}
					else
					{
						EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "common/bodydrop4.wav", 1, ATTN_NORM, 0, 90 );
					}
				}
				break;
			}
		case RPG_AE_OPENRPG:
			{
				SetBodygroup( RPG_GROUP_RLEFT, RPG_SUB_RHAND_OPEN);
				break;
			}
	}

}


//===============================================
//	Shoot 
//	Tir de la roquette
//===============================================

void CRpggrunt :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin;
	Vector zeroVector(0,0,0);
	GetAttachment( 0, vecShootOrigin, zeroVector );

	Vector vecShootDir = m_hEnemy->Center() - vecShootOrigin;
//	UTIL_ParticleEffect ( vecShootOrigin, UTIL_VecToAngles( vecShootDir ), 600, 255 );	// effet super mario hyper flashy tendance, mais dplac

	Vector VecShootAng = UTIL_VecToAngles( vecShootDir );
	VecShootAng.x = - VecShootAng.x;


	
	CRpgRocket *pRocket = CRpgRocket::CreateRpgRocket( vecShootOrigin, VecShootAng, NULL/*this*/, NULL );
	pRocket->pev->classname = MAKE_STRING("rpggrunt_rocket");
	pRocket->m_pTargetMonster = m_hEnemy;
	

}



//================================================
//	TraceAttack
//	Ricochets sur le casque et les roquettes
//================================================

void CRpggrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( FClassnameIs( ENT( pevAttacker ), "rpggrunt_rocket" ) )
	{
		return;			//pour eviter que le soldat ne meure en tirant sur un ennemi proche
	}

	// casque
	if (ptr->iHitgroup == 11)
	{
		flDamage -= 20;
		if (flDamage <= 0)
		{
			UTIL_Ricochet( ptr->vecEndPos, 1.0 );
			flDamage = 0.01;
		}
	}

	if ( ( pev->health - ( flDamage ) <= 0 )  && IsAlive() && m_iHasGibbed == 0 && ptr->iHitgroup == HITGROUP_HEAD )
	{
		SetBodygroup( RPG_GROUP_HEAD, HEAD_NO );
		MakeGib ( 1, pevAttacker );
	}


	CBaseMonster :: TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CRpggrunt :: MakeGib ( int body, entvars_t *pevAttacker )
{

	if ( m_iHasGibbed == 1 )
		return;
	m_iHasGibbed = 1;

	CGib *pGib = GetClassPtr( (CGib *)NULL );
	pGib->Spawn( "models/hg_gibs.mdl" );
	pGib->m_bloodColor = BLOOD_COLOR_RED;
	pGib->pev->body = body;

	pGib->pev->origin = pev->origin + Vector ( 0, 0, 40 );
	pGib->pev->velocity = ( Center() - pevAttacker->origin).Normalize() * 300;
	
	pGib->pev->avelocity.x = RANDOM_FLOAT ( 100, 200 );
	pGib->pev->avelocity.y = RANDOM_FLOAT ( 100, 300 );

}

//================================================
//	TakeDamage
//	
//================================================


int CRpggrunt :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{

/*	if ( ENT(pevAttacker) == edict() )
	{
		flDamage = 10;	//pour eviter que le soldat ne meure en tirant sur un ennemi proche
	}
*/
	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}



//=========================================================
// GibMonster - overriden
// Pour la projection du lance-roquettes quand le rpggrunt
// meurt par DMG_BLAST ou DMG_CRUSH
//
//=========================================================


void CRpggrunt :: GibMonster ( void )
{
	if ( GetBodygroup( 1 ) != 0 )
		return;

	Vector	vecGunPos = GetGunPosition ();
	Vector	vecGunAngles;

	Vector zeroVector(0,0,0);
	GetAttachment( 0, zeroVector, vecGunAngles );
	DropItem( "weapon_rpg", vecGunPos, vecGunAngles );

	CBaseMonster :: GibMonster();
}





//===============================================
//	Set activity
//
//	determine l animation a effectuer en fonction
//	de l activite demandee - sert notamment pour
//	les anims pouvant etre jouees accroupi
//	ou debout
//===============================================

void CRpggrunt :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_IDLE:
		{
			if ( m_bStanding == 0 )
			{
				NewActivity = ACT_CROUCHIDLE;
			}
			iSequence = LookupActivity ( NewActivity );
			break;
		}
	case ACT_FLINCH_HEAD:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "flinch_head" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_flinch_head" );
			}
			break;
		}
	case ACT_FLINCH_LEFTARM:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "flinch_arm_left" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_flinch_arm_left" );
			}
			break;
		}
	case ACT_FLINCH_RIGHTARM:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "flinch_arm_right" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_flinch_arm_right" );
			}
			break;
		}
	case ACT_FLINCH_LEFTLEG:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "flinch_leg_left" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_flinch_leg_left" );
			}
			break;
		}
	case ACT_FLINCH_RIGHTLEG:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "flinch_leg_right" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_flinch_leg_right" );
			}
			break;
		}
	case ACT_TURN_LEFT:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "180L" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_180L" );
			}
			break;
		}
	case ACT_TURN_RIGHT:
		{
			if ( m_bStanding == 1 )
			{
				iSequence = LookupSequence( "180R" );
			}
			else
			{
				iSequence = LookupSequence( "crouch_180R" );
			}
			break;
		}
	default:
		{

			iSequence = LookupActivity ( NewActivity );
			break;
		}
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
//	Start task
//	s execute avant le lancement de chaque tache
//=========================================================


void CRpggrunt :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RPG_CROUCH:
		{
			if ( m_bStanding == 1 )
			{
				m_bStanding = 0;
				m_IdealActivity = ACT_CROUCH;
				break;
			}
			else
			{
				TaskComplete();
			}
			break;
		}

/*	case TASK_RPG_STAND:
		{
			if ( m_bStanding == 0 )
			{
				m_bStanding = 1;
			}

			TaskComplete();
			break;
		}*/

	case TASK_RPG_FIRE:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			break;
		}

	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		{
			m_bStanding = 1;
			CBaseMonster :: StartTask( pTask );
			break;
		}

	default: 
		CBaseMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CRpggrunt :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RPG_CROUCH:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_RPG_FIRE:
		{
			if (m_hEnemy != NULL)
			{
				Vector vecShootDir = m_hEnemy->Center() - Center();
				Vector angDir = UTIL_VecToAngles( vecShootDir );
				SetBlending( 0, angDir.x );
			}

			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
//				m_Activity = ACT_IDLE;
				TaskComplete();
			}
			break;
		}



	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}






//================================================
//================================================
//
//	Intelligence artificielle
//
//================================================
//================================================




//================================================
//	Tableaux des taches
//
//================================================


//=========================================================
//  RangeAttack : tir

Task_t	tlRpgRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_RPG_CROUCH,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RPG_FIRE,			(float)0		},
};

Schedule_t	slRpgRangeAttack1[] =
{
	{ 
		tlRpgRangeAttack1,
		ARRAYSIZE ( tlRpgRangeAttack1 ), 
		0,
		0,
		"RpgRangeAttack1"
	},
};


//=========================================================
// Reload : rechargement

Task_t	tlRpgReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_RPG_CROUCH,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},

};

Schedule_t slRpgReload[] = 
{
	{
		tlRpgReload,
		ARRAYSIZE ( tlRpgReload ),
		0,
		0,
		
		"RpgReload"
	}
};


//========================================================
//	Cover from best sound : s eloigne d un son de danger

Task_t	tlRpgTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ORIGIN	},
	{ TASK_STOP_MOVING,					(float)0							},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0							},
	{ TASK_STORE_LASTPOSITION,			(float)0							},
	{ TASK_RUN_PATH,					(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0							},
	{ TASK_TURN_LEFT,					(float)179							},
};

Schedule_t	slRpgTakeCoverFromBestSound[] =
{
	{ 
		tlRpgTakeCoverFromBestSound,
		ARRAYSIZE ( tlRpgTakeCoverFromBestSound ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAVY_DAMAGE		,

		0,
		"RpgTakeCoverFromBestSound"
	},
};

//========================================================
//	Take Cover Reload : se met a couvert de l ennemi pour recharger

Task_t	tlRpgTakeCoverReload[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RPG_RELOAD				},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_STORE_LASTPOSITION,		(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_SET_SCHEDULE,			(float)SCHED_RPG_RELOAD				},
};

Schedule_t	slRpgTakeCoverReload[] =
{
	{ 
		tlRpgTakeCoverReload,
		ARRAYSIZE ( tlRpgTakeCoverReload ), 
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"RpgTakeCoverReload"
	},
};

//========================================================
//	Take Cover : se met a couvert de l ennemi

Task_t	tlRpgTakeCoverFromEnnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_COMBAT_FACE			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_STORE_LASTPOSITION,		(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
};

Schedule_t	slRpgTakeCoverFromEnnemy[] =
{
	{ 
		tlRpgTakeCoverFromEnnemy,
		ARRAYSIZE ( tlRpgTakeCoverFromEnnemy ), 
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"RpgTakeCoverFromEnnemy"
	},
};

//=========================================================
// tlRpgTakeCoverFromOrigin

Task_t	tlRpgTakeCoverFromOrigin[] =
{
	{ TASK_STOP_MOVING,					(float)0					},
//	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COMBAT_FACE	},
	{ TASK_FIND_COVER_FROM_ORIGIN,		(float)0					},
	{ TASK_STORE_LASTPOSITION,			(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slRpgTakeCoverFromOrigin[] =
{
	{ 
		tlRpgTakeCoverFromOrigin,
		ARRAYSIZE ( tlRpgTakeCoverFromOrigin ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_CAN_RANGE_ATTACK1	,
		
		bits_SOUND_DANGER,
		"RpgTakeCoverFromOrigin"
	},
};

//=========================================
// Chase enemy schedule

Task_t tlRpgChaseEnemy[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RPG_CHASE_ENEMY_FAILED},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},
};

Schedule_t slRpgChaseEnemy[] =
{
	{ 
		tlRpgChaseEnemy,
		ARRAYSIZE ( tlRpgChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"RpgChaseEnemy"
	},
};


//=========================================
// Chase failed

Task_t tlRpgChaseFailed[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_TAKE_COVER_FROM_ORIGIN		},
	{ TASK_GET_PATH_TO_LASTPOSITION,	(float)0						},
	{ TASK_RUN_PATH,					(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0						},
	{ TASK_CLEAR_LASTPOSITION,			(float)0						},
};

Schedule_t slRpgChaseFailed[] =
{
	{ 
		tlRpgChaseFailed,
		ARRAYSIZE ( tlRpgChaseFailed ),
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"RpgChaseFailed"
	},
};






//================================================
//	definition des tableaux de taches
//
//================================================



DEFINE_CUSTOM_SCHEDULES( CRpggrunt )
{
	slRpgRangeAttack1,
	slRpgReload,
	slRpgTakeCoverFromBestSound,
	slRpgTakeCoverReload,
	slRpgTakeCoverFromEnnemy,
	slRpgTakeCoverFromOrigin,
	slRpgChaseEnemy,
	slRpgChaseFailed,
};

IMPLEMENT_CUSTOM_SCHEDULES( CRpggrunt, CBaseMonster );




//================================================
//	Gestion des comportements
//
//================================================



Schedule_t *CRpggrunt :: GetSchedule( void )
{
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "RPG_GREN", RPG_VOICE_VOLUME, ATTN_NORM, 0, 100);
					JustSpoke();
				}

				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
		}
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "RPG_ALERT", RPG_VOICE_VOLUME, ATTN_NORM, 0, 100);
					JustSpoke();
				}
			}


			if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 75 && m_hEnemy != NULL )
				{
					ClearConditions( bits_COND_LIGHT_DAMAGE );

					if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) && !HasConditions ( bits_COND_NO_AMMO_LOADED ) )
						return GetScheduleOfType( SCHED_RPG_RANGE_ATTACK1 );
					
					else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
						return GetScheduleOfType( SCHED_RPG_RELOAD );

					else
						return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				else
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}


			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				return GetScheduleOfType( SCHED_RPG_TAKE_COVER_RELOAD );
			}

			else if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) && !HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "RPG_CHARGE", RPG_VOICE_VOLUME, ATTN_NORM, 0, 100);
					JustSpoke();
				}

				return GetScheduleOfType( SCHED_RPG_RANGE_ATTACK1 );
			}
			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}




//================================================
//	Gestion des comportements
//
//================================================



Schedule_t* CRpggrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{

	case SCHED_RPG_RELOAD:
		{	
			return &slRpgReload[ 0 ];
		}

	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slRpgTakeCoverFromBestSound[ 0 ];
		}

	case SCHED_RPG_TAKE_COVER_RELOAD:
		{
			return &slRpgTakeCoverReload[ 0 ];
		}

	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz( ENT(pev), "RPG_COVER", RPG_VOICE_VOLUME, ATTN_NORM, 0, 100);
				JustSpoke();
			}
			return &slRpgTakeCoverFromEnnemy[ 0 ];
		}
	case SCHED_TAKE_COVER_FROM_ORIGIN:
		{
			return &slRpgTakeCoverFromOrigin[ 0 ];
		}

	case SCHED_RPG_RANGE_ATTACK1:
		{
			return &slRpgRangeAttack1[ 0 ];
		}
	case SCHED_CHASE_ENEMY:
		{
			return &slRpgChaseEnemy[ 0 ];
		}
	case SCHED_RPG_CHASE_ENEMY_FAILED:
		{
			return &slRpgChaseFailed[ 0 ];
		}
	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}
