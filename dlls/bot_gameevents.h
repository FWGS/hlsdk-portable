/*
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the GNU General Public License as published by the
*   Free Software Foundation; either version 2 of the License, or (at
*   your option) any later version.
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software Foundation,
*   Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*   In addition, as a special exception, the author gives permission to
*   link the code of this program with the Half-Life Game Engine ("HL
*   Engine") and Modified Game Libraries ("MODs") developed by Valve,
*   L.L.C ("Valve").  You must obey the GNU General Public License in all
*   respects for all of the code used other than the HL Engine and MODs
*   from Valve.  If you modify this file, you may extend this exception
*   to your version of the file, but you are not obligated to do so.  If
*   you do not wish to do so, delete this exception statement from your
*   version.
*
*/

#ifndef GAME_EVENT_H
#define GAME_EVENT_H
#ifdef _WIN32
#pragma once
#endif

enum GameEventType
{
	EVENT_INVALID = 0,
	EVENT_WEAPON_FIRED,			// tell bots the player is attack (argumens: 1 = attacker, 2 = NULL)
	EVENT_WEAPON_FIRED_ON_EMPTY,		// tell bots the player is attack without clip ammo (argumens: 1 = attacker, 2 = NULL)
	EVENT_WEAPON_RELOADED,			// tell bots the player is reloading his weapon (argumens: 1 = reloader, 2 = NULL)

	EVENT_HE_GRENADE_EXPLODED,		// tell bots the HE grenade is exploded (argumens: 1 = grenade thrower, 2 = NULL)
	EVENT_GRENADE_BOUNCED,

	EVENT_BEING_SHOT_AT,
	EVENT_PLAYER_BLINDED_BY_FLASHBANG,	// tell bots the player is flashed (argumens: 1 = flashed player, 2 = NULL)
	EVENT_PLAYER_FOOTSTEP,			// tell bots the player is running (argumens: 1 = runner, 2 = NULL)
	EVENT_PLAYER_JUMPED,			// tell bots the player is jumped (argumens: 1 = jumper, 2 = NULL)
	EVENT_PLAYER_DIED,			// tell bots the player is killed (argumens: 1 = victim, 2 = killer)
	EVENT_PLAYER_LANDED_FROM_HEIGHT,	// tell bots the player is fell with some damage (argumens: 1 = felled player, 2 = NULL)
	EVENT_PLAYER_TOOK_DAMAGE,		// tell bots the player is take damage (argumens: 1 = victim, 2 = attacker)

	EVENT_DOOR,				// tell bots the door is moving (argumens: 1 = door, 2 = NULL)
	EVENT_BREAK_GLASS,			// tell bots the glass has break (argumens: 1 = glass, 2 = NULL)
	EVENT_BREAK_WOOD,			// tell bots the wood has break (argumens: 1 = wood, 2 = NULL)
	EVENT_BREAK_METAL,			// tell bots the metal/computer has break (argumens: 1 = metal/computer, 2 = NULL)
	EVENT_BREAK_FLESH,			// tell bots the flesh has break (argumens: 1 = flesh, 2 = NULL)
	EVENT_BREAK_CONCRETE,			// tell bots the concrete has break (argumens: 1 = concrete, 2 = NULL)

	EVENT_ROUND_DRAW,			// tell bots the round was a draw (argumens: 1 = NULL, 2 = NULL)
	EVENT_ROUND_WIN,			// tell carreer the round was a win (argumens: 1 = NULL, 2 = NULL)
	EVENT_ROUND_LOSS,			// tell carreer the round was a loss (argumens: 1 = NULL, 2 = NULL)
	EVENT_ROUND_START,			// tell bots the round was started (when freeze period is expired) (argumens: 1 = NULL, 2 = NULL)
	EVENT_PLAYER_SPAWNED,			// tell bots the player is spawned (argumens: 1 = spawned player, 2 = NULL)

	EVENT_DEATH_CAMERA_START,
	EVENT_KILL_ALL,
	EVENT_ROUND_TIME,
	EVENT_DIE,
	EVENT_KILL,
	EVENT_HEADSHOT,

	EVENT_START_RADIO_1,
	EVENT_RADIO_COVER_ME,
	EVENT_RADIO_YOU_TAKE_THE_POINT,
	EVENT_RADIO_HOLD_THIS_POSITION,
	EVENT_RADIO_REGROUP_TEAM,
	EVENT_RADIO_FOLLOW_ME,
	EVENT_RADIO_TAKING_FIRE,
	EVENT_START_RADIO_2,
	EVENT_RADIO_GO_GO_GO,
	EVENT_RADIO_TEAM_FALL_BACK,
	EVENT_RADIO_STICK_TOGETHER_TEAM,
	EVENT_RADIO_GET_IN_POSITION_AND_WAIT,
	EVENT_RADIO_STORM_THE_FRONT,
	EVENT_RADIO_REPORT_IN_TEAM,
	EVENT_START_RADIO_3,
	EVENT_RADIO_AFFIRMATIVE,
	EVENT_RADIO_ENEMY_SPOTTED,
	EVENT_RADIO_NEED_BACKUP,
	EVENT_RADIO_SECTOR_CLEAR,
	EVENT_RADIO_IN_POSITION,
	EVENT_RADIO_REPORTING_IN,
	EVENT_RADIO_GET_OUT_OF_THERE,
	EVENT_RADIO_NEGATIVE,
	EVENT_RADIO_ENEMY_DOWN,
	EVENT_END_RADIO,

	EVENT_NEW_MATCH,			// tell bots the game is new (argumens: 1 = NULL, 2 = NULL)
	EVENT_PLAYER_CHANGED_TEAM,		// tell bots the player is switch his team (also called from ClientPutInServer()) (argumens: 1 = switcher, 2 = NULL)
	EVENT_BULLET_IMPACT,			// tell bots the player is shoot at wall (argumens: 1 = shooter, 2 = shoot trace end position)
	EVENT_WEAPON_ZOOMED,			// tell bots the player is switch weapon zoom (argumens: 1 = zoom switcher, 2 = NULL)
	NUM_GAME_EVENTS,
};

extern const char *GameEventName[ NUM_GAME_EVENTS + 1 ];

#endif // GAME_EVENT_H
