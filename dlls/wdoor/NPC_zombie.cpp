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
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

Task_t	tlZombie_wakeup[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_STAND },
};

Schedule_t slZombie_wakeup[] = 
{
	{
		tlZombie_wakeup,
		ARRAYSIZE ( tlZombie_wakeup ),
		0,
		0,
		"Zombie Wake Up"
	}
};

class CZombie : public CBaseMonster
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

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie );
LINK_ENTITY_TO_CLASS( monster_zombie_sitting, CZombie );

TYPEDESCRIPTION	CZombie::m_SaveData[] = 
{
	DEFINE_FIELD( CZombie, m_fGunDrawn, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CZombie, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CZombie )
{
	slZombie_wakeup,
};

IMPLEMENT_CUSTOM_SCHEDULES( CZombie, CBaseMonster );

const char *CZombie::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
void CZombie :: RunAI( void )
{
	CBaseMonster :: RunAI();
	/*
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
    m_flGroundSpeed = 100;
	}*/
	if(pev->sequence == LookupSequence( "eatbody" ) || pev->sequence == LookupSequence( "zombie_eating" )
	|| pev->sequence == LookupSequence( "sit_idle" ) || pev->sequence == LookupSequence( "sit_to_stand" )){
	m_duckseq = 4;
	}
	else if(m_duckseq != 0){
	m_duckseq = 0;
	}
}

int	CZombie :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

BOOL CZombie :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 64;
	if(pev->impulse == 1){
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc = Center();
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 64;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
				if ( FClassnameIs( pEntity->pev, "func_breakable" )){
				return TRUE;
				}
			}
	}

	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	if ( flDist <= dist && flDot >= 0.7 && m_hEnemy != NULL && FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) )
	{
		return TRUE;
	}
	return FALSE;
}

void CZombie::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(m_crouchmode == 0){
		if(ptr->iHitgroup == 1)
		{
			m_zombiehead_health -= flDamage;
			if(m_zombiehead_health <= 0 && pev->health > flDamage){
			pev->health = 0;
			}
			flDamage *= 0.5;
		}
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CZombie :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slZombie_wakeup;
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
Schedule_t *CZombie :: GetSchedule ( void )
{
	if ( FClassnameIs(pev,"monster_zombie_sitting") ){
		if (!m_fGunDrawn && (m_hEnemy != NULL || pev->health <= 0 || m_allydeadcheck == 1) ){
			return GetScheduleOfType( SCHED_ARM_WEAPON );
		}
	}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie :: SetYawSpeed ( void )
{
	if(m_crouchmode == 0){
	pev->yaw_speed = 120;
	}
	else{
	pev->yaw_speed = 0;
	}
}

int CZombie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(m_crouchmode == 1){
		if(pev->sequence == LookupActivity ( ACT_STAND )){
		pev->health -= flDamage;
		}
		return 0;
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie::Killed( entvars_t *pevAttacker, int iGib )
{
	if(m_die == 0 && !ShouldGibMonster( iGib )){
		if ( m_zombiehead_health > 12 && pev->body == 0 )
		{// drop the gun!
			Vector vecGunPos;
			Vector vecGunAngles;

			pev->body = 1;

			GetAttachment( 0, vecGunPos, vecGunAngles );
			
			CBaseEntity *pCrab = CBaseEntity::Create( "monster_headcrab", vecGunPos, pev->angles, edict() );
			pCrab->pev->health = m_zombiehead_health - 12;

			if(m_diefadeout == 1){
			SetBits( pCrab->pev->spawnflags, SF_MONSTER_FADECORPSE );
			}

			pev->owner = ENT( pCrab->pev );
			UTIL_MakeVectors ( pev->angles ); 
			pCrab->pev->velocity = gpGlobals->v_forward * 50;
		}
		else{
		m_killed_exp = 40;
		}
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}

void CZombie :: PainSound( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + RANDOM_FLOAT(0.4, 0.7);

	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombie :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CZombie :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2;
	dmg1 = 20;
	dmg2 = 40;

	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg1, DMG_SLASH );
				if ( pHurt )
				{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_LEFT:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),0);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg1, DMG_SLASH );
				if ( pHurt )
				{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			AttackSound();
		}
		break;

		case ZOMBIE_AE_ATTACK_BOTH:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 75;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 75, dmg2, DMG_SLASH );
				if ( pHurt )
				{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}

			AttackSound();
		}
		break;

		case 5:
			{
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0.5;
				m_alert = 100;
				pev->flags		   &= ~FL_NOTARGET;
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
void CZombie :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/zombie.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 120;
	m_zombiehead_health = 36;
	}
	else{
	pev->health			= 100;
	m_zombiehead_health = 30;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	pev->body			= 0;

	MonsterInit();
	m_chase_mode = 2;
	m_chase_failed_max = 4;

	if ( FClassnameIs(pev,"monster_zombie_sitting") ){
	m_crouchmode        = 1;
	m_fGunDrawn         = FALSE;
	m_flFieldOfView		= 0;
	m_flDistLook        = 120;
	pev->flags			|= FL_NOTARGET;
	m_flFieldOfView		= 0;
	m_selfmode		    = TRUE;
	}
	m_killed_exp = 20;
	m_rpgms_level = 20;
	pev->netname = MAKE_STRING( "Zombie" );
	m_canbarnacle_mode = 1;
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie :: Precache()
{
	int i;

	PRECACHE_MODEL("models/zombie.mdl");
	UTIL_PrecacheOther( "monster_headcrab" );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CZombie::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (m_Activity == ACT_MELEE_ATTACK1)
	{
		iIgnore |= bits_COND_LIGHT_DAMAGE;	
	}

	return iIgnore;
	
}