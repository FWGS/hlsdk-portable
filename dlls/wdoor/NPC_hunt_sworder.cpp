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

class CHuntSworder : public CBaseMonster
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

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_hunt_sworder, CHuntSworder );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHuntSworder :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

void CHuntSworder :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 400;
	}

	CBaseMonster :: RunAI();
}


BOOL CHuntSworder :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 100;
	int height_attack = 0;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 96 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 96;
			height_attack = 1;
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL)
			{
				if (m_hEnemy->IsAlive()){
					if(height_attack == 1 && pev->sequence != LookupActivity ( ACT_WALK_SCARED )){
						pev->sequence = LookupActivity ( ACT_WALK_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
					}
					else if(height_attack == 0 && pev->sequence != LookupActivity ( ACT_RUN_SCARED ) && flDot >= 0.5){
						pev->sequence = LookupActivity ( ACT_RUN_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 60 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED ) 
					|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

Schedule_t* CHuntSworder :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CHuntSworder :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHuntSworder :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CHuntSworder :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CHuntSworder::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHuntSworder :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 75;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_d(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 100, dmg, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
			
		}
		break;

		case 2:
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
				pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
void CHuntSworder :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/hunt_sworder.mdl");
	UTIL_SetSize( pev, Vector(-24,-24,0), Vector(24,24,96) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 480;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;
	m_headdef	 = 2;//Í·²¿Ó²»¯

	m_lookignoremod = 1;

	pev->skin = 0;

	m_ignoredamage		= 1;

	m_ignoreFail_MAX = 20;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 140;
	m_rpgms_level = 60;
	pev->netname = MAKE_STRING( "Hunt.Sworder" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHuntSworder :: Precache()
{
	PRECACHE_MODEL("models/hunt_sworder.mdl");

	PRECACHE_SOUND ("tyant_boss/slash.wav");
}	
