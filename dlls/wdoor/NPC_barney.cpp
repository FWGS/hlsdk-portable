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

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define		BARNEY_AE_RELOAD	( 5 )
#define		BARNEY_AE_MELEE		( 6 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE			2

enum
{
	SCHED_BARNEY_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_BARNEY_TAKECOVER_FAILED,
	SCHED_BARNEY_USE_SHIELD,
};


class CBarney : public CTalkMonster
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
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

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
	float	m_flLegDamage;

	CUSTOM_SCHEDULES;
};
LINK_ENTITY_TO_CLASS( monster_barney, CBarney );
LINK_ENTITY_TO_CLASS( monster_barney_shield, CBarney );
LINK_ENTITY_TO_CLASS( monster_barney_hevshield, CBarney );
LINK_ENTITY_TO_CLASS( monster_barney_tr4, CBarney );

TYPEDESCRIPTION	CBarney::m_SaveData[] = 
{
	DEFINE_FIELD( CBarney, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CBarney, m_flLegDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarney, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE ( tlBaFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};


Task_t	tl_Barney_TakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_BARNEY_TAKECOVER_FAILED},
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_Barney_TakeCover[] =
{
	{ 
		tl_Barney_TakeCover1,
		ARRAYSIZE ( tl_Barney_TakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tl_Barney_HideReload[] =
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

Schedule_t sl_Barney_HideReload[] = 
{
	{
		tl_Barney_HideReload,
		ARRAYSIZE ( tl_Barney_HideReload ),
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_NEW_ENEMY,

		bits_SOUND_DANGER,
		"TakeCover"
	}
};

Task_t	tlBarneyWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slBarneyWaitInCover[] =
{
	{ 
		tlBarneyWaitInCover,
		ARRAYSIZE ( tlBarneyWaitInCover ), 
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


Task_t	tlBarneyShideDef[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_GUARD },
};

Schedule_t slBarneyShideDef[] = 
{
	{
		tlBarneyShideDef,
		ARRAYSIZE ( tlBarneyShideDef ),
		0,
		0,
		"Barney Shield Def"
	}
};

Task_t	tlBarneyShideDef2[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_CROUCHIDLE },
};

Schedule_t slBarneyShideDef2[] = 
{
	{
		tlBarneyShideDef2,
		ARRAYSIZE ( tlBarneyShideDef2 ),
		0,
		0,
		"Barney Shield Def2"
	}
};
//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] = 
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE ( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE ( tlBaFaceTarget ),
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


Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBaStand[] =
{
	{ 
		tlIdleBaStand,
		ARRAYSIZE ( tlIdleBaStand ), 
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

DEFINE_CUSTOM_SCHEDULES( CBarney )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBarneyShideDef,
	slBarneyShideDef2,
	slBaFaceTarget,
	slIdleBaStand,
	slBarneyWaitInCover,
	sl_Barney_HideReload,
	sl_Barney_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( CBarney, CTalkMonster );

void CBarney :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CBarney :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (pev->health <= pev->max_health * 0.5 && m_rpgms_skill1_learn == 44)
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
void CBarney :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 270;
	}

	if(pev->health <= pev->max_health * 0.5){
	m_cover_dist = 160;
	}
	else{
	m_cover_dist = 128;
	}

	if(pev->sequence == LookupActivity ( ACT_GUARD ) ){
	m_duckseq = 4;
	}
	else if(pev->sequence == LookupActivity ( ACT_CROUCHIDLE ) ){
	m_duckseq = 5;
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
	}

	//Bug Fix 3.0 ����ģʽ�µж���ͨ����Ѫ������
	if (pev->team == 1 && g_iSkillLevel == SKILL_HARD){
		if(pev->max_health == 80){
			pev->max_health	= 100;
			pev->health	= 100;
		}
		else if(pev->max_health == 120 && m_headdef == 2){
		//�жԺ�ɫ���ƾ���Ѫ��������
			pev->max_health	= 140;
			pev->health	= 140;
		}
	}

	if(m_EyeMod == 0 && m_diefadeout == 0){
		if ( m_hEnemy != NULL || pev->health < pev->max_health || m_alert > 0){
			m_EyeMod = 2;//���˻��ֵ��˾��䣬����ս��״̬��Ŀ��ģʽ
		}
	}

	if(m_killbyheadcrab >= 1){
				if ( GetBodygroup( 1 ) == 0 ){
				SetBodygroup( 1, 1 );
				pev->health = pev->max_health;
				}
				m_crabzombie_begain += 1;
				if(m_crabzombie_begain >= 80){
					if (IsMoving() || m_fightmode){
					m_crabzombie_begain -= 10;
					return;
					}
					Vector trace_origin;
					trace_origin = pev->origin + Vector(0,0,38);
					TraceResult trace;
					UTIL_TraceHull(trace_origin, trace_origin, dont_ignore_monsters, human_hull, ENT(pev),&trace);
					if ( trace.fStartSolid )
					{
					m_crabzombie_begain -= 10;
					return;
					}
					SpawnBlood(Center(), BloodColor(), 250);
					CBaseEntity *pZombie = Create( "monster_zombie_barney", pev->origin, pev->angles, edict() );
					pZombie->pev->angles.x = 0;
					pZombie->pev->angles.z = 0;
					CBaseMonster *pMonster = pZombie->MyMonsterPointer( );
					pMonster->SetActivity( ACT_FALL );
					pZombie->pev->impulse = pev->impulse;

					UTIL_Remove( this );
					m_killbyheadcrab = 0;
					return;
				}
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney :: ISoundMask ( void) 
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
int	CBarney :: Classify ( void )
{
	if ( pev->team == 1 ){
		return	CLASS_HUMAN_MILITARY;
	}
	else{
		return	CLASS_PLAYER_ALLY;
	}
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}
}

void CBarney :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

void CBarney :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		NewActivity = ACT_IDLE;

	CTalkMonster::SetActivity( NewActivity );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney :: CheckRangeAttack1 ( float flDot, float flDist )
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

BOOL CBarney :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney :: BarneyFirePistol ( void )
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

	if(FClassnameIs ( pev, "monster_barney_hevshield" )){
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.02,0.02,0.02), 2560, BULLET_14MM,1);
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
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
void CBarney :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		SetBodygroup( 2, 1 );
		m_fGunDrawn = TRUE;
		break;

	case BARNEY_AE_HOLSTER:
		SetBodygroup( 2, 0 );
		m_fGunDrawn = FALSE;
		break;

	case BARNEY_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barney/reload2.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case 6:
		if(m_hEnemy != NULL && m_HenemyEnemyMe >= 1){
		m_flPlayerDamage -= 10;
		m_flLegDamage -= 10;
		}
		else{
		m_flPlayerDamage = 0;
		m_flLegDamage = 0;
		}
		break;

	case 7:
		pev->solid = SOLID_NOT;
		pev->oldorigin = pev->origin;
		m_walkaround = FALSE;
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!BA_ZP7", VOL_NORM, 0.6, 0, PITCH_NORM );
		char text[256];
		
		sprintf( text, "Guard: Okay. I'm heading up.\n");
		
		UTIL_SayTextAll( text,this );
		break;

	case 8:
		ClearBits( pev->flags, FL_ONGROUND );
		UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );
		pev->movetype = MOVETYPE_FLY;
		pev->angles.y = 0;
		pev->velocity.x = 0;
		pev->velocity.y = 0;
		pev->velocity.z = 70;
		break;

	case 9:
		{
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
					if ( pEntity )
					{
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
						pPlayer->Clear_SayText();
						if(( pev->origin - pPlayer->pev->origin ).Length() <= 100.0){	//��ҿ���̫��
						EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!BA_MAD0", VOL_NORM, 0.5, 0, PITCH_NORM );

						char text[256];
						
						sprintf( text, "Guard: Hey! step back!\n");
					
						UTIL_SayTextAll( text,this );

						pev->impulse = 2;
						}
					}
					pev->movetype = MOVETYPE_STEP;
					pev->solid = SOLID_SLIDEBOX;
					pev->velocity.x = 0;
					pev->velocity.y = -100;
					pev->velocity.z = 350;
		}
		break;

	case 10:
		{
			if(pev->impulse == 2){//����ʧ�ܣ��Բ���
				pev->health = 0;
				Killed( pev, GIB_NEVER );
					CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
					if ( pEntity )
					{
						CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
						pPlayer->Clear_SayText();
					}
			}
			m_EyeMod = 1;
			m_groundElev2 = TRUE;//������ҵĸ����Ż�����
			m_lovehate = 60;//����Һøж�����
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney :: Spawn()
{
	Precache( );

	if ( FClassnameIs ( pev, "monster_barney" ) ){
		SET_MODEL(ENT(pev), "models/barney.mdl");
		if(pev->health == 0){
		pev->health			= 80;
		}
		m_canheadcrab_mode  = 1;

		pev->netname = MAKE_STRING( "Guard" );
	}
	else if ( FClassnameIs ( pev, "monster_barney_shield" ) 
	|| FClassnameIs ( pev, "monster_barney_tr4" ) ){
		SET_MODEL(ENT(pev), "models/barney_shield.mdl");
		m_headdef			= 2;//�߼�ͷ��
		m_canheadcrab_mode  = 0;//�޷���ͷз��ʬ����
			if(pev->health == 0){
			pev->health			= 120;
			}

			if(FClassnameIs ( pev, "monster_barney_tr4" )){
			pev->health			= 180;
			}

		pev->netname = MAKE_STRING( "Blk.Guard" );
	}
	else{
		SET_MODEL(ENT(pev), "models/hev_shield.mdl");
		m_headdef			= 3;//HEVͷ��
		m_canheadcrab_mode  = 0;//�޷���ͷз��ʬ����

		if (g_iSkillLevel == SKILL_HARD){
		pev->health			= 360;
		}
		else{
		pev->health			= 300;
		}

		pev->netname = MAKE_STRING( "HEV.Guard" );
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0;
	m_fGunDrawn			= FALSE;

	m_cClipSize	    	= 15;
	m_cAmmoLoaded		= m_cClipSize;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_canbarnacle_mode  = 1;

	m_candrownwater = 1;

	m_aimenemy_mod = 5;

	if ( pev->team == 0 ){
	m_lovehate = 40;
	}

	if ( FClassnameIs ( pev, "monster_barney_tr4" ) ){
	m_headdef  = 2;//�߼�ͷ��
	m_flFieldOfView = -1;
	pev->netname = MAKE_STRING( "Blk.Guard" );
	m_lovehate = 80;
	pev->health	= 200;
	}

	MonsterInit();
	SetUse( &CBarney::FollowerUse );

	m_aimflag_dist = 100.0;

	if(m_headdef == 3){
		m_killed_exp = 160;
		m_rpgms_level = 60;
		m_headdef = 2;
		m_cClipSize	    	= 7;
		m_cAmmoLoaded		= m_cClipSize;
		m_ignoredamage		= 2;
		pev->spawnflags |= SF_MONSTER_GAG;//��Ĭ����
		m_lovehate = 0;//ֻ�ܵж�
	}
	else if(m_headdef == 2){
		m_killed_exp = 120;
		m_rpgms_actor = 6;
		m_rpgms_level = 10;
		m_rpgms_exp = 0;
		m_rpgms_type = 1;
	}
	else{
		m_killed_exp = 80;
		m_rpgms_actor = 4;
		m_rpgms_level = 5;
		m_rpgms_exp = 0;
		m_rpgms_type = 1;
	}

	m_rpgms_skill1_learn = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney :: Precache()
{
	UTIL_PrecacheOther( "monster_zombie_barney" );

	PRECACHE_MODEL("models/barney.mdl");
	PRECACHE_MODEL("models/barney_shield.mdl");
	PRECACHE_MODEL("models/hev_shield.mdl");

	PRECACHE_SOUND("items/flatline.wav");
	//Bug Fix 2.0 HEV����������Чȱʧ

	PRECACHE_SOUND("barney/reload2.wav" );

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
void CBarney :: TalkInit()
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

	m_szGrp[TLK_SMELL] =	"NULL";
	
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


int CBarney :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	m_alert = 100;

	if(m_LastHitGroup == 6 || m_LastHitGroup == 7 || flDamage >= 1){
		if(m_flLegDamage < 40){
		m_flLegDamage += flDamage;
		}
		else if(m_flLegDamage > 40){
		m_flLegDamage = 40;
		}
	}

	if(m_flPlayerDamage < 40){
	m_flPlayerDamage += flDamage;
	}
	else if(m_flPlayerDamage > 40){
	m_flPlayerDamage = 40;
	}
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

	
//=========================================================
// PainSound
//=========================================================
void CBarney :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	if(!FClassnameIs ( pev, "monster_barney_hevshield" )){
		switch (RANDOM_LONG(0,2))
		{
			case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney :: DeathSound ( void )
{
	if(FClassnameIs ( pev, "monster_barney_hevshield" )){
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/flatline.wav", 1.0, 0.7, 0, 100 );
	}
	else{
		switch (RANDOM_LONG(0,2))
		{
			case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
			case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
}


void CBarney::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(ptr->iHitgroup == 10){//��û��ʲô���õ�ͷ��
		if ( pev->dmgtime != gpGlobals->time)
		{
		pev->dmgtime = gpGlobals->time;

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos);
		WRITE_BYTE(TE_STREAK_SPLASH);
		WRITE_COORD(ptr->vecEndPos.x);
		WRITE_COORD(ptr->vecEndPos.y);
		WRITE_COORD(ptr->vecEndPos.z);
		WRITE_COORD(ptr->vecPlaneNormal.x);
		WRITE_COORD(ptr->vecPlaneNormal.y);
		WRITE_COORD(ptr->vecPlaneNormal.z);
		WRITE_BYTE(5);
		WRITE_SHORT(22);
		WRITE_SHORT(25);
		WRITE_SHORT(65);
		MESSAGE_END();
		}

	ptr->iHitgroup = HITGROUP_HEAD;
	}

	if(FClassnameIs ( pev, "monster_barney_shield" ) || FClassnameIs ( pev, "monster_barney_tr4" )
	|| FClassnameIs ( pev, "monster_barney_hevshield" )){

			if(ptr->iHitgroup == 0 || ptr->iHitgroup == 8){
					flDamage -= 60;

					if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
					{
							pev->dmgtime = gpGlobals->time;

							if (RANDOM_LONG(0, 1))
								EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
							else
								EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

							UTIL_Sparks(ptr->vecEndPos);
					}

					if(flDamage < 1){
					return;
					}
			}
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CBarney::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( 2 ) != 2 && !m_undropgun)
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		SetBodygroup( 2, 2 );

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun;
		if(FClassnameIs ( pev, "monster_barney_hevshield" )){
		pGun = DropItem( "weapon_deagle", vecGunPos, vecGunAngles );
		}
		else{
		pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
		}

		if ( pGun )
		{
			pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-50,50), RANDOM_FLOAT(-50,50), RANDOM_FLOAT(150,200));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 300 ), 0 );
		}

		if(m_diefadeout == 1){
		pGun->pev->armorvalue = 100;
		}

	}


	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CBarney :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_BARNEY_USE_SHIELD:
		if ( m_hEnemy != NULL )
		{
			if(m_flLegDamage >= 20){
			return slBarneyShideDef2;
			}
			else{
			return slBarneyShideDef;
			}
		}
		break;

	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		return &sl_Barney_TakeCover[ 0 ];
	}

	case SCHED_BARNEY_TAKECOVER_FAILED:
	{
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}

		if(FClassnameIs ( pev, "monster_barney_shield" ) || FClassnameIs ( pev, "monster_barney_tr4" ) 
		|| FClassnameIs ( pev, "monster_barney_hevshield" )){
			return GetScheduleOfType ( SCHED_BARNEY_USE_SHIELD );
		}
		else{
			return GetScheduleOfType ( SCHED_FAIL );
		}
	}

	case SCHED_BARNEY_COVER_AND_RELOAD:
	{
		return &sl_Barney_HideReload[ 0 ];
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if(m_cleardally_enemy != 0){//Bug Fix 2.0 �����޸�����ս��תͷ����ң��������
			return slCombatFace;
		}
		else if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
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
Schedule_t *CBarney :: GetSchedule ( void )
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
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn ){
				return GetScheduleOfType( SCHED_ARM_WEAPON );
			}

			if(m_cAmmoLoaded <= -114){
				return GetScheduleOfType ( SCHED_RELOAD_DEEP );
			}

			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				if ( HasConditions(bits_COND_NEW_COVER_MODE)){
				return GetScheduleOfType( SCHED_BARNEY_COVER_AND_RELOAD );
				}
				else{
				return GetScheduleOfType ( SCHED_RELOAD_DEEP );
				}
			}

			if(FClassnameIs ( pev, "monster_barney_shield" ) || FClassnameIs ( pev, "monster_barney_tr4" ) ){
				if (m_flPlayerDamage >= 20){
				return GetScheduleOfType( SCHED_BARNEY_USE_SHIELD );
				}
			}
			else{
				if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) && pev->health <= pev->max_health * 0.6){
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
				else if ( HasConditions(bits_COND_NEW_COVER_MODE) ){
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}

		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( m_hEnemy == NULL && IsFollowing()  )
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

MONSTERSTATE CBarney :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void CBarney::DeclineFollowing( void )
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
class CDeadBarney : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[7];
};

char *CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "laser_top", "lying_on_half", "pain_lying", "lying_on_sit" };

void CDeadBarney::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_barney_dead, CDeadBarney );

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney :: Spawn( )
{
	PRECACHE_MODEL("models/barney.mdl");
	SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead barney with bad pose\n" );
	}

	pev->body = 0;

	// Corpses have less health
	pev->health			= 20;

	SetBodygroup( 2, 2 );//ǹ����û��ǹ
	m_die_dont_move = 1;

	MonsterInitDead();
}



//=========================================================

class CDeadORG : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_PLAYER_ALLY; }
	void  EXPORT glock_think ( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	CSprite		*m_pEyeGlow;		// Glow around the eyes

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_org_dead, CDeadORG );


TYPEDESCRIPTION	CDeadORG::m_SaveData[] = 
{
	DEFINE_FIELD( CDeadORG, m_pEyeGlow, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CDeadORG, CBaseMonster );

//=========================================================
// ********** Dead ORG SPAWN **********
//=========================================================
void CDeadORG :: Spawn( )
{
	PRECACHE_MODEL("models/dead_org.mdl");
	PRECACHE_MODEL("sprites/glow01.spr");

	SET_MODEL(ENT(pev), "models/dead_org.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	if(pev->frags == 0){
	pev->sequence = 1;
	}
	
	pev->health			= 20;

	MonsterInitDead();

	m_die_dont_move = 1;

	pev->nextthink = gpGlobals->time + 1.5;
	SetThink( &CDeadORG::glock_think );

	if(pev->frags == 0 && pev->body == 0){
	m_pEyeGlow = CSprite::SpriteCreate( "sprites/glow01.spr", pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 155, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 1 );
	m_pEyeGlow->pev->scale = 0.5;
	}
}

int CDeadORG :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return 1;
}

void CDeadORG :: glock_think( void )
{
	
	if ( pev->body == 0 )
	{
		Vector vecGunPos;
		Vector vecGunAngles;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )//���
			{
					float flDist = ( pEntity->pev->origin - vecGunPos).Length();
					if(flDist <= 64){
						CBaseEntity *pGun = DropItem( "weapon_9mmhandgun", pEntity->pev->origin, pev->angles );
						pev->body = 1;
						if(m_pEyeGlow){
						UTIL_Remove( m_pEyeGlow );
						m_pEyeGlow = NULL;
						}
						if (g_iSkillLevel == SKILL_HARD){//����ģʽ�ӹ�
						FireTargets( "fuckdown_man_glock", this, this, USE_TOGGLE, 0 );
						}
					}
			}
	}

	pev->nextthink = gpGlobals->time + 0.2;
		
}





class CExpBarr : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_MACHINE; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );
	void EXPORT EXPTouch ( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( monster_expbarara, CExpBarr );

//=========================================================
// ********** Dead ORG SPAWN **********
//=========================================================
void CExpBarr :: Spawn( )
{
	SET_MODEL( ENT(pev), "models/props_all.mdl" );
	pev->body = 5;

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= DONT_BLEED;

	pev->sequence = 0;

	pev->health			= 5;
	pev->avelocity.x    = 400;

	MonsterInitDead();
	SetTouch( &CExpBarr::EXPTouch );
}


void CExpBarr::EXPTouch( CBaseEntity *pOther )
{
		float flSpeed = -pev->velocity.z;
		if (flSpeed > 500){
			float fuckedspeed = flSpeed - 500;
			TakeDamage(VARS(eoNullEntity), VARS(eoNullEntity), fuckedspeed * 0.1, DMG_FALL);
		}
}

void CExpBarr :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->model = iStringNull;// make invisible
	SetThink( &CExpBarr::SUB_Remove );
	SetTouch( NULL );
	pev->nextthink = gpGlobals->time + 0.5;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	// play squeek blast

	int iContents = UTIL_PointContents ( pev->origin );
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		if (iContents != CONTENTS_WATER)
		{
			WRITE_SHORT( g_sModelIndexFireball );
		}
		else
		{
			WRITE_SHORT( g_sModelIndexWExplosion );
		}
		WRITE_BYTE( 0  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( 10 );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	FX_Trail( pev->origin, entindex(), PROJ_DUMBFIRE_DETONATE );

	::RadiusDamage2( pev->origin, pev, pev, 125, 250, CLASS_NONE, DMG_BLAST);

	CBaseMonster :: Killed( pevAttacker, GIB_ALWAYS );
}

void CExpBarr :: GibMonster( void )
{
}

