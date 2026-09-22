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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"animation.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CShadow : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void RunAI( void );
	void RunTask( Task_t *pTask );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_shadow, CShadow );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CShadow :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CShadow :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
		if (pev->skin == 1)
		{
			m_flGroundSpeed = 400;
		}
		else{
			m_flGroundSpeed = 300;
		}
	}


	CBaseMonster :: RunAI();
}

void CShadow::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
		pev->dmgtime = gpGlobals->time;

		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 6, 5, 100 );//puntos
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

BOOL CShadow :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 100;
	int height_attack = 0;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 120 )
		{
				if (m_hEnemy->pev->flags & FL_ONGROUND)
				{
					if(pev->flags & FL_ONGROUND && pev->velocity.Length() <= 150){
					pev->velocity.x += RANDOM_LONG(-600,600);
					pev->velocity.y += RANDOM_LONG(-600,600);
					}
					dist += 60;
				}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive()){
					if(pev->sequence != LookupActivity ( ACT_RUN_SCARED )){
						pev->sequence = LookupActivity ( ACT_RUN_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
						if (pev->skin == 1)
						{
							pev->framerate = 1.5;
						}
					}
				}
			}
			else if (flDist >= dist + 60 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED ) ){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
						if (pev->skin == 1)
						{
							pev->framerate = 1.5;
						}
					}
			}

	return FALSE;
}

Schedule_t* CShadow :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CShadow :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CShadow :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

void CShadow :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH:
		if (pev->skin == 1)
		{
			pev->framerate = 1.5;
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}

int CShadow :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_DARK) ){
	return 0;//暗黑抗性
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CShadow::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CShadow :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 30;

	switch( pEvent->event )
	{
		case 1:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 180){
					if(( pev->origin - m_hEnemy->pev->origin).Length2D() <= 90){
					m_hEnemy->TakeDamage( pev, pev, 30, DMG_SLASH );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					m_hEnemy->pev->velocity = m_hEnemy->pev->velocity + (m_hEnemy->pev->origin - pev->origin).Normalize() * 200;
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
void CShadow :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/shadow.mdl");
	UTIL_SetSize( pev, Vector(-32,-32,0), Vector(32,32,128) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 500;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;

	pev->skin = 0;

	pev->gravity = 1.5;

	m_ignoredamage		= 1;

	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_ignoreFail_OFF = 0;
	m_ignoreFail = 1919;//暴走の模式

	m_killed_exp = 120;
	m_rpgms_level = 40;
	pev->netname = MAKE_STRING( "Generic.Shadow" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShadow :: Precache()
{
	PRECACHE_MODEL("models/shadow.mdl");
}	
