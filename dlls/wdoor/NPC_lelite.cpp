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
#include	"player.h"
#include	"gamerules.h"
#include	"shake.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		GUS_AE_DRAW		( 2 )
#define		GUS_AE_SHOOT		( 3 )
#define		GUS_AE_HOLSTER	( 4 )
#define		GUS_AE_RELOAD	( 5 )


#define	GUS_BODY_GUNHOLSTERED	0
#define	GUS_BODY_GUNDRAWN		1
#define GUS_BODY_GUNGONE			2

enum
{
	SCHED_elite_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_elite_TAKECOVER_FAILED,
};


class CEliteBarney : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void BarneyFirePistol( void );
	void BarneyFirePistol2( void );
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
//	void PainSound( void );
	void SetActivity ( Activity NewActivity );

	void CheckAmmo ( void );
	void RunAI( void );

	void TalkInit( void );
	
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	EHANDLE	m_SatchalBambo;

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_coverTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_lelite, CEliteBarney );

TYPEDESCRIPTION	CEliteBarney::m_SaveData[] = 
{
	DEFINE_FIELD( CEliteBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEliteBarney, m_SatchalBambo, FIELD_EHANDLE ),
	DEFINE_FIELD( CEliteBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CEliteBarney, m_coverTime, FIELD_TIME ),
	DEFINE_FIELD( CEliteBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CEliteBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CEliteBarney, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlEliteFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slEliteFollow[] =
{
	{
		tlEliteFollow,
		ARRAYSIZE ( tlEliteFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tl_elite_TakeCover[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_elite_TAKECOVER_FAILED },
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_elite_TakeCover[] =
{
	{ 
		tl_elite_TakeCover,
		ARRAYSIZE ( tl_elite_TakeCover ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tl_elite_TakeCover2[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_elite_TAKECOVER_FAILED },
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_COVER_RUN_PATH,			(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_elite_TakeCover2[] =
{
	{ 
		tl_elite_TakeCover2,
		ARRAYSIZE ( tl_elite_TakeCover2 ), 
		0,
		0,
		"TakeCover Grenade"
	},
};

Task_t	tl_elite_HideReload2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_COVER_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t sl_elite_HideReload2[] = 
{
	{
		tl_elite_HideReload2,
		ARRAYSIZE ( tl_elite_HideReload2 ),
		0,
		0,
		"TakeCover Grenade"
	}
};

Task_t	tl_elite_HideReload[] =
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

Schedule_t sl_elite_HideReload[] = 
{
	{
		tl_elite_HideReload,
		ARRAYSIZE ( tl_elite_HideReload ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_HEAVY_DAMAGE,

		bits_SOUND_DANGER,
		"TakeCover"
	}
};

Task_t	tlEliteWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slEliteWaitInCover[] =
{
	{ 
		tlEliteWaitInCover,
		ARRAYSIZE ( tlEliteWaitInCover ), 
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
Task_t	tlEliteEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slEliteEnemyDraw[] = 
{
	{
		tlEliteEnemyDraw,
		ARRAYSIZE ( tlEliteEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlEliteFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slEliteFaceTarget[] =
{
	{
		tlEliteFaceTarget,
		ARRAYSIZE ( tlEliteFaceTarget ),
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


Task_t	tlIdleEliteStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleEliteStand[] =
{
	{ 
		tlIdleEliteStand,
		ARRAYSIZE ( tlIdleEliteStand ), 
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

DEFINE_CUSTOM_SCHEDULES( CEliteBarney )
{
	slEliteFollow,
	slEliteEnemyDraw,
	slEliteFaceTarget,
	slIdleEliteStand,
	slEliteWaitInCover,
	sl_elite_HideReload,
	sl_elite_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( CEliteBarney, CTalkMonster );

void CEliteBarney :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CEliteBarney :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (pev->health <= pev->max_health * 0.5)
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
void CEliteBarney :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->sequence == LookupActivity ( ACT_RUN ) || pev->sequence == LookupActivity ( ACT_USE ) ){
    m_flGroundSpeed = 300;
	}
	if (pev->sequence == LookupActivity ( ACT_RUN_HURT ) ){
    m_flGroundSpeed = 250;
	}
	if (pev->sequence == LookupActivity ( ACT_WALK ) ){
    m_flGroundSpeed = 80;
	}
	if (pev->sequence == LookupActivity ( ACT_WALK_HURT ) ){
    m_flGroundSpeed = 65;
	}

	if(pev->health <= pev->max_health * 0.4){
	m_cover_dist = 130;
	}
	else{
	m_cover_dist = 100;
	}

	if(IsAlive() && m_SatchalBambo != NULL && m_hEnemy != NULL){//ң���װ��Ӽ鿪ʼ
		float flDist = ( m_SatchalBambo->pev->origin - pev->origin).Length();
		if ( flDist > 256 ){
			if(m_SatchalBambo->pev->frags == 514){//׼������
				if(IsMoving()){
					if(pev->weapons == 1 && flDist > 512){
					RouteClear();
					}
					else if(pev->weapons == 0 && flDist > 384){
					RouteClear();
					}
				}
				else if(pev->sequence != LookupActivity ( ACT_TWITCH )  ){
				SetActivity ( ACT_TWITCH );//����!
				}
			}
		}
	}

	//���ܤν���
	if(m_rpgms_level >= 45 && m_rpgms_skill5_learn == 0){
	m_follow_mode = 2;
	m_rpgms_skill5_learn = 68;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CEliteBarney :: ISoundMask ( void) 
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
int	CEliteBarney :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

void CEliteBarney :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CEliteBarney :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		if ( pev->health <= pev->max_health * 0.5 )
		{
			// limp!
			iSequence = LookupActivity ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= pev->max_health * 0.5 )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	case ACT_IDLE:
		if ( pev->health <= pev->max_health * 0.5 )
		{
			NewActivity = ACT_STAND;
		}
		iSequence = LookupActivity ( NewActivity );
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
void CEliteBarney :: SetYawSpeed ( void )
{
	pev->yaw_speed = 270;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CEliteBarney :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( m_cAmmoLoaded <= 0 ){
	return FALSE;
	}

	float dist = 1280;

	if(m_guard_mode){
	dist += 256;
	}

	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 ){//����˸߶Ȳ�ϸ�ʱ�����ӹ�������
		dist += 256;
		}
	}

	if(m_aimenemy_mod != 4 && flDist < (dist * 0.25) ){//������
	m_aimenemy_mod = 4;//�۾�
	}
	else if(m_aimenemy_mod != 3 && flDist < (dist * 0.5) ){//�о���
	m_aimenemy_mod = 3;//��
	}
	else if(m_aimenemy_mod != 8 && flDist < (dist * 0.75) ){//��Զ����
	m_aimenemy_mod = 3;//����
	}
	else if(m_aimenemy_mod != 8){//Զ����
	m_aimenemy_mod = 8;//��
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
				if(m_aimenemy_mod == 8)
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
						if(pHitEntity->Classify() != Classify()){
								if(pHitEntity->Classify() == CLASS_PLAYER_ALLY){
								m_lastAttackCheck = FALSE;
								m_checkAttackTime = gpGlobals->time + 0.5;
								}
								else if(pHitEntity->Classify() == CLASS_PLAYER && m_lovehate > 0){
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
			}
			return m_lastAttackCheck;

	}
	return FALSE;
}



//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CEliteBarney :: BarneyFirePistol ( void )
{
	if ( m_cAmmoLoaded <= 0 ){
	return;
	}

	if(pev->body == 0){
	pev->body = 1;
	}

	Vector vecShootOrigin,vecdir;

	UTIL_MakeVectors(pev->angles);

	GetAttachment( 0, vecShootOrigin,vecdir);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );

	// make angles +-180
	if (angDir.x > 180)
	{
		angDir.x = angDir.x - 360;
	}

	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	if(m_rpgms_skill5_learn == 68){//ǹеǿ��
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_12MM,1,114);
	}
	else{
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_12MM,1);
	}
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "barney/glock18_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?

	m_cAmmoLoaded--;// take away a bullet!

}
		

void CEliteBarney :: BarneyFirePistol2 ( void )
{
	if ( m_cAmmoLoaded <= 0 ){
	return;
	}

	Vector vecShootOrigin,vecdir;

	UTIL_MakeVectors(pev->angles);

	GetAttachment( 1, vecShootOrigin,vecdir);

	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );

	// make angles +-180
	if (angDir.x > 180)
	{
		angDir.x = angDir.x - 360;
	}

	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2048, BULLET_12MM,1);
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "barney/glock18_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

//	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?

	m_cAmmoLoaded--;// take away a bullet!

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CEliteBarney :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		BarneyFirePistol2();
		break;

	case 3:
		BarneyFirePistol();
		break;

	case GUS_AE_DRAW:
		pev->body = 1;
		m_fGunDrawn = TRUE;
		break;

	case GUS_AE_HOLSTER:
		pev->body = 0;
		m_fGunDrawn = FALSE;
		break;

	case GUS_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barney/reload.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case 6:
		{
			if(m_SatchalBambo == NULL){
				Vector vecShootOrigin,vecdir;
				GetAttachment( 0, vecShootOrigin,vecdir);
				CBaseEntity *pSatchel = Create( "monster_satchel_green", vecShootOrigin, Vector( 0, 0, 0), edict() );
				m_SatchalBambo = pSatchel;
				if(m_hEnemy != NULL){
					float flDist = ( m_hEnemy->pev->origin - pev->origin).Length();
					UTIL_MakeVectors(pev->angles);
					Vector	vecSpitOffset;
					Vector	vecSpitDir,vecdir;

					if( m_hEnemy->IsMoving() ){//�����ƶ��У���ԭ�ؾ�����
						pev->weapons = 1;
						pSatchel->pev->velocity = pev->velocity;
					}
					else{//���˾�ֹ״̬��������һ��
						pev->weapons = 0;
						if ( flDist > 512 )
						flDist = 512;

						vecSpitOffset = vecShootOrigin + gpGlobals->v_forward * 16;
						vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();
						pSatchel->pev->velocity = pSatchel->pev->velocity + vecSpitDir * flDist;
					}
				}
			}
		}
		break;

	case 7:
		{
			if(m_SatchalBambo != NULL){
			m_movementActivity = ACT_RUN;
			}
		}
		break;

	case 8:
		{
			if(m_SatchalBambo != NULL){
				m_SatchalBambo = NULL;
				CBaseEntity *pSatchel = NULL;
				while ((pSatchel = UTIL_FindEntityInSphere( pSatchel, pev->origin, 4096 )) != NULL)
				{
					if (FClassnameIs( pSatchel->pev, "monster_satchel_green"))
					{
						if (pSatchel->pev->owner == edict())
						{
							pSatchel->Use( this, this, USE_ON, 0 );
						}
					}
				}
			}

		}
		break;

	case 9:
		{
			if(m_hEnemy != NULL){
			m_hEnemy = NULL;
			m_hOldEnemy[0] = NULL;
			m_hOldEnemy[1] = NULL;
			m_hOldEnemy[2] = NULL;
			m_hOldEnemy[3] = NULL;
			}
			ClearSchedule();
			SetYawSpeed();
		}
		break;

	case 10:
		{
			if(m_hEnemy != NULL && m_SatchalBambo == NULL){
			Vector vecShootOrigin = m_hEnemy->BodyTarget_e(vecShootOrigin);
			CBaseEntity *pSatchel = Create( "monster_satchel_green", vecShootOrigin, Vector( 0, 0, 0), edict() );
			pSatchel->pev->velocity = pev->velocity;
			m_SatchalBambo = pSatchel;

			m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 300;
			}
		}
		break;

	case 11:
		{
			if(m_hEnemy != NULL){//��׼
				Vector vecShootOrigin,vecdir;

				UTIL_MakeVectors(pev->angles);

				GetAttachment( 0, vecShootOrigin,vecdir);

				Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

				Vector angDir = UTIL_VecToAngles( vecShootDir );

				// make angles +-180
				if (angDir.x > 180)
				{
					angDir.x = angDir.x - 360;
				}

				SetBlending( 0, angDir.x );
			}
		}
		break;

	case 12:
		{
			FireTargets( "lelite_defuser", this, this, USE_TOGGLE, 0 );
			m_iTriggerCondition = 0;
			pev->body = 0;
			m_fGunDrawn = FALSE;
			m_cAmmoLoaded = m_cClipSize;
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		}
		break;

	case 13:
		{//������
				CBaseEntity *pTrip = NULL;
				while ((pTrip = UTIL_FindEntityInSphere( pTrip, pev->origin, 512 )) != NULL)
				{
					if (FClassnameIs( pTrip->pev, "monster_tripmine"))
					{
						pTrip->pev->frags = 1;
						pTrip->Killed( pev, GIB_NEVER );
						UTIL_Sparks(pTrip->pev->origin);
					}
				}
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CEliteBarney :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/lelite.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->health			= 360;
	m_lovehate          = 90;

	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_cClipSize	    	= 30;
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_canheadcrab_mode  = 0;
	m_crouchmode        = 0;
	m_canbarnacle_mode  = 1;

	m_candrownwater = 1;

	m_ignoredamage = 1;
	m_headdef = 2;//��Ӳͷ��
	//m_EyeMod = 2;//Ŀ��ģʽ

	MonsterInit();
	SetUse( &CEliteBarney::FollowerUse );

	m_chase_mode = 2;
	m_chase_failed_max = 4;

	m_MoveFail_SimpleRoad = TRUE;

	m_forcefuckdoor  = TRUE;

	m_rpgms_actor = 10;
	m_rpgms_level = 27;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	pev->netname = MAKE_STRING( "Lelite" );

	m_rpgms_skill1_learn = 25;
	m_rpgms_skill2_learn = 19;//����
	m_rpgms_skill3_learn = 39;//����
	m_rpgms_skill4_learn = 44;//����֮����
	m_rpgms_skill5_learn = 0;//ǹеǿ��68

	m_killed_exp = 600;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEliteBarney :: Precache()
{
	PRECACHE_MODEL("models/lelite.mdl");

	PRECACHE_SOUND("barney/glock18_fire.wav" );
	PRECACHE_SOUND("barney/reload.wav" );

	PRECACHE_SOUND("lelite/lelite_die1.wav");
	PRECACHE_SOUND("lelite/lelite_die2.wav");
	PRECACHE_SOUND("lelite/lelite_die3.wav");
	
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CEliteBarney :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"NULL";
	m_szGrp[TLK_QUESTION] =	"NULL";
	m_szGrp[TLK_IDLE] =		"NULL";
	m_szGrp[TLK_STARE] =	"NULL";
	m_szGrp[TLK_USE] =		"LELITE_OK";
	m_szGrp[TLK_UNUSE] =	"LELITE_WAIT";
	m_szGrp[TLK_STOP] =		"NULL";

	m_szGrp[TLK_NOSHOOT] =	"NULL";
	m_szGrp[TLK_HELLO] =	"NULL";

	m_szGrp[TLK_PLHURT1] =	"NULL";
	m_szGrp[TLK_PLHURT2] =	"NULL"; 
	m_szGrp[TLK_PLHURT3] =	"NULL";

	m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "NULL";		// UNDONE

	m_szGrp[TLK_SMELL] =	"NULL";
	
	m_szGrp[TLK_WOUND] =	"NULL";
	m_szGrp[TLK_MORTAL] =	"NULL";

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


int CEliteBarney :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if(pev->deadflag == DEAD_NO){//����Ч��
		if(!(bitsDamageType & (DMG_AIR | DMG_FALL | DMG_DROWN | DMG_NERVEGAS))){
			if (RANDOM_LONG(0,100) <= 15){
			return 0;
			}
		}
	}

	if ( pev->takedamage && pev->deadflag == DEAD_NO && m_godmode == FALSE &&
	((bitsDamageType & DMG_SLASH) || (bitsDamageType & DMG_CLUB)) ){
	//���ˣ������˺����벢����
			flDamage *= 0.5;
			if (pevAttacker)
			{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);

				if(pEntity->pev->deadflag == DEAD_NO && pEntity->pev->takedamage){
				pEntity->TakeDamage ( pev, pev, flDamage, DMG_GENERIC | DMG_NEVERGIB );//����!
				}
			}
	}

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	m_alert	= 100;
	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		if ( flDamage > 0 && m_lovehate > 0 && pev->deadflag == DEAD_NO && m_rpgms_inteam == 0 )
		{
			PlaySentence( "LELITE_NOSHOOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

//=========================================================
// DeathSound 
//=========================================================
void CEliteBarney :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "lelite/lelite_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "lelite/lelite_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "lelite/lelite_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CEliteBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CEliteBarney::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0){
		if ( (pevAttacker->flags & FL_CLIENT) && m_MonsterState != MONSTERSTATE_PRONE )
		{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
				if(pEntity){
				Alert_Clients(pEntity);
				}
		}
	}

	CTalkMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CEliteBarney :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slEliteEnemyDraw;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if(m_SatchalBambo == NULL){
		return &sl_elite_TakeCover2[ 0 ];
		}

		return &sl_elite_TakeCover[ 0 ];
	}

	case SCHED_elite_TAKECOVER_FAILED:
	{
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if(m_coverTime < gpGlobals->time){
			m_coverTime = gpGlobals->time + 2;
			}
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		return GetScheduleOfType ( SCHED_FAIL );
	}

	case SCHED_elite_COVER_AND_RELOAD:
	{
		if(m_hEnemy != NULL){
			float flDist = ( m_hEnemy->pev->origin - pev->origin).Length();

			if(m_SatchalBambo == NULL){
				if ( flDist < 640 ){
				return &sl_elite_HideReload2[ 0 ];
				}
			}

			if ( flDist < 960 ){
			return &sl_elite_HideReload[ 0 ];
			}
			else{
			return &slReload[ 0 ];
			}
		}
		else{
			return &slReload[ 0 ];
		}
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if(m_cleardally_enemy != 0){//Bug Fix 2.0 �����޸�Leliteս��תͷ����ң��������
			return slCombatFace;
		}
		else if (psched == slIdleStand)
			return slEliteFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slEliteFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleEliteStand;
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
Schedule_t *CEliteBarney :: GetSchedule ( void )
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
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
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn ){
				if ( m_hEnemy != NULL){
				return GetScheduleOfType( SCHED_ARM_WEAPON );
				}
			}

			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType ( SCHED_elite_COVER_AND_RELOAD );
			}

			if ( HasConditions(bits_COND_NEW_COVER_MODE))
			{
				if(gpGlobals->time > m_coverTime){
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}

		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				//StopFollowing( FALSE );
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

		if ( m_cleardally_enemy_long == 0){
			if ( m_cAmmoLoaded < m_cClipSize){
			return GetScheduleOfType ( SCHED_RELOAD_DEEP );//������
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) ){
		return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
		}
		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CEliteBarney :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void CEliteBarney::DeclineFollowing( void )
{

}
