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
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		ISLAVE_AE_CLAW		( 1 )
#define		ISLAVE_AE_CLAWRAKE	( 2 )
#define		ISLAVE_AE_ZAP_POWERUP	( 3 )
#define		ISLAVE_AE_ZAP_SHOOT		( 4 )
#define		ISLAVE_AE_ZAP_DONE		( 5 )

#define		ISLAVE_MAX_BEAMS	10

class CISlave_BOSS : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int	 ISoundMask( void );
	int  Classify ( void );
	int  IRelationship( CBaseEntity *pTarget );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeathSound( void );
	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );

	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

    void StartTask ( Task_t *pTask );
	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void ClearBeams( );
	void ArmBeam( int side );
	void WackBeam( int side, CBaseEntity *pEntity );
	void ZapBeam( int side );
	void BeamGlow( void );

	int m_iBravery;

	CBeam *m_pBeam[ISLAVE_MAX_BEAMS];

	int m_iBeams;
	Vector m_teleportorigin;
	float m_flNextAttack;
	float m_flNextAttack2;

	int	m_voicePitch;

	int g_sFireball;
	int m_iSpriteTexture;

	int m_telport_danger;

	EHANDLE m_hDead;

	BOOL	m_fCanHornetAttack;
	float	m_flNextHornetAttackCheck;

	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
};
LINK_ENTITY_TO_CLASS( monster_islave_boss, CISlave_BOSS );


TYPEDESCRIPTION	CISlave_BOSS::m_SaveData[] = 
{
	DEFINE_FIELD( CISlave_BOSS, m_iBravery, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave_BOSS, m_telport_danger, FIELD_INTEGER ),

	DEFINE_FIELD( CISlave_BOSS, m_fCanHornetAttack, FIELD_BOOLEAN ),
	DEFINE_FIELD( CISlave_BOSS, m_flNextHornetAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( CISlave_BOSS, m_teleportorigin, FIELD_POSITION_VECTOR ),

	DEFINE_ARRAY( CISlave_BOSS, m_pBeam, FIELD_CLASSPTR, ISLAVE_MAX_BEAMS ),
	DEFINE_FIELD( CISlave_BOSS, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CISlave_BOSS, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CISlave_BOSS, m_flNextAttack2, FIELD_TIME ),

	DEFINE_FIELD( CISlave_BOSS, m_voicePitch, FIELD_INTEGER ),

	DEFINE_FIELD( CISlave_BOSS, m_hDead, FIELD_EHANDLE ),

};

IMPLEMENT_SAVERESTORE( CISlave_BOSS, CSquadMonster );




const char *CISlave_BOSS::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CISlave_BOSS::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CISlave_BOSS::pPainSounds[] = 
{
	"aslave/slv_pain1.wav",
	"aslave/slv_pain2.wav",
};

const char *CISlave_BOSS::pDeathSounds[] = 
{
	"aslave/slv_die1.wav",
	"aslave/slv_die2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CISlave_BOSS :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

BOOL CISlave_BOSS :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void CISlave_BOSS :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
    m_flGroundSpeed = 200;
	}
}

int CISlave_BOSS::IRelationship( CBaseEntity *pTarget )
{
	if ( (pTarget->IsPlayer()) )
		if ( (pev->spawnflags & SF_MONSTER_WAIT_UNTIL_PROVOKED ) && ! (m_afMemory & bits_MEMORY_PROVOKED ))
			return R_NO;
	return CBaseMonster::IRelationship( pTarget );
}

BOOL CISlave_BOSS :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( HasConditions ( bits_COND_SEE_ENEMY ) && flDist <= 65 && flDot >= 0.6 && m_hEnemy != NULL )
	{
		return TRUE;
	}
	return FALSE;
}

void CISlave_BOSS :: CallForHelp( char *szClassname, float flDist, EHANDLE hEnemy, Vector &vecLocation )
{
	return;
}


//=========================================================
// ALertSound - scream
//=========================================================
void CISlave_BOSS :: AlertSound( void )
{
	if ( m_hEnemy != NULL )
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SLV_ALERT", 0.85, ATTN_NORM, 0, m_voicePitch);
	}
}

//=========================================================
// IdleSound
//=========================================================
void CISlave_BOSS :: IdleSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		SENTENCEG_PlayRndSz(ENT(pev), "SLV_IDLE", 0.85, ATTN_NORM, 0, m_voicePitch);
	}

#if 0
	int side = RANDOM_LONG( 0, 1 ) * 2 - 1;

	ClearBeams( );
	ArmBeam( side );

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_right * 2 * side;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(vecSrc.x);	// X
		WRITE_COORD(vecSrc.y);	// Y
		WRITE_COORD(vecSrc.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 255 );		// r
		WRITE_BYTE( 180 );		// g
		WRITE_BYTE( 96 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );

	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap1.wav", 1, ATTN_NORM, 0, 100 );
#endif
}

//=========================================================
// PainSound
//=========================================================
void CISlave_BOSS :: PainSound( void )
{
	if (RANDOM_LONG( 0, 2 ) == 0)
	{
		EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
	}
}

//=========================================================
// DieSound
//=========================================================

void CISlave_BOSS :: DeathSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CISlave_BOSS :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


void CISlave_BOSS::Killed( entvars_t *pevAttacker, int iGib )
{
	ClearBeams( );
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CISlave_BOSS :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CISlave_BOSS :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1;
	if (g_iSkillLevel == SKILL_HARD){
	dmg1 = 18;
	}
	else if (g_iSkillLevel == SKILL_EASY){
	dmg1 = 12;
	}
	else{
	dmg1 = 15;
	}

	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case ISLAVE_AE_CLAW:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 72;
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
				CBaseEntity *pHurt = CheckTraceHullAttack( 72, 15, DMG_SLASH );
				if ( pHurt )
				{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
			}
		}
		break;

		case ISLAVE_AE_CLAWRAKE:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 75, 25, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
			else
			{
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, m_voicePitch );
			}
		}
		break;

		case ISLAVE_AE_ZAP_POWERUP:
		{
			pev->framerate = 1.5;

			UTIL_MakeAimVectors( pev->angles );

			if (m_iBeams == 0)
			{
				Vector vecSrc = pev->origin + gpGlobals->v_forward * 2;
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
					WRITE_BYTE(TE_DLIGHT);
					WRITE_COORD(vecSrc.x);	// X
					WRITE_COORD(vecSrc.y);	// Y
					WRITE_COORD(vecSrc.z);	// Z
					WRITE_BYTE( 12 );		// radius * 0.1
					WRITE_BYTE( 255 );		// r
					WRITE_BYTE( 180 );		// g
					WRITE_BYTE( 96 );		// b
					WRITE_BYTE( 20 / pev->framerate );		// time * 10
					WRITE_BYTE( 0 );		// decay * 0.1
				MESSAGE_END( );

			}
			if (m_hDead != NULL)
			{
				WackBeam( 1, m_hDead );
				WackBeam( 2, m_hDead );
				WackBeam( 3, m_hDead );
			}
			else
			{
				ArmBeam( 1 );
				ArmBeam( 2 );
				ArmBeam( 3 );
				BeamGlow( );
			}

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "debris/zap4.wav", 1, ATTN_NORM, 0, 100 + m_iBeams * 10 );
			pev->skin = m_iBeams / 2;
		}
		break;

		case ISLAVE_AE_ZAP_SHOOT:
		{
			ClearBeams( );

			if (m_hDead != NULL)
			{
				Vector vecDest = m_hDead->pev->origin + Vector( 0, 0, 38 );
				TraceResult trace;
				UTIL_TraceHull( vecDest, vecDest, dont_ignore_monsters, human_hull, m_hDead->edict(), &trace );

				if ( !trace.fStartSolid )
				{
					WackBeam( 1, m_hDead );
					WackBeam( 2, m_hDead );
					WackBeam( 3, m_hDead );
					m_hDead->Spawn();
					break;
				}
			}

			ClearMultiDamage();

			UTIL_MakeAimVectors( pev->angles );

			ZapBeam( 1 );
			ZapBeam( 2 );
			ZapBeam( 3 );

			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "hassault/hw_shoot1.wav", 1, ATTN_NORM, 0, RANDOM_LONG( 130, 160 ) );
			// STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
			ApplyMultiDamage(pev, pev);

			m_flNextAttack = gpGlobals->time + RANDOM_FLOAT( 0.5, 4.0 );
			m_iBravery = 0;
		}
		break;

		case ISLAVE_AE_ZAP_DONE:
		{
			ClearBeams( );
		}
		break;

		case 6:
		{
			
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CISlave_BOSS :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 1000;

	if(pev->waterlevel >= 2){
	return FALSE;
	}

	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	if ( gpGlobals->time < m_flNextHornetAttackCheck )
	{
		return m_fCanHornetAttack;
	}

	if ( m_cleardally_enemy > 0 && flDist <= dist && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;
		Vector	vecArmPos, vecArmDir;

		// verify that a shot fired from the gun will hit the enemy before the world.
		// !!!LATER - we may wish to do something different for projectile weapons as opposed to instant-hit
		vecArmPos = EyePosition();

		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget_h(vecArmPos), dont_ignore_monsters,dont_ignore_glass, ENT(pev), &tr);
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if(pEntity->pev->takedamage != DAMAGE_NO && pEntity->pev->solid == SOLID_BSP){
		UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget_h(vecArmPos), dont_ignore_monsters,ignore_glass, ENT(pev), &tr);
		}

		if ( tr.flFraction == 1.0 || tr.pHit == m_hEnemy->edict() )
		{
			m_flNextHornetAttackCheck = gpGlobals->time + 3;
			m_fCanHornetAttack = TRUE;
			return m_fCanHornetAttack;
		}
	}
	
	m_flNextHornetAttackCheck = gpGlobals->time + 0.2;// don't check for half second if this check wasn't successful
	m_fCanHornetAttack = FALSE;
	return m_fCanHornetAttack;
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CISlave_BOSS :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}

	m_hDead = NULL;
	m_iBravery = 0;

	CBaseEntity *pEntity = NULL;
	while ((pEntity = UTIL_FindEntityByClassname( pEntity, "monster_alien_slave" )) != NULL)
	{
		TraceResult tr;

		UTIL_TraceLine( EyePosition( ), pEntity->EyePosition( ), ignore_monsters, ENT(pev), &tr );
		if (tr.flFraction == 1.0 || tr.pHit == pEntity->edict())
		{
			if (pEntity->pev->deadflag == DEAD_DEAD)
			{
				float d = (pev->origin - pEntity->pev->origin).Length();
				if (d < flDist)
				{
					m_hDead = pEntity;
					flDist = d;
				}
				m_iBravery--;
			}
			else
			{
				m_iBravery++;
			}
		}
	}
	if (m_hDead != NULL)
		return TRUE;
	else
		return FALSE;
}


//=========================================================
// StartTask
//=========================================================
void CISlave_BOSS :: StartTask ( Task_t *pTask )
{
	ClearBeams( );

	CSquadMonster :: StartTask ( pTask );
}


//=========================================================
// Spawn
//=========================================================
void CISlave_BOSS :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/islave_boss.mdl");
	UTIL_SetSize(pev, Vector( -24, -24, 0 ), Vector( 24, 24, 72 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 1200;
	}
	else if (g_iSkillLevel == SKILL_EASY){
	pev->health			= 800;
	}
	else{
	pev->health			= 1000;
	}

	pev->armortype		= 0;
	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK2 | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 85, 110 );

	m_aimenemy_mod		= 2;
	m_EyeMod			= 2;//目力模式

	MonsterInit();

	m_headdef			= 2;//坚硬の头部

	pev->gravity		= 1.1;

	m_chase_mode = 3;
	m_chase_failed_max = 2;
	m_forcefuckdoor  = TRUE;
	m_killed_exp = 40;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CISlave_BOSS :: Precache()
{
	int i;
	g_sFireball = PRECACHE_MODEL("sprites/b-tele1.spr");
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );

	PRECACHE_MODEL("models/islave_boss.mdl");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");
	PRECACHE_SOUND("hassault/hw_shoot1.wav");
	PRECACHE_SOUND("zombie/zo_pain2.wav");
	PRECACHE_SOUND("headcrab/hc_headbite.wav");
	PRECACHE_SOUND("weapons/cbar_miss1.wav");
	
	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	UTIL_PrecacheOther( "test_effect" );
}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CISlave_BOSS :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (bitsDamageType & DMG_SHOCK){
	return 0;
	}
	m_afMemory |= bits_MEMORY_PROVOKED;
	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CISlave_BOSS::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (bitsDamageType == DMG_SHOCK){
		return;
	}

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t	tlSlaveBOSSAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slSlaveBOSSAttack1[] =
{
	{ 
		tlSlaveBOSSAttack1,
		ARRAYSIZE ( tlSlaveBOSSAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};

Task_t	tlSlaveBOSSAttack2[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slSlaveBOSSAttack2[] =
{
	{ 
		tlSlaveBOSSAttack2,
		ARRAYSIZE ( tlSlaveBOSSAttack2 ), 
		0,
		0,
		"Slave Range Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CISlave_BOSS )
{
	slSlaveBOSSAttack1,
	slSlaveBOSSAttack2,
};

IMPLEMENT_CUSTOM_SCHEDULES( CISlave_BOSS, CSquadMonster );


//=========================================================
//=========================================================
Schedule_t *CISlave_BOSS :: GetSchedule( void )
{
	ClearBeams( );

/*
	if (pev->spawnflags)
	{
		pev->spawnflags = 0;
		return GetScheduleOfType( SCHED_RELOAD );
	}
*/

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );

		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) ){
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
// dead enemy
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CBaseMonster :: GetSchedule();
		}
		if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
		{
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
		break;
	}
	return CSquadMonster::GetSchedule( );
}


Schedule_t *CISlave_BOSS :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_FAIL:
		if (HasConditions( bits_COND_CAN_MELEE_ATTACK1 ))
		{
		return CSquadMonster :: GetScheduleOfType( SCHED_MELEE_ATTACK1 ); ;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slSlaveBOSSAttack1;
	case SCHED_RANGE_ATTACK2:
		return slSlaveBOSSAttack2;
	}
	return CSquadMonster :: GetScheduleOfType( Type );
}


//=========================================================
// ArmBeam - small beam from arm to nearby geometry
//=========================================================

void CISlave_BOSS :: ArmBeam( int side )
{
	TraceResult tr;
	float flDist = 1.0;
	int arm_vect = -1;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	if(side == 1){
	arm_vect = 1;
	}
	else if(side == 2){
	arm_vect = -1;
	}
	else if(side == 3){
	arm_vect = 0;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * arm_vect * 16 + gpGlobals->v_forward * 32;

	for (int i = 0; i < 3; i++)
	{
		Vector vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		TraceResult tr1;
		UTIL_TraceLine ( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, ENT( pev ), &tr1);
		if (flDist > tr1.flFraction)
		{
			tr = tr1;
			flDist = tr.flFraction;
		}
	}

	// Couldn't find anything close enough
	if ( flDist == 1.0 )
		return;

	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side );
	// m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
	m_pBeam[m_iBeams]->SetBrightness( 64 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}


//=========================================================
// BeamGlow - brighten all beams
//=========================================================
void CISlave_BOSS :: BeamGlow( )
{
	int b = m_iBeams * 32;
	if (b > 255)
		b = 255;

	for (int i = 0; i < m_iBeams; i++)
	{
		if (m_pBeam[i]->GetBrightness() != 255) 
		{
			m_pBeam[i]->SetBrightness( b );
		}
	}
}


//=========================================================
// WackBeam - regenerate dead colleagues
//=========================================================
void CISlave_BOSS :: WackBeam( int side, CBaseEntity *pEntity )
{
	Vector vecDest;
	float flDist = 1.0;
	
	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	if (pEntity == NULL)
		return;

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( pEntity->Center(), entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 80 );
	m_iBeams++;
}

//=========================================================
// ZapBeam - heavy damage directly forward
//=========================================================
void CISlave_BOSS :: ZapBeam( int side )
{
	Vector vecSrc, vecAim;
	TraceResult tr;
	CBaseEntity *pEntity;

	if (m_iBeams >= ISLAVE_MAX_BEAMS)
		return;

	vecSrc = pev->origin + gpGlobals->v_up * 36;
	
	vecAim = ShootAtEnemy( vecSrc );
	float deflection = 0.015;
	float dist = 1250;
	vecAim = vecAim + side * gpGlobals->v_right * RANDOM_FLOAT( 0, deflection ) + gpGlobals->v_up * RANDOM_FLOAT( -deflection, deflection );
	UTIL_TraceLine ( vecSrc, vecSrc + vecAim * dist, dont_ignore_monsters, ENT( pev ), &tr);

	m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	if (!m_pBeam[m_iBeams])
		return;

	m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex( ) );
	m_pBeam[m_iBeams]->SetEndAttachment( side );
	m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
	m_pBeam[m_iBeams]->SetBrightness( 255 );
	m_pBeam[m_iBeams]->SetNoise( 20 );
	m_iBeams++;

	pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity != NULL)
	{
		int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, tr.vecEndPos);
		int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, tr.vecEndPos,Classify(),0);
		FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

		int dmg1;
		if (g_iSkillLevel == SKILL_HARD){
		dmg1 = 25;
		}
		else if (g_iSkillLevel == SKILL_EASY){
		dmg1 = 15;
		}
		else{
		dmg1 = 20;
		}

		FX_ImpBeam( tr.vecEndPos, tr.vecPlaneNormal, pEntity->IsBSPModel()?1:0, IMPBEAM_PLASMABALL );
		if(pEntity->pev->takedamage){
		pEntity->TraceAttack( pev, dmg1, vecAim, &tr, DMG_SHOCK);
		}
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
}


//=========================================================
// ClearBeams - remove all beams
//=========================================================
void CISlave_BOSS :: ClearBeams( )
{
	for (int i = 0; i < ISLAVE_MAX_BEAMS; i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;

	STOP_SOUND( ENT(pev), CHAN_WEAPON, "debris/zap4.wav" );
}