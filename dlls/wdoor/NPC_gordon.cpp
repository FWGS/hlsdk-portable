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

class CGordon : public CBaseMonster
{
public:
	void RunAI( void );
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	void SetActivity ( Activity NewActivity );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
};
LINK_ENTITY_TO_CLASS( monster_gordon, CGordon );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGordon :: Classify ( void )
{
	return	CLASS_PLAYER;
}

void CGordon :: RunAI( void )
{
	CBaseMonster :: RunAI();
}

void CGordon :: SetActivity ( Activity NewActivity )
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
void CGordon :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

int CGordon :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return 1;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGordon :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
		Vector vecShootOrigin,vecdir;

		UTIL_MakeVectors(pev->angles);
		GetAttachment( 0, vecShootOrigin,vecdir);

		Vector vecShootDir = gpGlobals->v_forward;

		CBaseEntity *pRocket = CBaseEntity::Create( "rpg_rocket", vecShootOrigin, pev->angles, edict() );
		pRocket->pev->velocity = gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
		pRocket->pev->angles = UTIL_VecToAngles (vecShootDir * 2000) + Vector(5,0,0);
		pRocket->pev->armortype = 578;

		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "weapons/rocketfire1.wav", 1, 0.6 );
		break;
		}

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGordon :: ISoundMask ( void )
{
	return	NULL;
}

void CGordon :: Precache()
{
	PRECACHE_MODEL("models/gordon.mdl");
}	


//=========================================================
// Spawn
//=========================================================
void CGordon :: Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gordon.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= 300;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

//	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
//	{
//		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
//	}
}