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
#include	"decals.h"

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CMajoFireBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( fire_bolt, CMajoFireBall );

void CMajoFireBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "fire_bolt" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/fire_bolt.mdl");
	pev->frame = 0;
	pev->frame = 1.0;
	pev->body = 0;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	FX_Trail(pev->origin, entindex(), PROJ_FLAME );

	pev->effects		= EF_DIMLIGHT;
}

void CMajoFireBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CMajoFireBall *pSpit = GetClassPtr( (CMajoFireBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CMajoFireBall :: Touch ( CBaseEntity *pOther )
{
		entvars_t *pevOwner = VARS( pev->owner );

		if(pOther->pev->takedamage){
		pOther->TakeDamage ( pev, pevOwner, 20, DMG_BURN );
		}

		::RadiusDamage( pev->origin, pev, pevOwner, 40, 150, CLASS_HUMAN_ASS, DMG_BURN );

		FX_Trail(pev->origin, entindex(), PROJ_REMOVE );
		FX_Explosion( pev->origin, 136);

		SetThink ( &CMajoFireBall::SUB_Remove );
		pev->nextthink = gpGlobals->time;
}

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================


//=========================================================
// repel 
//=========================================================
Task_t	tlMajoRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		    },
	{ TASK_FACE_IDEAL,			(float)0		    },
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slMajoRepel[] =
{
	{ 
		tlMajoRepel,
		ARRAYSIZE ( tlMajoRepel ), 
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
// repel 
//=========================================================
Task_t	tlMajoRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slMajoRepelAttack[] =
{
	{ 
		tlMajoRepelAttack,
		ARRAYSIZE ( tlMajoRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

enum
{
	SCHED_MAJO_REPEL = LAST_COMMON_SCHEDULE + 1,
	SCHED_MAJO_REPEL_ATTACK,
};

class CMajo : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int	 ISoundMask( void );
	int  Classify ( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeathSound( void );

	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

	void StartTask ( Task_t *pTask );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;
	Vector m_teleportorigin2;
	Vector m_speedvec;

	float m_flNextFireTime;
	float m_flNextTeleportTime;

	int	m_voicePitch;

	int g_sFireball;
	int majo_fire;
	int m_iSpriteTexture;
	int m_iBravery;
	int m_telport_danger;

	static const char *pDeathSounds[];
};
LINK_ENTITY_TO_CLASS( monster_majo, CMajo );

TYPEDESCRIPTION	CMajo::m_SaveData[] = 
{
	DEFINE_FIELD( CMajo, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CMajo, m_teleportorigin2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CMajo, m_flNextTeleportTime, FIELD_TIME ),
	DEFINE_FIELD( CMajo, m_flNextFireTime, FIELD_TIME ),
	DEFINE_FIELD( CMajo, m_voicePitch, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMajo, CSquadMonster );

const char *CMajo::pDeathSounds[] = 
{
	"majo/die1.wav",
	"majo/die2.wav",
	"majo/die3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMajo :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CMajo :: FCanCheckAttacks ( void )
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

void CMajo :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(IsAlive()){
		if(pev->body == 1 && pev->movetype != MOVETYPE_FLY){
			pev->movetype = MOVETYPE_FLY;
			pev->flags	 |= FL_FLY;
			pev->gravity = 0.2;
			m_flFieldOfView = -1;
			m_teleportorigin = pev->origin;
			pev->velocity.z = RANDOM_LONG(-64,64);
			UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
			SetActivity ( ACT_GLIDE );
		}
		else if(pev->movetype == MOVETYPE_FLY){
				if (m_teleportorigin.z - pev->origin.z > 80){
					if(pev->velocity.z < 64){
					pev->velocity.z += 16;
					}
				}
				if (m_teleportorigin.z - pev->origin.z < -80){
					if(pev->velocity.z > -64){
					pev->velocity.z -= 16;
					}
				}

				if(m_hEnemy != NULL){
						if(( pev->origin - m_hEnemy->pev->origin).Length2D() > 128){
							m_speedvec = (m_hEnemy->pev->origin - pev->origin).Normalize() * 128;
							pev->velocity.x = m_speedvec.x;
							pev->velocity.y = m_speedvec.y;
						}
				}
		}
	}

	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 300;
	}

	if(pev->health > 0 && m_iBravery >= 1){
		if(m_iBravery == 3){
			TraceResult trace;
			UTIL_TraceHull(m_teleportorigin, m_teleportorigin, dont_ignore_monsters, human_hull, ENT(pev),&trace);
			if (trace.fStartSolid != 0)
			{
				UTIL_SetOrigin( pev, m_teleportorigin2 );
				//pev->origin = m_teleportorigin2;
			}
			m_iBravery = 0;
		}
		else{
				m_iBravery += 1;
		}
	}
}



//=========================================================
// DieSound
//=========================================================

void CMajo :: DeathSound( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "majo/die1.wav", 1, ATTN_NORM, 0, m_voicePitch); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "majo/die2.wav", 1, ATTN_NORM, 0, m_voicePitch); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "majo/die3.wav", 1, ATTN_NORM, 0, m_voicePitch); break;
	}
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CMajo :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


void CMajo::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CMajo :: SetYawSpeed ( void )
{
	pev->yaw_speed = 240;
}


//=========================================================
// start task
//=========================================================
void CMajo :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CBaseMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CMajo :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case 1:
		{
			if(m_flNextFireTime <= gpGlobals->time){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			Vector  vangle;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			Vector Sog = vecSpitOffset + gpGlobals->v_forward * 4;
			// do stuff for this event.
			//AttackSound();

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, Sog);
			WRITE_BYTE(3);
			WRITE_COORD( Sog.x);	// pos
			WRITE_COORD( Sog.y);	
			WRITE_COORD( Sog.z);
			WRITE_SHORT(majo_fire);
			WRITE_BYTE(5);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
			MESSAGE_END();

			CMajoFireBall::Shoot( pev, vecSpitOffset, vecSpitDir * 1800 );
			m_flNextFireTime = gpGlobals->time + 2;
			}
		}
		break;

		case 3:
		{
			ClearSchedule();
		}
		break;

		case 4:
		{
			pev->gravity = 1;
		}
		break;

		case 5:
		{
			pev->body = 0;
		}
		break;

		case 2:
		{
			m_teleportorigin2 = pev->origin;
			TraceResult trace;
			UTIL_TraceHull(m_teleportorigin, m_teleportorigin, dont_ignore_monsters, human_hull, ENT(pev),&trace);
			if (trace.fStartSolid == 0)
			{
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "majo/teleport.wav", 1, ATTN_NORM, 0, 100 );

				// ��ըspr
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION);		
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 16 );
				WRITE_SHORT( g_sFireball );
				WRITE_BYTE( 10  );
				WRITE_BYTE( 15  );
				WRITE_BYTE( 4 );
				MESSAGE_END();

				// ���������
					MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
						WRITE_BYTE( TE_BEAMCYLINDER );
						WRITE_COORD( pev->origin.x);
						WRITE_COORD( pev->origin.y);
						WRITE_COORD( pev->origin.z + 16);
						WRITE_COORD( pev->origin.x);
						WRITE_COORD( pev->origin.y);
						WRITE_COORD( pev->origin.z + 16 + ( 194 / 2 ) / .2); // reach damage radius over .3 seconds
						WRITE_SHORT( m_iSpriteTexture );
						WRITE_BYTE( 0 ); // startframe
						WRITE_BYTE( 0 ); // framerate
						WRITE_BYTE( 2 ); // life
						WRITE_BYTE( 16 );  // width
						WRITE_BYTE( 0 );   // noise
						WRITE_BYTE( 0   );
						WRITE_BYTE( 255 );
						WRITE_BYTE( 0  );
						WRITE_BYTE( 255 ); //brightness
						WRITE_BYTE( 0 );		// speed
					MESSAGE_END();

				// ��ըspr2
				MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, m_teleportorigin );
				WRITE_BYTE( TE_EXPLOSION);		
				WRITE_COORD( m_teleportorigin.x );
				WRITE_COORD( m_teleportorigin.y );
				WRITE_COORD( m_teleportorigin.z);
				WRITE_SHORT( g_sFireball );
				WRITE_BYTE( 10  );
				WRITE_BYTE( 15  );
				WRITE_BYTE( 4 );
				MESSAGE_END();

			//pev->origin = m_teleportorigin;
			UTIL_SetOrigin( pev, m_teleportorigin );
			pev->angles.y = m_hEnemy->pev->angles.y;
			pev->velocity.z += 10;
			m_iBravery = 1;
			}
			else{
			ClearBits( pev->flags, FL_ONGROUND );
			pev->velocity.x += RANDOM_LONG(-300,300);
			pev->velocity.y += RANDOM_LONG(-300,300);
			pev->velocity.z += 350;
			}
			m_flNextTeleportTime = gpGlobals->time + 5.0;
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
BOOL CMajo :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if(pev->waterlevel >= 2){
		return FALSE;
	}

	if (m_flNextFireTime > gpGlobals->time)
	{
		return FALSE;
	}

	if ( flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		return TRUE;
	}
	
	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CMajo :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if (m_flNextTeleportTime > gpGlobals->time || pev->movetype == MOVETYPE_FLY){
	return FALSE;
	}

	float dist = 2048;

	if ( m_HenemyEnemyMe >= 1 && m_hEnemy != NULL && flDist < dist ){

			if(RANDOM_LONG(0,1) && pev->health == pev->max_health){
				m_flNextTeleportTime = gpGlobals->time + 0.5;
				return FALSE;
			}

			Vector trace_origin;
			UTIL_MakeVectors ( pev->angles );
			TraceResult trace;
			trace_origin = m_hEnemy->pev->origin + gpGlobals->v_forward * RANDOM_LONG(128,256);
			UTIL_TraceHull(trace_origin, trace_origin, dont_ignore_monsters, head_hull, ENT(pev),&trace);

			if ( trace.flFraction == 1.0 )
			{
				m_teleportorigin = trace_origin;
				m_flNextTeleportTime = gpGlobals->time + 0.5;
				return TRUE;
			}
			else{
				m_flNextTeleportTime = gpGlobals->time + 0.3;
				return FALSE;
			}
	}
	else if ( m_hasenemy_nosee >= 30 && m_hEnemy != NULL && flDist < 2048 ){

			if(!RANDOM_LONG(0,4)){
				m_flNextTeleportTime = gpGlobals->time + 0.5;
				return FALSE;
			}

			Vector trace_origin;
			UTIL_MakeVectors ( pev->angles );
			TraceResult trace;
			trace_origin = m_hEnemy->pev->origin + gpGlobals->v_forward * RANDOM_LONG(128,256);
			UTIL_TraceHull(trace_origin, trace_origin, dont_ignore_monsters, head_hull, ENT(pev),&trace);

			if ( trace.flFraction == 1.0 )
			{
				m_teleportorigin = trace_origin;
				m_flNextTeleportTime = gpGlobals->time + 0.5;
				return TRUE;
			}
			else{
				m_flNextTeleportTime = gpGlobals->time + 0.3;
				return FALSE;
			}
	}

	return FALSE;
}


//=========================================================
// Spawn
//=========================================================
void CMajo :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/majo.mdl");
	UTIL_SetSize(pev, Vector( -16, -16, 0 ), Vector( 16, 16, 64 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 90;
	}
	else{
	pev->health			= 75;
	}

	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_voicePitch		= RANDOM_LONG( 100, 110 );

	MonsterInit();

	m_killed_exp = 90;
	m_rpgms_level = 50;
	m_chase_mode = 3;
	m_chase_failed_max = 2;
//	m_forcefuckdoor  = TRUE;

	m_flNextTeleportTime = gpGlobals->time + 2.0;
	pev->netname = MAKE_STRING( "Witch" );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMajo :: Precache()
{
	int i;
	g_sFireball = PRECACHE_MODEL("sprites/b-tele1.spr");
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );

	PRECACHE_MODEL("models/majo.mdl");
	PRECACHE_MODEL("models/fire_bolt.mdl");
	
	PRECACHE_SOUND("majo/flame_hitwall.wav");
	PRECACHE_SOUND("majo/teleport.wav");

	majo_fire =  PRECACHE_MODEL("sprites/anim_spr9.spr");

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CMajo :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if ( (bitsDamageType & DMG_FREEZE) || (bitsDamageType & DMG_ENERGYBEAM)
	|| (bitsDamageType & DMG_DARK) || (bitsDamageType & DMG_SHOCK) 
	|| (bitsDamageType & DMG_ENERGYBLAST) || (bitsDamageType & DMG_SONIC)){
		flDamage *= 0.4;
	}
	else if ((bitsDamageType & DMG_BURN)){
		flDamage *= 0.2;
	}

	return CSquadMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


void CMajo::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CSquadMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


//=========================================================
// AI Schedules Specific to this monster
//=========================================================



// primary range attack
Task_t	tlMajoAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slMajoAttack1[] =
{
	{ 
		tlMajoAttack1,
		ARRAYSIZE ( tlMajoAttack1 ), 
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_HEAVY_DAMAGE, 

		bits_SOUND_DANGER,
		"Slave Range Attack1"
	},
};

Task_t	tlMajoAttack2[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_RANGE_ATTACK2,		(float)0		},
};

Schedule_t	slMajoAttack2[] =
{
	{ 
		tlMajoAttack2,
		ARRAYSIZE ( tlMajoAttack2 ), 
		0,
		0,
		"Slave Range Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES( CMajo )
{
	slMajoAttack1,
	slMajoAttack2,
	slMajoRepel,
	slMajoRepelAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CMajo, CSquadMonster );


//=========================================================
//=========================================================
Schedule_t *CMajo :: GetSchedule( void )
{

	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		// repel down a rope, 
		if ( m_MonsterState == MONSTERSTATE_COMBAT && m_freezetime == 0 )
			return GetScheduleOfType ( SCHED_MAJO_REPEL_ATTACK );
		else
			return GetScheduleOfType ( SCHED_MAJO_REPEL );

	}

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

			if (!HasConditions( bits_COND_CAN_RANGE_ATTACK1 )
			&& !HasConditions( bits_COND_CAN_RANGE_ATTACK2 )
			&& HasConditions( bits_COND_ENEMY_FACING_ME ))
			{
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}

		break;
	}
	return CSquadMonster::GetSchedule( );
}


Schedule_t *CMajo :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_FAIL:
		if (HasConditions( bits_COND_CAN_RANGE_ATTACK2 ))
		{
		return CSquadMonster :: GetScheduleOfType( SCHED_RANGE_ATTACK2 ); ;
		}
		break;
	case SCHED_RANGE_ATTACK1:
		return slMajoAttack1;
	case SCHED_RANGE_ATTACK2:
		return slMajoAttack2;

	case SCHED_MAJO_REPEL:
		{
			return &slMajoRepel[ 0 ];
		}
	case SCHED_MAJO_REPEL_ATTACK:
		{
			return &slMajoRepelAttack[ 0 ];
		}

	}
	return CSquadMonster :: GetScheduleOfType( Type );
}




