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

Task_t	tlAniki_wakeup[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_STAND },
};

Schedule_t slAniki_wakeup[] = 
{
	{
		tlAniki_wakeup,
		ARRAYSIZE ( tlAniki_wakeup ),
		0,
		0,
		"Aniki Wake Up"
	}
};

class CCofMs9 : public CBaseMonster
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
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
};

const char *CCofMs9::pAttackSounds[] = 
{
	"cof/slower_attack1.wav",
	"cof/slower_attack2.wav",
};

const char *CCofMs9::pAlertSounds[] = 
{
	"cof/slower_alert10.wav",
	"cof/slower_alert20.wav",
	"cof/slower_alert30.wav",
};

const char *CCofMs9::pPainSounds[] = 
{
	"cof/slower_pain1.wav",
	"cof/slower_pain2.wav",
};

LINK_ENTITY_TO_CLASS( monster_cof_ms9, CCofMs9 );

TYPEDESCRIPTION	CCofMs9::m_SaveData[] = 
{
	DEFINE_FIELD( CCofMs9, m_fGunDrawn, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CCofMs9, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CCofMs9 )
{
	slAniki_wakeup,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCofMs9, CBaseMonster );

BOOL CCofMs9 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
			// flying?
			if ( pev->movetype == MOVETYPE_TOSS)
			{
				if (pev->flags & FL_ONGROUND)
				{
					pev->movetype = MOVETYPE_STEP;
				}
			}

	float dist = 70;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 10;
			}
		}
	}
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= dist && flDot >= 0.7 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCofMs9 :: Classify ( void )
{
	if ( pev->team == 1 ){
		return	CLASS_PLAYER_ALLY;
	}
	else{
		return	CLASS_HUMAN_ASS;
	}
}

void CCofMs9::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CCofMs9 :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slAniki_wakeup;
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
Schedule_t *CCofMs9 :: GetSchedule ( void )
{
	if (!m_fGunDrawn && (m_hEnemy != NULL || m_alert > 0 || m_allydeadcheck == 1) ){
		if(pev->body == 0){
			return GetScheduleOfType( SCHED_ARM_WEAPON );
		}
		else{
			m_crouchmode        = 0;
			m_fGunDrawn         = TRUE;
			m_flDistLook		= 4096;
			m_flFieldOfView		= 0;
			m_facing_fucking_mode = 1;
		}
	}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCofMs9 :: SetYawSpeed ( void )
{
	if(m_fGunDrawn){
	pev->yaw_speed = 180;
	}
	else{
	pev->yaw_speed = 0;
	}
}

int CCofMs9 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(m_flDistLook < 4096)
	m_flDistLook = 4096;

	m_alert	= 100;
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CCofMs9::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void CCofMs9 :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs9 :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CCofMs9 :: IdleSound( void )
{

}

void CCofMs9 :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCofMs9 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2,dmg3;
	dmg1 = 30;//����ն
	dmg2 = 20;//��ն
	dmg3 = 15;//��ն

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 ){
				vecSrc	= BodyTarget(pev->origin);
				}
			}
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 85;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 85, dmg1, DMG_SLASH );
				if(pHurt){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			AttackSound();
		}
		break;


		case 3:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 ){
				vecSrc	= BodyTarget(pev->origin);
				}
			}
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 85;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 85, dmg2, DMG_SLASH );
				if(pHurt){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			AttackSound();
		}
		break;

		case 2:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 ){
				vecSrc	= BodyTarget(pev->origin);
				}
			}
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 85;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg3, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );

				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 85, dmg3, DMG_SLASH );
				if(pHurt){
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

		}
		break;

		case 4:
		{
			pev->movetype = MOVETYPE_TOSS;
			pev->flags &= ~FL_ONGROUND;
			UTIL_MakeVectors(pev->angles);
			pev->velocity = gpGlobals->v_forward * 300;
			pev->velocity.z += 100;
		}
		break;

		case 5:
		{
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0;
				m_facing_fucking_mode = 1;
				if(pev->health < 1){
				pev->health = 1;
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
void CCofMs9 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/chopper.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 150;
	}
	else{
	pev->health			= 120;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;

	if(pev->frags == 1){
	m_crouchmode        = 1;
	m_fGunDrawn         = FALSE;
	m_flFieldOfView		= -1;
	m_flDistLook        = 64;
	m_MoveFail_FuckRoad_Mode = 1;
	}
	else{
	m_crouchmode        = 0;
	m_fGunDrawn         = TRUE;
	m_flFieldOfView		= 0;
	}

	SetTouch( &CCofMs9::DeadTouch );
	m_killed_exp = 60;
	m_rpgms_level = 35;
	pev->netname = MAKE_STRING( "Nirvana.Chopper" );

	m_singdelay_max = 0;//0��Ӧ
	m_singdelay_use = m_singdelay_max;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCofMs9 :: Precache()
{
	PRECACHE_MODEL("models/chopper.mdl");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CCofMs9::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}