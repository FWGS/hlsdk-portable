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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CBryan : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_bryan, CBryan );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBryan :: Classify ( void )
{
	return	CLASS_PLAYER;
}

void CBryan :: RunAI( void )
{
	CBaseMonster :: RunAI();
}

void CBryan :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_WALK:
		iSequence = LookupActivity ( NewActivity );
		break;
	case ACT_IDLE:
		iSequence = LookupActivity ( NewActivity );
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBryan :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

int CBryan :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return 0;
}

void CBryan::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if(pev->frags == 1 && pev->deadflag == DEAD_NO){
		pev->skin = 1;
		pev->sequence = 1;
		ResetSequenceInfo( );
		pev->frame = 0;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBryan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	CBaseMonster::HandleAnimEvent( pEvent );
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CBryan :: ISoundMask ( void )
{
	return	NULL;
}

void CBryan :: Precache()
{
	PRECACHE_MODEL("models/bryan_origin.mdl");
}	


//=========================================================
// Spawn
//=========================================================
void CBryan :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/bryan_origin.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 300;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
	m_die_dont_alert = 1;
//	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
//	{
//		pev->solid = SOLID_NOT;
//		pev->takedamage = DAMAGE_NO;
//	}
}