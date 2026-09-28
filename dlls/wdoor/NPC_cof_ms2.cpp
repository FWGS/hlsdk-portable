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

Task_t	tlGhost_wakeup[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_STAND },
};

Schedule_t slGhost_wakeup[] = 
{
	{
		tlGhost_wakeup,
		ARRAYSIZE ( tlGhost_wakeup ),
		0,
		0,
		"Ghost Wake Up"
	}
};


class CCofMs2 : public CBaseMonster
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

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
};

const char *CCofMs2::pAttackSounds[] = 
{
	"cof/zo_attack1.wav",
	"cof/zo_attack2.wav",
};

const char *CCofMs2::pAlertSounds[] = 
{
	"cof/zo_alert10.wav",
	"cof/zo_alert20.wav",
	"cof/zo_alert30.wav",
};

const char *CCofMs2::pPainSounds[] = 
{
	"cof/zo_pain1.wav",
	"cof/zo_pain2.wav",
};

LINK_ENTITY_TO_CLASS( monster_cof_ms2, CCofMs2 );

TYPEDESCRIPTION	CCofMs2::m_SaveData[] = 
{
	DEFINE_FIELD( CCofMs2, m_fGunDrawn, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CCofMs2, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CCofMs2 )
{
	slGhost_wakeup,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCofMs2, CBaseMonster );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs2 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CCofMs2::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs2 :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slGhost_wakeup;
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CCofMs2 :: GetSchedule ( void )
{
	if (!m_fGunDrawn && (m_hEnemy != NULL || pev->health <= 0 || m_allydeadcheck == 1) ){
		return GetScheduleOfType( SCHED_ARM_WEAPON );
	}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs2 :: SetYawSpeed ( void )
{
	if(m_fGunDrawn){
	pev->yaw_speed = 150;
	}
	else{
	pev->yaw_speed = 0;
	}
}

int CCofMs2 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs2::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CCofMs2 :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs2 :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs2 :: IdleSound( void )
{

}

void CCofMs2 :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs2 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 12;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, dmg, DMG_SLASH );
				if(pHurt){
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbod1.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			if (RANDOM_LONG(0,1) )
			AttackSound();
		}
		break;

		case 2:
			{
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0.5;
				pev->frags			= 0;
				pev->flags		   &= ~FL_FROZEN;
				m_facing_fucking_mode = 1;
				if(pev->health < 1){
				pev->health = 1;
				}
				pev->takedamage		 = DAMAGE_AIM;
			}
			break;

		case 3:
			{
			TraceResult tr;
			UTIL_TraceLine(Center(), pev->origin, ignore_monsters, ENT(pev), &tr);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, Center(), 0, 191, 0 );
			//EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/water_small_splash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
void CCofMs2 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/dreamercrazy.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 50;
	}
	else{
	pev->health			= 40;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;

	if(pev->frags == 1){
	m_crouchmode        = 1;
	m_fGunDrawn         = FALSE;
	m_flFieldOfView		= 0;
	m_flDistLook        = 180;
	pev->flags			|= FL_FROZEN;
	pev->takedamage		 = DAMAGE_YES;
	m_singdelay_max      = 1;
	}
	else{
	m_crouchmode        = 0;
	m_fGunDrawn         = TRUE;
	m_flFieldOfView		= 0.5;
	}
	m_MoveFail_SimpleRoad = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_killed_exp = 20;
	m_rpgms_level = 15;
	pev->netname = MAKE_STRING( "White" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs2 :: Precache()
{
	PRECACHE_MODEL("models/dreamercrazy.mdl");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CCofMs2::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}