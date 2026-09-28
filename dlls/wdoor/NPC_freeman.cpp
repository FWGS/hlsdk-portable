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

class CFreeman : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunAI( void );

	void DeathSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_freeman, CFreeman );

void CFreeman :: RunAI( void )
{
	if (pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 320;
	}

	CBaseMonster :: RunAI();
}

void CFreeman :: DeathSound ( void )
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/flatline.wav", 1.0, 0.7, 0, 100 );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CFreeman :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}

BOOL CFreeman :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// flying?
	if ( pev->movetype == MOVETYPE_TOSS)
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
		}
	}

	float dist = 96;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
				dist += 64;
				if( (pev->flags & FL_ONGROUND) && pev->velocity.Length() <= 150){
				pev->velocity.x += RANDOM_LONG(-600,600);
				pev->velocity.y += RANDOM_LONG(-600,600);
				}
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_RUN )){
					pev->sequence = LookupActivity ( ACT_RUN_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 64 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

Schedule_t* CFreeman :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CFreeman :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CFreeman :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

int CFreeman :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CFreeman::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFreeman :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 25;

	switch( pEvent->event )
	{
		case 1://瞄准
		{
			if(m_hEnemy != NULL){
				UTIL_MakeVectors(pev->angles);
				Vector vecShootOrigin = pev->origin + Vector(0,0,60);
				Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
				Vector angDir = UTIL_VecToAngles( vecShootDir );
				SetBlending( 0, angDir.x );
			}
			else{
				ClearSchedule();
				SetYawSpeed();
			}
		}
		break;

		case 2:
		{
			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					if(( tr.vecEndPos - vecSrc).Length() <= 64){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
					ApplyMultiDamage( pev, pev );

					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/cbar_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
void CFreeman :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/freeman.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 500;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;
	m_headdef	 = 2;//头部硬化

	m_ignoreFail_MAX = 1919;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 100;
	m_rpgms_level = 60;
	pev->netname = MAKE_STRING( "Freeman" );

	m_ignoredamage		= 1;

	m_die_for_back = 5;//可复活5次!
	m_diefadeout   = 1;//消失!
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFreeman :: Precache()
{
	PRECACHE_MODEL("models/freeman.mdl");
	PRECACHE_SOUND("items/flatline.wav");
}	
