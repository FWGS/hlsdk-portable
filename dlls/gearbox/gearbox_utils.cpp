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
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"


/*
=====================
UTIL_GetActivityNameBySequence
=====================
*/
const char* UTIL_GetActivityNameBySequence(const int sequence)
{
	switch (sequence)
	{
	case ACT_RESET:				return "ACT_RESET";
	case ACT_IDLE:				return "ACT_IDLE";
	case ACT_GUARD:				return "ACT_GUARD";
	case ACT_WALK:				return "ACT_WALK";
	case ACT_RUN:				return "ACT_RUN";
	case ACT_FLY:				return "ACT_FLY";
	case ACT_SWIM:				return "ACT_SWIM";
	case ACT_HOP:				return "ACT_HOP";
	case ACT_LEAP:				return "ACT_LEAP";
	case ACT_FALL:				return "ACT_FALL";
	case ACT_LAND:				return "ACT_LAND";
	case ACT_STRAFE_LEFT:		return "ACT_STRAFE_LEFT";
	case ACT_STRAFE_RIGHT:		return "ACT_STRAFE_RIGHT";
	case ACT_ROLL_LEFT:			return "ACT_ROLL_LEFT";
	case ACT_ROLL_RIGHT:		return "ACT_ROLL_RIGHT";
	case ACT_TURN_LEFT:			return "ACT_TURN_LEFT";
	case ACT_TURN_RIGHT:		return "ACT_TURN_RIGHT";
	case ACT_CROUCH:			return "ACT_CROUCH";
	case ACT_CROUCHIDLE:		return "ACT_CROUCHIDLE";
	case ACT_STAND:				return "ACT_STAND";
	case ACT_USE:				return "ACT_USE";
	case ACT_SIGNAL1:			return "ACT_SIGNAL1";
	case ACT_SIGNAL2:			return "ACT_SIGNAL2";
	case ACT_SIGNAL3:			return "ACT_SIGNAL3";
	case ACT_TWITCH:			return "ACT_TWITCH";
	case ACT_COWER:				return "ACT_COWER";
	case ACT_SMALL_FLINCH:		return "ACT_SMALL_FLINCH";
	case ACT_BIG_FLINCH:		return "ACT_BIG_FLINCH";
	case ACT_RANGE_ATTACK1:		return "ACT_RANGE_ATTACK1";
	case ACT_RANGE_ATTACK2:		return "ACT_RANGE_ATTACK2";
	case ACT_MELEE_ATTACK1:		return "ACT_MELEE_ATTACK1";
	case ACT_MELEE_ATTACK2:		return "ACT_MELEE_ATTACK2";
	case ACT_RELOAD:			return "ACT_RELOAD";
	case ACT_ARM:				return "ACT_ARM";
	case ACT_DISARM:			return "ACT_DISARM";
	case ACT_EAT:				return "ACT_EAT";
	case ACT_DIESIMPLE:			return "ACT_DIESIMPLE";
	case ACT_DIEBACKWARD:		return "ACT_DIEBACKWARD";
	case ACT_DIEFORWARD:		return "ACT_DIEFORWARD";
	case ACT_DIEVIOLENT:		return "ACT_DIEVIOLENT";
	case ACT_BARNACLE_HIT:		return "ACT_BARNACLE_HIT";
	case ACT_BARNACLE_PULL:		return "ACT_BARNACLE_PULL";
	case ACT_BARNACLE_CHOMP:	return "ACT_BARNACLE_CHOMP";		// barnacle latches on to the monster
	case ACT_BARNACLE_CHEW:		return "ACT_BARNACLE_CHEW";		// barnacle is holding the monster in its mouth ( loop )
	case ACT_SLEEP:				return "ACT_SLEEP";
	case ACT_INSPECT_FLOOR:		return "ACT_INSPECT_FLOOR";		// for active idles"; look at something on or near the floor
	case ACT_INSPECT_WALL:		return "ACT_INSPECT_WALL";		// for active idles"; look at something directly ahead of you ( doesn't HAVE to be a wall or on a wall )
	case ACT_IDLE_ANGRY:		return "ACT_IDLE_ANGRY";			// alternate idle animation in which the monster is clearly agitated. (loop)
	case ACT_WALK_HURT:			return "ACT_WALK_HURT";			// limp  (loop)
	case ACT_RUN_HURT:			return "ACT_RUN_HURT";			// limp  (loop)
	case ACT_HOVER:				return "ACT_HOVER";				// Idle while in flight
	case ACT_GLIDE:				return "ACT_GLIDE";				// Fly (don't flap)
	case ACT_FLY_LEFT:			return "ACT_FLY_LEFT";			// Turn left in flight
	case ACT_FLY_RIGHT:			return "ACT_FLY_RIGHT";			// Turn right in flight
	case ACT_DETECT_SCENT:		return "ACT_DETECT_SCENT";		// this means the monster smells a scent carried by the air
	case ACT_SNIFF:				return "ACT_SNIFF";				// this is the act of actually sniffing an item in front of the monster
	case ACT_BITE:				return "ACT_BITE";				// some large monsters can eat small things in one bite. This plays one time"; EAT loops.
	case ACT_THREAT_DISPLAY:	return "ACT_THREAT_DISPLAY";		// without attacking"; monster demonstrates that it is angry. (Yell"; stick out chest"; etc )
	case ACT_FEAR_DISPLAY:		return "ACT_FEAR_DISPLAY";		// monster just saw something that it is afraid of
	case ACT_EXCITED:			return "ACT_EXCITED";			// for some reason"; monster is excited. Sees something he really likes to eat"; or whatever.
	case ACT_SPECIAL_ATTACK1:	return "ACT_SPECIAL_ATTACK1";	// very monster specific special attacks.
	case ACT_SPECIAL_ATTACK2:	return "ACT_SPECIAL_ATTACK2";
	case ACT_COMBAT_IDLE:		return "ACT_COMBAT_IDLE";		// agitated idle.
	case ACT_WALK_SCARED:		return "ACT_WALK_SCARED";
	case ACT_RUN_SCARED:		return "ACT_RUN_SCARED";
	case ACT_VICTORY_DANCE:		return "ACT_VICTORY_DANCE";		// killed a player"; do a victory dance.
	case ACT_DIE_HEADSHOT:		return "ACT_DIE_HEADSHOT";		// die"; hit in head. 
	case ACT_DIE_CHESTSHOT:		return "ACT_DIE_CHESTSHOT";		// die"; hit in chest
	case ACT_DIE_GUTSHOT:		return "ACT_DIE_GUTSHOT";		// die"; hit in gut
	case ACT_DIE_BACKSHOT:		return "ACT_DIE_BACKSHOT";		// die"; hit in back
	case ACT_FLINCH_HEAD:		return "ACT_FLINCH_HEAD";
	case ACT_FLINCH_CHEST:		return "ACT_FLINCH_CHEST";
	case ACT_FLINCH_STOMACH:	return "ACT_FLINCH_STOMACH";
	case ACT_FLINCH_LEFTARM:	return "ACT_FLINCH_LEFTARM";
	case ACT_FLINCH_RIGHTARM:	return "ACT_FLINCH_RIGHTARM";
	case ACT_FLINCH_LEFTLEG:	return "ACT_FLINCH_LEFTLEG";
	case ACT_FLINCH_RIGHTLEG:	return "ACT_FLINCH_RIGHTLEG";
	default:					return "Unknown sequence!";
	}
}


/*
=====================
UTIL_GetActivityNameBySequence
=====================
*/
const char*	UTIL_GetEntityActivityName(struct entvars_s* pev)
{
	return UTIL_GetActivityNameBySequence(pev->sequence);
}

/*
=====================
UTIL_GetEntityActivityName
=====================
*/
const char*	UTIL_GetEntityActivityName(struct edict_s* ent)
{
	return UTIL_GetEntityActivityName(VARS(ent));
}

/*
=====================
UTIL_GetEntityActivityName
=====================
*/
const char*	UTIL_GetEntityActivityName(CBaseEntity* pEntity)
{
	return UTIL_GetEntityActivityName(pEntity->edict());
}

/*
=====================
UTIL_PrintActivity
=====================
*/
void UTIL_PrintActivity(struct entvars_s* pev, const ALERT_TYPE alert_type)
{
#ifndef CLIENT_DLL
	ALERT(alert_type, "ALERT: entity %s with classname %s is using animation %s\n",
		STRING(pev->targetname), 
		STRING(pev->classname), 
		UTIL_GetActivityNameBySequence(pev->sequence));
#endif
}

/*
=====================
UTIL_PrintActivity
=====================
*/
void UTIL_PrintActivity(struct edict_s* ent, const ALERT_TYPE alert_type)
{
	UTIL_PrintActivity(VARS(ent), alert_type);
}

/*
=====================
UTIL_PrintActivity
=====================
*/
void UTIL_PrintActivity(CBaseEntity* pEntity, const ALERT_TYPE alert_type)
{
	UTIL_PrintActivity(pEntity->edict(), alert_type);
}