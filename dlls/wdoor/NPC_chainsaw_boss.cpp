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

class CChainSawBOSS : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void Killed( entvars_t *pevAttacker, int iGib );

	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	void AlertSound( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void DeathSound( void );
};

LINK_ENTITY_TO_CLASS( monster_chainsaw_boss, CChainSawBOSS );

const char *CChainSawBOSS::pAttackSounds[] = 
{
	"cof/sawrunner_attack1.wav",
	"cof/sawrunner_attack2.wav",
};

const char *CChainSawBOSS::pAlertSounds[] = 
{
	"cof/sawrunner_alert10.wav",
	"cof/sawrunner_alert20.wav",
	"cof/sawrunner_alert30.wav",
};


void CChainSawBOSS :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


void CChainSawBOSS :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )){
    m_flGroundSpeed = 500;
	}
}

void CChainSawBOSS :: AlertSound( void )
{
	int pitch = 100;

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

BOOL CChainSawBOSS :: CheckMeleeAttack1 ( float flDot, float flDist )
{	
	float dist = 75;
	if(m_hEnemy != NULL){
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
			{
				if (m_hEnemy->pev->flags & FL_ONGROUND)
				{
					if( (pev->flags & FL_ONGROUND) && pev->velocity.Length() <= 150){
					pev->velocity.x += RANDOM_LONG(-600,600);
					pev->velocity.y += RANDOM_LONG(-600,600);
					}
					dist += 45;
				}
			}
	}

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= dist && flDot >= 0.5 && m_hEnemy != NULL )
	{
		return TRUE;
	}

	m_facing_fucking_mode = 1;
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CChainSawBOSS :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CChainSawBOSS::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CChainSawBOSS :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CChainSawBOSS :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChainSawBOSS :: SetYawSpeed ( void )
{
	pev->yaw_speed = 500;
}

int CChainSawBOSS :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( (bitsDamageType & DMG_MORTAR) || (bitsDamageType & DMG_VALVE_SWORD)
	|| (bitsDamageType & DMG_ENERGYBLAST) || (bitsDamageType & DMG_BLAST) ){
		flDamage *= 1.25;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CChainSawBOSS::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChainSawBOSS :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg			= 450;//5把电锯の极高伤害！

	switch( pEvent->event )
	{
		case 1:
		{
			AttackSound();

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);

			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 100;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/chainsaw_attack_hit.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 100, dmg, DMG_SLASH );
				if(pHurt){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/chainsaw_attack_hit.wav", 1.0, ATTN_NORM, 0, 100);
				}
				else if(m_hEnemy != NULL){
					if(( Center() - m_hEnemy->Center()).Length() <= 100){
					m_hEnemy->TakeDamage( pev, pev, dmg, DMG_SLASH );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/chainsaw_attack_hit.wav", 1.0, ATTN_NORM, 0, 100);
					}
					else{
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "cof/chainsaw_attack_miss.wav", 1.0, ATTN_NORM, 0, 100);
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
void CChainSawBOSS :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/darksaw_boss.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 96 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 2400;
	}
	else{
	pev->health			= 2000;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;

	pev->gravity   = 1.5;

	m_candrownwater = 1;

	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 1200;
	m_rpgms_level = 70;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "ChainSawMan" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChainSawBOSS :: Precache()
{
	PRECACHE_MODEL("models/darksaw_boss.mdl");
	PRECACHE_SOUND("cof/chainsaw_attack_hit.wav");
	PRECACHE_SOUND("cof/chainsaw_attack_miss.wav");
	PRECACHE_SOUND("cof/mace_scream.wav");

	int i;
	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
	PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
	PRECACHE_SOUND((char *)pAlertSounds[i]);
}	

void CChainSawBOSS :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/mace_scream.wav", 1, ATTN_NORM, 0, 100);
}
