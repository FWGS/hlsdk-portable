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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"

int SonicRings;
extern cvar_t cwc;
int bravery = 0; //He is brave... for now.

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CChrisChan : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Killed( entvars_t *pevAttacker, int iGib );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	//Schedule_t *GetSchedule( void ); // Handles some schedules 

	void IdleSound( void );
	void PainSound( void );
	void AlertSound( void );

	CUSTOM_SCHEDULES;

	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
};
LINK_ENTITY_TO_CLASS( monster_chrischan, CChrisChan );

//====================
// AI BULL
//===================

Task_t	tlCWCHide[] =
{
	//{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slCWCHide[] =
{
	{ 
		tlCWCHide,
		ARRAYSIZE ( tlCWCHide ), 
		bits_COND_SEE_FEAR,
		bits_SOUND_DANGER,
		"CWCHide"
	},
};

DEFINE_CUSTOM_SCHEDULES( CChrisChan )
{
slCWCHide,
};

const char *CChrisChan::pIdleSounds[] = 
{
	"cwc/cwc_idle1.wav",
	"cwc/cwc_idle2.wav",
	"cwc/cwc_idle3.wav",
	"cwc/cwc_idle4.wav",
};

const char *CChrisChan::pAlertSounds[] =
{
	"cwc/cwc_alert1.wav",
	"cwc/cwc_alert2.wav",
	"cwc/cwc_alert3.wav",
	"cwc/cwc_alert4.wav"

};

const char *CChrisChan::pPainSounds[] = 
{
	"cwc/cwc_pain1.wav",
	"cwc/cwc_pain2.wav",
	"cwc/cwc_pain3.wav",
	"cwc/cwc_pain4.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CChrisChan :: Classify ( void )
{
	if (cwc.value == 1)
	{
	return	CLASS_HUMAN_PASSIVE;
	}
	else if (cwc.value == 0)
	{
	return	CLASS_CWC;
	}
	return CLASS_HUMAN_PASSIVE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChrisChan :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 360;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChrisChan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CChrisChan :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/chrischan.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_LAVENDER; //There's part of why I chose this.
	pev->health			= CBaseMonster::GetHealth( 251, 3 );
	pev->view_ofs		= Vector ( 0, 0, 0 );// poBsition of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChrisChan :: Precache()
{
	PRECACHE_SOUND("cwc/death.wav");

	PRECACHE_MODEL("models/chrischan.mdl");
	SonicRings = PRECACHE_MODEL("sprites/s2ring.spr");// client side spittle of coins!

	int i;
	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND(pIdleSounds[i]);
	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND(pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

void CChrisChan :: IdleSound( void )
{
	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
}

void CChrisChan :: AlertSound( void )
{
	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
}

void CChrisChan :: PainSound( void )
{
	// Play a random pain sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, 100 );
}

void CChrisChan::Killed( entvars_t *pevAttacker, int iGib )
{
	//Blows up in coins, disappears. Scott Pilgrim and River City Ransom-esque.
	const Vector position = pev->origin;
	const Vector &direction = Vector(0,0,1);
	int count = RANDOM_LONG(45,160);

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "cwc/death.wav", 1, ATTN_NORM );

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( position.x);	// pos
		WRITE_COORD( position.y);	
		WRITE_COORD( position.z);	
		WRITE_COORD( direction.x);	// dir
		WRITE_COORD( direction.y);	
		WRITE_COORD( direction.z);	
		WRITE_SHORT( SonicRings );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( 130 );			// speed
		WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
	MESSAGE_END();
	UTIL_Remove(this);
}
//=========================================================
// TakeDamage - overridden for chris-chan so we can simulate
// trolling via attackers. Once he is attacked, he will switch. Like the real cwc.
//=========================================================
int CChrisChan :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (bravery == 0)
	{
		bravery = RANDOM_LONG(0,1); //Roll the Dice, for Bravery, simulating fight or flight.
	}

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		Remember( bits_MEMORY_PROVOKED );
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

IMPLEMENT_CUSTOM_SCHEDULES( CChrisChan, CBaseMonster );
