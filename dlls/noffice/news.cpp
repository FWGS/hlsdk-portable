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

#define		NUM_NEWS_HEADS		3 // three heads available for news model
enum { HEAD_TREVOR = 0, HEAD_SALLY = 1, HEAD_FIREMAN = 2 };

//=========================================================
// News
//=========================================================
class CNews : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);

	BOOL	CanHeal(void) { return FALSE; }
};

LINK_ENTITY_TO_CLASS(monster_news, CNews);

//=========================================================
// Spawn
//=========================================================
void CNews::Spawn(void)
{
	if (pev->body == -1)
	{// -1 chooses a random head
		pev->body = RANDOM_LONG(0, NUM_NEWS_HEADS - 1);// pick a head, any head
	}
	Precache();

	SET_MODEL(ENT(pev), "models/news.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	//	m_flDistTooFar		= 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS;

	MonsterInit();
	SetUse(NULL);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CNews::Precache(void)
{
	PRECACHE_MODEL("models/news.mdl");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}
