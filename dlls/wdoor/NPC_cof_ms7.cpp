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

class CCofMs7 : public CBaseMonster
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

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

const char *CCofMs7::pAttackSounds[] = 
{
	"cof/slower_attack1.wav",
	"cof/slower_attack2.wav",
};

const char *CCofMs7::pAlertSounds[] = 
{
	"cof/slower_alert10.wav",
	"cof/slower_alert20.wav",
	"cof/slower_alert30.wav",
};

const char *CCofMs7::pPainSounds[] = 
{
	"cof/slower_pain1.wav",
	"cof/slower_pain2.wav",
};

LINK_ENTITY_TO_CLASS( monster_cof_ms7, CCofMs7 );

void CCofMs7 :: RunAI( void )
{
	CBaseMonster :: RunAI();
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
    m_flGroundSpeed = 150;
	}
}

BOOL CCofMs7 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
			if (flDist <= 180 && m_hEnemy != NULL)
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
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs7 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CCofMs7::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs7 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs7 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs7 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CCofMs7 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs7::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CCofMs7 :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs7 :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs7 :: IdleSound( void )
{

}

void CCofMs7 :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs7 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 10;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 80;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/hammer_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) ); break;

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg, DMG_SLASH );
				if(pHurt){
						if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
						|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
						|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
						FX_Explosion( pHurt->Center(), 234 );
						}
						else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
						FX_Explosion( pHurt->Center(), 235 );
						}

	
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/hammer_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) ); break;

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
void CCofMs7 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/slowerno.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 70;
	}
	else{
	pev->health			= 60;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 30;
	m_rpgms_level = 30;
	pev->netname = MAKE_STRING( "Slowerno" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs7 :: Precache()
{
	PRECACHE_MODEL("models/slowerno.mdl");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CCofMs7::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}