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
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"squadmonster.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BIGSMOKE_AE_DRAW		( 2 )
#define		BIGSMOKE_AE_SHOOT		( 3 )
#define		BIGSMOKE_AE_HOLSTER	( 4 )

#define	BIGSMOKE_BODY_GUNHOLSTERED	0
#define	BIGSMOKE_BODY_GUNDRAWN		1
#define BIGSMOKE_BODY_GUNGONE		2

class CBigSmoke : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int ISoundMask( void );
	void BigSmokeFirePistol( void );
	void AlertSound( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunTask( Task_t *pTask );
	void StartTask( Task_t *pTask );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	BOOL CheckRangeAttack1( float flDot, float flDist );

	void DeclineFollowing( void );

	// Override these to set behavior
	Schedule_t *GetScheduleOfType( int Type );
	Schedule_t *GetSchedule( void );
	MONSTERSTATE GetIdealState( void );

	void DeathSound( void );
	void PainSound( void );

	void TalkInit( void );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void Killed( entvars_t *pevAttacker, int iGib );

	static TYPEDESCRIPTION m_SaveData[];
	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;

	// UNDONE: What is this for?  It isn't used?
	float m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES
};

LINK_ENTITY_TO_CLASS( monster_big_smoke, CBigSmoke )

TYPEDESCRIPTION	CBigSmoke::m_SaveData[] =
{
	DEFINE_FIELD( CBigSmoke, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBigSmoke, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CBigSmoke, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CBigSmoke, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBigSmoke, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CBigSmoke, CSquadMonster )

//=========================================================
// AI Schedules Specific to this monster
//=====

Task_t tlBsFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, (float)128 },	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slBsFollow[] =
{
	{
		tlBsFollow,
		ARRAYSIZE( tlBsFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw - much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t tlBigSmokeEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float) ACT_ARM },
};

Schedule_t slBigSmokeEnemyDraw[] =
{
	{
		tlBigSmokeEnemyDraw,
		ARRAYSIZE( tlBigSmokeEnemyDraw ),
		0,
		0,
		"BigSmoke Enemy Draw"
	}
};

Task_t tlBsFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};

Schedule_t slBsFaceTarget[] =
{
	{
		tlBsFaceTarget,
		ARRAYSIZE( tlBsFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlIdleBsStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, (float)2 }, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET, (float)0 }, // reset head position
};

Schedule_t slIdleBsStand[] =
{
	{
		tlIdleBsStand,
		ARRAYSIZE( tlIdleBsStand ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,
		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER |
		//bits_SOUND_WORLD |
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBigSmoke )
{
	slBsFollow,
	slBigSmokeEnemyDraw,
	slBsFaceTarget,
	slIdleBsStand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBigSmoke, CSquadMonster )

void CBigSmoke::StartTask( Task_t *pTask )
{
	CSquadMonster::StartTask( pTask );	
}

void CBigSmoke::RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if( m_hEnemy != NULL && ( m_hEnemy->IsPlayer() ) )
		{
			pev->framerate = 1.5;
		}
		CSquadMonster::RunTask( pTask );
		break;
	default:
		CSquadMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBigSmoke::ISoundMask( void) 
{
	return bits_SOUND_WORLD |
			bits_SOUND_COMBAT |
			bits_SOUND_CARCASS |
			bits_SOUND_MEAT |
			bits_SOUND_GARBAGE |
			bits_SOUND_DANGER |
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CBigSmoke::Classify( void )
{
	return CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBigSmoke::AlertSound( void )
{
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBigSmoke::SetYawSpeed( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBigSmoke::CheckRangeAttack1( float flDot, float flDist )
{
	if( flDist <= 1024 && flDot >= 0.5 )
	{
		if( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if( tr.flFraction == 1.0 || ( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy ) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}

//=========================================================
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBigSmoke::BigSmokeFirePistol( void )
{
	Vector vecShootOrigin;

	UTIL_MakeVectors( pev->angles );
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );

	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "BigSmoke/fire1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBigSmoke::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case BIGSMOKE_AE_SHOOT:
		BigSmokeFirePistol();
		break;
	case BIGSMOKE_AE_DRAW:
		// barney's bodygroup switches here so he can pull gun from holster
		pev->body = BIGSMOKE_BODY_GUNDRAWN;
		m_fGunDrawn = TRUE;
		break;
	case BIGSMOKE_AE_HOLSTER:
		// change bodygroup to replace gun in holster
		pev->body = BIGSMOKE_BODY_GUNHOLSTERED;
		m_fGunDrawn = FALSE;
		break;
	default:
		CSquadMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CBigSmoke::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/bigsmoke.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 350;
	pev->view_ofs = Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	pev->body = 0; // gun in holster
	m_fGunDrawn = FALSE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBigSmoke::Precache()
{
	PRECACHE_MODEL( "models/bigsmoke.mdl" );

	PRECACHE_SOUND( "bigsmoke/fire1.wav" );


	CSquadMonster::Precache();
}	


static BOOL IsFacing( entvars_t *pevTest, const Vector &reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );

	// He's facing me, he meant it
	if( DotProduct( forward, vecDir ) > 0.96 )	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}

int CBigSmoke::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
			{
			}
			else
			{
			}
		}
		else if( !( m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
		}
	}

	return ret;
}

//=========================================================
// PainSound
//=========================================================
void CBigSmoke::PainSound( void )
{
	if( gpGlobals->time < m_painTime )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );

}

//=========================================================
// DeathSound 
//=========================================================
void CBigSmoke::DeathSound( void )
{
}

void CBigSmoke::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	switch( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
		{
			flDamage = flDamage / 2;
		}
		break;
	case 10:
		if( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
		{
			flDamage -= 20;
			if( flDamage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}

		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CBigSmoke::Killed( entvars_t *pevAttacker, int iGib )
{
	if( pev->body < BIGSMOKE_BODY_GUNGONE )
	{
		// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		pev->body = BIGSMOKE_BODY_GUNGONE;

		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun = DropItem( "weapon_9mmhandgun", vecGunPos, vecGunAngles );
	}

	SetUse( NULL );	
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Schedule_t *CBigSmoke::GetScheduleOfType( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slBigSmokeEnemyDraw;
		}
		break;
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CSquadMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
			return slBsFaceTarget;	// override this for different target face behavior
		else
			return psched;
	case SCHED_TARGET_CHASE:
		return slBsFollow;
	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CSquadMonster::GetScheduleOfType( Type );

		if( psched == slIdleStand )
		{
			// just look straight ahead.
			return slIdleBsStand;
		}
		else
			return psched;	
	}

	return CSquadMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBigSmoke::GetSchedule( void )
{
	if( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
			// dead enemy
			if( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster::GetSchedule();
			}

			// always act surprized with a new enemy
			if( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );

			// wait for one schedule to draw gun
			if( !m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}


		if( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		break;
	default:
		break;
	}

	return CSquadMonster::GetSchedule();
}

MONSTERSTATE CBigSmoke::GetIdealState( void )
{
	return CSquadMonster::GetIdealState();
}

void CBigSmoke::DeclineFollowing( void )
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}
