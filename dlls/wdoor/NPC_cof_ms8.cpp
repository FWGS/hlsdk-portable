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

class CCofMs8 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void DeathSound( void );
};

LINK_ENTITY_TO_CLASS( monster_cof_ms8, CCofMs8 );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs8 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CCofMs8 :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
    m_flGroundSpeed = 90;
	}
}

BOOL CCofMs8 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
			if (flDist <= 120 && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_WALK )){
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED ) && (pev->frame <= 10 || pev->frame >= 170) ){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}


void CCofMs8 :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/mace_scream.wav", 1, 0.6, 0, 100);
}

void CCofMs8::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == 10)
	{
		if ( pev->dmgtime != gpGlobals->time)
		{
		pev->dmgtime = gpGlobals->time;
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		}
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs8 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs8 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs8 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

int CCofMs8 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs8::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CCofMs8 :: PainSound( void )
{

}

void CCofMs8 :: AlertSound( void )
{

}

void CCofMs8 :: IdleSound( void )
{

}

void CCofMs8 :: AttackSound( void )
{

}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs8 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg			= 100;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;

			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_o(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;

			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH );
				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
				pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 300;
				}

				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/mace_hitflesh.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 100, dmg, DMG_SLASH );
				if(pHurt){
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
						|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
						|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
						FX_Explosion( pHurt->Center(), 236 );
						}
						else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
						FX_Explosion( pHurt->Center(), 237 );
						}
						pHurt->pev->velocity = pHurt->pev->velocity + (pHurt->pev->origin - pev->origin).Normalize() * 300;
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/mace_hitflesh.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			if (RANDOM_LONG(0,1) )
			AttackSound();
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
void CCofMs8 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sewer_boss.mdl");
	UTIL_SetSize(pev, Vector(-25, -25, 0), Vector(25, 25, 100));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 420;
	}
	else{
	pev->health			= 360;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	pev->gravity		= 2.0;

	MonsterInit();

	m_ignoredamage = 1;
	m_headdef	   = 2;//Í·²¿Ó²»¯

	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 100;
	m_rpgms_level = 40;
	pev->netname = MAKE_STRING( "Mace" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs8 :: Precache()
{
	PRECACHE_MODEL("models/sewer_boss.mdl");
	PRECACHE_SOUND("cof/mace_hitflesh.wav");
	PRECACHE_SOUND("cof/mace_scream.wav");
}	

int CCofMs8::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	if ( m_hEnemy != NULL ){
	iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}
	return iIgnore;
}