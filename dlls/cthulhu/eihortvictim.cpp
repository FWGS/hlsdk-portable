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

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is EihortVictim dying for scripted sequences?
#define		EIHORTVICTIM_AE_BURST		( 2 )
#define		EIHORTVICTIM_AE_PAIN		( 3 )

#include "EihortVictim.h"

LINK_ENTITY_TO_CLASS( monster_eihortvictim, CEihortVictim );

TYPEDESCRIPTION	CEihortVictim::m_SaveData[] = 
{
	DEFINE_FIELD( CEihortVictim, m_painTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CEihortVictim, CTalkMonster );


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CEihortVictim :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CEihortVictim :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_NONE;
}
		
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CEihortVictim :: SetYawSpeed ( void )
{
	pev->yaw_speed = 0;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CEihortVictim :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case EIHORTVICTIM_AE_BURST:
		Burst();
		break;

	case EIHORTVICTIM_AE_PAIN:
		if (IsTalking()) return;
		if (RANDOM_LONG(0,2) == 0)
		{
			//play scientist pain sound 1, 4, 5, 7 or 10
			PainSound();
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

void CEihortVictim :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	// victims do not turn
	case TASK_TLK_IDEALYAW:
		TaskComplete();
		break;
	default:
		CTalkMonster::StartTask( pTask );	
	}
}

//=========================================================
// Spawn
//=========================================================
void CEihortVictim :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/eihort_victim.mdl");
	//UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,8));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= 100;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	MonsterInit();
	SetUse( NULL );
	// this monster cannot turn
	SetYawSpeed();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CEihortVictim :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/eihort_victim.mdl");

	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");
	PRECACHE_SOUND("scientist/sci_pain7.wav");
	PRECACHE_SOUND("scientist/sci_pain10.wav");

	UTIL_PrecacheOther( "monster_babycrab" );

	// every new EihortVictim must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void CEihortVictim :: TalkInit()
{
	CTalkMonster::TalkInit();

	// EihortVictim speech group names (group names are in sentences.txt)

	if (!m_iszSpeakAs)
	{
		//m_szGrp[TLK_ANSWER]		=	"BA_ANSWER";
		//m_szGrp[TLK_QUESTION]	=	"BA_QUESTION";
		//m_szGrp[TLK_IDLE]		=	"BA_IDLE";
		//m_szGrp[TLK_STARE]		=	"BA_STARE";
		m_szGrp[TLK_STOP] =		"BA_STOP";

		m_szGrp[TLK_NOSHOOT] =	"BA_SCARED";
		//m_szGrp[TLK_HELLO] =	"BA_HELLO";

		//m_szGrp[TLK_PHELLO] =	NULL;	//"BA_PHELLO";		// UNDONE
		//m_szGrp[TLK_PIDLE] =	NULL;	//"BA_PIDLE";			// UNDONE
		//m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

		//m_szGrp[TLK_SMELL] =	"BA_SMELL";
	
		m_szGrp[TLK_WOUND] =	"BA_WOUND";
		m_szGrp[TLK_MORTAL] =	"BA_MORTAL";
	}

	// get voice for head - just one EihortVictim voice for now
	m_voicePitch = 100;
}

int CEihortVictim :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	return 0;

	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void CEihortVictim :: PainSound ( void )
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch ( RANDOM_LONG(0,4) )
	{
	case 0:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, m_voicePitch );	
		break;
	case 1:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, m_voicePitch );	
		break;
	case 2:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, m_voicePitch );	
		break;
	case 3:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain7.wav", 1, ATTN_NORM, 0, m_voicePitch );	
		break;
	case 4:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "scientist/sci_pain10.wav", 1, ATTN_NORM, 0, m_voicePitch );	
		break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CEihortVictim :: DeathSound ( void )
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "EihortVictim/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "EihortVictim/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "EihortVictim/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CEihortVictim::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	Burst();
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CEihortVictim::Burst()
{
	pev->body = 1;
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM);		
	CGib::SpawnRandomGibs( pev, 4, 1 );	// throw some human gibs.

	// now make some baby headcrabs
	for (int x = 0; x <= 1; x++)
	{
		for (int y = 0; y <= 1; y++)
		{
			//CBaseEntity *pChild = CBaseEntity::Create( "monster_babycrab", pev->origin + 24*Vector(x-0.5,y-0.5,0) + Vector(0,0,1), pev->angles, edict() );
			CBaseEntity *pChild = CBaseEntity::Create( "monster_babycrab", pev->origin + 24*Vector(x-0.5,y-0.5,0) + Vector(0,0,8), pev->angles, edict() );
			pChild->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND;
			pChild->pev->spawnflags |= SF_MONSTER_NO_YELLOW_BLOBS;
		}
	}
}





