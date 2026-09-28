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
// bullSciHead - big, spotty tentacle-mouthed meanie.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"weapons.h"
#include	"player.h"

#define		SciHead_SPRINT_DIST	256 // how close the SciHead has to get before starting to sprint and refusing to swerve

int			   iSciHeadSpitSprite;
extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SciHead_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SciHead_SMELLFOOD,
	SCHED_SciHead_SEECRAB,
	SCHED_SciHead_EAT,
	SCHED_SciHead_SNIFF_AND_EAT,
	SCHED_SciHead_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_SciHead_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// BullSciHead's spit projectile
//=========================================================
class CSciHeadSpit2 : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( SciHeadspit2, CSciHeadSpit2 );

TYPEDESCRIPTION	CSciHeadSpit2::m_SaveData[] = 
{
	DEFINE_FIELD( CSciHeadSpit2, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CSciHeadSpit2, CBaseEntity );

void CSciHeadSpit2:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "sciheadspit" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/bigspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CSciHeadSpit2::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void CSciHeadSpit2::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CSciHeadSpit2 *pSpit = GetClassPtr( (CSciHeadSpit2 *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( &CSciHeadSpit2::Animate );
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSciHeadSpit2 :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}


	Vector vecSrc = pev->origin;
	Vector vecEnd	= vecSrc + pev->velocity * 10;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	int tex = (int)TEXTURETYPE_Trace(&tr, pev->origin, vecEnd);
	int surface = (int)SURFACETYPE_Trace(&tr, pev->origin, vecEnd,Classify(),0);
	FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, pev->origin, surface, BULLET_CROWBAR, (float)tex );


	if ( !pOther->pev->takedamage )
	{
		// make some flecks
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);	
			WRITE_COORD( tr.vecEndPos.z);	
			WRITE_COORD( tr.vecPlaneNormal.x);	// dir
			WRITE_COORD( tr.vecPlaneNormal.y);	
			WRITE_COORD( tr.vecPlaneNormal.z);	
			WRITE_SHORT( iSciHeadSpitSprite );	// model
			WRITE_BYTE ( 5 );			// count
			WRITE_BYTE ( 30 );			// speed
			WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
		MESSAGE_END();
		// make a splat on the wall
	//	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0,1));
	}
	else
	{
		int dmg3;
		dmg3 = 20;

		ClearMultiDamage( );
		pOther->TraceAttack(pev, dmg3, gpGlobals->v_forward, &tr, DMG_GENERIC ); 
		ApplyMultiDamage( pev, pev );
	}

	SetThink ( &CSciHeadSpit2::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}



//=========================================================
// BullSciHead's Claw projectile
//=========================================================
class CSciHeadClaw : public CBaseEntity
{
public:
	void Spawn( void );

	void EXPORT cathink( void );
};

LINK_ENTITY_TO_CLASS( sciheadclaw, CSciHeadClaw );

void CSciHeadClaw:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "sciheadclaw" );
	
	pev->health = 30;
	pev->solid = SOLID_NOT;

	pev->body = 1;

	pev->takedamage = DAMAGE_NO;

	pev->sequence = 5;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;

	SET_MODEL(ENT(pev), "models/scihead_boss.mdl");

	UTIL_SetSize( pev, Vector( -16, -16, 0), Vector(16, 16, 16) );

	SetThink ( &CSciHeadClaw::cathink );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CSciHeadClaw::cathink( void )
{
	if(pev->health <= 0){
		entvars_t *pevOwner = NULL;
		if ( pev->owner ){
			pevOwner = VARS(pev->owner);
			if(pevOwner){
			pevOwner->armortype = 0;
			}
		}

		SetThink ( NULL );
		UTIL_Remove( this );
	
		return;
	}
	else{
		pev->nextthink = gpGlobals->time + 0.1;

		pev->health--;
		if(pev->health == 26){
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "war3/impalehit.wav", 1, 0.5 );
			if ( pev->enemy ){
				if(pev->enemy->v.deadflag == DEAD_NO){
				pev->velocity = (pev->enemy->v.origin - pev->origin).Normalize() * 40;
				pev->velocity.z = 0;
				}
			}
		}

		if(pev->health <= 25 && pev->health >= 10 && pev->frags == 0){
			UTIL_Sparks( pev->origin );
			::RadiusDamage_limit( pev->origin, pev, pev, 8, 64, CLASS_HUMAN_ASS, DMG_SLASH | DMG_CONCUSSION);			
		}
	}
}



//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BSciHead_AE_SPIT		( 1 )
#define		BSciHead_AE_BITE		( 2 )
#define		BSciHead_AE_BLINK		( 3 )
#define		BSciHead_AE_TAILWHIP	( 4 )
#define		BSciHead_AE_HOP		( 5 )
#define		BSciHead_AE_THROW		( 6 )

class CSciHead : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  ISoundMask( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void PainSound( void );
	void DeathSound( void );

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -30, -30, 0 );
		pev->absmax = pev->origin + Vector( 30, 30, 52 );
	}

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];


	float m_flNextSpitTime;// last time the bullSciHead used the spit attack.
	float m_flNextShakeTime;
};
LINK_ENTITY_TO_CLASS( monster_scihead_boss, CSciHead );

TYPEDESCRIPTION	CSciHead::m_SaveData[] = 
{
	DEFINE_FIELD( CSciHead, m_flNextShakeTime, FIELD_TIME ),
	DEFINE_FIELD( CSciHead, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSciHead, CBaseMonster );


//=========================================================
// TakeDamage - overridden for bullSciHead so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CSciHead :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	m_alert = 100;

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CSciHead :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if(pev->armortype == 1){
	return FALSE;
	}

	if ( flDist <= 2048 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullSciHead is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CSciHead :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	return FALSE;
}

BOOL CSciHead :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if(pev->armortype == 1){
	return FALSE;
	}

	if ( flDist <= 2048 && flDot >= 0.5 && gpGlobals->time >= m_flNextShakeTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( !m_hEnemy->IsPlayer() )
			{
				return FALSE;
			}
		}

		m_facing_fucking_mode = 1;
		return TRUE;
	}

	m_facing_fucking_mode = 0;
	return FALSE;
}  

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CSciHead :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSciHead :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CSciHead :: DeathSound ( void )
{
	EMIT_SOUND( ENT(pev), CHAN_STREAM, "scihead/scihead_die.wav", 1, 0.6 );	
}

//=========================================================
// PainSound 
//=========================================================
void CSciHead :: PainSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_STREAM, "scihead/scihead_pain1.wav", 1, 0.6 );	
		break;
	case 1:	
		EMIT_SOUND( ENT(pev), CHAN_STREAM, "scihead/scihead_pain2.wav", 1, 0.6 );	
		break;
	case 2:	
		EMIT_SOUND( ENT(pev), CHAN_STREAM, "scihead/scihead_pain3.wav", 1, 0.6 );	
		break;
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSciHead :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSciHead :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{
			if(m_hEnemy != NULL){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = ( gpGlobals->v_forward * 36 + gpGlobals->v_up * 28 );		
			vecSpitOffset = ( pev->origin + vecSpitOffset );
			vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );

			// spew the spittle temporary ents.
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecSpitOffset.x);	// pos
				WRITE_COORD( vecSpitOffset.y);	
				WRITE_COORD( vecSpitOffset.z);	
				WRITE_COORD( vecSpitDir.x);	// dir
				WRITE_COORD( vecSpitDir.y);	
				WRITE_COORD( vecSpitDir.z);	
				WRITE_SHORT( iSciHeadSpitSprite );	// model
				WRITE_BYTE ( 15 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 25 );			// noise ( client will divide by 100 )
			MESSAGE_END();

			CSciHeadSpit2::Shoot( pev, vecSpitOffset, vecSpitDir * 1250 );

			m_flNextSpitTime = gpGlobals->time + RANDOM_FLOAT( 2.0, 6.0 );
			}
		}
		break;

		case 2:
		{
			if(m_hEnemy != NULL){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = ( gpGlobals->v_forward * 36 + gpGlobals->v_up * 28 );		
			vecSpitOffset = ( pev->origin + vecSpitOffset );
			vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );

			CSciHeadSpit2::Shoot( pev, vecSpitOffset, vecSpitDir * 1250 );
			}
		}
		break;

		case 3:
		{
			if ( m_hEnemy != NULL ){
				if(m_hEnemy->pev->deadflag == DEAD_NO){
				Vector pos = m_hEnemy->pev->origin;
				pos.z = pev->origin.z - 70;

				CBaseEntity *pEnt = CBaseEntity::Create( "sciheadclaw", pos, pev->angles, edict() );
				pEnt->pev->enemy = m_hEnemy->edict();
				}
			}
		}
		break;

		case 4:
		{
			m_flNextShakeTime = gpGlobals->time + RANDOM_FLOAT( 2.0, 6.0 );
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

void CSciHead::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// Spawn
//=========================================================
void CSciHead :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/scihead_boss.mdl");
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->effects		= 0;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 1200;
	}
	else{
	pev->health			= 1000;
	}

	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpitTime = gpGlobals->time;
	m_flNextShakeTime = gpGlobals->time;

	MonsterInit();

	m_ignoredamage = 1;

	m_headdef	= 2;

	m_EyeMod	= 1;//�޷���������

	pev->gravity = 1.6;

	m_killed_exp = 1000;
	m_rpgms_level = 60;
	m_is_the_boss = TRUE;
	pev->netname = MAKE_STRING( "Sci.HeadSquid" );

	m_singdelay_max = 1;
	m_singdelay_use = m_singdelay_max;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSciHead :: Precache()
{
	PRECACHE_MODEL("models/scihead_boss.mdl");
	
	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.
	
	iSciHeadSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("war3/impalehit.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("scihead/scihead_die.wav");

	PRECACHE_SOUND("scihead/scihead_pain1.wav");
	PRECACHE_SOUND("scihead/scihead_pain2.wav");
	PRECACHE_SOUND("scihead/scihead_pain3.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

}	


//========================================================
// RunAI - overridden for bullSciHead because there are things
// that need to be checked every think.
//========================================================
void CSciHead :: RunAI ( void )
{
	if(pev->armortype == 1){
		if(pev->sequence != LookupActivity ( ACT_EAT )){
		SetActivity ( ACT_EAT );
		}
	}

	if ( m_hEnemy != NULL && pev->weapons == 0){
		pev->weapons = 1;//����������̨
	}

	// first, do base class stuff
	CBaseMonster :: RunAI();
}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlSciHeadRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSciHeadRangeAttack1[] =
{
	{ 
		tlSciHeadRangeAttack1,
		ARRAYSIZE ( tlSciHeadRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"SciHead Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlSciHeadChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},// !!!OEM - this will stop nasty SciHead oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slSciHeadChaseEnemy[] =
{
	{ 
		tlSciHeadChaseEnemy1,
		ARRAYSIZE ( tlSciHeadChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"SciHead Chase Enemy"
	},
};

Task_t tlSciHeadHurtHop[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_SciHead_HOPTURN,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// in case SciHead didn't turn all the way in the air.
};

Schedule_t slSciHeadHurtHop[] =
{
	{
		tlSciHeadHurtHop,
		ARRAYSIZE ( tlSciHeadHurtHop ),
		0,
		0,
		"SciHeadHurtHop"
	}
};

Task_t tlSciHeadSeeCrab[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0			},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_EXCITED	},
	{ TASK_FACE_ENEMY,			(float)0			},
};

Schedule_t slSciHeadSeeCrab[] =
{
	{
		tlSciHeadSeeCrab,
		ARRAYSIZE ( tlSciHeadSeeCrab ),
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE,
		0,
		"SciHeadSeeCrab"
	}
};

// SciHead walks to something tasty and eats it.
Task_t tlSciHeadEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the SciHead can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSciHeadEat[] =
{
	{
		tlSciHeadEat,
		ARRAYSIZE( tlSciHeadEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SciHeadEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the SciHead. This schedule plays a sniff animation before going to the source of food.
Task_t tlSciHeadSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the SciHead can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSciHeadSniffAndEat[] =
{
	{
		tlSciHeadSniffAndEat,
		ARRAYSIZE( tlSciHeadSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"SciHeadSniffAndEat"
	}
};

// SciHead does this to stinky things. 
Task_t tlSciHeadWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the SciHead can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR},
	{ TASK_EAT,						(float)50				},// keeps SciHead from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slSciHeadWallow[] =
{
	{
		tlSciHeadWallow,
		ARRAYSIZE( tlSciHeadWallow ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"SciHeadWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES( CSciHead ) 
{
	slSciHeadRangeAttack1,
	slSciHeadChaseEnemy,
	slSciHeadHurtHop,
	slSciHeadSeeCrab,
	slSciHeadEat,
	slSciHeadSniffAndEat,
	slSciHeadWallow
};

IMPLEMENT_CUSTOM_SCHEDULES( CSciHead, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CSciHead :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				return GetScheduleOfType ( SCHED_SciHead_HURTHOP );
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound		*pSound;

				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_SciHead_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_SciHead_EAT );
			}

			if ( HasConditions(bits_COND_SMELL) )
			{
				// there's something stinky. 
				CSound		*pSound;

				pSound = PBestScent();
				if ( pSound )
					return GetScheduleOfType( SCHED_SciHead_WALLOW);
			}

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CSciHead :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slSciHeadRangeAttack1[ 0 ];
		break;
	case SCHED_SciHead_HURTHOP:
		return &slSciHeadHurtHop[ 0 ];
		break;
	case SCHED_SciHead_SEECRAB:
		return &slSciHeadSeeCrab[ 0 ];
		break;
	case SCHED_SciHead_EAT:
		return &slSciHeadEat[ 0 ];
		break;
	case SCHED_SciHead_SNIFF_AND_EAT:
		return &slSciHeadSniffAndEat[ 0 ];
		break;
	case SCHED_SciHead_WALLOW:
		return &slSciHeadWallow[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slSciHeadChaseEnemy[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}
