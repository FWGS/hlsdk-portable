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
// rat - environmental monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

#define	FORKLIFT_ATTACK		1

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CForklift : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  Classify(void);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void Killed(entvars_t *pevAttacker, int iGib);
	int pGibName;
};
LINK_ENTITY_TO_CLASS(monster_forklift, CForklift);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CForklift::Classify(void)
{
	return	CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CForklift::SetYawSpeed(void)
{
	pev->yaw_speed = 600;
}

void CForklift::Killed(entvars_t *pevAttacker, int iGib)
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal1.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	case 1:	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "debris/bustmetal2.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	}
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BREAKMODEL);

	// position
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	// size
	WRITE_COORD(pev->size.x * 2);
	WRITE_COORD(pev->size.y * 2);
	WRITE_COORD(pev->size.z * 2);

	// velocity
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);

	// randomization
	WRITE_BYTE(25);

	// Model
	WRITE_SHORT(pGibName);	//model id#

	// # of shards
	WRITE_BYTE(0);	// let client decide

	// duration
	WRITE_BYTE(25);// 2.5 seconds

	// flags
	WRITE_BYTE(BREAK_METAL);
	MESSAGE_END();

	CGib::SpawnRandomGibs(pev, 6, 1);
	CBaseMonster::Killed(pevAttacker, iGib);
	UTIL_Remove(this);
}

//=========================================================
// Spawn
//=========================================================
void CForklift::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/forklift.mdl");
	UTIL_SetSize(pev, Vector(-32, -14, 0), Vector(32, 14, 84));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 8;
	pev->view_ofs = Vector(0, 0, 6);// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.9;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CForklift::Precache()
{
	PRECACHE_MODEL("models/forklift.mdl");
	PRECACHE_SOUND("generic/forklift.wav");
	PRECACHE_SOUND("doors/doormove4.wav");
	PRECACHE_SOUND("debris/bustmetal1.wav");
	PRECACHE_SOUND("debris/bustmetal2.wav");
	pGibName = PRECACHE_MODEL("models/computergibs.mdl");
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
void CForklift::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case FORKLIFT_ATTACK:
	{
								   // do stuff for this event.
								   //		ALERT( at_console, "Slash right!\n" );
								   CBaseEntity *pHurt = CheckTraceHullAttack(110, 2, DMG_SLASH);
								   if (pHurt)
								   {
									   if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
									   {
										   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 300;
									   }
									   // Play a random attack hit sound
									   
								   }
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "doors/doormove4.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

BOOL CForklift::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 100)
	{
		return TRUE;
	}
	return FALSE;
}