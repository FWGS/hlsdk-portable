/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "eiface.h"
#include "util.h"
#include "game.h"

cvar_t tfc_spam_penalty1 = { "tfc_spam_penalty1", "8.0" };
cvar_t tfc_spam_penalty2 = { "tfc_spam_penalty2", "2.0" };
cvar_t tfc_spam_limit = { "tfc_spam_limit", "20.0" };
cvar_t tfc_autokick_time = { "tfc_autokick_time", "180.0" };
cvar_t tfc_autokick_kills = { "tfc_autokick_kills", "5.0" };
cvar_t tfc_autoteam = { "tfc_autoteam", "0", FCVAR_SERVER };
cvar_t tfc_birthday = { "tfc_birthday", "0" };
cvar_t tfc_fragscoring = { "tfc_fragscoring", "0" };
cvar_t tfc_respawndelay = { "tfc_respawndelay", "0", FCVAR_SERVER };
cvar_t tfc_clanbattle = { "tfc_clanbattle", "0", FCVAR_SERVER };
cvar_t tfc_clanbattle_prematch = { "tfc_clanbattle_prematch", "0" };
cvar_t tfc_prematch = { "tfc_prematch", "0" };
cvar_t tfc_clanbattle_ceasefire = { "tfc_clanbattle_ceasefire", "0" };
cvar_t tfc_clanbattle_locked = { "tfc_clanbattle_locked", "0" };
cvar_t tfc_adminpwd = { "tfc_adminpwd", "0", FCVAR_SERVER | FCVAR_PROTECTED };
cvar_t tfc_balance_teams = { "tfc_balance_teams", "1.0", FCVAR_SERVER };
cvar_t tfc_balance_scores = { "tfc_balance_scores", "1.0", FCVAR_SERVER };
cvar_t allow_spectators = { "allow_spectators", "1.0", FCVAR_SERVER };    // 0 prevents players from being spectators
cvar_t tfc_spectchat = { "tfc_spectchat", "0", FCVAR_SERVER };
cvar_t tfc_playerid = { "tfc_playerid", "0", FCVAR_SERVER };

cvar_t displaysoundlist = {"displaysoundlist","0"};

// multiplayer server rules
cvar_t fragsleft	= { "mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t timeleft		= { "mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t teamplay		= { "mp_teamplay","0", FCVAR_SERVER };
cvar_t fraglimit	= { "mp_fraglimit","0", FCVAR_SERVER };
cvar_t timelimit	= { "mp_timelimit","0", FCVAR_SERVER };
cvar_t friendlyfire	= { "mp_friendlyfire","0", FCVAR_SERVER };
cvar_t falldamage	= { "mp_falldamage","0", FCVAR_SERVER };
cvar_t weaponstay	= { "mp_weaponstay","0", FCVAR_SERVER };
cvar_t forcerespawn	= { "mp_forcerespawn","1", FCVAR_SERVER };
cvar_t flashlight	= { "mp_flashlight","0", FCVAR_SERVER };
cvar_t aimcrosshair	= { "mp_autocrosshair","1", FCVAR_SERVER };
cvar_t decalfrequency	= { "decalfrequency","30", FCVAR_SERVER };
cvar_t bhopcap		= { "mp_bhopcap", "1", FCVAR_SERVER };

cvar_t cr_scout		= { "cr_scout", "0", FCVAR_SERVER };
cvar_t cr_sniper	= { "cr_sniper", "0", FCVAR_SERVER };
cvar_t cr_soldier	= { "cr_soldier", "0", FCVAR_SERVER };
cvar_t cr_demoman	= { "cr_demoman", "0", FCVAR_SERVER };
cvar_t cr_medic		= { "cr_medic", "0", FCVAR_SERVER };
cvar_t cr_hwguy		= { "cr_hwguy", "0", FCVAR_SERVER };
cvar_t cr_pyro		= { "cr_pyro", "0", FCVAR_SERVER };
cvar_t cr_spy		= { "cr_spy", "0", FCVAR_SERVER };
cvar_t cr_engineer	= { "cr_engineer", "0", FCVAR_SERVER };
cvar_t cr_random	= { "cr_random", "0", FCVAR_SERVER };

cvar_t mp_chattime	= { "mp_chattime","10", FCVAR_SERVER };

// Engine Cvars
cvar_t *g_psv_gravity;
cvar_t *g_footsteps;

// END Cvars for Skill Level settings

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	// Register cvars here:

	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );

	CVAR_REGISTER( &tfc_spam_penalty1 );
	CVAR_REGISTER( &tfc_spam_penalty2 );
	CVAR_REGISTER( &tfc_spam_limit );
	CVAR_REGISTER( &tfc_autoteam );
	CVAR_REGISTER( &tfc_birthday );
	CVAR_REGISTER( &tfc_fragscoring );
	CVAR_REGISTER( &tfc_respawndelay );
	CVAR_REGISTER( &tfc_clanbattle );
	CVAR_REGISTER( &tfc_clanbattle_prematch );
	CVAR_REGISTER( &tfc_prematch );
	CVAR_REGISTER( &tfc_clanbattle_ceasefire );
	CVAR_REGISTER( &tfc_clanbattle_locked );
	CVAR_REGISTER( &tfc_adminpwd );
	CVAR_REGISTER( &tfc_balance_teams );
	CVAR_REGISTER( &tfc_balance_scores );
	CVAR_REGISTER( &tfc_autokick_time );
	CVAR_REGISTER( &tfc_autokick_kills );

	CVAR_REGISTER( &allow_spectators );

	CVAR_REGISTER( &tfc_spectchat );
	CVAR_REGISTER( &tfc_playerid );

	CVAR_REGISTER( &displaysoundlist );

	CVAR_REGISTER( &teamplay );
	CVAR_REGISTER( &fraglimit );
	CVAR_REGISTER( &timelimit );

	CVAR_REGISTER( &fragsleft );
	CVAR_REGISTER( &timeleft );

	CVAR_REGISTER( &friendlyfire );
	CVAR_REGISTER( &falldamage );
	CVAR_REGISTER( &weaponstay );
	CVAR_REGISTER( &forcerespawn );
	CVAR_REGISTER( &flashlight );
	CVAR_REGISTER( &aimcrosshair );
	CVAR_REGISTER( &decalfrequency );

	CVAR_REGISTER( &cr_scout );
	CVAR_REGISTER( &cr_sniper );
	CVAR_REGISTER( &cr_soldier );
	CVAR_REGISTER( &cr_demoman );
	CVAR_REGISTER( &cr_medic );
	CVAR_REGISTER( &cr_hwguy );
	CVAR_REGISTER( &cr_pyro );
	CVAR_REGISTER( &cr_spy );
	CVAR_REGISTER( &cr_engineer );
	CVAR_REGISTER( &cr_random );

	CVAR_REGISTER( &bhopcap );

	CVAR_REGISTER( &mp_chattime );
}

void GameDLLShutdown( void )
{
}
