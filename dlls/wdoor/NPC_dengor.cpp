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
#include	"shake.h"
#include	"player.h"
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		DENGOR_AE_DRAW		( 2 )
#define		DENGOR_AE_SHOOT		( 3 )
#define		DENGOR_AE_HOLSTER	( 4 )
#define		DENGOR_AE_RELOAD	( 5 )
#define		DENGOR_AE_MELEE		( 6 )

#define	DENGOR_BODY_GUNHOLSTERED	0
#define	DENGOR_BODY_GUNDRAWN		1
#define DENGOR_BODY_GUNGONE			2

enum
{
	SCHED_DENGOR_TAKECOVER_FAILED = LAST_COMMON_SCHEDULE + 1,
};


class CDengor : public CTalkMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	void DENGORFirePistol( int mode );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	virtual int	ObjectCaps( void ) { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );

	void DeclineFollowing( void );

	int IRelationship ( CBaseEntity *pTarget );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );
	MONSTERSTATE GetIdealState ( void );

	void SetActivity ( Activity NewActivity );

	BOOL FCanCheckAttacks ( void );
	void RunAI( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );
	
	void DeathSound( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	float m_dyingtime;
    float m_SkillTime;
	float m_Skill2Time;

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	int dengor_fire;

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_dengor, CDengor );

TYPEDESCRIPTION	CDengor::m_SaveData[] = 
{
	DEFINE_FIELD( CDengor, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDengor, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CDengor, m_SkillTime, FIELD_TIME ),
	DEFINE_FIELD( CDengor, m_Skill2Time, FIELD_TIME ),
	DEFINE_FIELD( CDengor, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CDengor, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDengor, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CDengor, m_dyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDengor, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlDengorFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)100		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slDengorFollow[] =
{
	{
		tlDengorFollow,
		ARRAYSIZE ( tlDengorFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_SEE_ENEMY |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};


Task_t	tl_DENGOR_TakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_DENGOR_TAKECOVER_FAILED},
	{ TASK_WAIT,					(float)0.1							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_FACE_ENEMY,				(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
};

Schedule_t	sl_DENGOR_TakeCover[] =
{
	{ 
		tl_DENGOR_TakeCover1,
		ARRAYSIZE ( tl_DENGOR_TakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tlDENGORWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slDENGORWaitInCover[] =
{
	{ 
		tlDENGORWaitInCover,
		ARRAYSIZE ( tlDENGORWaitInCover ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"DENGORWaitInCover"
	},
};

//=========================================================
// DENGORDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlDENGOREnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slDENGOREnemyDraw[] = 
{
	{
		tlDENGOREnemyDraw,
		ARRAYSIZE ( tlDENGOREnemyDraw ),
		0,
		0,
		"DENGOR Enemy Draw"
	}
};

Task_t	tlDengorFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slDengorFaceTarget[] =
{
	{
		tlDengorFaceTarget,
		ARRAYSIZE ( tlDengorFaceTarget ),
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


Task_t	tlIdleHlStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleHlStand[] =
{
	{ 
		tlIdleHlStand,
		ARRAYSIZE ( tlIdleHlStand ), 
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

DEFINE_CUSTOM_SCHEDULES( CDengor )
{
	slDengorFollow,
	slDENGOREnemyDraw,
	slDengorFaceTarget,
	slIdleHlStand,
	slDENGORWaitInCover,
	sl_DENGOR_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( CDengor, CTalkMonster );

void CDengor :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CDengor :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/flatline.wav", 1.0, 0.7, 0, 100 );
}

void CDengor :: RunTask( Task_t *pTask )
{
	CTalkMonster::RunTask( pTask );
}


//=========================================================
// RunAI
//=========================================================
void CDengor :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ){
		m_dyingtime++;
		if(m_dyingtime > 30){//��������
		BarnacleVictimReleased();
		m_dyingtime = -20;
		m_canbarnacle_mode  = 0;
		}
	}
	else if(m_dyingtime > 0){
		m_dyingtime--;
	}
	else if(m_dyingtime < 0){
		m_dyingtime++;
		if(m_dyingtime == 0){
		m_canbarnacle_mode  = 1;
		}
	}

		if (pev->sequence == LookupActivity ( ACT_RUN )){
		m_flGroundSpeed = 320;
		}
		else if(pev->sequence == LookupActivity ( ACT_WALK )){
		m_flGroundSpeed = 100;
		}

		if(m_cAmmoLoaded < 15){
		m_cAmmoLoaded++;
		}

		if ( m_hEnemy != NULL ){
			if ( FClassnameIs(m_hEnemy->pev, "monster_human_grunt") 
			|| FClassnameIs(m_hEnemy->pev, "monster_alien_slave") 
			|| FClassnameIs(m_hEnemy->pev, "monster_alien_grunt")
			|| FClassnameIs(m_hEnemy->pev, "monster_alien_controller")
			|| FClassnameIs(m_hEnemy->pev, "monster_gargantua")){
			m_cover_dist = 1024;
			}
			else{
			m_cover_dist = 128;
			}
		}

		if(m_flPlayerDamage > 0){
			if(m_flPlayerDamage > 100 || pev->health < pev->max_health * 0.3){
			m_flPlayerDamage = 100;
			m_chase_mode = -1;
			}
			else{
			m_flPlayerDamage -= 1;
			m_chase_mode = 3;
			}
		}

	//���ܤν���
	if(m_rpgms_level >= 60 && m_rpgms_skill4_learn == 0){
		m_follow_mode = 2;
		m_rpgms_skill3_learn = 69;
		m_rpgms_skill4_learn = 71;
		m_rpgms_skill5_learn = 80;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CDengor :: ISoundMask ( void) 
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
int	CDengor :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

int CDengor::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_freeman" ) )
	{
		return R_NM;
	}

	return CTalkMonster::IRelationship( pTarget );
}

void CDengor :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( NewActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE ){
		if(m_crouchmode == 1){
		NewActivity = ACT_CROUCHIDLE;
		}
		else{
		NewActivity = ACT_IDLE;
		}
	}

	if(NewActivity == ACT_IDLE){
		if(m_crouchmode == 1){
		NewActivity = ACT_CROUCHIDLE;
		}
		else if ( m_cleardally_enemy > 0 )
		{
		NewActivity = ACT_IDLE_ANGRY;
		}
	}
	else if(NewActivity == ACT_WALK || NewActivity == ACT_RUN){
		if(m_crouchmode == 1){
		NewActivity = ACT_CROUCH;
		m_movementActivity = ACT_CROUCH;
			if(!FBitSet(pev->effects, EF_DIMLIGHT) && pev->impulse == 0){
			SetBits(pev->effects, EF_DIMLIGHT);//ս���ֵ磬��
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/flashlight1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			}
		}
	}

	CTalkMonster::SetActivity( NewActivity );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDengor :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL CDengor :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CDengor :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if(m_rpgms_skill4_learn == 71 && m_SkillTime < gpGlobals->time){
		return TRUE;
	}

	return FALSE;
}

BOOL CDengor :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if(HasConditions ( bits_COND_SEE_ENEMY ) && flDot >= 0.5 && m_rpgms_skill5_learn == 80 
	&& m_Skill2Time < gpGlobals->time && flDist >= 256){
		return TRUE;
	}

	return FALSE;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CDengor :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 2560;//����������

	if (flDist <= dist && flDot >= 0.5 )
	{
			TraceResult	tr;

			if (flDist <= 75 )
			{
				if ( m_hEnemy != NULL )
				{
					Vector org1 = BodyTarget_c(pev->origin);
					Vector org2 = m_hEnemy->BodyTarget_c(m_hEnemy->pev->origin);
					if ( fabs( org1.z - org2.z ) <= 15 ){
					return FALSE;
					}
				}
			}

			if ( gpGlobals->time > m_checkAttackTime && m_hEnemy != NULL )
			{
				Vector vecShootOrigin,vecdir;
				vecShootOrigin = pev->origin + Vector( 0, 0, 50 );
				TraceResult tr;
				
				Vector shootOrigin = vecShootOrigin;
				CBaseEntity *pEnemy = m_hEnemy;
				Vector shootTarget = ( (pEnemy->BodyTarget_c( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				UTIL_TraceLine( shootOrigin, shootTarget, ignore_monsters,ignore_glass, ENT(pev), &tr );
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) ){
					m_checkAttackTime = gpGlobals->time + 0.5;
					m_lastAttackCheck = TRUE;
				}
				else{
					if(CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_ALIEN_MONSTER
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_ALIEN_MILITARY
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_HUMAN_ASS
					|| CBaseEntity::Instance(tr.pHit)->Classify() == CLASS_HUMAN_MILITARY){
					m_checkAttackTime = gpGlobals->time + 0.5;
					m_lastAttackCheck = TRUE;
					}
					else{
					m_lastAttackCheck = FALSE;
					m_checkAttackTime = gpGlobals->time + 1.0;
					}
				}
			}
			return m_lastAttackCheck;

	}
	return FALSE;
}

BOOL CDengor :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float cover_dist = 75;

	if ( m_hEnemy != NULL )
	{
		if(FClassnameIs(m_hEnemy->pev, "monster_doma_boss")){
		cover_dist += 160;
		}
	}

	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= cover_dist && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// DENGORFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CDengor :: DENGORFirePistol ( int mode )
{
	if(m_cAmmoLoaded != 15){
	SetActivity ( ACT_IDLE_ANGRY );
	return;
	}
	Vector org,vecdir;
	GetAttachment( 0, org,vecdir);
	m_HackedGunPos = org;
	Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

	if(mode == 1){
	vecShootDir = Vector(0,64,0);
	}

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );

	UTIL_Sparks( m_HackedGunPos );

	FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_GAUSS_HEV,0);

	FireBeam(m_HackedGunPos, vecShootDir, 20, 100, pev);//BEAM_GAUSSCHARGED

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/gauss2.wav", 1, ATTN_NORM, 0, 100);

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded = 0;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CDengor :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 3:
		{
			if(pev->body != 2){
			pev->body = 2;//�ͳ���˹ǹ
			}
			DENGORFirePistol(0);
		}
		break;

	case DENGOR_AE_DRAW:
		{
			if(pev->body != 2){
			pev->body = 2;//�ͳ���˹ǹ
			}

			Vector org,vecdir;
			GetAttachment( 0, org,vecdir);
			m_HackedGunPos = org;
			Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

			Vector angDir = UTIL_VecToAngles( vecShootDir );
			SetBlending( 0, angDir.x );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "ambience/pulsemachine.wav", 1, ATTN_NORM, 0, 100);
		}
		break;

	case 4:
		{//��׼
				if(pev->body != 2){
				pev->body = 2;//�ͳ���˹ǹ
				}

				Vector org,vecdir;
				GetAttachment( 0, org,vecdir);
				m_HackedGunPos = org;
				Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

				Vector angDir = UTIL_VecToAngles( vecShootDir );
				SetBlending( 0, angDir.x );
		}
		break;

	case DENGOR_AE_RELOAD:
		//EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barney/reload2.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case 6:
		{

			if(m_hEnemy != NULL){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					
					Vector vecSrc	= BodyTarget( pev->origin );
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 ){
						if(( vecSrc - tr.vecEndPos).Length() <= 90){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, 60, gpGlobals->v_forward, &tr, DMG_CLUB); 

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 150;
						}

						ApplyMultiDamage( pev, pev );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
						}
					}
					else if(( pev->origin - m_hEnemy->pev->origin).Length() <= 90){
						if ( m_hEnemy->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						m_hEnemy->pev->velocity = (m_hEnemy->pev->origin - pev->origin).Normalize() * 150;
						}

						m_hEnemy->TakeDamage( pev, pev, 60, DMG_CLUB );
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
			}
			else{
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget_c(pev->origin);
				Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
				int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

				ClearMultiDamage( );
				pEntity->TraceAttack(pev, 60, gpGlobals->v_forward, &tr, DMG_CLUB ); 
					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity = (pEntity->pev->origin - pev->origin).Normalize() * 150;
					}
				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
					CBaseEntity *pHurt = CheckTraceHullAttack( 90, 60, DMG_CLUB );
					if ( pHurt )
					{
						if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						pHurt->pev->velocity = (pHurt->pev->origin - pev->origin).Normalize() * 150;
						}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}

	
		}
		break;

	case 7:
		{
			CBaseEntity *pEntity = NULL;
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 128 )) != NULL)//����С��Χ
			{
				if ( FClassnameIs(pEntity->pev, "item_battery") )
				{
				UTIL_Remove( pEntity );
				EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/gunpickup2.wav", 1, 0.5);
				FX_Explosion(pEntity->pev->origin + Vector(0,0,8), EXPLOSION_SHIELDIMPACT );
				TakeHealth(200, DMG_GENERIC);
				break;
				}
			}
		}
		break;

	case 8:
		{
			if(pev->impulse == 2){
			FireTargets( "dengor_vant2", this, this, USE_TOGGLE, 0 );
			}
			else{
			FireTargets( "dengor_vant", this, this, USE_TOGGLE, 0 );
			}
		}
		break;

	case 9:
		{
		//	ClearSchedule();
		//	SetBits(pev->effects, EF_DIMLIGHT);//ս���ֵ磬��
		//	EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/flashlight1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
			m_crouchmode = 1;
			m_walkaround = TRUE;
			m_walkaroundFail = TRUE;
			UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_DUCK);//�Ǹ����
		}
		break;

	case 10:
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "items/suitchargeok1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		break;

	case 11:
		{
			if(pev->angles.y != 90){
			pev->angles.y = 90;
			}

			DENGORFirePistol(1);

			ClearBits( pev->flags, FL_ONGROUND );
			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );

			UTIL_MakeVectors ( pev->angles );
			pev->velocity = pev->velocity - gpGlobals->v_forward * 800 + gpGlobals->v_up * 200;
		}
		break;

	case 13://�˹�
		{
			pev->body = 1;
		}
		break;

	case 14://�ٻ��˹���ħ
		{
			if(m_SkillTime < gpGlobals->time){
			m_SkillTime = gpGlobals->time + 30;
			CBaseEntity *pChild = CBaseEntity::Create( "monster_crowbar", pev->origin, pev->angles, edict() );
			pChild->pev->owner = ENT( pev );

			SetBits( pChild->pev->spawnflags, SF_MONSTER_FADECORPSE );

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
			WRITE_BYTE(3);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT(g_sModelIndexCteleport);
			WRITE_BYTE(15);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
			MESSAGE_END();
						
			EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "debris/beamstart2old.wav", 1, ATTN_NORM, 0, 100 );
			}
		}
		break;

	case 15://��˹ǹ
		{
			pev->body = 2;
		}
		break;

	case 16://ǿ��������
			if(m_Skill2Time <= gpGlobals->time && m_hEnemy != NULL && m_rpgms_skill5_learn == 80 ){
				m_Skill2Time = gpGlobals->time + 15;

				Vector dvt1,dvt2;

				GetAttachment( 1, dvt1, dvt2 );

				MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, dvt1);
				WRITE_BYTE(3);
				WRITE_COORD( dvt1.x);	// pos
				WRITE_COORD( dvt1.y);	
				WRITE_COORD( dvt1.z);
				WRITE_SHORT(dengor_fire);
				WRITE_BYTE(5);
				WRITE_BYTE(15);
				WRITE_BYTE(4);
				MESSAGE_END();

				Vector expcenter = m_hEnemy->Center();

				MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, expcenter );
					WRITE_BYTE( TE_EXPLOSION);
					WRITE_COORD( expcenter.x );
					WRITE_COORD( expcenter.y );
					WRITE_COORD( expcenter.z );
					WRITE_SHORT( g_sModelIndexFireball );
					WRITE_BYTE( 0 ); // no sprite
					WRITE_BYTE( 15  ); // framerate
					WRITE_BYTE( TE_EXPLFLAG_NONE );
				MESSAGE_END();

				FX_Explosion( expcenter, EXPLOSION_TRIPMINE );

				::RadiusDamage_limit( expcenter, pev, pev, 180, 360, CLASS_PLAYER_ALLY, DMG_BLAST);

			}
		break;

/*
	case 12:
		{
			pev->body = 2;//�ͳ���˹ǹ
			m_crouchmode = 0;
			UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
		}
		break;
*/
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CDengor :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/dengor.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->health			= 450;//�����ι⻷
	m_lovehate          = 80;

	m_flFieldOfView		= VIEW_FIELD_FULL; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_cClipSize	    	= 15;
	m_cAmmoLoaded		= 15;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_canheadcrab_mode  = 0;
	m_canbarnacle_mode  = 1;

	m_ignoredamage = 1;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
//	m_walkaround = TRUE;
//	m_walkaroundFail = TRUE;
	m_EyeMod = 2;
	m_headdef = 2;//��Ӳͷ��
	m_aimenemy_mod = 1;

	MonsterInit();

	m_groundElev2 = TRUE;//���ݷ���
	m_aimflag_dist = 80.0;

	SetUse( &CDengor::FollowerUse );

	m_chase_failed_max = 4;

	m_rpgms_actor = 9;
	m_rpgms_level = 20;
	m_rpgms_exp = 0;
	m_rpgms_type = 1;

	pev->netname = MAKE_STRING( "Dengor" );

	m_SkillTime = gpGlobals->time + 3;

	pev->takedamage     = DAMAGE_YES;

	m_rpgms_skill1_learn = 21;//��������
	m_rpgms_skill2_learn = 70;//��������
	m_rpgms_skill3_learn = 0;//����ǿ��
	m_rpgms_skill4_learn = 0;//�˹���ħ
	m_rpgms_skill5_learn = 0;//ǿ��������
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDengor :: Precache()
{
	PRECACHE_MODEL("models/dengor.mdl");
	PRECACHE_SOUND("items/flatline.wav");
	PRECACHE_SOUND("items/hevsuit_pickup.wav");
	
	dengor_fire =  PRECACHE_MODEL("sprites/anim_spr9.spr");

	UTIL_PrecacheOther( "monster_crowbar" );
	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CDengor :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"NULL";
	m_szGrp[TLK_QUESTION] =	"NULL";
	m_szGrp[TLK_IDLE] =		"NULL";
	m_szGrp[TLK_STARE] =		"NULL";
	m_szGrp[TLK_USE] =		"NULL";
	m_szGrp[TLK_UNUSE] =	"NULL";
	m_szGrp[TLK_STOP] =		"NULL";

	m_szGrp[TLK_NOSHOOT] =	"NULL";
	m_szGrp[TLK_HELLO] =	"NULL";

	m_szGrp[TLK_PLHURT1] =	"NULL";
	m_szGrp[TLK_PLHURT2] =	"NULL"; 
	m_szGrp[TLK_PLHURT3] =	"NULL";

	m_szGrp[TLK_PHELLO] =	NULL;	//"HM_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"HM_PIDLE";			// UNDONE
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


int CDengor :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	//��������
	if( (bitsDamageType & DMG_BLAST) || (bitsDamageType & DMG_CRUSH)
	|| (bitsDamageType & DMG_SLASH) || (bitsDamageType & DMG_BULLET)
	|| (bitsDamageType & DMG_CLUB) || (bitsDamageType & DMG_GENERIC)){
		flDamage *= 0.8;
	}

	if(m_rpgms_skill3_learn == 69){//����ǿ��
		flDamage *= 0.5;
	}

	if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK2 )){
	flDamage *= 0.5;
	}

	m_flPlayerDamage += flDamage;

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	return ret;
}



void CDengor::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CDengor::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_follow_mode == 2){
	iGib = GIB_NEVER;//������ʬ!
	}

	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CDengor :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slDENGOREnemyDraw;
		}
		break;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		return &sl_DENGOR_TakeCover[ 0 ];
	}

	case SCHED_DENGOR_TAKECOVER_FAILED:
	{
		if(m_flPlayerDamage > 20){
		m_flPlayerDamage -= 20;
		}
		if ( HasConditions(bits_COND_CAN_MELEE_ATTACK2) )
		{//ǿ��������!
			return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
		}
		if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
		{//�ٻ��˹���ħ!
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		return GetScheduleOfType ( SCHED_FAIL );
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if(m_cleardally_enemy != 0){//Bug Fix 2.0 �����޸�Dengorս��תͷ����ң��������
			return slCombatFace;
		}
		else if (psched == slIdleStand)
			return slDengorFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slDengorFollow;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleHlStand;
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
Schedule_t *CDengor :: GetSchedule ( void )
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
			
			if ( HasConditions(bits_COND_CAN_MELEE_ATTACK2) )
			{//ǿ��������!
				return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
			}

			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
			{//�ٻ��˹���ħ!
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}

			// wait for one schedule to draw gun
			//if (!m_fGunDrawn ){
			//	return GetScheduleOfType( SCHED_ARM_WEAPON );
			//}

			if ( (pev->health < pev->max_health * 0.25 || m_flPlayerDamage >= 80) 
			&& HasConditions(bits_COND_NEW_COVER_MODE) && m_fightmode == 0 )
			{
				// ûѪ������ش�
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:

		if ( HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

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

MONSTERSTATE CDengor :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}

void CDengor::DeclineFollowing( void )
{
//	PlaySentence( "HM_POK", 2, VOL_NORM, ATTN_NORM );
}


//�˹���ħ

class CCrowbarMonster : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_crowbar, CCrowbarMonster );

void CCrowbarMonster :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->deadflag == DEAD_NO){
		if(pev->armorvalue > 0){
		pev->armorvalue--;
		}
		else{
		Killed( pev, GIB_NEVER );//ʱ�䵽����
		return;
		}

		if(pev->sequence == LookupActivity ( ACT_WALK )
		|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		m_flGroundSpeed = 360;
		}
	}
}
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCrowbarMonster :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

BOOL CCrowbarMonster :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 100;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 60;
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_WALK )){
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 45 )
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

void CCrowbarMonster::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time)
	{
	pev->dmgtime = gpGlobals->time;
	UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCrowbarMonster :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CCrowbarMonster :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCrowbarMonster::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCrowbarMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 60;

	switch( pEvent->event )
	{
		case 1:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 180){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
				int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

				ClearMultiDamage( );
				pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH); 

				if(!FNullEnt(pev->owner)){
				entvars_t	*pevOwner;
				pevOwner = VARS( pev->owner );
				ApplyMultiDamage( pev, pevOwner );
				}
				else{
				ApplyMultiDamage( pev, pev );
				}

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				}
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CCrowbarMonster :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/unknow_crowbar.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;

	pev->health			= 360;
	pev->armorvalue     = 180;
	m_lovehate          = 90;

	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;
	m_ignoredamage = 1;

	m_ignoreFail_MAX = 20;
	m_ignoreFail_OFF = 0;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	
	m_rpgms_inteam = 5;

	pev->netname = MAKE_STRING( "Crowbar" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCrowbarMonster :: Precache()
{
	PRECACHE_MODEL("models/unknow_crowbar.mdl");
}	
