/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "squeakgrenade.h"

enum w_scorpion_e {
	WSCORP_IDLE1 = 0,
	WSCORP_FIDGET,
	WSCORP_JUMP,
	WSCORP_RUN,
};

class CScorpion : public CSqueakGrenade
{
public:
	void Spawn(void);
	void Precache(void);
	void EXPORT ScorpTouch(CBaseEntity *pOther);
	void EXPORT HuntThink(void);
	int  BloodColor(void) { return BLOOD_COLOR_RED; }
	void Killed(entvars_t *pevAttacker, int iGib);
	void GibMonster(void);

	static const char* pHuntSounds[];
};

LINK_ENTITY_TO_CLASS(monster_scorpion, CScorpion);

const char* CScorpion::pHuntSounds[] =
{
	"scorpion/scorp_hunt1.wav",
	"scorpion/scorp_hunt2.wav",
	"scorpion/scorp_hunt3.wav",
};

void CScorpion::Spawn(void)
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/w_scorpion.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, 0), Vector(4, 4, 8));
	UTIL_SetOrigin(pev, pev->origin);

	SetTouch(&CScorpion::ScorpTouch);
	SetThink(&CScorpion::HuntThink);
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;

	pev->dmg = gSkillData.snarkDmgPop;

	m_flDie = 0;

	m_flFieldOfView = 0; // 180 degrees

	if (pev->owner)
		m_hOwner = Instance(pev->owner);

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a snark is spawned.

	pev->sequence = WSCORP_RUN;
	ResetSequenceInfo();
}

void CScorpion::Precache(void)
{
	PRECACHE_MODEL("models/w_scorpion.mdl");
	PRECACHE_SOUND("scorpion/scorp_die1.wav");

	PRECACHE_SOUND_ARRAY(pHuntSounds);

	PRECACHE_SOUND("scorpion/scorp_deploy1.wav");
}


void CScorpion::Killed(entvars_t *pevAttacker, int iGib)
{
	pev->model = iStringNull;// make invisible
	SetThink(&CScorpion::SUB_Remove);
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time + 0.1;

	// since squeak grenades never leave a body behind, clear out their takedamage now.
	// Squeaks do a bit of radius damage when they pop, and that radius damage will
	// continue to call this function unless we acknowledge the Squeak's death now. (sjb)
	pev->takedamage = DAMAGE_NO;

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scorpion/scorp_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(0, 0x3F));

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);

	UTIL_BloodDrips(pev->origin, g_vecZero, BloodColor(), 80);

	// reset owner so death message happens
	if (m_hOwner != NULL)
		pev->owner = m_hOwner->edict();

	CBaseMonster::Killed(pevAttacker, GIB_ALWAYS);
}

void CScorpion::GibMonster(void)
{
}

void CScorpion::HuntThink(void)
{
	// ALERT( at_console, "think\n" );

	if (!IsInWorld())
	{
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// float
	if (pev->waterlevel != 0)
	{
		if (pev->movetype == MOVETYPE_BOUNCE)
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if (pev->movetype = MOVETYPE_FLY)
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if (m_flNextHunt > gpGlobals->time)
		return;

	m_flNextHunt = gpGlobals->time + 2.0;

	CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);

	if (m_hEnemy == NULL || !m_hEnemy->IsAlive())
	{
		// find target, bounce a bit towards it.
		Look(512);
		m_hEnemy = BestVisibleEnemy();
	}

	if (m_hEnemy != NULL)
	{
		if (FVisible(m_hEnemy))
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize();
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / (flVel + 10.0);

		if (flAdj > 1.2)
			flAdj = 1.2;

		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	pev->avelocity = Vector(0, 0, 0);

	if ((pev->origin - m_posPrev).Length() < 1.0)
	{
		pev->velocity.x = RANDOM_FLOAT(-100, 100);
		pev->velocity.y = RANDOM_FLOAT(-100, 100);
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.z = 0;
	pev->angles.x = 0;
}


void CScorpion::ScorpTouch(CBaseEntity *pOther)
{
	TraceResult tr = UTIL_GetGlobalTrace();

	// don't hit the guy that launched this grenade
	if (pev->owner && pOther->edict() == pev->owner)
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if (m_flNextHit > gpGlobals->time)
		return;

	if (pOther->pev->takedamage && m_flNextAttack < gpGlobals->time)
	{
		// attack!

		// make sure it's me who has touched them
		if (tr.pHit == pOther->edict())
		{
			// and it's not another squeakgrenade
			if (tr.pHit->v.modelindex != pev->modelindex)
			{
				// ALERT( at_console, "hit enemy\n");
				ClearMultiDamage();
				pOther->TraceAttack(pev, gSkillData.snarkDmgBite, gpGlobals->v_forward, &tr, DMG_POISON);
				if (m_hOwner != NULL)
					ApplyMultiDamage(pev, m_hOwner->pev);
				else
					ApplyMultiDamage(pev, pev);

				pev->dmg += gSkillData.snarkDmgPop; // add more explosion damage
				// m_flDie += 2.0; // add more life

				// make bite sound
				EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "scorpion/scorp_deploy1.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n");
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if (g_pGameRules->IsMultiplayer())
	{
		// in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
		if (gpGlobals->time < m_flNextBounceSoundTime)
		{
			// too soon!
			return;
		}
	}

	if (!(pev->flags & FL_ONGROUND))
	{
		// play bounce sound

		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pHuntSounds), 1, ATTN_NORM, 0, PITCH_NORM);
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 256, 0.25);
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 100, 0.1);
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}