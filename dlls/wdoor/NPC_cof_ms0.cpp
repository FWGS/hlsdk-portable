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

class CCofMs0 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	void AlertSound( void );

	int m_chain_xuli;

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void DeathSound( void );
};

LINK_ENTITY_TO_CLASS( monster_cof_ms0, CCofMs0 );

void CCofMs0 :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )){
    m_flGroundSpeed = 300;
	}
}

void CCofMs0 :: AlertSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "cof/dblsawloop.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

BOOL CCofMs0 :: CheckMeleeAttack1 ( float flDot, float flDist )
{	
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= 75 && flDot >= 0.7 && m_hEnemy != NULL )
	{
		if(m_chain_xuli <= 4){
		m_chain_xuli += 1;
		return FALSE;
		}

		return TRUE;
	}

	m_facing_fucking_mode = 1;
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs0 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CCofMs0::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs0 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs0 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs0 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 300;
}

int CCofMs0 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs0::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs0 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg	= 45;

	switch( pEvent->event )
	{
		case 1:
		{
			if ( m_hEnemy == NULL ){
			return;
			}

			TraceResult tr,tr2,tr3;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);

			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 90;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			Vector vecEnd2	= vecSrc + gpGlobals->v_right * 90;
			UTIL_TraceLine( vecSrc, vecEnd2, dont_ignore_monsters, ENT( pev ), &tr2 );
			CBaseEntity *pEntity2 = CBaseEntity::Instance(tr2.pHit);

			Vector vecEnd3	= vecSrc + gpGlobals->v_right * -90;
			UTIL_TraceLine( vecSrc, vecEnd3, dont_ignore_monsters, ENT( pev ), &tr3 );
			CBaseEntity *pEntity3 = CBaseEntity::Instance(tr3.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				m_chain_xuli = 0;
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hitbody1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			if ( tr2.flFraction < 1.0 && pEntity2->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr2, vecSrc, vecEnd2);
			int surface = (int)SURFACETYPE_Trace(&tr2, vecSrc, vecEnd2,Classify(),1);
			FX_ImpBullet( tr2.vecEndPos, tr2.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity2->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr2, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				m_chain_xuli = 0;
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hitbody1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			if ( tr3.flFraction < 1.0 && pEntity3->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr3, vecSrc, vecEnd3);
			int surface = (int)SURFACETYPE_Trace(&tr3, vecSrc, vecEnd3,Classify(),1);
			FX_ImpBullet( tr3.vecEndPos, tr3.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity3->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr3, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				m_chain_xuli = 0;
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hitbody1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}

			if ( tr.flFraction >= 1.0 && tr2.flFraction >= 1.0 && tr3.flFraction >= 1.0 ){
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, dmg, DMG_SLASH );
				if(pHurt){
					m_chain_xuli = 0;

					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 236 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 237 );
					}

					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/chainsaw_hitbody1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
void CCofMs0 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sawcrazy.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 180;
	}
	else{
	pev->health			= 150;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;
	m_headdef	   = 2;//头部硬化

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 140;
	m_rpgms_level = 42;
	pev->netname = MAKE_STRING( "Chainsaw" );

	m_singdelay_max = 0;//0反应
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs0 :: Precache()
{
	PRECACHE_MODEL("models/sawcrazy.mdl");
	PRECACHE_SOUND("weapons/chainsaw_hitbody1.wav");
	PRECACHE_SOUND("weapons/chainsaw_attack_miss.wav");
	PRECACHE_SOUND("cof/random1.wav");
	PRECACHE_SOUND("cof/dblsawloop.wav");
}	

void CCofMs0 :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/random1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5));
}
