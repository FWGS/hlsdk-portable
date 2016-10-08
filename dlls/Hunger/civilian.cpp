/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"
#include	"scientist.h"

#define		NUM_CIVILIAN_HEADS		5 // four heads available for civilian model
enum { HEAD_GLASSES = 0, HEAD_FRANKLIN = 1, HEAD_ECHELON_OFFICER = 2, HEAD_SLICK = 3, HEAD_ORDELY = 4, };

class CCivilian : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
};

LINK_ENTITY_TO_CLASS(einar_civ, CCivilian);

//=========================================================
// Spawn
//=========================================================
void CCivilian::Spawn(void)
{
	Precache( );

	SET_MODEL(ENT(pev), "models/civ.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.scientistHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	if ( pev->body == -1 )
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_HEADS-1);// pick a head, any head
	}
	
	MonsterInit();
	SetUse( &CCivilian::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCivilian :: Precache( void )
{
	PRECACHE_MODEL("models/civ.mdl");
	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	// every new civilian must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

//=========================================================
// Dead Civilian PROP
//=========================================================
class CDeadCivilian : public CDeadScientist
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(einar_civ_dead, CDeadCivilian);

//
// ********** DeadCivilian SPAWN **********
//
void CDeadCivilian::Spawn()
{
	PRECACHE_MODEL("models/civ.mdl");
	SET_MODEL(ENT(pev), "models/civ.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_HEADS - 1);// pick a head, any head
	}

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead civilian with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Civilian PROP
//=========================================================
class CSittingCivilian : public CSittingScientist
{
public:
	void Spawn(void);
	void  Precache(void);
};

LINK_ENTITY_TO_CLASS(einar_civ_sit, CSittingCivilian);

//
// ********** Civilian SPAWN **********
//
void CSittingCivilian::Spawn()
{
	PRECACHE_MODEL("models/civ.mdl");
	SET_MODEL(ENT(pev), "models/civ.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 50;

	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_CIVILIAN_HEADS - 1);// pick a head, any head
	}

	m_baseSequence = LookupSequence("sitlookleft");
	pev->sequence = m_baseSequence + RANDOM_LONG(0, 4);
	ResetSequenceInfo();

	SetThink(&CSittingCivilian::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR(ENT(pev));
}

void CSittingCivilian::Precache(void)
{
	m_baseSequence = LookupSequence("sitlookleft");
	TalkInit();
}