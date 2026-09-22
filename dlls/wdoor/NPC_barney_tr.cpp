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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define		BARNEY_AE_RELOAD	( 5 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE			2

class CBarney_Tr : public CTalkMonster
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
	
	void RunAI( void );
	void CheckAmmo ( void );

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

LINK_ENTITY_TO_CLASS( monster_barney_tr1, CBarney_Tr );
LINK_ENTITY_TO_CLASS( monster_barney_tr2, CBarney_Tr );
LINK_ENTITY_TO_CLASS( monster_barney_tr3, CBarney_Tr );
LINK_ENTITY_TO_CLASS( monster_barney_legless, CBarney_Tr );

TYPEDESCRIPTION	CBarney_Tr::m_SaveData[] = 
{
	DEFINE_FIELD( CBarney_Tr, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney_Tr, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney_Tr, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBarney_Tr, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBarney_Tr, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBarney_Tr, CTalkMonster );


enum
{
	SCHED_BARNEY_COVER_AND_RELOAD = LAST_COMMON_SCHEDULE + 1,
	SCHED_BARNEY_TAKECOVER_FAILED,
};


//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBa_Tr_Follow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBa_Tr_Follow[] =
{
	{
		tlBa_Tr_Follow,
		ARRAYSIZE ( tlBa_Tr_Follow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBa_Tr_rneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slBa_Tr_rneyEnemyDraw[] = 
{
	{
		tlBa_Tr_rneyEnemyDraw,
		ARRAYSIZE ( tlBa_Tr_rneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlBa_Tr_FaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBa_Tr_FaceTarget[] =
{
	{
		tlBa_Tr_FaceTarget,
		ARRAYSIZE ( tlBa_Tr_FaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tl_Barney_Tr_TakeCover1[] =
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

Schedule_t	sl_Barney_Tr_TakeCover[] =
{
	{ 
		tl_Barney_Tr_TakeCover1,
		ARRAYSIZE ( tl_Barney_Tr_TakeCover1 ), 
		0,
		0,
		"TakeCover"
	},
};

Task_t	tlIdleBa_Tr_Stand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBa_Tr_Stand[] =
{
	{ 
		tlIdleBa_Tr_Stand,
		ARRAYSIZE ( tlIdleBa_Tr_Stand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
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

Task_t	tl_Barney_TR_HideReload[] =
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

Schedule_t sl_Barney_TR_HideReload[] = 
{
	{
		tl_Barney_TR_HideReload,
		ARRAYSIZE ( tl_Barney_TR_HideReload ),
		0,
		0,
		"TakeCover"
	}
};

DEFINE_CUSTOM_SCHEDULES( CBarney_Tr )
{
	slBa_Tr_Follow,
	slBa_Tr_rneyEnemyDraw,
	slBa_Tr_FaceTarget,
	slIdleBa_Tr_Stand,
	sl_Barney_Tr_TakeCover,
};


IMPLEMENT_CUSTOM_SCHEDULES( CBarney_Tr, CTalkMonster );

void CBarney_Tr :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void CBarney_Tr :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
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


void CBarney_Tr :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 80;
	}
	if (pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 270;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney_Tr :: ISoundMask ( void) 
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
int	CBarney_Tr :: Classify ( void )
{
	return	CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney_Tr :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
	//		PlaySentence( "BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney_Tr :: SetYawSpeed ( void )
{
	if ( !FClassnameIs ( pev, "monster_barney_legless" ) ){
	pev->yaw_speed = 180;
	}
	else{
	pev->yaw_speed = 0;
	}
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBarney_Tr :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 1024;

	if ( !FClassnameIs ( pev, "monster_barney_tr1" ) ){
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
		m_aimenemy_mod = 8;//����
		}
		else if(m_aimenemy_mod != 5){//Զ����
		m_aimenemy_mod = 5;//��
		}
	}

	if ( flDist <= dist && flDot >= 0.5 )
	{
		if ( FClassnameIs ( pev, "monster_barney_tr1" ) ){
			if ( gpGlobals->time > m_checkAttackTime )
			{
				TraceResult tr;
				
				Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
				CBaseEntity *pEnemy = m_hEnemy;
				Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
				UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
				m_checkAttackTime = gpGlobals->time + 1;
				if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
					m_lastAttackCheck = TRUE;
				else
					m_lastAttackCheck = FALSE;
				m_checkAttackTime = gpGlobals->time + 1.5;
			}
			return m_lastAttackCheck;
		}

			if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
				//���˼��
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
					Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
					UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
					if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) ){
						m_checkAttackTime = gpGlobals->time + 0.25;
						m_lastAttackCheck = TRUE;
					}
					else{
							CBaseEntity *pHitEntity = CBaseEntity::Instance(tr.pHit);
							if(pHitEntity){
								if(pHitEntity->Classify() == CLASS_ALIEN_MONSTER
								|| pHitEntity->Classify() == CLASS_ALIEN_MILITARY
								|| pHitEntity->Classify() == CLASS_MACHINE
								|| pHitEntity->Classify() == CLASS_MACHINE_ASS
								|| pHitEntity->Classify() == CLASS_HUMAN_ASS
								|| pHitEntity->Classify() == CLASS_HUMAN_MILITARY){
								m_checkAttackTime = gpGlobals->time + 0.25;
								m_lastAttackCheck = TRUE;
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

	}
	return FALSE;
}


//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney_Tr :: BarneyFirePistol ( void )
{
	if ( m_cAmmoLoaded <= 0 ){
	return;
	}

	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects |= EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_12MM,1 );
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

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
void CBarney_Tr :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;

	case BARNEY_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
		SetBodygroup( 2, 1 );
		}
		else{
		pev->body = BARNEY_BODY_GUNDRAWN;
		}
		m_fGunDrawn = TRUE;
		break;

	case BARNEY_AE_RELOAD:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "barney/reload2.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

	case BARNEY_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
		SetBodygroup( 2, 0 );
		}
		else{
		pev->body = BARNEY_BODY_GUNHOLSTERED;
		}
		m_fGunDrawn = FALSE;
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBarney_Tr :: Spawn()
{
	Precache( );

	
	if ( FClassnameIs ( pev, "monster_barney_tr1" ) ){
	m_aimenemy_mod = 6;
	m_lovehate = 810;
	SET_MODEL(ENT(pev), "models/barney_tr1.mdl");
	pev->netname = MAKE_STRING( "Rookie.Guard" );
	m_no_cover_mode = 1;//�޷��ӱ�
	pev->health	= 80;
	}
	else if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
	m_aimenemy_mod = 4;
	m_lovehate = 810;
	SET_MODEL(ENT(pev), "models/barney.mdl");
	pev->netname = MAKE_STRING( "Guard" );
	m_no_cover_mode = 2;//�޷�����λ���ӱ�
	pev->health	= 100;
	}
	else{
	SET_MODEL(ENT(pev), "models/barney_tr3.mdl");
	pev->netname = MAKE_STRING( "Blk.Guard" );
	pev->health			= 120;
	}

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	pev->body			= 0; // gun in holster
	m_fGunDrawn			= FALSE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( &CBarney_Tr::FollowerUse );

	if ( FClassnameIs ( pev, "monster_barney_tr3" ) ){
	m_lovehate = 100;
	m_longming = 1;
	pev->takedamage = DAMAGE_NO;
	m_godmode = TRUE;
	}

	if ( FClassnameIs ( pev, "monster_barney_legless" ) ){//���Ȥΰ���
	SET_MODEL(ENT(pev), "models/barney_tr2.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	pev->netname = MAKE_STRING( "Legless.Guard" );
	m_rpgms_type = 0;
	m_selfmode = TRUE;
	pev->health	 = 1;
	pev->max_health	 = 100;
	pev->gravity = 1.6;
	m_lovehate = 810;
	m_killed_exp = 100;
	}

	m_cClipSize	    	= 15;
	m_cAmmoLoaded		= m_cClipSize;

	m_cover_dist = 128;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney_Tr :: Precache()
{
	PRECACHE_MODEL("models/barney.mdl");
	PRECACHE_MODEL("models/barney_tr1.mdl");
	PRECACHE_MODEL("models/barney_tr2.mdl");
	PRECACHE_MODEL("models/barney_tr3.mdl");

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
void CBarney_Tr :: TalkInit()
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


int CBarney_Tr :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) && pev->takedamage && !m_godmode )
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

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "BA_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
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
void CBarney_Tr :: PainSound ( void )
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
void CBarney_Tr :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CBarney_Tr::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void CBarney_Tr::Killed( entvars_t *pevAttacker, int iGib )
{
	/*
	if ( pev->body < BARNEY_BODY_GUNGONE )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BARNEY_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );
		
		CBaseEntity *pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );

		if ( pGun )
		{
			pGun->pev->velocity = pev->velocity + Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}
	*/

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CBarney_Tr :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBa_Tr_rneyEnemyDraw;
		}
		break;

	case SCHED_BARNEY_COVER_AND_RELOAD:
	{
		return &sl_Barney_TR_HideReload[ 0 ];
	}

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBa_Tr_FaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBa_Tr_Follow;

	case SCHED_TAKE_COVER_FROM_ENEMY:
	{
		if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
		return &sl_Barney_Tr_TakeCover[ 0 ];
		}
	}

	case SCHED_BARNEY_TAKECOVER_FAILED:
	{
		if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}
		}
		return GetScheduleOfType ( SCHED_FAIL );
	}

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBa_Tr_Stand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}


void CBarney_Tr :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBarney_Tr :: GetSchedule ( void )
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

			if ( FClassnameIs ( pev, "monster_barney_tr2" ) ){
				if ( HasConditions(bits_COND_NEW_COVER_MODE))
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo. 
				// He's going to try to find cover to run to and reload, but rarely, if 
				// none is available, he'll drop and reload in the open here. 
				return GetScheduleOfType( SCHED_BARNEY_COVER_AND_RELOAD );
			}

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
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
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CBarney_Tr :: GetIdealState ( void )
{
	return CTalkMonster::GetIdealState();
}



void CBarney_Tr::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}