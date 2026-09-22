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
// Agrunt - Dominant, warlike alien grunt monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"hornet.h"

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_AGRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_AGRUNT_THREAT_DISPLAY,
	SCHED_AGRUNT_REPEL,
	SCHED_AGRUNT_REPEL_LAND,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_AGRUNT_SETUP_HIDE_ATTACK = LAST_COMMON_TASK + 1,
	TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE,
};

int iAgruntMuzzleFlash;
extern DLL_GLOBAL int		g_iSkillLevel;
//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		AGRUNT_AE_HORNET1	( 1 )
#define		AGRUNT_AE_HORNET2	( 2 )
#define		AGRUNT_AE_HORNET3	( 3 )
#define		AGRUNT_AE_HORNET4	( 4 )
#define		AGRUNT_AE_HORNET5	( 5 )
// some events are set up in the QC file that aren't recognized by the code yet.
#define		AGRUNT_AE_ROCKET		( 6 )
#define		AGRUNT_AE_BITE		( 7 )

#define		AGRUNT_AE_LEFT_FOOT	 ( 10 )
#define		AGRUNT_AE_RIGHT_FOOT ( 11 )

#define		AGRUNT_AE_LEFT_PUNCH ( 12 )
#define		AGRUNT_AE_RIGHT_PUNCH ( 13 )



#define		AGRUNT_MELEE_DIST	100

class CAGrunt : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	int  ISoundMask ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void SetObjectCollisionBox( void )
	{
		if ( FClassnameIs(pev, "monster_alien_grunt_big")){
		pev->absmin = pev->origin + Vector( -48, -48, 0 );
		pev->absmax = pev->origin + Vector( 48, 48, 128 );
		}
		else if ( FClassnameIs(pev, "monster_alien_xing_tian")){
		pev->absmin = pev->origin + Vector( -64, -64, 0 );
		pev->absmax = pev->origin + Vector( 64, 64, 128 );
		}
		else{
		pev->absmin = pev->origin + Vector( -32, -32, 0 );
		pev->absmax = pev->origin + Vector( 32, 32, 85 );
		}
	}

//	int IgnoreConditions ( void );

	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void RunAI( void );
	void StartTask ( Task_t *pTask );
	void AlertSound( void );
	void DeathSound ( void );
	void PainSound ( void );
	void AttackSound ( void );
	void PrescheduleThink ( void );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int IRelationship( CBaseEntity *pTarget );
	BOOL ShouldSpeak( void );
	CUSTOM_SCHEDULES;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pAttackSounds[];
	static const char *pDieSounds[];
	static const char *pPainSounds[];
	static const char *pAlertSounds[];

	BOOL	m_fCanHornetAttack;
	float	m_flNextHornetAttackCheck;

	float m_flNextPainTime;
};

LINK_ENTITY_TO_CLASS( monster_alien_grunt, CAGrunt );
LINK_ENTITY_TO_CLASS( monster_alien_grunt_big, CAGrunt );
LINK_ENTITY_TO_CLASS( monster_alien_xing_tian, CAGrunt );

TYPEDESCRIPTION	CAGrunt::m_SaveData[] = 
{
	DEFINE_FIELD( CAGrunt, m_fCanHornetAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAGrunt, m_flNextHornetAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( CAGrunt, m_flNextPainTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CAGrunt, CSquadMonster );

const char *CAGrunt::pAttackSounds[] =
{
	"agrunt/ag_attack1.wav",
	"agrunt/ag_attack2.wav",
	"agrunt/ag_attack3.wav",
};

const char *CAGrunt::pDieSounds[] =
{
	"agrunt/ag_die2.wav",
	"agrunt/ag_die4.wav",
	"agrunt/ag_die5.wav",
};

const char *CAGrunt::pPainSounds[] =
{
	"agrunt/ag_pain2.wav",
	"agrunt/ag_pain3.wav",
	"agrunt/ag_pain5.wav",
};

const char *CAGrunt::pAlertSounds[] =
{
	"agrunt/ag_alert2.wav",
	"agrunt/ag_alert3.wav",
	"agrunt/ag_alert4.wav",
};

void CAGrunt :: RunAI( void )
{
	if(pev->deadflag == DEAD_NO){
		if(m_EyeMod == 0){
			if ( m_hEnemy != NULL || pev->health < pev->max_health || m_alert > 0){
				m_EyeMod = 2;//受伤或发现敌人警戒，进入战斗状态，目力模式
			}
		}

		if (pev->sequence != LookupActivity ( ACT_RANGE_ATTACK1 )){
			if(m_cAmmoLoaded <= 238){//恢复弹药
			m_cAmmoLoaded += 2;
			}
		}
	}

	CBaseMonster :: RunAI();
}

BOOL CAGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}
//=========================================================
// IRelationship - overridden because Human Grunts are 
// Alien Grunt's nemesis.
//=========================================================
int CAGrunt::IRelationship ( CBaseEntity *pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
	{
		return R_NM;
	}

	return CSquadMonster :: IRelationship( pTarget );
}

//=========================================================
// ISoundMask 
//=========================================================
int CAGrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER;
}


int CAGrunt :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// TraceAttack
//=========================================================
void CAGrunt :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 1){
	flDamage *= 0.75;//头部超硬化，伤害只有1.5倍
	}

	if(FClassnameIs(pev, "monster_alien_xing_tian")){
		if ( ptr->iHitgroup == 0 || ptr->iHitgroup == 8)
		{
			flDamage -= 80;//防御力极高的护甲

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

	if ( ptr->iHitgroup == 10)
	{
		flDamage -= 60;//防御力很高的护甲

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

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// ShouldSpeak - Should this agrunt be talking?
//=========================================================
BOOL CAGrunt::ShouldSpeak( void )
{
	return FALSE;
}

//=========================================================
// PrescheduleThink 
//=========================================================
void CAGrunt :: PrescheduleThink ( void )
{
	
}

//=========================================================
// DieSound
//=========================================================
void CAGrunt :: DeathSound ( void )
{
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pDieSounds[RANDOM_LONG(0,ARRAYSIZE(pDieSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// AlertSound
//=========================================================
void CAGrunt :: AlertSound ( void )
{
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// AttackSound
//=========================================================
void CAGrunt :: AttackSound ( void )
{
	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// PainSound
//=========================================================
void CAGrunt :: PainSound ( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.6;

	EMIT_SOUND ( ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1)], 1.0, ATTN_NORM );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CAGrunt :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CAGrunt :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CAGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1;
	dmg1 = 40;//近身强化

	int meldist = 110;
	if ( FClassnameIs(pev, "monster_alien_grunt_big") || FClassnameIs(pev, "monster_alien_xing_tian")){
	meldist = 170;
	dmg1 = 60;
	}

	switch( pEvent->event )
	{
	case AGRUNT_AE_HORNET1:
	case AGRUNT_AE_HORNET2:
	case AGRUNT_AE_HORNET3:
	case AGRUNT_AE_HORNET4:
	case AGRUNT_AE_HORNET5:
		{
			if(m_cAmmoLoaded < 10){//弹药用尽
			m_cAmmoLoaded = 240;
			m_flNextHornetAttackCheck = gpGlobals->time + 4;//填装弹药，暂时禁用蜂巢枪
			m_fCanHornetAttack = FALSE;
			}
			else{
			m_cAmmoLoaded -= 10;
			}

			// m_vecEnemyLKP should be center of enemy body
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if (HasConditions( bits_COND_SEE_ENEMY))
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			// make angles +-180
			if (angDir.x > 180)
			{
				angDir.x = angDir.x - 360;
			}

			SetBlending( 0, angDir.x );
			GetAttachment( 0, vecArmPos, vecArmDir );

			vecArmPos = vecArmPos + vecDirToEnemy * 8;

			CBaseEntity *pHornet = CBaseEntity::Create( "hornet", vecArmPos, UTIL_VecToAngles( vecDirToEnemy ), edict() );
			UTIL_MakeVectors ( pHornet->pev->angles );
			pHornet->pev->velocity = gpGlobals->v_forward * 300;
			
			if ( FClassnameIs(pev, "monster_alien_grunt_big")){
			pHornet->pev->body = 1;
			pHornet->pev->health = 4;
			pHornet->pev->dmg = 12;
			}

			switch ( RANDOM_LONG ( 0 , 2 ) )
			{
				case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire1.wav", 1.0, ATTN_NORM, 0, 100 );	break;
				case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire2.wav", 1.0, ATTN_NORM, 0, 100 );	break;
				case 2:	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "agrunt/ag_fire3.wav", 1.0, ATTN_NORM, 0, 100 );	break;
			}

			CBaseMonster *pHornetMonster = pHornet->MyMonsterPointer();

			if ( pHornetMonster )
			{
				pHornetMonster->m_hEnemy = m_hEnemy;
			}
		}
		break;

	case 14:
		{
			Vector vecArmPos, vecArmDir;
			Vector vecDirToEnemy;
			Vector angDir;

			if (HasConditions( bits_COND_SEE_ENEMY))
			{
				vecDirToEnemy = ( ( m_vecEnemyLKP ) - pev->origin );
				angDir = UTIL_VecToAngles( vecDirToEnemy );
				vecDirToEnemy = vecDirToEnemy.Normalize();
			}
			else
			{
				angDir = pev->angles;
				UTIL_MakeAimVectors( angDir );
				vecDirToEnemy = gpGlobals->v_forward;
			}

			// make angles +-180
			if (angDir.x > 180)
			{
				angDir.x = angDir.x - 360;
			}

			SetBlending( 0, angDir.x );
		}
		break;

	case AGRUNT_AE_LEFT_FOOT:
		switch (RANDOM_LONG(0,1))
		{
		// left foot
		case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder2.wav", 1, ATTN_NORM, 0, 70 );	break;
		case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder4.wav", 1, ATTN_NORM, 0, 70 );	break;
		}
		break;
	case AGRUNT_AE_RIGHT_FOOT:
		// right foot
		switch (RANDOM_LONG(0,1))
		{
		case 0:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder1.wav", 1, ATTN_NORM, 0, 70 );	break;
		case 1:	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/pl_ladder3.wav", 1, ATTN_NORM, 0 ,70);	break;
		}
		break;

		case AGRUNT_AE_ROCKET:
		{

		}
		break;

	case AGRUNT_AE_LEFT_PUNCH:
	case AGRUNT_AE_RIGHT_PUNCH:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * meldist;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
				}
			ApplyMultiDamage( pev, pev );
					if(FClassnameIs(pev, "monster_alien_xing_tian")){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
					else{
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}	
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( meldist, dmg1, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 300;
					}
					if(FClassnameIs(pev, "monster_alien_xing_tian")){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
					else{
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
		}
		break;



	default:
		CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CAGrunt :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/agrunt.mdl");
	UTIL_SetSize(pev, Vector(-24, -24, 0), Vector(24, 24, 72));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 160;
	}
	else{
	pev->health			= 120;
	}

	m_headdef			= 2;
	//m_EyeMod			= 2;//目力模式
	m_ignoredamage		= 2;

	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= 0;
	m_afCapability		|= bits_CAP_SQUAD;

	m_HackedGunPos		= Vector( 24, 64, 48 );

	MonsterInit();

	pev->gravity		= 1.5;

	m_chase_mode = 2;
	m_chase_failed_max = 2;

	m_aimflag_dist      = 80.0;

	m_killed_exp = 80;
	m_rpgms_level = 50;
	pev->netname = MAKE_STRING( "Alien.Grunt" );

	if ( FClassnameIs(pev, "monster_alien_grunt_big")){
		SET_MODEL(ENT(pev), "models/agrunt_giant.mdl");
		UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 72 ) );
		if (g_iSkillLevel == SKILL_HARD){
		pev->health	  = 400;
		}
		else{
		pev->health	 = 300;
		}
		pev->max_health	= pev->health;
		m_killed_exp = 180;
		m_rpgms_level = 60;
		m_ignoredamage		= 1;
		pev->netname = MAKE_STRING( "Hell.Grunt" );
		SetEyePosition();
	}

	if ( FClassnameIs(pev, "monster_alien_xing_tian")){
		SET_MODEL(ENT(pev), "models/alien_xing_tian.mdl");
		UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 72 ) );
		if (g_iSkillLevel == SKILL_HARD){//20倍血量の超级肉盾!
		pev->health	 = 3200;
		}
		else{
		pev->health	 = 2400;
		}
		pev->max_health	= pev->health;
		m_killed_exp = 700;//经验值较低!
		m_rpgms_level = 70;
		m_ignoredamage = 1;
		pev->netname = MAKE_STRING( "Alien.Xing.Tian" );
		SetEyePosition();
	}

	m_cAmmoLoaded = 240;//弹药上限
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CAGrunt :: Precache()
{
	int i;

	PRECACHE_MODEL("models/agrunt.mdl");
	PRECACHE_MODEL("models/agrunt_giant.mdl");
	PRECACHE_MODEL("models/alien_xing_tian.mdl");

	for ( i = 0; i < ARRAYSIZE( pDieSounds ); i++ )
		PRECACHE_SOUND((char *)pDieSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	iAgruntMuzzleFlash = PRECACHE_MODEL( "sprites/muz4.spr" );

	UTIL_PrecacheOther( "hornet" );
}	
	
//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// repel 
//=========================================================
Task_t	tlAGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slAGruntRepel[] =
{
	{ 
		tlAGruntRepel,
		ARRAYSIZE ( tlAGruntRepel ), 
		0,
		0, 
		"Agrunt Repel"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlAGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
};

Schedule_t	slAGruntRepelLand[] =
{
	{ 
		tlAGruntRepelLand,
		ARRAYSIZE ( tlAGruntRepelLand ), 
		0,
		0, 
		"Repel Land"
	},
};


//=========================================================
// Fail Schedule
//=========================================================
Task_t	tlAGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
};

Schedule_t	slAGruntFail[] =
{
	{
		tlAGruntFail,
		ARRAYSIZE ( tlAGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Fail"
	},
};

//=========================================================
// Combat Fail Schedule
//=========================================================
Task_t	tlAGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
};

Schedule_t	slAGruntCombatFail[] =
{
	{
		tlAGruntCombatFail,
		ARRAYSIZE ( tlAGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"AGrunt Combat Fail"
	},
};

//=========================================================
// Standoff schedule. Used in combat when a monster is 
// hiding in cover or the enemy has moved out of sight. 
// Should we look around in this schedule?
//=========================================================
Task_t	tlAGruntStandoff[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)2					},
};

Schedule_t slAGruntStandoff[] = 
{
	{
		tlAGruntStandoff,
		ARRAYSIZE ( tlAGruntStandoff ),
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2		|
		bits_COND_CAN_MELEE_ATTACK1		|
		bits_COND_SEE_ENEMY				|
		bits_COND_NEW_ENEMY				|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"Agrunt Standoff"
	}
};

//=========================================================
// Suppress
//=========================================================
Task_t	tlAGruntSuppressHornet[] =
{
	{ TASK_STOP_MOVING,		(float)0		},
	{ TASK_RANGE_ATTACK1,	(float)0		},
};

Schedule_t slAGruntSuppress[] =
{
	{
		tlAGruntSuppressHornet,
		ARRAYSIZE ( tlAGruntSuppressHornet ),
		0,
		0,
		"AGrunt Suppress Hornet",
	},
};

//=========================================================
// primary range attacks
//=========================================================
Task_t	tlAGruntRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slAGruntRangeAttack1[] =
{
	{ 
		tlAGruntRangeAttack1,
		ARRAYSIZE ( tlAGruntRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE,
		
		0,
		"AGrunt Range Attack1"
	},
};


Task_t	tlAGruntHiddenRangeAttack1[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_STANDOFF },
	{ TASK_AGRUNT_SETUP_HIDE_ATTACK,	0				},
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_IDEAL,					0				},
	{ TASK_RANGE_ATTACK1_NOTURN,		(float)0		},
};

Schedule_t	slAGruntHiddenRangeAttack[] =
{
	{ 
		tlAGruntHiddenRangeAttack1,
		ARRAYSIZE ( tlAGruntHiddenRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER,
		"AGrunt Hidden Range Attack1"
	},
};

//=========================================================
// Take cover from enemy! Tries lateral cover before node 
// cover! 
//=========================================================
Task_t	tlAGruntTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
};

Schedule_t	slAGruntTakeCoverFromEnemy[] =
{
	{ 
		tlAGruntTakeCoverFromEnemy,
		ARRAYSIZE ( tlAGruntTakeCoverFromEnemy ), 
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"TakeCover"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlAGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_AGRUNT_THREAT_DISPLAY	},
	{ TASK_WAIT,							(float)0.2					},
	{ TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE,	(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_THREAT_DISPLAY	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_CROUCH			},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_STAND			},
};

Schedule_t	slAGruntVictoryDance[] =
{
	{ 
		tlAGruntVictoryDance,
		ARRAYSIZE ( tlAGruntVictoryDance ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_SEE_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE  |
		bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER,
		"AGruntVictoryDance"
	},
};

//=========================================================
//=========================================================
Task_t	tlAGruntThreatDisplay[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_THREAT_DISPLAY	},
};

Schedule_t	slAGruntThreatDisplay[] =
{
	{ 
		tlAGruntThreatDisplay,
		ARRAYSIZE ( tlAGruntThreatDisplay ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		
		bits_SOUND_PLAYER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_WORLD,
		"AGruntThreatDisplay"
	},
};

DEFINE_CUSTOM_SCHEDULES( CAGrunt )
{
	slAGruntFail,
	slAGruntCombatFail,
	slAGruntStandoff,
	slAGruntSuppress,
	slAGruntRangeAttack1,
	slAGruntHiddenRangeAttack,
	slAGruntTakeCoverFromEnemy,
	slAGruntVictoryDance,
	slAGruntThreatDisplay,
	slAGruntRepelLand,
	slAGruntRepel,
};

IMPLEMENT_CUSTOM_SCHEDULES( CAGrunt, CSquadMonster );

//=========================================================
// FCanCheckAttacks - this is overridden for alien grunts
// because they can use their smart weapons against unseen
// enemies. Base class doesn't attack anyone it can't see.
//=========================================================
BOOL CAGrunt :: FCanCheckAttacks ( void )
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

//=========================================================
// CheckMeleeAttack1 - alien grunts zap the crap out of 
// any enemy that gets too close. 
//=========================================================
BOOL CAGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	int meldist = 100;
	if ( FClassnameIs(pev, "monster_alien_grunt_big")){
		meldist = 150;
	}
	if ( FClassnameIs(pev, "monster_alien_xing_tian") ){
			meldist = 140;

			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 64 )
				{
					if (m_hEnemy->pev->flags & FL_ONGROUND)
					{
						if(pev->flags & FL_ONGROUND && pev->velocity.Length() <= 150){
						pev->velocity.x += RANDOM_LONG(-600,600);
						pev->velocity.y += RANDOM_LONG(-600,600);
						}
					}
				}
			}
	}

	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= meldist && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 
//
// !!!LATER - we may want to load balance this. Several
// tracelines are done, so we may not want to do this every
// server frame. Definitely not while firing. 
//=========================================================
BOOL CAGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FClassnameIs(pev, "monster_alien_xing_tian") ){
		return FALSE;
	}

	if ( gpGlobals->time < m_flNextHornetAttackCheck )
	{
		return m_fCanHornetAttack;
	}

	float dist = 1024;

	if ( FClassnameIs(pev, "monster_alien_grunt_big")){
	dist = 1536;//射程增加50%
	}

	if(m_hEnemy != NULL){
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 192 ){//与敌人高度差较高时，增加攻击距离
			dist += 256;
			}
	}

	if ( m_cleardally_enemy > 0 && flDist >= AGRUNT_MELEE_DIST && flDist <= dist && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		UTIL_MakeVectors( pev->angles );
		GetAttachment( 0, vecArmPos, vecArmDir );
//		UTIL_TraceLine( vecArmPos, vecArmPos + gpGlobals->v_forward * 256, ignore_monsters, ENT(pev), &tr);
		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget(vecArmPos), dont_ignore_monsters, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextHornetAttackCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );
			m_fCanHornetAttack = TRUE;
			return m_fCanHornetAttack;
		}
	}
	
	m_flNextHornetAttackCheck = gpGlobals->time + 0.2;// don't check for half second if this check wasn't successful
	m_fCanHornetAttack = FALSE;
	return m_fCanHornetAttack;
}

//=========================================================
// StartTask
//=========================================================
void CAGrunt :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_AGRUNT_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			if ( BuildRoute ( m_vecEnemyLKP - gpGlobals->v_forward * 50, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT ( at_aiconsole, "AGruntGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;

	case TASK_AGRUNT_SETUP_HIDE_ATTACK:
		// alien grunt shoots hornets back out into the open from a concealed location. 
		// try to find a spot to throw that gives the smart weapon a good chance of finding the enemy.
		// ideally, this spot is along a line that is perpendicular to a line drawn from the agrunt to the enemy.

		CBaseMonster	*pEnemyMonsterPtr;

		pEnemyMonsterPtr = m_hEnemy->MyMonsterPointer();

		if ( pEnemyMonsterPtr )
		{
			Vector		vecCenter;
			TraceResult	tr;
			BOOL		fSkip;

			fSkip = FALSE;
			vecCenter = Center();

			UTIL_VecToAngles( m_vecEnemyLKP - pev->origin );

			UTIL_TraceLine( Center() + gpGlobals->v_forward * 128, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
			if ( tr.flFraction == 1.0 )
			{
				MakeIdealYaw ( pev->origin + gpGlobals->v_right * 128 );
				fSkip = TRUE;
				TaskComplete();
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 128, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin - gpGlobals->v_right * 128 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() + gpGlobals->v_forward * 256, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin + gpGlobals->v_right * 256 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				UTIL_TraceLine( Center() - gpGlobals->v_forward * 256, m_vecEnemyLKP, ignore_monsters, ENT(pev), &tr);
				if ( tr.flFraction == 1.0 )
				{
					MakeIdealYaw ( pev->origin - gpGlobals->v_right * 256 );
					fSkip = TRUE;
					TaskComplete();
				}
			}
			
			if ( !fSkip )
			{
				TaskFail();
			}
		}
		else
		{
			ALERT ( at_aiconsole, "AGRunt - no enemy monster ptr!!!\n" );
			TaskFail();
		}
		break;

	default:
		CSquadMonster :: StartTask ( pTask );
		break;
	}
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CAGrunt :: GetSchedule ( void )
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_AGRUNT_REPEL_LAND );
	}

	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
		{
			// dangerous sound nearby!
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType( SCHED_WAKE_ANGRY );
			}

	// zap player!
			if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				AttackSound();// this is a total hack. Should be parto f the schedule
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) && pev->health <= pev->max_health * 0.6 )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}

			if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot ( bits_SLOTS_AGRUNT_HORNET ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( OccupySlot ( bits_SLOT_AGRUNT_CHASE ) )
			{
				return GetScheduleOfType ( SCHED_CHASE_ENEMY );
			}

			return GetScheduleOfType ( SCHED_STANDOFF );
		}
	}

	return CSquadMonster :: GetSchedule();
}

//=========================================================
//=========================================================
Schedule_t* CAGrunt :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return &slAGruntTakeCoverFromEnemy[ 0 ];
		break;
	
	case SCHED_RANGE_ATTACK1:
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			//normal attack
			return &slAGruntRangeAttack1[ 0 ];
		}
		else
		{
			// attack an unseen enemy
			// return &slAGruntHiddenRangeAttack[ 0 ];
			return &slAGruntRangeAttack1[ 0 ];
		}
		break;

	case SCHED_AGRUNT_THREAT_DISPLAY:
		return &slAGruntThreatDisplay[ 0 ];
		break;

	case SCHED_AGRUNT_SUPPRESS:
		return &slAGruntSuppress[ 0 ];
		break;

	case SCHED_VICTORY_DANCE:
		return &slAGruntVictoryDance[ 0 ];
		break;

	case SCHED_AGRUNT_REPEL:
		{
			return &slAGruntRepel[ 0 ];
		}

	case SCHED_AGRUNT_REPEL_LAND:
		{
			return &slAGruntRepelLand[ 0 ];
		}

	case SCHED_STANDOFF:
		return &slAGruntStandoff[ 0 ];
		break;

	case SCHED_FAIL:
		// no fail schedule specified, so pick a good generic one.
		{
			if ( m_hEnemy != NULL )
			{
				// I have an enemy
				// !!!LATER - what if this enemy is really far away and i'm chasing him?
				// this schedule will make me stop, face his last known position for 2 
				// seconds, and then try to move again
				return &slAGruntCombatFail[ 0 ];
			}

			return &slAGruntFail[ 0 ];
		}
		break;

	}

	return CSquadMonster :: GetScheduleOfType( Type );
}


//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadAGrunt : public CBaseMonster
{
public:
	void Spawn( void );
	int	Classify ( void ) { return	CLASS_ALIEN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadAGrunt::m_szPoses[] = { "dead_1", "dead_2" };

void CDeadAGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_agrunt_dead, CDeadAGrunt );

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadAGrunt :: Spawn( void )
{
	PRECACHE_MODEL("models/agrunt.mdl");
	SET_MODEL(ENT(pev), "models/agrunt.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_YELLOW;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hgrunt with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 30;

	MonsterInitDead();
}
