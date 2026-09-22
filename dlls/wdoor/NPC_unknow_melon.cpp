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
#define		UNML_AE_JUMPATTACK	( 2 )

Task_t	tlUNMLRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slUNMLRangeAttack1[] =
{
	{ 
		tlUNMLRangeAttack1,
		ARRAYSIZE ( tlUNMLRangeAttack1 ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"UNMLRangeAttack1"
	},
};

Task_t	tlUNMLRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slUNMLRangeAttack1Fast[] =
{
	{ 
		tlUNMLRangeAttack1Fast,
		ARRAYSIZE ( tlUNMLRangeAttack1Fast ), 
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"UNMLRAFast"
	},
};

class CUnknowMelon : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void RunTask ( Task_t *pTask );
	void StartTask ( Task_t *pTask );
	void SetYawSpeed ( void );
	void EXPORT LeapTouch ( CBaseEntity *pOther );
	Vector BodyTarget( const Vector &posSrc );
	int  Classify ( void );

	void DeathSound( void );

	void RunAI( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void Killed( entvars_t *pevAttacker, int iGib );

	//virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite; }
	virtual int GetVoicePitch( void ) { return 100; }
	virtual float GetSoundVolue( void ) { return 1.0; }
	Schedule_t* GetScheduleOfType ( int Type );

	CUSTOM_SCHEDULES;
};
LINK_ENTITY_TO_CLASS( monster_unknow_melon, CUnknowMelon );

DEFINE_CUSTOM_SCHEDULES( CUnknowMelon )
{
	slUNMLRangeAttack1,
	slUNMLRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( CUnknowMelon, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CUnknowMelon :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

Vector CUnknowMelon :: BodyTarget( const Vector &posSrc ) 
{ 
	return Center( );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CUnknowMelon :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

void CUnknowMelon :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 360;
	}

	CBaseMonster :: RunAI();
}


void CUnknowMelon::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CUnknowMelon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 2:
		{
			ClearBits( pev->flags, FL_ONGROUND );

			//UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
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
				
				if (distance > 800)
				{
					vecJumpDir = vecJumpDir * ( 800.0 / distance );
				}
			}
			else
			{
				// jump hop, don't care where
				vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;
			}

			pev->velocity = vecJumpDir;
			m_flNextAttack = gpGlobals->time + 2;
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
void CUnknowMelon :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/unknow_melon.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 150;
	}
	else{
	pev->health			= 120;
	}
	
	pev->view_ofs		= Vector ( 0, 0, 20 );// position of the eyes relative to monster's origin.
	pev->yaw_speed		= 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_ignoredamage		= 1;

	m_killed_exp = 30;
	m_rpgms_level = 30;
	pev->netname = MAKE_STRING( "Face.Melon" );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CUnknowMelon :: Precache()
{
	PRECACHE_SOUND("headcrab/hc_attack1.wav");
	PRECACHE_SOUND("headcrab/hc_die1.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");

	PRECACHE_MODEL("models/unknow_melon.mdl");
}	


//=========================================================
// RunTask 
//=========================================================
void CUnknowMelon :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
	case TASK_RANGE_ATTACK2:
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
void CUnknowMelon :: LeapTouch ( CBaseEntity *pOther )
{
	int dmg;
	dmg			= 20;

	if ( !pOther->pev->takedamage || pev->deadflag != DEAD_NO){
		return;
	}

	if ( pOther->Classify() == Classify() ){
		return;
	}

	TraceResult tr;

	// Don't hit if back on ground
	if ( !FBitSet( pev->flags, FL_ONGROUND ) )
	{
		ClearMultiDamage( );

		Vector vecDir = pOther->pev->origin - pev->origin;
		vecDir = vecDir.Normalize( );
		Vector monsterangles = UTIL_VecToAngles( vecDir );
		monsterangles.x = 0;
		monsterangles.z = 0;
		UTIL_MakeVectors(monsterangles);

		Vector vecSrc = Center();
		Vector vecEnd	= vecSrc + gpGlobals->v_forward * 48;
		UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "headcrab/hc_headbite.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
		pOther->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
		ApplyMultiDamage( pev, pev );
	}
	SetTouch( NULL );
}

void CUnknowMelon :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		{
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "headcrab/hc_attack1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
			m_IdealActivity = ACT_RANGE_ATTACK1;
			SetTouch ( &CUnknowMelon::LeapTouch );
			break;
		}
	default:
		{
			CBaseMonster :: StartTask( pTask );
		}
	}
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CUnknowMelon :: CheckRangeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CUnknowMelon :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( FBitSet( pev->flags, FL_ONGROUND ) && flDist <= 384 && flDot >= 0.6 )
	{
		return TRUE;
	}

	return FALSE;
}

int CUnknowMelon :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// DeathSound 
//=========================================================
void CUnknowMelon :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "headcrab/hc_die1.wav", GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch() );
}

Schedule_t* CUnknowMelon :: GetScheduleOfType ( int Type )
{
	switch	( Type )
	{
		case SCHED_RANGE_ATTACK1:
		{
			return &slUNMLRangeAttack1[ 0 ];
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}