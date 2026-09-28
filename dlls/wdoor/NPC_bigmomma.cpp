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
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

//=========================================================
// monster template
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"decals.h"
#include	"weapons.h"
#include	"game.h"
#include	"soundent.h"

//=========================================================
// Mortar shot entity
//=========================================================
class CBMortar : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );

	static CBMortar *Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
	void EXPORT Animate( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS( bmortar, CBMortar );

TYPEDESCRIPTION	CBMortar::m_SaveData[] = 
{
	DEFINE_FIELD( CBMortar, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBMortar, CBaseEntity );


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	BIG_AE_STEP1				1		// Footstep left
#define	BIG_AE_STEP2				2		// Footstep right
#define	BIG_AE_STEP3				3		// Footstep back left
#define	BIG_AE_STEP4				4		// Footstep back right
#define BIG_AE_SACK					5		// Sack slosh
#define BIG_AE_DEATHSOUND			6		// Death sound

#define	BIG_AE_MELEE_ATTACKBR		8		// Leg attack
#define	BIG_AE_MELEE_ATTACKBL		9		// Leg attack
#define	BIG_AE_MELEE_ATTACK1		10		// Leg attack
#define BIG_AE_MORTAR_ATTACK1		11		// Launch a mortar
#define BIG_AE_LAY_CRAB				12		// Lay a headcrab
#define BIG_AE_JUMP_FORWARD			13		// Jump up and forward
#define BIG_AE_SCREAM				14		// alert sound
#define BIG_AE_PAIN_SOUND			15		// pain sound
#define BIG_AE_ATTACK_SOUND			16		// attack sound
#define BIG_AE_BIRTH_SOUND			17		// birth sound
#define BIG_AE_EARLY_TARGET			50		// Fire target early



// User defined conditions
#define bits_COND_NODE_SEQUENCE			( bits_COND_SPECIAL1 )		// pev->netname contains the name of a sequence to play

// Attack distance constants
#define	BIG_ATTACKDIST		170
#define BIG_MORTARDIST		1024
#define BIG_MAXCHILDREN		20			// Max # of live headcrab children


#define bits_MEMORY_CHILDPAIR		(bits_MEMORY_CUSTOM1)
#define bits_MEMORY_ADVANCE_NODE	(bits_MEMORY_CUSTOM2)
#define bits_MEMORY_COMPLETED_NODE	(bits_MEMORY_CUSTOM3)
#define bits_MEMORY_FIRED_NODE		(bits_MEMORY_CUSTOM4)

int gSpitSprite, gSpitDebrisSprite;
Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight );
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count );


// UNDONE:	
//
#define BIG_CHILDCLASS		"monster_headcrab"

class CBigMomma : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int IgnoreConditions ( void );

	void		RunTask( Task_t *pTask );
	void		StartTask( Task_t *pTask );
	Schedule_t	*GetSchedule( void );
	Schedule_t	*GetScheduleOfType( int Type );
	void		TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	void NodeStart( int iszNextNode );
	void NodeReach( void );
	BOOL ShouldGoToNode( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void LayHeadcrab( void );

	int GetNodeSequence( void )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if ( pTarget )
		{
			return pTarget->pev->netname;	// netname holds node sequence
		}
		return 0;
	}


	int GetNodePresequence( void )
	{
		return 0;
	}

	float GetNodeDelay( void )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if ( pTarget )
		{
			return pTarget->pev->speed;	// Speed holds node delay
		}
		return 0;
	}

	float GetNodeRange( void )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if ( pTarget )
		{
			return pTarget->pev->scale;	// Scale holds node delay
		}
		return 1e6;
	}

	float GetNodeYaw( void )
	{
		CBaseEntity *pTarget = m_hTargetEnt;
		if ( pTarget )
		{
			if ( pTarget->pev->angles.y != 0 )
				return pTarget->pev->angles.y;
		}
		return pev->angles.y;
	}
	
	// Restart the crab count on each new level
	void OverrideReset( void )
	{
		m_crabCount = 0;
	}

	void DeathNotice( entvars_t *pevChild );

	BOOL CanLayCrab( void ) 
	{ 
		if ( m_crabTime < gpGlobals->time && m_crabCount < BIG_MAXCHILDREN && pev->weapons > 0 )
		{
			// Don't spawn crabs inside each other
			Vector mins = pev->origin - Vector( 32, 32, 0 );
			Vector maxs = pev->origin + Vector( 32, 32, 0 );

			CBaseEntity *pList[2];
			int count = UTIL_EntitiesInBox( pList, 2, mins, maxs, FL_MONSTER );
			for ( int i = 0; i < count; i++ )
			{
				if ( pList[i] != this )	// Don't hurt yourself!
					return FALSE;
			}
			return TRUE;
		}

		return FALSE;
	}

	void LaunchMortar( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -95, -95, 0 );
		pev->absmax = pev->origin + Vector( 95, 95, 190 );
	}

	BOOL CheckMeleeAttack1( float flDot, float flDist );	// Slash
	BOOL CheckMeleeAttack2( float flDot, float flDist );	// Lay a crab
	BOOL CheckRangeAttack1( float flDot, float flDist );	// Mortar launch

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	static const char *pChildDieSounds[];
	static const char *pSackSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackSounds[];
	static const char *pAttackHitSounds[];
	static const char *pBirthSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pFootSounds[];

	CUSTOM_SCHEDULES;

private:
	float	m_nodeTime;
	float	m_crabTime;
	float	m_mortarTime;
	float	m_painSoundTime;
	int		m_crabCount;
};
LINK_ENTITY_TO_CLASS( monster_bigmomma, CBigMomma );

TYPEDESCRIPTION	CBigMomma::m_SaveData[] = 
{
	DEFINE_FIELD( CBigMomma, m_nodeTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_crabTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_mortarTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_painSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CBigMomma, m_crabCount, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBigMomma, CBaseMonster );

const char *CBigMomma::pChildDieSounds[] = 
{
	"gonarch/gon_childdie1.wav",
	"gonarch/gon_childdie2.wav",
	"gonarch/gon_childdie3.wav",
};

const char *CBigMomma::pSackSounds[] = 
{
	"gonarch/gon_sack1.wav",
	"gonarch/gon_sack2.wav",
	"gonarch/gon_sack3.wav",
};

const char *CBigMomma::pDeathSounds[] = 
{
	"gonarch/gon_die1.wav",
};

const char *CBigMomma::pAttackSounds[] = 
{
	"gonarch/gon_attack1.wav",
	"gonarch/gon_attack2.wav",
	"gonarch/gon_attack3.wav",
};
const char *CBigMomma::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CBigMomma::pBirthSounds[] = 
{
	"gonarch/gon_birth1.wav",
	"gonarch/gon_birth2.wav",
	"gonarch/gon_birth3.wav",
};

const char *CBigMomma::pAlertSounds[] = 
{
	"gonarch/gon_alert1.wav",
	"gonarch/gon_alert2.wav",
	"gonarch/gon_alert3.wav",
};

const char *CBigMomma::pPainSounds[] = 
{
	"gonarch/gon_pain2.wav",
	"gonarch/gon_pain4.wav",
	"gonarch/gon_pain5.wav",
};

const char *CBigMomma::pFootSounds[] = 
{
	"gonarch/gon_step1.wav",
	"gonarch/gon_step2.wav",
	"gonarch/gon_step3.wav",
};



void CBigMomma :: KeyValue( KeyValueData *pkvd )
{
#if 0
	if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
#endif
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBigMomma :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBigMomma :: SetYawSpeed ( void )
{
	pev->yaw_speed = 150;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CBigMomma :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int		power;

	power = 75;

	switch( pEvent->event )
	{
		case BIG_AE_MELEE_ATTACKBR:
		case BIG_AE_MELEE_ATTACKBL:
		case BIG_AE_MELEE_ATTACK1:
		{
			Vector forward, right;

			UTIL_MakeVectorsPrivate( pev->angles, forward, right, NULL );

			Vector center = pev->origin + forward * 128;
			Vector mins = center - Vector( 64, 64, 64 );
			Vector maxs = center + Vector( 64, 64, 64 );

			CBaseEntity *pList[8];
			int count = UTIL_EntitiesInBox( pList, 8, mins, maxs, FL_MONSTER|FL_CLIENT );
			CBaseEntity *pHurt = NULL;

			for ( int i = 0; i < count && !pHurt; i++ )
			{
				if ( pList[i] != this )
				{
					if ( pList[i]->pev->owner != edict() )
						pHurt = pList[i];
				}
			}
					
			if ( pHurt )
			{
				pHurt->TakeDamage( pev, pev, power, DMG_SLASH );
				pHurt->pev->punchangle.x = 15;
/*
			TraceResult tr = UTIL_GetGlobalTrace( );
			if (tr.pHit == pHurt->edict())
			{
				int tex = (int)TEXTURETYPE_Trace(&tr, center, tr.vecEndPos);
				int surface = (int)SURFACETYPE_Trace(&tr, center, tr.vecEndPos,Classify(),1);
				FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, center, surface, BULLET_CROWBAR, (float)tex );
				ClearMultiDamage( );
				pHurt->TraceAttack(pev, 15, gpGlobals->v_forward, &tr, DMG_SLASH ); 
				ApplyMultiDamage( pev, pev );
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod3.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
*/
				switch( pEvent->event )
				{
					case BIG_AE_MELEE_ATTACKBR:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0,0,250) - (right * 200);
					break;

					case BIG_AE_MELEE_ATTACKBL:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0,0,250) + (right * 200);
					break;

					case BIG_AE_MELEE_ATTACK1:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 220) + Vector(0,0,200);
					break;
				}

				pHurt->pev->flags &= ~FL_ONGROUND;
				EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;
		
		case BIG_AE_SCREAM:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds );
			break;
		
		case BIG_AE_PAIN_SOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds );
			break;
		
		case BIG_AE_ATTACK_SOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pAttackSounds );
			break;

		case BIG_AE_BIRTH_SOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_BODY, pBirthSounds );
			break;

		case BIG_AE_SACK:
			if ( RANDOM_LONG(0,100) < 30 )
				EMIT_SOUND_ARRAY_DYN( CHAN_BODY, pSackSounds );
			break;

		case BIG_AE_DEATHSOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pDeathSounds );
			break;

		case BIG_AE_STEP1:		// Footstep left
		case BIG_AE_STEP3:		// Footstep back left
			EMIT_SOUND_ARRAY_DYN( CHAN_ITEM, pFootSounds );
			break;

		case BIG_AE_STEP4:		// Footstep back right
		case BIG_AE_STEP2:		// Footstep right
			EMIT_SOUND_ARRAY_DYN( CHAN_BODY, pFootSounds );
			break;

		case BIG_AE_MORTAR_ATTACK1:
			LaunchMortar();
			EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pAttackSounds );
			break;

		case BIG_AE_LAY_CRAB:
			LayHeadcrab();
			break;

		case BIG_AE_JUMP_FORWARD:
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (pev, pev->origin + Vector ( 0 , 0 , 1) );// take him off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			pev->velocity = (gpGlobals->v_forward * 200) + gpGlobals->v_up * 500;
			break;

		case 20:
			m_mortarTime = gpGlobals->time + RANDOM_FLOAT( 4, 10 );
			break;

		case BIG_AE_EARLY_TARGET:
			{
				CBaseEntity *pTarget = m_hTargetEnt;
				if ( pTarget && pTarget->pev->message )
					FireTargets( STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0 );
				Remember( bits_MEMORY_FIRED_NODE );
			}
			break;

		case 51:
			{
				if(pev->takedamage){
				CallGibMonster();
				EMIT_SOUND(ENT(pev), CHAN_BODY, "common/bodysplat.wav", 1, ATTN_NORM);	
				SpawnBlood(Center(), BloodColor(), 250);
				}
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CBigMomma :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( ptr->iHitgroup != 10 )
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20) )
		{
			pev->dmgtime = gpGlobals->time;
			
			if (RANDOM_LONG(0, 1))
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
			else
				EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

			UTIL_Sparks(ptr->vecEndPos);
		}

		flDamage -= 80;//���������ߵĻ���

		if(flDamage < 1){
		return;
		}
	}
	else if ( gpGlobals->time > m_painSoundTime )
	{
		if(pev->deadflag == DEAD_NO){
		m_painSoundTime = gpGlobals->time + RANDOM_LONG(1, 2);
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds );
		}
	}


	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


int CBigMomma :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( bitsDamageType & DMG_ACID ){
	return 0;
	}

	if(m_EyeMod == 0){
	m_EyeMod = 2;//���˽���ս��״̬��Ŀ��ģʽ
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CBigMomma :: LayHeadcrab( void )
{
	pev->weapons -= 1;

	CBaseEntity *pChild = CBaseEntity::Create( BIG_CHILDCLASS, pev->origin, pev->angles, edict() );
	pChild->pev->owner = ENT( pev );

	pChild->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;

	m_crabTime = gpGlobals->time + RANDOM_FLOAT( 5, 10 );

	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,100), ignore_monsters, edict(), &tr);
	UTIL_DecalTrace( &tr, DECAL_MOMMABIRTH );

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBirthSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	m_crabCount++;
}



void CBigMomma::DeathNotice( entvars_t *pevChild )
{
	if ( m_crabCount > 0 )		// Some babies may cross a transition, but we reset the count then
		m_crabCount--;
	if ( IsAlive() )
	{
		// Make the "my baby's dead" noise!
		EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pChildDieSounds );
	}
}


void CBigMomma::LaunchMortar( void )
{
	Vector startPos = pev->origin;
	startPos.z += 180;

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pSackSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	CBMortar *pBomb = CBMortar::Shoot( edict(), startPos, pev->movedir );
	MortarSpray( startPos, Vector(0,0,1), gSpitSprite, 24 );
}

//=========================================================
// Spawn
//=========================================================
void CBigMomma :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/big_mom.mdl");
	UTIL_SetSize( pev, Vector( -40, -40, 0 ), Vector( 40, 40, 160 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 1800;
	}
	else{
	pev->health			= 1500;
	}

	pev->gravity        = 2.0;

	pev->view_ofs		= Vector ( 0, 0, 128 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	
	MonsterInit();

	m_canbarnacle_mode = 0;

	m_killed_exp = 600;
	m_rpgms_level = 60;

	m_chase_mode = 2;
	m_chase_failed_max = 4;
	pev->netname = MAKE_STRING( "Gonarch" );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBigMomma :: Precache()
{
	PRECACHE_MODEL("models/big_mom.mdl");

	PRECACHE_SOUND_ARRAY( pChildDieSounds );
	PRECACHE_SOUND_ARRAY( pSackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pBirthSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pFootSounds );

	UTIL_PrecacheOther( BIG_CHILDCLASS );

	// TEMP: Squid
	PRECACHE_MODEL("sprites/mommaspit.spr");// spit projectile.
	gSpitSprite = PRECACHE_MODEL("sprites/mommaspout.spr");// client side spittle.
	gSpitDebrisSprite = PRECACHE_MODEL("sprites/mommablob.spr" );

	PRECACHE_SOUND( "weapons/cluster_explode.wav" );
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}	


void CBigMomma::Activate( void )
{
	if ( m_hTargetEnt == NULL )
		Remember( bits_MEMORY_ADVANCE_NODE );	// Start 'er up
}


void CBigMomma::NodeStart( int iszNextNode )
{
	pev->netname = iszNextNode;

	CBaseEntity *pTarget = NULL;

	if ( pev->netname )
	{
		edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME ( NULL, STRING(pev->netname) );

		if ( !FNullEnt(pentTarget) )
			pTarget = Instance( pentTarget );
	}


	if ( !pTarget )
	{
		ALERT( at_aiconsole, "BM: Finished the path!!\n" );
		Remember( bits_MEMORY_PATH_FINISHED );
		return;
	}
	Remember( bits_MEMORY_ON_PATH );
	m_hTargetEnt = pTarget;
}


void CBigMomma::NodeReach( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;

	Forget( bits_MEMORY_ADVANCE_NODE );

	if ( !pTarget )
		return;

	if ( pTarget->pev->health )
		pev->max_health = pev->health = pTarget->pev->health * 150;

	if ( !HasMemory( bits_MEMORY_FIRED_NODE ) )
	{
		if ( pTarget->pev->message )
			FireTargets( STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0 );
	}
	Forget( bits_MEMORY_FIRED_NODE );

	pev->netname = pTarget->pev->target;
	if ( pTarget->pev->health == 0 )
		Remember( bits_MEMORY_ADVANCE_NODE );	// Move on if no health at this node
}


	// Slash
BOOL CBigMomma::CheckMeleeAttack1( float flDot, float flDist )
{
	if (flDot >= 0.7)
	{
		if ( flDist <= BIG_ATTACKDIST )
			return TRUE;
	}
	return FALSE;
}


// Lay a crab
BOOL CBigMomma::CheckMeleeAttack2( float flDot, float flDist )
{
	return CanLayCrab();
}


// Mortar launch
BOOL CBigMomma::CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist <= 2000 && m_mortarTime < gpGlobals->time )
	{
		CBaseEntity *pEnemy = m_hEnemy;

		if ( pEnemy )
		{
			Vector startPos = pev->origin;
			startPos.z += 180;
			pev->movedir = VecCheckSplatToss( pev, startPos, pEnemy->BodyTarget_o( pev->origin ), RANDOM_LONG(400,500));
			if ( pev->movedir != g_vecZero )
				return TRUE;
		}
	}
	return FALSE;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	SCHED_BIG_NODE = LAST_COMMON_SCHEDULE + 1,
	SCHED_NODE_FAIL,
};

enum
{
	TASK_MOVE_TO_NODE_RANGE = LAST_COMMON_TASK + 1,	// Move within node range
	TASK_FIND_NODE,									// Find my next node
	TASK_PLAY_NODE_PRESEQUENCE,						// Play node pre-script
	TASK_PLAY_NODE_SEQUENCE,						// Play node script
	TASK_PROCESS_NODE,								// Fire targets, etc.
	TASK_WAIT_NODE,									// Wait at the node
	TASK_NODE_DELAY,								// Delay walking toward node for a bit. You've failed to get there
	TASK_NODE_YAW,									// Get the best facing direction for this node
};


Task_t	tlBigNode[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_NODE_FAIL },
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FIND_NODE,			(float)0		},	// Find my next node
	{ TASK_PLAY_NODE_PRESEQUENCE,(float)0		},	// Play the pre-approach sequence if any
	{ TASK_MOVE_TO_NODE_RANGE,	(float)0		},	// Move within node range
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_NODE_YAW,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_WAIT_NODE,			(float)0		},	// Wait for node delay
	{ TASK_PLAY_NODE_SEQUENCE,	(float)0		},	// Play the sequence if one exists
	{ TASK_PROCESS_NODE,		(float)0		},	// Fire targets, etc.
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slBigNode[] =
{
	{ 
		tlBigNode,
		ARRAYSIZE ( tlBigNode ), 
		0,
		0,
		"Big Node"
	},
};


Task_t	tlNodeFail[] =
{
	{ TASK_NODE_DELAY,			(float)10		},	// Try to do something else for 10 seconds
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slNodeFail[] =
{
	{ 
		tlNodeFail,
		ARRAYSIZE ( tlNodeFail ), 
		0,
		0,
		"NodeFail"
	},
};

DEFINE_CUSTOM_SCHEDULES( CBigMomma )
{
	slBigNode,
	slNodeFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CBigMomma, CBaseMonster );




Schedule_t *CBigMomma::GetScheduleOfType( int Type )
{
	switch( Type )
	{
		case SCHED_BIG_NODE:
			return slBigNode;
		break;

		case SCHED_NODE_FAIL:
			return slNodeFail;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


BOOL CBigMomma::ShouldGoToNode( void )
{
	if ( HasMemory( bits_MEMORY_ADVANCE_NODE ) )
	{
		if ( m_nodeTime < gpGlobals->time )
			return TRUE;
	}
	return FALSE;
}



Schedule_t *CBigMomma::GetSchedule( void )
{
	if ( ShouldGoToNode() )
	{
		return GetScheduleOfType( SCHED_BIG_NODE );
	}

	return CBaseMonster::GetSchedule();
}

int CBigMomma::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	iIgnore |= bits_COND_LIGHT_DAMAGE;

	return iIgnore;
	
}

void CBigMomma::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FIND_NODE:
		{
			CBaseEntity *pTarget = m_hTargetEnt;
			if ( !HasMemory( bits_MEMORY_ADVANCE_NODE ) )
			{
				if ( pTarget )
					pev->netname = m_hTargetEnt->pev->target;
			}
			NodeStart( pev->netname );
			TaskComplete();
			ALERT( at_aiconsole, "BM: Found node %s\n", STRING(pev->netname) );
		}
		break;

	case TASK_NODE_DELAY:
		m_nodeTime = gpGlobals->time + pTask->flData;
		TaskComplete();
		ALERT( at_aiconsole, "BM: FAIL! Delay %.2f\n", pTask->flData );
		break;

	case TASK_PROCESS_NODE:
		ALERT( at_aiconsole, "BM: Reached node %s\n", STRING(pev->netname) );
		NodeReach();
		TaskComplete();
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		{
			int sequence;
			if ( pTask->iTask == TASK_PLAY_NODE_SEQUENCE )
				sequence = GetNodeSequence();
			else
				sequence = GetNodePresequence();

			ALERT( at_aiconsole, "BM: Playing node sequence %s\n", STRING(sequence) );
			if ( sequence )
			{
				sequence = LookupSequence( STRING( sequence ) );
				if ( sequence != -1 )
				{
					pev->sequence = sequence;
					pev->frame = 0;
					ResetSequenceInfo( );
					ALERT( at_aiconsole, "BM: Sequence %s\n", STRING(GetNodeSequence()) );
					return;
				}
			}
			TaskComplete();
		}
		break;

	case TASK_NODE_YAW:
		pev->ideal_yaw = GetNodeYaw();
		TaskComplete();
		break;

	case TASK_WAIT_NODE:
		m_flWait = gpGlobals->time + GetNodeDelay();
		break;


	case TASK_MOVE_TO_NODE_RANGE:
		{
			CBaseEntity *pTarget = m_hTargetEnt;
			if ( !pTarget )
				TaskFail();
			else
			{
				if ( (pTarget->pev->origin - pev->origin).Length() < GetNodeRange() )
					TaskComplete();
				else
				{
					Activity act = ACT_WALK;

					m_vecMoveGoal = pTarget->pev->origin;
					if ( !MoveToTarget( act, 2 ) )
					{
						TaskFail();
					}
				}
			}
		}
		ALERT( at_aiconsole, "BM: Moving to node %s\n", STRING(pev->netname) );

		break;

	case TASK_MELEE_ATTACK1:
		// Play an attack sound here
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAttackSounds), 1.0, ATTN_NORM, 0, PITCH_NORM );
		CBaseMonster::StartTask( pTask );
		break;

	default: 
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CBigMomma::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_MOVE_TO_NODE_RANGE:
		{
			float distance;

			if ( m_hTargetEnt == NULL )
				TaskFail();
			else
			{
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( (distance < GetNodeRange()) || MovementIsComplete() )
				{
					ALERT( at_aiconsole, "BM: Reached node!\n" );
					TaskComplete();
					RouteClear();		// Stop moving
				}
			}
		}

		break;

	case TASK_WAIT_NODE:
		if ( gpGlobals->time > m_flWaitFinished )
			TaskComplete();
		ALERT( at_aiconsole, "BM: The WAIT is over!\n" );
		break;

	case TASK_PLAY_NODE_PRESEQUENCE:
	case TASK_PLAY_NODE_SEQUENCE:
		if ( m_fSequenceFinished )
		{
			m_Activity = ACT_RESET;
			TaskComplete();
		}
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}



Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight )
{
	TraceResult		tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = g_psv_gravity->value;

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,maxHeight), ignore_monsters, ENT(pev), &tr);
	vecApex = tr.vecEndPos;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, ENT(pev), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	float height = (vecApex.z - vecSpot1.z) - 15;
	// How fast does the grenade need to travel to reach that height given gravity?
	float speed = sqrt( 2 * flGravity * height );
	
	// How much time does it take to get there?
	float time = speed / flGravity;
	vecGrenadeVel = (vecSpot2 - vecSpot1);
	vecGrenadeVel.z = 0;
	float distance = vecGrenadeVel.Length();
	
	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel * ( 0.5 / time );
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}




// ---------------------------------
//
// Mortar
//
// ---------------------------------
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( position.x);	// pos
		WRITE_COORD( position.y);	
		WRITE_COORD( position.z);	
		WRITE_COORD( direction.x);	// dir
		WRITE_COORD( direction.y);	
		WRITE_COORD( direction.z);	
		WRITE_SHORT( spriteModel );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( 130 );			// speed
		WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
	MESSAGE_END();
}


// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar:: Spawn( void )
{
	Precache( );

	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING( "bmortar" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/mommaspit.spr");
	pev->frame = 0;
	pev->scale = 2.5;

	pev->gravity = 1.0;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->dmgtime = gpGlobals->time + 0.4;

	SetThink ( &CBMortar::Animate );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBMortar::Animate( void )
{
	pev->nextthink = gpGlobals->time + 0.1;
	CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * 0.5, 400, 0.2 );
	//if ( gpGlobals->time > pev->dmgtime )
	//{
	//	pev->dmgtime = gpGlobals->time + 0.2;
	//	MortarSpray( pev->origin, -pev->velocity.Normalize(), gSpitSprite, 3 );
	//}
	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

CBMortar *CBMortar::Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity )
{
	CBMortar *pSpit = GetClassPtr( (CBMortar *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = pOwner;

	return pSpit;
}

void CBMortar :: Precache()
{
	gSpitSprite = PRECACHE_MODEL("sprites/mommaspout.spr");
}

void CBMortar::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	int		power;

	power = 150;

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

	if ( pOther->IsBSPModel() )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_MOMMASPLAT);
	}
	else
	{
		tr.vecEndPos = pev->origin;
		tr.vecPlaneNormal = -1 * pev->velocity.Normalize();
	}

	MortarSpray( tr.vecEndPos, tr.vecPlaneNormal, gSpitSprite, 24 );

	entvars_t *pevOwner = NULL;
	if ( pev->owner )
		pevOwner = VARS(pev->owner);

	RadiusDamage( pev->origin, pev, pevOwner, power, 275, CLASS_NONE, DMG_ACID );
	FX_Explosion( tr.vecEndPos + (tr.vecPlaneNormal * 15), EXPLOSION_CLUSTERBABY );

	UTIL_Remove( this );
}

#endif
