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
/*

===== gearbox_triggers.cpp ========================================================

spawn and use functions for editor-placed triggers

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"
#include "gamerules.h"
#include "triggers.h"
#include "skill.h"

//=========================================================
// CTriggerXenReturn
//=========================================================

class CTriggerXenReturn : public CTriggerTeleport
{
public:
	void Spawn(void);
	void EXPORT TeleportTouch(CBaseEntity *pOther);
};


LINK_ENTITY_TO_CLASS(trigger_xen_return, CTriggerXenReturn);


void CTriggerXenReturn::Spawn(void)
{
	CTriggerTeleport::Spawn();

	SetTouch(&CTriggerXenReturn::TeleportTouch);
}

void CTriggerXenReturn::TeleportTouch(CBaseEntity* pOther)
{
	entvars_t* pevToucher = pOther->pev;
	edict_t	*pentTarget = NULL;

	// Only teleport monsters or clients
	if (!FBitSet(pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered(m_sMaster, pOther))
		return;

	if (!(pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS))
	{// no monsters allowed!
		if (FBitSet(pevToucher->flags, FL_MONSTER))
		{
			return;
		}
	}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS))
	{// no clients allowed
		if (pOther->IsPlayer())
		{
			return;
		}
	}

	pentTarget = FIND_ENTITY_BY_CLASSNAME(pentTarget, "info_displacer_earth_target");
	if (FNullEnt(pentTarget))
		return;

	Vector tmp = VARS(pentTarget)->origin;

	if (pOther->IsPlayer())
	{
		tmp.z -= pOther->pev->mins.z;// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
	}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin(pevToucher, tmp);

	pevToucher->angles = pentTarget->v.angles;

	if (pOther->IsPlayer())
	{
		pevToucher->v_angle = pentTarget->v.angles;
	}

	pevToucher->fixangle = TRUE;
	pevToucher->velocity = pevToucher->basevelocity = g_vecZero;

	if (pOther->IsPlayer())
	{
		// Ensure the current player is marked as being
		// on earth.
		((CBasePlayer*)pOther)->m_fInXen = FALSE;

		// Reset gravity to default.
		pOther->pev->gravity = 1.0f;
	}

	// Play teleport sound.
	EMIT_SOUND(ENT(pOther->pev), CHAN_STATIC, "debris/beamstart7.wav", 1, ATTN_NORM );
}

//=========================================================
// CTriggerGenewormHit
//=========================================================

#define SF_TRIGGER_HURT_TARGETONCE	1// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2//spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8//spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32// only clients may touch this trigger.

class CTriggerGenewormHit : public CBaseTrigger
{
public:
	void Spawn();
	void Precache();
	void EXPORT GeneWormTouch(CBaseEntity *pOther);

	static const char* pAttackSounds[];

	static TYPEDESCRIPTION m_SaveData[];

	virtual int Save( CSave &save );
	virtual int Restore( CRestore &restore );

	float m_flLastDamageTime;
};

TYPEDESCRIPTION CTriggerGenewormHit::m_SaveData[] =
{
	DEFINE_FIELD(CTriggerGenewormHit, m_flLastDamageTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CTriggerGenewormHit, CBaseTrigger)

const char *CTriggerGenewormHit::pAttackSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav"
};

void CTriggerGenewormHit::Spawn()
{
	Precache();
	InitTrigger();

	SetTouch(&CTriggerGenewormHit::GeneWormTouch);

	if(pev->targetname)
		SetUse(&CBaseTrigger::ToggleUse);


	if(pev->spawnflags & SF_TRIGGER_HURT_START_OFF)
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin(pev, pev->origin);
	pev->dmg = gSkillData.gwormDmgHit;
	m_flLastDamageTime = gpGlobals->time;
}

void CTriggerGenewormHit::Precache()
{
	PRECACHE_SOUND_ARRAY(pAttackSounds);
}

void CTriggerGenewormHit::GeneWormTouch(CBaseEntity *pOther)
{
	if( gpGlobals->time - m_flLastDamageTime < 2 || !pOther->pev->takedamage )
		return;

	if( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
	{
		// this trigger is only allowed to touch clients, and this ain't a client.
		return;
	}

	if( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if( g_pGameRules->IsMultiplayer() )
	{
		if( pev->dmgtime > gpGlobals->time )
		{
			if( gpGlobals->time != pev->pain_finished )
			{
				// too early to hurt again, and not same frame with a different entity
				if( pOther->IsPlayer() )
				{
					int playerMask = 1 << ( pOther->entindex() - 1 );

					// If I've already touched this player (this time), then bail out
					if( pev->impulse & playerMask )
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
				}
				else
				{
					return;
				}
			}
		}
		else
		{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if( pOther->IsPlayer() )
			{
				int playerMask = 1 << ( pOther->entindex() - 1 );

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
			}
		}
	}
	else	// Original code -- single player
	{
		if( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
		{
			// too early to hurt again, and not same frame with a different entity
			return;
		}
	}

	// If this is time_based damage (poison, radiation), override the pev->dmg with a
	// default for the given damage type.  Monsters only take time-based damage
	// while touching the trigger.  Player continues taking damage for a while after
	// leaving the trigger

	pOther->TakeDamage( pev, pev, gSkillData.gwormDmgHit, m_bitsDamageInflict );

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;// half second delay until this trigger can hurt toucher again

	EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, RANDOM_SOUND_ARRAY(pAttackSounds), VOL_NORM, 0.1, 0, 100 + RANDOM_FLOAT(-5,5));
	m_flLastDamageTime = gpGlobals->time;

	if( pev->target )
	{
		// trigger has a target it wants to fire.
		if( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE )
		{
			// if the toucher isn't a client, don't fire the target!
			if( !pOther->IsPlayer() )
			{
				return;
			}
		}

		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

LINK_ENTITY_TO_CLASS(trigger_geneworm_hit, CTriggerGenewormHit)

//=========================================================
// CPlayerFreeze
//=========================================================

class CTriggerPlayerFreeze : public CBaseDelay
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int ObjectCaps( void ) { return CBaseDelay::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
};

LINK_ENTITY_TO_CLASS( trigger_playerfreeze, CTriggerPlayerFreeze )

void CTriggerPlayerFreeze::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( !pActivator || !pActivator->IsPlayer() )
		pActivator = CBaseEntity::Instance( g_engfuncs.pfnPEntityOfEntIndex( 1 ) );

	if( pActivator && (pActivator->pev->flags & FL_FROZEN) )
		( (CBasePlayer *)( (CBaseEntity *)pActivator ) )->EnableControl( TRUE );
	else
		( (CBasePlayer *)( (CBaseEntity *)pActivator ) )->EnableControl( FALSE );
}
