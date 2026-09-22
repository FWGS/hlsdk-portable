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

class CCofMs6 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;
	
	int the_blood;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	void RunAI( void );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void DeathSound( void );

	static const char *pAlertSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

const char *CCofMs6::pAlertSounds[] = 
{
	"cof/flygare_idle1.wav",
	"cof/flygare_idle2.wav",
	"cof/flygare_idle3.wav",
};

LINK_ENTITY_TO_CLASS( monster_cof_ms6, CCofMs6 );

void CCofMs6 :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cof/flygare_death.wav", 1, 0.6, 0, 100);
}

//=========================================================
// RunAI
//=========================================================
void CCofMs6 :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_IDLE )){
		
		
	UTIL_MakeVectors ( pev->angles );
	if ( m_hEnemy != NULL ){

		if(the_blood == 0){
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			FX_Trail(vecGunPos, entindex(), PROJ_GUTS );
			the_blood = 33;
		}
		else{
			the_blood--;
		}

		if ( m_hEnemy->IsPlayer() ){
			if(pev->origin.z - m_hEnemy->pev->origin.z > 36){
			pev->velocity = gpGlobals->v_forward * 150 + gpGlobals->v_up * -50;
			}
			else if(pev->origin.z - m_hEnemy->pev->origin.z < -36){
			pev->velocity = gpGlobals->v_forward * 150 + gpGlobals->v_up * 50;
			}
			else{
			pev->velocity = gpGlobals->v_forward * 150;
			}
		}
		else{
			if(pev->origin.z - m_hEnemy->pev->origin.z > 72){
			pev->velocity = gpGlobals->v_forward * 150 + gpGlobals->v_up * -50;
			}
			else if(pev->origin.z - m_hEnemy->pev->origin.z < 1){
			pev->velocity = gpGlobals->v_forward * 150 + gpGlobals->v_up * 50;
			}
			else{
			pev->velocity = gpGlobals->v_forward * 150;
			}
		}
	}
    
	int dmg,radius;
	dmg	= 16;
	radius = 64;

	Vector vecGunPos,vecGunAngles;
	GetAttachment( 0, vecGunPos, vecGunAngles );
	::RadiusDamage_limit( vecGunPos, pev, pev, dmg, radius, CLASS_HUMAN_ASS, DMG_SLASH | DMG_NEVERGIB);
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs6 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CCofMs6::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs6 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs6 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs6 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int CCofMs6 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs6::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

void CCofMs6 :: PainSound( void )
{

}

void CCofMs6 :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs6 :: IdleSound( void )
{

}

void CCofMs6 :: AttackSound( void )
{
	
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs6 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{
				pev->flags |= FL_NOTARGET;
				pev->body = 1;
				pev->solid = SOLID_NOT;
				pev->movetype = MOVETYPE_TOSS;

				EMIT_SOUND(ENT(pev), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM);		
				SpawnBlood(Center(), BloodColor(), 150);

						CGib::SpawnHeadGib( pev,0,m_diefadeout );
						CGib::SpawnRandomGibs( pev, 1, 1,m_diefadeout );
						CGib::SpawnRandomGibs( pev, 1, 2,m_diefadeout );
						CGib::SpawnRandomGibs( pev, 1, 3,m_diefadeout );
						CGib::SpawnRandomGibs( pev, 1, 4,m_diefadeout );
						CGib::SpawnRandomGibs( pev, 1, 5,m_diefadeout );
		}
		break;

		case 2:
		{
			FadeMonster();
			/*
			if(pev->solid != SOLID_NOT){
			pev->flags &= ~FL_NOTARGET;
			Spawn();
			m_die = 0;
			}*/

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
void CCofMs6 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/hangingman.mdl");
	UTIL_SetSize( pev, Vector(-16,-16,0), Vector(16,16,96) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 90;
	}
	else{
	pev->health			= 70;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= -1;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	pev->body			= 0;

	MonsterInit();

	m_ignoredamage = 1;
	m_forcefuckdoor  = TRUE;
	m_killed_exp = 60;
	m_rpgms_level = 30;
	pev->netname = MAKE_STRING( "Hanging.Bed" );

	m_singdelay_max = 1;//快速反应
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs6 :: Precache()
{
	PRECACHE_MODEL("models/hangingman.mdl");

	int i;
	PRECACHE_SOUND("cof/flygare_death.wav");
	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);
}	

int CCofMs6::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}