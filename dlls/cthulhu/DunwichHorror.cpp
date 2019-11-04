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

#include "bm.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	DH_AE_STEP1				1		// Footstep left
#define	DH_AE_STEP2				2		// Footstep right
#define	DH_AE_STEP3				3		// Footstep back left
#define	DH_AE_STEP4				4		// Footstep back right
#define DH_AE_SACK					5		// Sack slosh
#define DH_AE_DEATHSOUND			6		// Death sound

#define	DH_AE_MELEE_ATTACKBR		8		// Leg attack
#define	DH_AE_MELEE_ATTACKBL		9		// Leg attack
#define	DH_AE_MELEE_ATTACK1		10		// Leg attack
//#define DH_AE_MORTAR_ATTACK1		11		// Launch a mortar
//#define DH_AE_LAY_CRAB				12		// Lay a headcrab
#define DH_AE_JUMP_FORWARD			13		// Jump up and forward
#define DH_AE_SCREAM				14		// alert sound
#define DH_AE_PAIN_SOUND			15		// pain sound
#define DH_AE_ATTACK_SOUND			16		// attack sound
//#define DH_AE_BIRTH_SOUND			17		// birth sound
#define DH_AE_EARLY_TARGET			50		// Fire target early



// User defined conditions
#define bits_COND_NODE_SEQUENCE			( bits_COND_SPECIAL1 )		// pev->netname contains the name of a sequence to play

// Attack distance constants
#define	DH_ATTACKDIST		170
//#define DH_MORTARDIST		800
//#define DH_MAXCHILDREN		20			// Max # of live headcrab children


//#define bits_MEMORY_CHILDPAIR		(bits_MEMORY_CUSTOM1)
#define bits_MEMORY_ADVANCE_NODE	(bits_MEMORY_CUSTOM2)
#define bits_MEMORY_COMPLETED_NODE	(bits_MEMORY_CUSTOM3)
#define bits_MEMORY_FIRED_NODE		(bits_MEMORY_CUSTOM4)

int gDHSpitSprite, gDHSpitDebrisSprite;


class CDunwichHorror : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void Activate( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	void		RunTask( Task_t *pTask );
	void		StartTask( Task_t *pTask );
	Schedule_t	*GetSchedule( void );
	Schedule_t	*GetScheduleOfType( int Type );
	void		TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void		SetActivity ( Activity NewActivity );
	virtual void MonsterThink( void );

	void NodeStart( int iszNextNode );
	void NodeReach( void );
	BOOL ShouldGoToNode( void );

	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

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
		CInfoBM *pTarget = (CInfoBM *)(CBaseEntity *)m_hTargetEnt;
		if ( pTarget )
		{
			return pTarget->m_preSequence;
		}
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
	
	//void LaunchMortar( void );

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

	static const char *pSackSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pFootSounds[];

	CUSTOM_SCHEDULES;

private:
	float	m_nodeTime;
	float	m_mortarTime;
	float	m_painSoundTime;

	BOOL	m_bVisible;
	BOOL	m_bBecomingVisible; 
};
LINK_ENTITY_TO_CLASS( monster_dunwichhorror, CDunwichHorror );

TYPEDESCRIPTION	CDunwichHorror::m_SaveData[] = 
{
	DEFINE_FIELD( CDunwichHorror, m_nodeTime, FIELD_TIME ),
	DEFINE_FIELD( CDunwichHorror, m_mortarTime, FIELD_TIME ),
	DEFINE_FIELD( CDunwichHorror, m_painSoundTime, FIELD_TIME ),
	DEFINE_FIELD( CDunwichHorror, m_bVisible, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDunwichHorror, m_bBecomingVisible, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CDunwichHorror, CBaseMonster );

const char *CDunwichHorror::pSackSounds[] = 
{
	"gonarch/gon_sack1.wav",
	"gonarch/gon_sack2.wav",
	"gonarch/gon_sack3.wav",
};

const char *CDunwichHorror::pDeathSounds[] = 
{
	"gonarch/gon_die1.wav",
};

const char *CDunwichHorror::pAttackSounds[] = 
{
	"gonarch/gon_attack1.wav",
	"gonarch/gon_attack2.wav",
	"gonarch/gon_attack3.wav",
};
const char *CDunwichHorror::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDunwichHorror::pAlertSounds[] = 
{
	"gonarch/gon_alert1.wav",
	"gonarch/gon_alert2.wav",
	"gonarch/gon_alert3.wav",
};

const char *CDunwichHorror::pPainSounds[] = 
{
	"gonarch/gon_pain2.wav",
	"gonarch/gon_pain4.wav",
	"gonarch/gon_pain5.wav",
};

const char *CDunwichHorror::pFootSounds[] = 
{
	"gonarch/gon_step1.wav",
	"gonarch/gon_step2.wav",
	"gonarch/gon_step3.wav",
};



void CDunwichHorror :: KeyValue( KeyValueData *pkvd )
{
#if 0
	if (FStrEq(pkvd->szKeyName, "volume"))
	{
		m_volume = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
#endif
	if (FStrEq(pkvd->szKeyName, "visible"))
	{
		m_bVisible = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDunwichHorror :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDunwichHorror :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 100;
		break;
	default:
		ys = 90;
	}
	pev->yaw_speed = ys;
}

void CDunwichHorror :: MonsterThink ( void )
{
	// if we are becoming visible, then update the fx amount...
	if (m_bBecomingVisible)
	{
		// update the fx amount
		pev->renderamt += 5;
		// don't go above 255
		if (pev->renderamt > 255) pev->renderamt = 255;
		// if we are at max fx amount, then
		if (pev->renderamt == 255)
		{
			// stop updating the fx amount
			m_bBecomingVisible = FALSE;
		}
	}

	CBaseMonster :: MonsterThink ();
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CDunwichHorror :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case DH_AE_MELEE_ATTACKBR:
		case DH_AE_MELEE_ATTACKBL:
		case DH_AE_MELEE_ATTACK1:
		{
			Vector forward, right;

			UTIL_MakeVectorsPrivate( pev->angles, forward, right, NULL );

			Vector center = pev->origin + forward * 128;
			Vector mins = center - Vector( 64, 64, 0 );
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
				pHurt->TakeDamage( pev, pev, gSkillData.bigmommaDmgSlash, DMG_CRUSH | DMG_SLASH );
				pHurt->pev->punchangle.x = 15;
				switch( pEvent->event )
				{
					case DH_AE_MELEE_ATTACKBR:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0,0,250) - (right * 200);
					break;

					case DH_AE_MELEE_ATTACKBL:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 150) + Vector(0,0,250) + (right * 200);
					break;

					case DH_AE_MELEE_ATTACK1:
						pHurt->pev->velocity = pHurt->pev->velocity + (forward * 220) + Vector(0,0,200);
					break;
				}

				pHurt->pev->flags &= ~FL_ONGROUND;
				EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackHitSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
		}
		break;
		
		case DH_AE_SCREAM:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pAlertSounds );
			break;
		
		case DH_AE_PAIN_SOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds );
			break;
		
		case DH_AE_ATTACK_SOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_WEAPON, pAttackSounds );
			break;

		case DH_AE_SACK:
			if ( RANDOM_LONG(0,100) < 30 )
				EMIT_SOUND_ARRAY_DYN( CHAN_BODY, pSackSounds );
			break;

		case DH_AE_DEATHSOUND:
			EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pDeathSounds );
			break;

		case DH_AE_STEP1:		// Footstep left
		case DH_AE_STEP3:		// Footstep back left
			EMIT_SOUND_ARRAY_DYN( CHAN_ITEM, pFootSounds );
			break;

		case DH_AE_STEP4:		// Footstep back right
		case DH_AE_STEP2:		// Footstep right
			EMIT_SOUND_ARRAY_DYN( CHAN_BODY, pFootSounds );
			break;

//		case DH_AE_MORTAR_ATTACK1:
//			LaunchMortar();
//			break;

		case DH_AE_JUMP_FORWARD:
			ClearBits( pev->flags, FL_ONGROUND );

			UTIL_SetOrigin (this, pev->origin + Vector ( 0 , 0 , 1) );// take her off ground so engine doesn't instantly reset onground 
			UTIL_MakeVectors ( pev->angles );

			pev->velocity = (gpGlobals->v_forward * 200) + gpGlobals->v_up * 500;
			break;

		case DH_AE_EARLY_TARGET:
			{
				CBaseEntity *pTarget = m_hTargetEnt;
				if ( pTarget && pTarget->pev->message )
					FireTargets( STRING(pTarget->pev->message), this, this, USE_TOGGLE, 0 );
				Remember( bits_MEMORY_FIRED_NODE );
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CDunwichHorror :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if ( ptr->iHitgroup != 1 )
	{
		// didn't hit the sack?
		
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,10) < 1) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2) );
			pev->dmgtime = gpGlobals->time;
		}

		flDamage = 0.1;// don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	else if ( gpGlobals->time > m_painSoundTime )
	{
		m_painSoundTime = gpGlobals->time + RANDOM_LONG(1, 3);
		EMIT_SOUND_ARRAY_DYN( CHAN_VOICE, pPainSounds );
	}


	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


int CDunwichHorror :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_POWDER_IBN && !m_bVisible)
	{
		m_bVisible = TRUE;
		m_bBecomingVisible = TRUE;
		return 0;
	}

	// if we are invisible, we are invulnerable
	if (!m_bVisible) return 0;

	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( bitsDamageType & DMG_ACID )
		flDamage = 0;

	if ( !HasMemory(bits_MEMORY_PATH_FINISHED) )
	{
		if ( pev->health <= flDamage )
		{
			pev->health = flDamage + 1;
			Remember( bits_MEMORY_ADVANCE_NODE | bits_MEMORY_COMPLETED_NODE );
			ALERT( at_aiconsole, "BM: Finished node health!!!\n" );
		}
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


/*
void CDunwichHorror::LaunchMortar( void )
{
	m_mortarTime = gpGlobals->time + RANDOM_FLOAT( 2, 15 );
	
	Vector startPos = pev->origin;
	startPos.z += 180;
	Vector vecLaunch = g_vecZero;

	if (m_pCine) // is a scripted_action making me shoot?
	{
		if (m_hTargetEnt != NULL) // don't check m_fTurnType- bigmomma can fire in any direction.
		{
			vecLaunch = VecCheckSplatToss( pev, startPos, m_hTargetEnt->pev->origin, RANDOM_FLOAT( 150, 500 ) );
		}
		if (vecLaunch == g_vecZero)
		{
			vecLaunch = pev->movedir;
		}
	}
	else
	{
		vecLaunch = pev->movedir;
	}

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pSackSounds), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
	CBMortar *pBomb = CBMortar::Shoot( edict(), startPos, vecLaunch );
	pBomb->pev->gravity = 1.0;
	MortarSpray( startPos, Vector(0,0,1), gDHSpitSprite, 24 );
}
*/

//=========================================================
// Spawn
//=========================================================
void CDunwichHorror :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/big_mom.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= 150 * gSkillData.bigmommaHealthFactor;
	pev->view_ofs		= Vector ( 0, 0, 128 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_bBecomingVisible	= FALSE;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDunwichHorror :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/big_mom.mdl");

	PRECACHE_SOUND_ARRAY( pSackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pFootSounds );

	// TEMP: Squid
	PRECACHE_MODEL("sprites/mommaspit.spr");// spit projectile.
	gDHSpitSprite = PRECACHE_MODEL("sprites/mommaspout.spr");// client side spittle.
	gDHSpitDebrisSprite = PRECACHE_MODEL("sprites/mommablob.spr" );

	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}	


void CDunwichHorror::Activate( void )
{
	if ( m_hTargetEnt == NULL )
		Remember( bits_MEMORY_ADVANCE_NODE );	// Start 'er up

	CBaseMonster::Activate();
}


void CDunwichHorror::NodeStart( int iszNextNode )
{
	pev->netname = iszNextNode;

	CBaseEntity *pTarget = NULL;

	if ( pev->netname )
	{
		pTarget = UTIL_FindEntityByTargetname( NULL, STRING(pev->netname) );
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


void CDunwichHorror::NodeReach( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;

	Forget( bits_MEMORY_ADVANCE_NODE );

	if ( !pTarget )
		return;

	if ( pTarget->pev->health )
		pev->max_health = pev->health = pTarget->pev->health * gSkillData.bigmommaHealthFactor;

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
BOOL CDunwichHorror::CheckMeleeAttack1( float flDot, float flDist )
{
	if (flDot >= 0.7)
	{
		if ( flDist <= DH_ATTACKDIST )
			return TRUE;
	}
	return FALSE;
}


// Lay a crab
BOOL CDunwichHorror::CheckMeleeAttack2( float flDot, float flDist )
{
	return FALSE;
}


// Mortar launch
BOOL CDunwichHorror::CheckRangeAttack1( float flDot, float flDist )
{
	/*
	if ( flDist <= DH_MORTARDIST && m_mortarTime < gpGlobals->time )
	{
		CBaseEntity *pEnemy = m_hEnemy;

		if ( pEnemy )
		{
			Vector startPos = pev->origin;
			startPos.z += 180;
			pev->movedir = VecCheckSplatToss( pev, startPos, pEnemy->BodyTarget( pev->origin ), RANDOM_FLOAT( 150, 500 ) );
			if ( pev->movedir != g_vecZero )
				return TRUE;
		}
	}
	*/
	return FALSE;
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

enum
{
	SCHED_DH_NODE = LAST_COMMON_SCHEDULE + 1,
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


Task_t	tlDHBigNode[] =
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

Schedule_t	slDHBigNode[] =
{
	{ 
		tlDHBigNode,
		ARRAYSIZE ( tlDHBigNode ), 
		0,
		0,
		"DH Big Node"
	},
};


Task_t	tlDHNodeFail[] =
{
	{ TASK_NODE_DELAY,			(float)10		},	// Try to do something else for 10 seconds
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slDHNodeFail[] =
{
	{ 
		tlDHNodeFail,
		ARRAYSIZE ( tlDHNodeFail ), 
		0,
		0,
		"DH NodeFail"
	},
};


DEFINE_CUSTOM_SCHEDULES( CDunwichHorror )
{
	slDHBigNode,
	slDHNodeFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDunwichHorror, CBaseMonster );




Schedule_t *CDunwichHorror::GetScheduleOfType( int Type )
{
	switch( Type )
	{
		case SCHED_DH_NODE:
			return slDHBigNode;
		break;

		case SCHED_NODE_FAIL:
			return slDHNodeFail;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


BOOL CDunwichHorror::ShouldGoToNode( void )
{
	if ( HasMemory( bits_MEMORY_ADVANCE_NODE ) )
	{
		if ( m_nodeTime < gpGlobals->time )
			return TRUE;
	}
	return FALSE;
}

// Overridden to make Dunwich Horror jump on command; the model doesn't support it otherwise.
void CDunwichHorror :: SetActivity ( Activity NewActivity )
{
	int	iSequence;

	if (NewActivity == ACT_HOP)
	{
		iSequence = LookupSequence( "jump" );
	}
	else
	{
		iSequence = LookupActivity ( NewActivity );
	}

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			// don't reset frame between walk and run
			if ( !(m_Activity == ACT_WALK || m_Activity == ACT_RUN) || !(NewActivity == ACT_WALK || NewActivity == ACT_RUN))
				pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_aiconsole, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
	// In case someone calls this with something other than the ideal activity
	m_IdealActivity = m_Activity;
}

Schedule_t *CDunwichHorror::GetSchedule( void )
{
	if ( ShouldGoToNode() )
	{
		return GetScheduleOfType( SCHED_DH_NODE );
	}

	return CBaseMonster::GetSchedule();
}


void CDunwichHorror::StartTask( Task_t *pTask )
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
		if ( m_hTargetEnt->pev->spawnflags & SF_INFOBM_WAIT )
			ALERT( at_aiconsole, "BM: Wait at node %s forever\n", STRING(pev->netname) );
		else
			ALERT( at_aiconsole, "BM: Wait at node %s for %.2f\n", STRING(pev->netname), GetNodeDelay() );
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
					if ( pTarget->pev->spawnflags & SF_INFOBM_RUN )
						act = ACT_RUN;

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
void CDunwichHorror::RunTask( Task_t *pTask )
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
		if ( m_hTargetEnt != NULL && (m_hTargetEnt->pev->spawnflags & SF_INFOBM_WAIT) )
			return;

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


#endif
