/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"animation.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		GUS_AE_DRAW		( 2 )
#define		GUS_AE_SHOOT		( 3 )
#define		GUS_AE_HOLSTER	( 4 )
#define		GUS_AE_RELOAD	( 5 )
#define		GUS_AE_MELEE		( 6 )

#define	GUS_BODY_GUNHOLSTERED	0
#define	GUS_BODY_GUNDRAWN		1
#define GUS_BODY_GUNGONE			2

enum
{
	SCHED_police_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_police_TAKECOVER_FAILED,
};


class CPolice : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void BarneyFirePistol( void );
	void AlertSound( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	
	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void DeathSound( void );
	void PainSound( void );
	void SetActivity ( Activity NewActivity );

	void CheckAmmo ( void );
	void RunAI( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_police, CPolice );

TYPEDESCRIPTION	CPolice::m_SaveData[] = 
{
	DEFINE_FIELD( CPolice, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPolice, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CPolice, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CPolice, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CPolice, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CPolice, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlPoliceFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)96		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slPoliceFollow[] =
{
	{
		tlPoliceFollow,
		ARRAYSIZE ( tlPoliceFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlPoliceFollow_duck[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)64		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slPoliceFollow_duck[] =
{
	{
		tlPoliceFollow_duck,
		ARRAYSIZE ( tlPoliceFollow_duck ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tl_police_TakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_police_TAKECOVER_FAILED},
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_police_TakeCover[] =
{
	{ 
		tl_police_TakeCover1,
		ARRAYSIZE ( tl_police_TakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tl_police_HideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t sl_police_HideReload[] = 
{
	{
		tl_police_HideReload,
		ARRAYSIZE ( tl_police_HideReload ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_DANGER,
		"TakeCover"
	}
};

Task_t	tlPoliceWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slPoliceWaitInCover[] =
{
	{ 
		tlPoliceWaitInCover,
		ARRAYSIZE ( tlPoliceWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"BarneyWaitInCover"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlPoliceEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slPoliceEnemyDraw[] = 
{
	{
		tlPoliceEnemyDraw,
		ARRAYSIZE ( tlPoliceEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlPoliceFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slPoliceFaceTarget[] =
{
	{
		tlPoliceFaceTarget,
		ARRAYSIZE ( tlPoliceFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdlePoliceStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdlePoliceStand[] =
{
	{ 
		tlIdlePoliceStand,
		ARRAYSIZE ( tlIdlePoliceStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CPolice )
{
	slPoliceFollow,
	slPoliceFollow_duck,
	slPoliceEnemyDraw,
	slPoliceFaceTarget,
	slIdlePoliceStand,
	slPoliceWaitInCover,
	sl_police_HideReload,
	sl_police_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( CPolice, CTalkMonster );

void CPolice :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CPolice :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && m_lovehate <= 0 )
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// RunAI
//=========================================================
void CPolice :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if (pev->sequence == LookupActivity ( ACT_RUN ) ){
    m_flGroundSpeed = 250;
	}

	if(pev->health <= pev->max_health * 0.5){
	m_cover_dist = 120;
	}
	else{
	m_cover_dist = 90;
	}

			if(GetBodygroup( 2 ) == 2){//ս���ֵ�Ͳ
				if(!FBitSet(pev->effects, EF_DIMLIGHT)){
				SetBits(pev->effects, EF_DIMLIGHT);
				}
			}
			else{
				if(FBitSet(pev->effects, EF_DIMLIGHT)){
				ClearBits(pev->effects, EF_DIMLIGHT);
				}
			}

			if(pev->impulse == 1 && m_lovehate > 0){//����������𾯲죿
				if(m_hPlayer != NULL){
					if(m_hPlayer->pev->deadflag == DEAD_NO && FVisible(m_hPlayer)){
						float flDist = ( m_hPlayer->pev->origin - pev->origin).Length();
						if(flDist <= 100){//�������
						m_lovehate = 0;//ɱ!
						SetBodygroup( 2, 1);
						m_voicePitch = 70;
						m_ignoredamage = 2;
						EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!BA_MAD0", VOL_NORM, 0.5, 0, PITCH_NORM-30);
						}
					}
				}
			}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CPolice :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPolice :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CPolice :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}


void CPolice :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CPolice :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RANGE_ATTACK1:
		if ( pev->weapons == 1 )
		{
			// limp!
			iSequence = LookupSequence( "MP5" );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_RELOAD:
		if ( pev->weapons == 1 )
		{
			// limp!
			iSequence = LookupSequence( "reload2" );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( GetBodygroup( 2 ) == 2 && pev->weapons == 1 && m_hEnemy != NULL )
		{
			iSequence = LookupSequence( "idle5" );
		}
		else{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	m_IdealActivity = m_Activity;

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
		iSequence = LookupActivity ( ACT_IDLE );

		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();

		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	}

}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPolice :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CPolice :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_cAmmoLoaded <= 0 ){
	return FALSE;
	}

	float dist = 1200;

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 ){//����˸߶Ȳ�ϸ�ʱ�����ӹ�������
		dist += 300;
		}
	}

	if(pev->weapons != 1){
		if(m_aimenemy_mod != 4 && flDist < (dist * 0.25) ){//������
		m_aimenemy_mod = 4;//�۾�
		}
		else if(m_aimenemy_mod != 3 && flDist < (dist * 0.5) ){//�о���
		m_aimenemy_mod = 3;//��
		}
		else if(m_aimenemy_mod != 8 && flDist < (dist * 0.75) ){//��Զ����
		m_aimenemy_mod = 8;//����
		}
		else if(m_aimenemy_mod != 5){//Զ����
		m_aimenemy_mod = 5;//��
		}
	}

	if (flDist <= dist && flDot >= 0.5 )
	{
			//���˼��
			TraceResult	tr;

			if ( gpGlobals->time > m_checkAttackTime )
			{
				TraceResult tr;
				Vector shootOrigin;
				shootOrigin = pev->origin + Vector( 0, 0, 55 );
				CBaseEntity *pEnemy = m_hEnemy;
				if ( !pEnemy )
				{
					m_lastAttackCheck = FALSE;
					m_checkAttackTime = gpGlobals->time + 0.5;
					return FALSE;
				}
				Vector shootTarget;
				if(m_aimenemy_mod == 5)
				shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				else if(m_aimenemy_mod == 8)
				shootTarget = ( (pEnemy->BodyTarget_b( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				else if(m_aimenemy_mod == 3)
				shootTarget = ( (pEnemy->BodyTarget_h( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				else if(m_aimenemy_mod == 4)
				shootTarget = ( (pEnemy->BodyTarget_e( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );

				UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) ){
					m_checkAttackTime = gpGlobals->time + 0.25;
					m_lastAttackCheck = TRUE;
				}
				else{
						CBaseEntity *pHitEntity = CBaseEntity::Instance(tr.pHit);
						if(pHitEntity){
							if(pHitEntity->Classify() != Classify()){
								if(pHitEntity->Classify() == CLASS_PLAYER && Classify() == CLASS_PLAYER_ALLY && m_lovehate > 0){
								m_lastAttackCheck = FALSE;
								m_checkAttackTime = gpGlobals->time + 0.5;
								}
								else{
								m_checkAttackTime = gpGlobals->time + 0.25;
								m_lastAttackCheck = TRUE;
								}
							}
							else{
							m_lastAttackCheck = FALSE;
							m_checkAttackTime = gpGlobals->time + 0.5;
							}
						}
						else{
							m_lastAttackCheck = FALSE;
							m_checkAttackTime = gpGlobals->time + 0.5;
						}
				}
			}
			return m_lastAttackCheck;

	}
	return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CPolice :: BarneyFirePistol ( void )
{
	if ( m_cAmmoLoaded <= 0 ){
	return;
	}

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);

	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );

	// make angles +-180
	if (angDir.x > 180)
	{
		angDir.x = angDir.x - 360;
	}

	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	

	if ( pev->weapons == 1 ){
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.051,0.051,0.051), 2048, BULLET_8mm,1);
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/mp5_fire-1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	}
	else{
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.017,0.017,0.017), 2048, BULLET_12MM,1);
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	}
	
	

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?

	m_cAmmoLoaded--;// take away a bullet!

}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CPolice :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case GUS_AE_SHOOT:
		BarneyFirePistol();
		break;

	case GUS_AE_DRAW:
		{
		SetBodygroup( 2, 1);
		m_fGunDrawn = TRUE;
		}
		break;

	case GUS_AE_HOLSTER:
		{
		SetBodygroup( 2, 0);
		m_fGunDrawn = FALSE;
		}
		break;

	case GUS_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "police/reload.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CPolice :: Spawn()
{
	Precache( );

	int oldbody = pev->body;
	pev->body = 0;


	SET_MODEL(ENT(pev), "models/gus_police.mdl");
	m_lovehate          = 40;
	pev->health			= 80;

	if ( pev->weapons == 1 ){
	m_cClipSize	    	= 30;
	m_fGunDrawn			= TRUE;
	m_aimenemy_mod		= 5;
	SetBodygroup( 2, 3);
	}
	else{
	m_cClipSize	    	= 15;
	m_fGunDrawn			= FALSE;
	m_aimenemy_mod		= 0;
	}

	if ( oldbody < 0 || oldbody > 2 )
	{
		SetBodygroup( 1, RANDOM_LONG(0,2));
	}
	else{
		SetBodygroup( 1, oldbody);
	}

	UTIL_SetSize(pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_canheadcrab_mode  = 0;

	m_crouchmode        = 0;
	m_canbarnacle_mode  = 1;
	m_candrownwater = 1;
	
	MonsterInit();
	SetUse( &CPolice::FollowerUse );
	m_forcefuckdoor  = TRUE;
	m_killed_exp = 100;
	m_rpgms_level = 15;
	pev->netname = MAKE_STRING( "Police" );

	if(pev->armortype == 1){
	pev->health			= 120;//�����£�HP����+40
	SetBodygroup( 0, 1);
	m_killed_exp = 150;
	m_rpgms_level = 20;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPolice :: Precache()
{
	PRECACHE_MODEL("models/gus_police.mdl");

	PRECACHE_SOUND("police/reload.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CPolice :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"BA_ANSWER";
	m_szGrp[TLK_QUESTION] =	"BA_QUESTION";
	m_szGrp[TLK_IDLE] =		"BA_IDLE";
	m_szGrp[TLK_STARE] =		"BA_STARE";
	m_szGrp[TLK_USE] =		"BA_OK";
	m_szGrp[TLK_UNUSE] =	"BA_WAIT";
	m_szGrp[TLK_STOP] =		"BA_STOP";

	m_szGrp[TLK_NOSHOOT] =	"BA_SCARED";
	m_szGrp[TLK_HELLO] =	"BA_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!BA_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!BA_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"BA_SMELL";
	
	m_szGrp[TLK_WOUND] =	"BA_WOUND";
	m_szGrp[TLK_MORTAL] =	"BA_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	// He's facing me, he meant it
	if ( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CPolice :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if(pev->impulse == 1 && m_lovehate > 0){//����������𾯲죿
		m_lovehate = 0;//ɱ!
		m_voicePitch = 70;
		m_ignoredamage = 2;
		SetBodygroup( 2, 1);
	}

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "BA_MAD", 4, VOL_NORM, ATTN_NORM );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CPolice :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CPolice :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CPolice::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CPolice::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( 2 ) != 2 && GetBodygroup( 2 ) != 5 )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( 2, 5);

		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun;
		if ( pev->weapons == 1 ){
		pGun = DropItem( "weapon_smg", vecGunPos, vecGunAngles );
		}
		else{
		pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
		}

		if ( pGun )
		{
			pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}


	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CPolice :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slPoliceEnemyDraw;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		return &sl_police_TakeCover[ 0 ];
	}

	case SCHED_police_TAKECOVER_FAILED:
	{
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		return GetScheduleOfType ( SCHED_FAIL );
	}

	case SCHED_police_COVER_AND_RELOAD:
	{
		return &sl_police_HideReload[ 0 ];
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slPoliceFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slPoliceFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdlePoliceStand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CPolice :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}
				
			if ( pev->weapons != 1 && pev->impulse != 1 ){
					// wait for one schedule to draw gun
					if (!m_fGunDrawn ){
						if ( m_hEnemy != NULL){
						return GetScheduleOfType( SCHED_ARM_WEAPON );
						}
					}
			}
		

			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_police_COVER_AND_RELOAD );
			}

			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
			{
						// flinch if hurt
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			if ( HasConditions(bits_COND_NEW_COVER_MODE) )
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
			if ( m_cleardally_enemy_long == 0){
				if ( pev->weapons == 1 || m_fGunDrawn || pev->impulse == 1 ){
					if ( m_cAmmoLoaded < m_cClipSize){
					return GetScheduleOfType ( SCHED_RELOAD_DEEP );//������
					}
					else if ( pev->weapons != 1 && m_hEnemy == NULL ){
					return GetScheduleOfType( SCHED_DISARM_WEAPON );
					}
				}
			}
		
		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
				// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				else{
				return GetScheduleOfType( SCHED_TARGET_FACE );
				}
			}
		}
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CPolice :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void CPolice::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}



//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadPolice : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadPolice::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach" };

void CDeadPolice::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_police_dead, CDeadPolice );
//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadPolice :: Spawn( )
{
	PRECACHE_MODEL("models/gus_police.mdl");


	SET_MODEL(ENT(pev), "models/gus_police.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead Police with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 10;//gSkillData.barneyHealth;

	if(pev->frags == 1){
	pev->health			= 30;
	SetBits(pev->effects, EF_DIMLIGHT);
	SetBodygroup( 2, 4 );//�ֵ�Ͳ����
	}
	else{
	SetBodygroup( 2, 5 );//û��ǹ
	}

	MonsterInitDead();
}


