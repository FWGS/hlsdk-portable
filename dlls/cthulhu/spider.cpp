/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// spider
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"
#include	"customentity.h"
#include	"soundent.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SPIDER_AE_ATTACK		0x01

#define SPIDER_FLINCH_DELAY			3		// at most one flinch every n secs

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SPIDER_REPEL = LAST_COMMON_SCHEDULE + 1,
	SCHED_SPIDER_REPEL_LAND,
};

//=========================================================
// repel 
//=========================================================
Task_t	tlSpiderRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slSpiderRepel[] =
{
	{ 
		tlSpiderRepel,
		ARRAYSIZE ( tlSpiderRepel ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlSpiderRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slSpiderRepelLand[] =
{
	{ 
		tlSpiderRepelLand,
		ARRAYSIZE ( tlSpiderRepelLand ), 
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER, 
		"Repel Land"
	},
};


class CSpider : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	virtual int GetVoicePitch( void ) { return 95 + RANDOM_LONG(0,9); }
	virtual float GetSoundVolue( void ) { return 1.0; }

	// No range attacks
	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	virtual float GetDamageAmount( void ) { return gSkillData.zombieDmgOneSlash; }
	virtual float GetKnockback( void ) { return 200; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int GetHeightFix(void)	{ return 0; };

	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

};

LINK_ENTITY_TO_CLASS( monster_giantspider, CSpider );

DEFINE_CUSTOM_SCHEDULES( CSpider )
{
	slSpiderRepel,
	slSpiderRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSpider, CBaseMonster );


const char *CSpider::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CSpider::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CSpider::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CSpider::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CSpider::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CSpider::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSpider :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSpider :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

int CSpider :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSpider :: PainSound( void )
{
	int pitch = GetVoicePitch();

	if (RANDOM_LONG(0,5) < 3)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );
}

void CSpider :: AlertSound( void )
{
	int pitch = GetVoicePitch();

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );
}

void CSpider :: IdleSound( void )
{
	int pitch = GetVoicePitch();

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );
}

void CSpider :: AttackSound( void )
{
	int pitch = GetVoicePitch();

	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSpider :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SPIDER_AE_ATTACK:
		{
			int pitch = GetVoicePitch();

			// do stuff for this event.
			// Cthulhu: bug fix (hack) for origin being below ground!
			pev->size.z += GetHeightFix();
			CBaseEntity *pHurt = CheckTraceHullAttack( 120, GetDamageAmount(), DMG_SLASH );
			pev->size.z -= GetHeightFix();
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -GetKnockback();
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], GetSoundVolue(), ATTN_NORM, 0, pitch );

			if (RANDOM_LONG(0,1))
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
void CSpider :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/spider.mdl");
	UTIL_SetSize( pev, Vector( -36, -36, 0 ), Vector( 36, 36, 76 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.zombieHealth * 2;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_JUMP | bits_CAP_CLIMB | bits_CAP_HEAR | bits_CAP_AUTO_DOORS | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSpider :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/spider.mdl");

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

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CSpider::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + SPIDER_FLINCH_DELAY;
	}

	return iIgnore;	
}

BOOL CSpider :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	//int iGround = FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND );

	// Cthulhu: but this stops the monster from hitting flying monsters that are
	// melee attacking it (e.g. nightgaunt)
	// Solution: explicitly check for monster types that we cannot hit

	BOOL bHit = TRUE;	// we can hit by default
	if (m_hEnemy)
	{
		if (FClassnameIs( m_hEnemy->pev, "hornet"))			bHit = FALSE;
		if (FClassnameIs( m_hEnemy->pev, "monster_snark"))	bHit = FALSE;
	}

	if ( flDist <= 80 && flDot >= 0.7 && m_hEnemy != NULL && bHit )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CSpider :: GetSchedule( void )
{
	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling. 
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_SPIDER_REPEL_LAND );
		}
		else
		{
			// repel down a rope, 
			return GetScheduleOfType ( SCHED_SPIDER_REPEL );
		}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
Schedule_t* CSpider :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_SPIDER_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slSpiderRepel[ 0 ];
		}
	case SCHED_SPIDER_REPEL_LAND:
		{
			return &slSpiderRepelLand[ 0 ];
		}
	default:
		{
			return CBaseMonster :: GetScheduleOfType ( Type );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//=========================================================
// CSpiderRepel - when triggered, spawns a monster_giantspider
// repelling down a web.
//=========================================================

class CSpiderRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

LINK_ENTITY_TO_CLASS( monster_spider_repel, CSpiderRepel );

void CSpiderRepel::Spawn( void )
{
	Precache( );
	pev->solid = SOLID_NOT;

	SetUse( RepelUse );
}

void CSpiderRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_giantspider" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CSpiderRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP) 
		return NULL;
	*/

	CBaseEntity *pEntity = Create( "monster_giantspider", pev->origin, pev->angles );
	CBaseMonster *pSpider = pEntity->MyMonsterPointer( );
	pSpider->pev->movetype = MOVETYPE_FLY;
	pSpider->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pSpider->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pSpider->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pSpider->entindex() );
	pBeam->SetEndAttachment( 1 );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( SUB_Remove );
	pBeam->SetNextThink( -4096.0 * tr.flFraction / pSpider->pev->velocity.z + 0.5 );

	UTIL_Remove( this );
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CBabySpider : public CSpider
{
public:
	void Spawn( void );
	void Precache( void );

	virtual int GetVoicePitch( void ) { return PITCH_NORM + RANDOM_LONG(40,50); }
	virtual float GetSoundVolue( void ) { return 0.6; }

	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	virtual float GetDamageAmount( void ) { return gSkillData.headcrabDmgBite * 0.5; }
	virtual float GetKnockback( void ) { return 50; }

	virtual int GetHeightFix(void)	{ return 32; };
};

LINK_ENTITY_TO_CLASS( monster_babygiantspider, CBabySpider );

void CBabySpider :: Spawn( void )
{
	CSpider::Spawn();

	SET_MODEL(ENT(pev), "models/monsters/babyspider.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 16));

	// this is always true
	pev->health	= gSkillData.headcrabHealth;
}

void CBabySpider :: Precache( void )
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL( "models/monsters/babyspider.mdl" );
	CSpider::Precache();
}

BOOL CBabySpider :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	//int iGround = FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND );

	// Cthulhu: but this stops the monster from hitting flying monsters that are
	// melee attacking it (e.g. nightgaunt)
	// Solution: explicitly check for monster types that we cannot hit

	BOOL bHit = TRUE;	// we can hit by default
	if (m_hEnemy)
	{
		if (FClassnameIs( m_hEnemy->pev, "hornet"))			bHit = FALSE;
		if (FClassnameIs( m_hEnemy->pev, "monster_snark"))	bHit = FALSE;
	}

	if ( flDist <= 48 && flDot >= 0.7 && m_hEnemy != NULL && bHit )
	{
		return TRUE;
	}
	return FALSE;
}

