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

class CNvDeath : public CBaseMonster
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

	void PainSound( void );
	void AlertSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_nirvana_death, CNvDeath );

const char *CNvDeath::pAttackSounds[] = 
{
	"cof/zo_attack1.wav",
	"cof/zo_attack2.wav",
};

const char *CNvDeath::pAlertSounds[] = 
{
	"cof/zo_alert10.wav",
	"cof/zo_alert20.wav",
	"cof/zo_alert30.wav",
};

const char *CNvDeath::pPainSounds[] = 
{
	"cof/zo_pain1.wav",
	"cof/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CNvDeath :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CNvDeath :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 400;
	}

	if(pev->skin == 0){
		if(pev->health <= pev->max_health * 0.5){//²ÐÑª±äÁ³
		pev->skin = 1;
		}
	}

	CBaseMonster :: RunAI();
}


BOOL CNvDeath :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 100;
	int height_attack = 0;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 96 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 80;
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

Schedule_t* CNvDeath :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CNvDeath :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CNvDeath :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CNvDeath :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_DARK) ){
	flDamage *= 0.8;//°µºÚ¿¹ÐÔ
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CNvDeath::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CNvDeath :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CNvDeath :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CNvDeath :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CNvDeath :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 70;

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

			AttackSound();
			
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

				AttackSound();
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
void CNvDeath :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/death.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, Vector(16,16,96) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 480;
	}
	else{
	pev->health			= 400;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;

	pev->skin = 0;

	m_ignoredamage		= 2;

	m_ignoreFail_MAX = 20;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 120;
	m_rpgms_level = 50;
	pev->netname = MAKE_STRING( "Nirvana.Death" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNvDeath :: Precache()
{
	PRECACHE_MODEL("models/death.mdl");

	PRECACHE_SOUND ("tyant_boss/slash.wav");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	
