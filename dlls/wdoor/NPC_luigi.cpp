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
// headcrab.cpp - tiny, jumpy alien parasite
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include    "weapons.h"
#include	"player.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		LUYGI_AE_JUMPATTACK	( 2 )

Task_t	tlLUYGIRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slLUYGIRangeAttack1[] =
{
	{ 
		tlLUYGIRangeAttack1,
		ARRAYSIZE ( tlLUYGIRangeAttack1 ), 
		0,
		0,
		"LUYGIRangeAttack1"
	},
};

Task_t	tlLUYGIRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slLUYGIRangeAttack1Fast[] =
{
	{ 
		tlLUYGIRangeAttack1Fast,
		ARRAYSIZE ( tlLUYGIRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"LUYGIRAFast"
	},
};

class CLuigi : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	int  Classify ( void );

	void DeathSound( void );

	void RunAI( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Killed( entvars_t *pevAttacker, int iGib );

	//virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch( void ) { return 100; }
	virtual float GetSoundVolue( void ) { return 1.0; }
	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES;
};
LINK_ENTITY_TO_CLASS( monster_luigi, CLuigi );

DEFINE_CUSTOM_SCHEDULES( CLuigi )
{
	slLUYGIRangeAttack1,
	slLUYGIRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CLuigi, CBaseMonster );

enum
{
	SCHED_DOMA_REPEL = LAST_COMMON_SCHEDULE + 1,
	SCHED_DOMA_REPEL_ATTACK,
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CLuigi :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CLuigi :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

void CLuigi :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 400;
	}

	if(m_cAmmoLoaded < 60){
	m_cAmmoLoaded++;
	}

	if(m_hEnemy != NULL && m_freezetime == 0){
		if(pev->sequence == LookupActivity ( ACT_IDLE )){
			if(pev->armorvalue <= 40){
			pev->armorvalue++;
			}
		}
		else if(pev->armorvalue > 0){
			pev->armorvalue--;
		}
	}

	CBaseMonster :: RunAI();
}


void CLuigi::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CLuigi :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_MakeVectors ( pev->angles );

			Vector vecJumpDir;
			if (m_hEnemy != NULL)
			{
				float gravity = g_psv_gravity->value;
				if (gravity <= 1)
					gravity = 1;

				// How fast does the headcrab need to travel to reach that height given gravity?
				float height = (m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);
				float speed = sqrt( 2 * gravity * height );
				float time = speed / gravity;

				// Scale the sideways velocity to get there at the right time
				vecJumpDir = (m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin);
				vecJumpDir = vecJumpDir * ( 1.0 / time );

				// Speed to offset gravity at the desired height
				vecJumpDir.z = speed;

				// Don't jump too far/fast
				float distance = vecJumpDir.Length();
				
				if (distance > 900)
				{
					vecJumpDir = vecJumpDir * ( 900.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
			vecJumpDir.z = 650;
			FX_Explosion( Center(), EXPLOSION_SPARKSHOWER );//������Ծ!
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "newadd/pl_jump.wav", 1, ATTN_NORM);
			}
			else if( vecJumpDir.z <= 120 )
			{
			vecJumpDir.x = RANDOM_LONG(-600,600);
			vecJumpDir.y = RANDOM_LONG(-600,600);
			vecJumpDir.z = 150;
			}

			pev->velocity = vecJumpDir;

			m_flNextAttack = gpGlobals->time + 2;
		}
		break;

		case 2:
		{
			if(m_hEnemy != NULL){
			Vector	vecSpitDir;
			UTIL_MakeVectors ( pev->angles );
			vecSpitDir = ( m_hEnemy->Center() - Center() ).Normalize();
			pev->velocity.x = vecSpitDir.x * 500;
			pev->velocity.y = vecSpitDir.y * 500;
			}
		}
		break;

		case 3:
		{
			if(m_hEnemy != NULL){
			Vector	vecSpitDir;
			UTIL_MakeVectors ( pev->angles );
			vecSpitDir = ( m_hEnemy->Center() - Center() ).Normalize();
			pev->velocity = vecSpitDir * 1500;
			}
			m_cAmmoLoaded = 0;
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
void CLuigi :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/luigi.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 500;
	}
	
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_ignoredamage		= 1;
	pev->gravity        = 0.6;

	m_killed_exp = 100;
	m_rpgms_level = 60;
	pev->netname = MAKE_STRING( "Luigi" );

	m_ignoredamage		= 1;
	m_headdef			= 2;

	m_die_for_back = 5;//�ɸ���5��!
	m_diefadeout   = 1;//��ʧ!
	m_cAmmoLoaded = 0;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CLuigi :: Precache()
{
	PRECACHE_MODEL("models/luigi.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void CLuigi :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_MELEE_ATTACK1:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
				SetTouch( NULL );
				m_IdealActivity = ACT_IDLE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask(pTask);
		}
	}
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CLuigi :: LeapTouch ( CBaseEntity *pOther )
{
	int dmg;
	dmg	= 40;

	if(pev->sequence == LookupActivity ( ACT_RANGE_ATTACK1 )){
	dmg *= 2.5;
	}

	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}

	TraceResult tr;

	// Don't hit if back on ground
	if( pev->velocity.Length() >= 120 )
	{
		ClearMultiDamage( );

		Vector vecSrc = Center();
		Vector vecEnd = pOther->Center();
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB); 
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "newadd/fist_hitbod3.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
		ApplyMultiDamage( pev, pev );	
	}

	SetTouch( NULL );
}

void CLuigi :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &CLuigi::LeapTouch );
			break;
		}
	case TASK_MELEE_ATTACK1:
		{
			m_IdealActivity = ACT_MELEE_ATTACK1;
			SetTouch ( &CLuigi::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CLuigi :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 768 && flDot >= 0.6 )
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CLuigi :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if(FBitSet( pev->flags, FL_ONGROUND ) && pev->armorvalue >= 40){
		return TRUE;
	}

	if ( FBitSet( pev->flags, FL_ONGROUND ) && m_cAmmoLoaded >= 60 && flDist <= 1024 )
	{
		return TRUE;
	}

	return FALSE;
}

int CLuigi :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( !FBitSet( pev->flags, FL_ONGROUND ) ){
		UTIL_MakeVectors( pev->angles );
		float Dam = flDamage;
		if(Dam > 60){
		Dam = 60;
		pev->velocity = pev->velocity + gpGlobals->v_forward * -(Dam * 10);
		}
		flDamage *= 1.2;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// DeathSound 
//=========================================================
void CLuigi :: DeathSound ( void )
{
//	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "headcrab/hc_die1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CLuigi :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slLUYGIRangeAttack1[ 0 ];
		}
		break;
		case SCHED_MELEE_ATTACK1:
		{
			return &slLUYGIRangeAttack1Fast[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}